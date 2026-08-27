"""Registre des boutons EnOcean connus, persisté en JSON.

Volontairement trivial : un banc d'essai n'a pas besoin d'une base. Le fichier
est lisible et modifiable à la main, ce qui compte plus ici que la performance.
"""

from __future__ import annotations

import json
import threading
import time
from dataclasses import asdict, dataclass
from pathlib import Path

CHEMIN_DEFAUT = Path.home() / ".local/share/banc-enocean/boutons.json"


@dataclass(slots=True)
class Bouton:
    id: int                  # identifiant 32 bits gravé en usine
    nom: str
    ajoute_le: float         # epoch, secondes

    @property
    def code(self) -> str:
        """Forme lisible de l'identifiant, telle qu'imprimée sur le module."""
        return f"{self.id:08X}"

    def json(self) -> dict:
        d = asdict(self)
        d["code"] = self.code
        return d


class Registre:
    def __init__(self, chemin: Path | str = CHEMIN_DEFAUT) -> None:
        self.chemin = Path(chemin)
        self._verrou = threading.Lock()
        self._boutons: dict[int, Bouton] = {}
        self._charger()

    def _charger(self) -> None:
        if not self.chemin.exists():
            return
        try:
            brut = json.loads(self.chemin.read_text())
        except (OSError, json.JSONDecodeError):
            # Un fichier corrompu ne doit pas empêcher le banc de démarrer :
            # on repart d'un registre vide plutôt que de refuser de servir.
            return
        for e in brut.get("boutons", []):
            try:
                b = Bouton(int(e["id"]), str(e["nom"]), float(e.get("ajoute_le", 0)))
            except (KeyError, TypeError, ValueError):
                continue
            self._boutons[b.id] = b

    def _ecrire(self) -> None:
        self.chemin.parent.mkdir(parents=True, exist_ok=True)
        corps = {"boutons": [b.json() for b in self._boutons.values()]}
        # Écriture atomique : une coupure ne doit pas laisser un JSON tronqué.
        tmp = self.chemin.with_suffix(".tmp")
        tmp.write_text(json.dumps(corps, indent=2, ensure_ascii=False))
        tmp.replace(self.chemin)

    def tous(self) -> list[Bouton]:
        with self._verrou:
            return sorted(self._boutons.values(), key=lambda b: b.nom.lower())

    def get(self, id_: int) -> Bouton | None:
        with self._verrou:
            return self._boutons.get(id_)

    def ajouter(self, id_: int, nom: str) -> Bouton:
        with self._verrou:
            if id_ in self._boutons:
                raise ValueError(f"le bouton {id_:08X} est déjà enregistré "
                                 f"sous le nom « {self._boutons[id_].nom} »")
            b = Bouton(id_, nom.strip() or f"Bouton {id_:08X}", time.time())
            self._boutons[id_] = b
            self._ecrire()
            return b

    def renommer(self, id_: int, nom: str) -> Bouton:
        with self._verrou:
            b = self._boutons.get(id_)
            if b is None:
                raise KeyError(id_)
            b.nom = nom.strip() or b.nom
            self._ecrire()
            return b

    def supprimer(self, id_: int) -> bool:
        with self._verrou:
            if self._boutons.pop(id_, None) is None:
                return False
            self._ecrire()
            return True


def parse_id(texte: str) -> int:
    """Accepte 00:29:B1:C4, 0029B1C4 ou 0x0029B1C4, TOUJOURS en hexadécimal.

    Les identifiants sont imprimés sous des formes variées selon le fabricant
    de l'enveloppe murale ; refuser une seule d'entre elles serait une source
    d'erreur de saisie évitable.

    ⚠ La lecture est hexadécimale sans exception, y compris pour une chaîne
    ne contenant que des chiffres : « 2732996 » est un hexadécimal valide, et
    tenter de deviner entre les deux bases produirait un identifiant faux une
    fois sur deux : le genre de panne qu'on met une après-midi à comprendre.
    Les identifiants EnOcean sont toujours notés en hexadécimal.
    """
    t = texte.strip().replace(":", "").replace("-", "").replace(" ", "")
    if not t:
        raise ValueError("identifiant vide")
    if t.lower().startswith("0x"):
        t = t[2:]
    try:
        valeur = int(t, 16)
    except ValueError as exc:
        raise ValueError(f"« {texte.strip()} » n'est pas un identifiant "
                         f"hexadécimal valide") from exc
    if not 0 <= valeur <= 0xFFFFFFFF:
        raise ValueError("un identifiant EnOcean tient sur 32 bits")
    return valeur
