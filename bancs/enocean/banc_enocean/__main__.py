"""Point d'entrée du banc EnOcean.

    python3 -m banc_enocean                     # dongle détecté automatiquement
    python3 -m banc_enocean --port /dev/ttyUSB0
    python3 -m banc_enocean --simulation        # sans matériel, pour la mise au point
"""

from __future__ import annotations

import argparse
import logging
import sys
import threading
import time

from .app import creer_serveur
from .bus import BusEvenements
from .dongle import FakeDongle, SerialDongle, detecter_port, trame_rps
from .lecteur import Lecteur
from .store import CHEMIN_DEFAUT, Registre


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--adresse", default="0.0.0.0", help="interface d'écoute")
    ap.add_argument("--http-port", type=int, default=8080)
    ap.add_argument("--port", default=None, help="port série du dongle EnOcean")
    ap.add_argument("--registre", default=str(CHEMIN_DEFAUT))
    ap.add_argument("--simulation", action="store_true",
                    help="dongle factice : un appui toutes les 5 s")
    ap.add_argument("-v", "--verbeux", action="store_true")
    args = ap.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbeux else logging.INFO,
                        format="%(asctime)s %(levelname)-7s %(message)s")
    log = logging.getLogger("banc")

    registre = Registre(args.registre)
    bus = BusEvenements()

    if args.simulation:
        dongle = FakeDongle()
        log.warning("MODE SIMULATION — aucun matériel n'est lu")
    else:
        try:
            dongle = SerialDongle(args.port)
        except ImportError:
            log.error("pyserial absent. Sur Debian : sudo apt install python3-serial")
            return 2
        except FileNotFoundError as exc:
            log.error("%s", exc)
            log.error("ports vus : %s", detecter_port() or "aucun")
            return 2

    lecteur = Lecteur(dongle, registre, bus)
    lecteur.demarrer()

    if args.simulation:
        def battements() -> None:
            faux = [0x0029B1C4, 0xFEFF1234]
            i = 0
            while True:
                time.sleep(5.0)
                dongle.injecter(trame_rps(faux[i % len(faux)]))
                i += 1
        threading.Thread(target=battements, daemon=True).start()

    serveur = creer_serveur(args.adresse, args.http_port, registre, lecteur, bus)
    log.info("banc EnOcean sur http://%s:%d — dongle %s, %d bouton(s) enregistré(s)",
             args.adresse, args.http_port, getattr(dongle, "port", "?"),
             len(registre.tous()))
    try:
        serveur.serve_forever()
    except KeyboardInterrupt:
        log.info("arrêt demandé")
    finally:
        lecteur.arreter()
        serveur.server_close()
        dongle.fermer()
    return 0


if __name__ == "__main__":
    sys.exit(main())
