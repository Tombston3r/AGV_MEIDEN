#!/usr/bin/env python3
"""Test du CONTRAT de l'API du planning, contre le vrai serveur du banc.

Lance le binaire (port éphémère), pilote l'horloge simulée, et déroule le
scénario d'exploitation complet : publication avec verrou optimiste,
validation quotidienne, mission émise à l'heure, expiration de la validation
au changement de jour, saut sur demande, gel sur heure non fiable, et
révocation de l'autorisation à la modification du planning.
"""

import json
import os
import subprocess
import sys
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

BIN = os.environ.get("BANC_API_BIN",
                     str(Path(__file__).resolve().parents[2] / "build/serveur_banc_api"))


class Serveur:
    def __init__(self):
        racine = Path(__file__).resolve().parents[2]
        self.proc = subprocess.Popen(
            [BIN, "--port", "0", "--web", str(racine / "banc_api/web")],
            stdout=subprocess.PIPE, text=True)
        ligne = self.proc.stdout.readline()
        assert "PORT=" in ligne, f"démarrage inattendu : {ligne!r}"
        self.port = int(ligne.strip().split("PORT=")[1])
        self.base = f"http://127.0.0.1:{self.port}"

    def arreter(self):
        self.proc.terminate()
        self.proc.wait(timeout=5)


SRV: Serveur


def requete(methode, chemin, corps=None, entetes=None):
    data = json.dumps(corps).encode() if corps is not None else None
    req = urllib.request.Request(SRV.base + chemin, data=data, method=methode,
                                 headers=entetes or {})
    try:
        with urllib.request.urlopen(req, timeout=5) as rep:
            return rep.status, json.loads(rep.read() or b"{}"), dict(rep.headers)
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read() or b"{}"), dict(exc.headers)


def attendre_missions(n, delai_s=3.0):
    fin = time.time() + delai_s
    while time.time() < fin:
        _, corps, _ = requete("GET", "/api/missions")
        if len(corps["missions"]) >= n:
            return corps["missions"]
        time.sleep(0.05)
    _, corps, _ = requete("GET", "/api/missions")
    return corps["missions"]


def motifs_du_journal(jour=None):
    """Motifs du journal, restreints à une date locale si demandé — les
    événements produits au démarrage (heure réelle) ne comptent pas."""
    _, corps, _ = requete("GET", "/api/journal")
    return [e["motif"] for e in corps["evenements"]
            if jour is None or e["quand"].startswith(jour)]


PLANNING = {
    "schema": 1,
    "entrees": [
        {"id": "livraison-matin", "heure": "06:00", "jours": 127, "station": 42},
    ],
}


class TestContratApi(unittest.TestCase):
    """L'ordre des tests EST le scénario : ils partagent le même serveur."""

    def test_01_planning_vide_et_etag_initial(self):
        code, corps, entetes = requete("GET", "/api/planning")
        self.assertEqual(code, 200)
        self.assertEqual(corps["entrees"], [])
        self.assertEqual(entetes["ETag"], '"v1"')

    def test_02_put_sans_if_match_refuse(self):
        code, corps, _ = requete("PUT", "/api/planning", PLANNING)
        self.assertEqual(code, 428)
        self.assertIn("If-Match", corps["erreur"])

    def test_03_put_version_perimee_409(self):
        code, corps, _ = requete("PUT", "/api/planning", PLANNING,
                                 {"If-Match": '"v99"'})
        self.assertEqual(code, 409)
        self.assertIn("perimee", corps["erreur"])

    def test_04_put_valide_et_json_invalide(self):
        code, corps, _ = requete("PUT", "/api/planning",
                                 {"schema": 1, "entrees": [
                                     {"id": "x", "heure": "06:00", "station": 4096}]},
                                 {"If-Match": '"v1"'})
        self.assertEqual(code, 400)
        self.assertIn("station hors bornes", corps["erreur"])

        code, _, entetes = requete("PUT", "/api/planning", PLANNING,
                                   {"If-Match": '"v1"'})
        self.assertEqual(code, 200)
        self.assertEqual(entetes["ETag"], '"v2"')

    def test_05_mission_part_a_l_heure_une_seule_fois(self):
        requete("POST", "/api/sim/heure", {"locale": "2026-09-01 05:59"})
        code, corps, _ = requete("GET", "/api/planning/next")
        self.assertEqual(code, 200)
        self.assertEqual(corps["occurrences"][0]["locale"], "2026-09-01 06:00:00")

        # Sans validation, rien ne doit partir.
        requete("POST", "/api/sim/avancer", {"secondes": 120})
        self.assertEqual(len(attendre_missions(1, 0.5)), 0)

        # Validée dans la fenêtre de grâce : la mission part, une fois.
        requete("POST", "/api/planning/validate", {"par": "pytest"})
        missions = attendre_missions(1)
        self.assertEqual(len(missions), 1)
        self.assertEqual(missions[0]["station"], 42)
        requete("POST", "/api/sim/avancer", {"secondes": 300})
        self.assertEqual(len(attendre_missions(2, 0.5)), 1)  # idempotence

    def test_06_la_validation_expire_a_minuit(self):
        requete("POST", "/api/sim/heure", {"locale": "2026-09-02 05:59"})
        _, corps, _ = requete("GET", "/api/time")
        self.assertFalse(corps["journee_validee"])  # celle d'hier ne vaut plus
        requete("POST", "/api/sim/avancer", {"secondes": 600})
        self.assertEqual(len(attendre_missions(2, 0.8)), 1)
        self.assertIn("sautee : journee non validee", motifs_du_journal("2026-09-02"))

    def test_07_saut_sur_demande(self):
        requete("POST", "/api/sim/heure", {"locale": "2026-09-03 05:59"})
        requete("POST", "/api/planning/validate", {"par": "pytest"})
        requete("POST", "/api/planning/skip")
        requete("POST", "/api/sim/avancer", {"secondes": 120})
        self.assertEqual(len(attendre_missions(2, 0.8)), 1)
        self.assertIn("sautee : sur demande operateur", motifs_du_journal("2026-09-03"))

    def test_08_heure_non_fiable_gele_puis_reprend(self):
        requete("POST", "/api/sim/heure", {"locale": "2026-09-04 05:59"})
        requete("POST", "/api/planning/validate", {"par": "pytest"})
        requete("POST", "/api/sim/fiable", {"fiable": False})
        requete("POST", "/api/sim/avancer", {"secondes": 120})
        self.assertEqual(len(attendre_missions(2, 0.8)), 1)  # gelé
        requete("POST", "/api/sim/fiable", {"fiable": True})
        self.assertEqual(len(attendre_missions(2)), 2)  # dans la grâce : part
        self.assertIn("heure non fiable : planning gele", motifs_du_journal("2026-09-04"))

    def test_09_modifier_le_planning_revoque_la_validation(self):
        requete("POST", "/api/sim/heure", {"locale": "2026-09-05 05:59"})
        requete("POST", "/api/planning/validate", {"par": "pytest"})
        _, _, entetes = requete("GET", "/api/planning")
        code, _, _ = requete("PUT", "/api/planning", PLANNING,
                             {"If-Match": entetes["ETag"]})
        self.assertEqual(code, 200)
        _, corps, _ = requete("GET", "/api/time")
        self.assertFalse(corps["journee_validee"])  # à revalider après édition

    def test_10_pause_visible_dans_l_etat(self):
        requete("POST", "/api/planning/pause", {"actif": True})
        _, corps, _ = requete("GET", "/api/time")
        self.assertTrue(corps["pause"])
        requete("POST", "/api/planning/pause", {"actif": False})


if __name__ == "__main__":
    SRV = Serveur()
    try:
        unittest.main(verbosity=1)
    finally:
        SRV.arreter()
