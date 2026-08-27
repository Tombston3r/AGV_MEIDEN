"""Boucle de lecture du dongle : octets -> télégrammes -> événements.

C'est ici que vivent les deux règles qui font la différence entre un banc
utilisable et un banc qui ment :

  1. **Déduplication** : un PTM 210 émet TROIS sous-télégrammes par appui.
     Sans filtre, une pression afficherait trois fenêtres.
  2. **Appui seulement** : le module émet aussi au RELÂCHEMENT. On ne retient
     que le front d'appui, sinon chaque pression compte double.
"""

from __future__ import annotations

import logging
import threading
import time

from .bus import BusEvenements
from .esp3 import Deduplicator, Esp3Decoder, parse_rps
from .store import Registre

LOG = logging.getLogger(__name__)


class Lecteur:
    def __init__(self, dongle, registre: Registre, bus: BusEvenements) -> None:
        self._dongle = dongle
        self._registre = registre
        self._bus = bus
        self._decodeur = Esp3Decoder()
        self._dedup = Deduplicator(window_ms=400)
        self._stop = threading.Event()
        self._fil: threading.Thread | None = None

        self._verrou = threading.Lock()
        self._apprentissage_jusqu_a = 0.0
        self._dernier_appui: dict | None = None
        self.appuis = 0
        self.inconnus = 0

    # --- Mode apprentissage --------------------------------------------------
    def armer_apprentissage(self, duree_s: float = 30.0) -> float:
        with self._verrou:
            self._apprentissage_jusqu_a = time.time() + duree_s
            return self._apprentissage_jusqu_a

    def desarmer_apprentissage(self) -> None:
        with self._verrou:
            self._apprentissage_jusqu_a = 0.0

    def apprentissage_actif(self) -> bool:
        with self._verrou:
            return time.time() < self._apprentissage_jusqu_a

    def dernier_appui(self) -> dict | None:
        with self._verrou:
            return dict(self._dernier_appui) if self._dernier_appui else None

    # --- Boucle --------------------------------------------------------------
    def demarrer(self) -> None:
        self._fil = threading.Thread(target=self._boucle, name="dongle", daemon=True)
        self._fil.start()

    def arreter(self) -> None:
        self._stop.set()
        if self._fil:
            self._fil.join(timeout=2.0)

    def _boucle(self) -> None:
        while not self._stop.is_set():
            try:
                octets = self._dongle.lire()
            except Exception:                      # pragma: no cover - matériel
                LOG.exception("lecture du dongle interrompue")
                time.sleep(1.0)
                continue
            if not octets:
                time.sleep(0.01)
                continue
            for b in octets:
                self.traiter_octet(b)

    def traiter_octet(self, octet: int) -> None:
        paquet = self._decodeur.feed(octet)
        if paquet is None:
            return
        rps = parse_rps(paquet)
        if rps is None:
            return
        self._traiter_rps(rps)

    def _traiter_rps(self, rps) -> None:
        maintenant = time.time()
        # Le relâchement porte data = 0 : ce n'est pas un appui.
        if not rps.pressed:
            return
        if not self._dedup.accept(rps.sender_id, rps.data, maintenant * 1000.0):
            return

        self.appuis += 1
        code = f"{rps.sender_id:08X}"
        info = {
            "id": rps.sender_id,
            "code": code,
            "rssi_dbm": rps.rssi_dbm,
            "bascule": rps.rocker,
            "heure": maintenant,
        }
        with self._verrou:
            self._dernier_appui = dict(info)
            apprend = maintenant < self._apprentissage_jusqu_a

        if apprend:
            self._bus.publier("apprentissage", info)
            return

        bouton = self._registre.get(rps.sender_id)
        if bouton is None:
            self.inconnus += 1
            # On le signale au lieu de l'ignorer : au banc, un appui non
            # reconnu est justement ce qu'on cherche à voir.
            self._bus.publier("inconnu", info)
            return

        self._bus.publier("appui", {**info, "nom": bouton.nom})
