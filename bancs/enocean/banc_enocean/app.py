"""Serveur HTTP du banc : API JSON + flux d'événements + page web.

Bibliothèque standard uniquement, hors `pyserial`. Sur une UniPi, chaque
dépendance est une chose de plus à installer sur une machine qui n'a pas
forcément accès au dépôt Debian : le banc doit démarrer avec ce qui est là.
"""

from __future__ import annotations

import json
import logging
import queue
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

from .bus import BusEvenements
from .lecteur import Lecteur
from .store import Registre, parse_id

LOG = logging.getLogger(__name__)
RACINE_WEB = Path(__file__).resolve().parent.parent / "web"

TYPES = {".html": "text/html; charset=utf-8",
         ".js": "application/javascript; charset=utf-8",
         ".css": "text/css; charset=utf-8"}


class Handler(BaseHTTPRequestHandler):
    server_version = "banc-enocean"
    registre: Registre
    lecteur: Lecteur
    bus: BusEvenements

    # --- Utilitaires ---------------------------------------------------------
    def _json(self, code: int, corps) -> None:
        brut = json.dumps(corps, ensure_ascii=False).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(brut)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(brut)

    def _corps_json(self) -> dict:
        n = int(self.headers.get("Content-Length") or 0)
        if n <= 0:
            return {}
        try:
            return json.loads(self.rfile.read(n) or b"{}")
        except json.JSONDecodeError:
            return {}

    def log_message(self, fmt, *args):  # noqa: D102 - journal maîtrisé
        LOG.debug("%s %s", self.address_string(), fmt % args)

    # --- Routage -------------------------------------------------------------
    def do_GET(self) -> None:  # noqa: N802
        chemin = urlparse(self.path).path
        if chemin == "/api/boutons":
            return self._json(200, {"boutons": [b.json() for b in self.registre.tous()]})
        if chemin == "/api/etat":
            return self._json(200, {
                "port": getattr(self.lecteur._dongle, "port", "?"),
                "appuis": self.lecteur.appuis,
                "inconnus": self.lecteur.inconnus,
                "apprentissage": self.lecteur.apprentissage_actif(),
                "dernier_appui": self.lecteur.dernier_appui(),
                "clients": self.bus.nb_abonnes,
            })
        if chemin == "/api/evenements":
            return self._flux()
        return self._fichier(chemin)

    def do_POST(self) -> None:  # noqa: N802
        chemin = urlparse(self.path).path
        corps = self._corps_json()

        if chemin == "/api/boutons":
            try:
                id_ = parse_id(str(corps.get("id", "")))
            except ValueError as exc:
                return self._json(400, {"erreur": str(exc)})
            try:
                b = self.registre.ajouter(id_, str(corps.get("nom", "")))
            except ValueError as exc:
                return self._json(409, {"erreur": str(exc)})
            return self._json(201, b.json())

        if chemin == "/api/apprentissage":
            fin = self.lecteur.armer_apprentissage(float(corps.get("duree_s", 30)))
            return self._json(200, {"actif": True, "expire_le": fin,
                                    "dernier_appui": self.lecteur.dernier_appui()})

        if chemin == "/api/apprentissage/annuler":
            self.lecteur.desarmer_apprentissage()
            return self._json(200, {"actif": False})

        return self._json(404, {"erreur": "route inconnue"})

    def do_DELETE(self) -> None:  # noqa: N802
        chemin = urlparse(self.path).path
        if chemin.startswith("/api/boutons/"):
            try:
                id_ = parse_id(chemin.rsplit("/", 1)[-1])
            except ValueError as exc:
                return self._json(400, {"erreur": str(exc)})
            if not self.registre.supprimer(id_):
                return self._json(404, {"erreur": "bouton inconnu"})
            return self._json(200, {"supprime": True})
        return self._json(404, {"erreur": "route inconnue"})

    # --- Flux d'événements ---------------------------------------------------
    def _flux(self) -> None:
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Connection", "keep-alive")
        self.end_headers()

        f = self.bus.abonner()
        try:
            self.wfile.write(b": connecte\n\n")
            self.wfile.flush()
            while True:
                try:
                    msg = f.get(timeout=15.0)
                except queue.Empty:
                    # Battement : sans trafic, un mandataire couperait la
                    # connexion au bout d'une minute.
                    msg = ": battement\n\n"
                self.wfile.write(msg.encode())
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            self.bus.desabonner(f)

    # --- Fichiers statiques --------------------------------------------------
    def _fichier(self, chemin: str) -> None:
        nom = "index.html" if chemin in ("/", "") else chemin.lstrip("/")
        cible = (RACINE_WEB / nom).resolve()
        # Un chemin qui remonte hors de web/ est refusé sans discussion.
        if RACINE_WEB not in cible.parents or not cible.is_file():
            return self._json(404, {"erreur": "introuvable"})
        brut = cible.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", TYPES.get(cible.suffix, "application/octet-stream"))
        self.send_header("Content-Length", str(len(brut)))
        self.end_headers()
        self.wfile.write(brut)


def creer_serveur(adresse: str, port: int, registre: Registre, lecteur: Lecteur,
                  bus: BusEvenements) -> ThreadingHTTPServer:
    classe = type("HandlerLie", (Handler,),
                  {"registre": registre, "lecteur": lecteur, "bus": bus})
    serveur = ThreadingHTTPServer((adresse, port), classe)
    serveur.daemon_threads = True
    return serveur
