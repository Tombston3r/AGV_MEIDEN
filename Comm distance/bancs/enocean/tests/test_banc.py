"""Tests du banc EnOcean, sans matériel.

Ils portent sur ce qui casse en vrai : la déduplication des sous-télégrammes,
le rejet du relâchement, l'analyse des identifiants et le cycle
d'apprentissage. Le reste est de la plomberie HTTP.
"""

from __future__ import annotations

import sys
import tempfile
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from banc_enocean.bus import BusEvenements
from banc_enocean.dongle import FakeDongle, trame_rps
from banc_enocean.esp3 import Esp3Decoder, parse_rps
from banc_enocean.lecteur import Lecteur
from banc_enocean.store import Registre, parse_id

ID_A = 0x0029B1C4
ID_B = 0xFEFF1234


def banc(tmp: Path):
    reg = Registre(tmp / "boutons.json")
    bus = BusEvenements()
    return reg, bus, Lecteur(FakeDongle(), reg, bus)


def pousser(lecteur: Lecteur, trame: bytes) -> None:
    for octet in trame:
        lecteur.traiter_octet(octet)


class TestIdentifiants(unittest.TestCase):
    def test_les_formes_imprimees_sont_toutes_acceptees(self):
        for texte in ("00:29:B1:C4", "0029B1C4", "0x0029B1C4", "00-29-b1-c4"):
            self.assertEqual(parse_id(texte), ID_A, texte)

    def test_la_lecture_est_hexadecimale_sans_exception(self):
        # « 2732996 » est un hexadécimal valide : deviner la base donnerait un
        # identifiant faux une fois sur deux.
        self.assertEqual(parse_id("2732996"), 0x02732996)

    def test_une_saisie_invalide_est_refusee_avec_un_message_utile(self):
        with self.assertRaises(ValueError) as ctx:
            parse_id("ZZZ")
        self.assertIn("ZZZ", str(ctx.exception))


class TestTelegrammes(unittest.TestCase):
    def test_une_trame_construite_se_relit(self):
        d = Esp3Decoder()
        rps = None
        for octet in trame_rps(ID_A, rssi_dbm=-72):
            paquet = d.feed(octet)
            if paquet:
                rps = parse_rps(paquet)
        self.assertIsNotNone(rps)
        self.assertEqual(rps.sender_id, ID_A)
        self.assertEqual(rps.rssi_dbm, -72)
        self.assertTrue(rps.pressed)

    def test_un_octet_parasite_ne_desynchronise_pas(self):
        d = Esp3Decoder()
        ok = 0
        for octet in b"\x12\x34" + trame_rps(ID_A) + b"\xff" + trame_rps(ID_B):
            if d.feed(octet):
                ok += 1
        self.assertEqual(ok, 2)


class TestAppuis(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.reg, self.bus, self.lec = banc(self.tmp)
        self.file = self.bus.abonner()

    def test_les_trois_sous_telegrammes_ne_font_qu_un_appui(self):
        # LE test de ce banc : sans déduplication, une pression en afficherait
        # trois, et en production déclencherait trois courses.
        self.reg.ajouter(ID_A, "Station 4")
        for _ in range(3):
            pousser(self.lec, trame_rps(ID_A))
        self.assertEqual(self.lec.appuis, 1)
        self.assertEqual(self.file.qsize(), 1)

    def test_le_relachement_n_est_pas_un_appui(self):
        self.reg.ajouter(ID_A, "Station 4")
        pousser(self.lec, trame_rps(ID_A))            # appui
        pousser(self.lec, trame_rps(ID_A, data=0x00))  # relâchement
        self.assertEqual(self.lec.appuis, 1)

    def test_deux_boutons_differents_passent_tous_les_deux(self):
        self.reg.ajouter(ID_A, "Station 4")
        self.reg.ajouter(ID_B, "Station 9")
        pousser(self.lec, trame_rps(ID_A))
        pousser(self.lec, trame_rps(ID_B))
        self.assertEqual(self.lec.appuis, 2)

    def test_un_bouton_inconnu_est_signale_pas_ignore(self):
        pousser(self.lec, trame_rps(ID_B))
        self.assertEqual(self.lec.inconnus, 1)
        self.assertIn("event: inconnu", self.file.get_nowait())

    def test_l_evenement_porte_le_nom_le_code_et_l_heure(self):
        self.reg.ajouter(ID_A, "Station 4")
        pousser(self.lec, trame_rps(ID_A))
        message = self.file.get_nowait()
        self.assertIn("event: appui", message)
        self.assertIn('"nom": "Station 4"', message)
        self.assertIn('"code": "0029B1C4"', message)
        self.assertIn('"heure"', message)


class TestApprentissage(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.reg, self.bus, self.lec = banc(self.tmp)
        self.file = self.bus.abonner()

    def test_un_appui_arme_donne_un_evenement_d_apprentissage(self):
        self.lec.armer_apprentissage(30)
        pousser(self.lec, trame_rps(ID_B))
        self.assertIn("event: apprentissage", self.file.get_nowait())

    def test_l_apprentissage_capte_meme_un_bouton_deja_connu(self):
        # Utile pour relever l'identifiant d'un bouton dont on a perdu le nom.
        self.reg.ajouter(ID_A, "Station 4")
        self.lec.armer_apprentissage(30)
        pousser(self.lec, trame_rps(ID_A))
        self.assertIn("event: apprentissage", self.file.get_nowait())

    def test_un_reamorcage_prolonge_la_fenetre(self):
        # L'interface réarme toutes les 8 s tant que la boîte d'ajout est
        # ouverte : traverser l'atelier prend plus que la fenêtre initiale.
        self.lec.armer_apprentissage(0.05)
        fin_1 = self.lec._apprentissage_jusqu_a
        self.lec.armer_apprentissage(20)
        self.assertGreater(self.lec._apprentissage_jusqu_a, fin_1)
        pousser(self.lec, trame_rps(ID_B))
        self.assertIn("event: apprentissage", self.file.get_nowait())

    def test_l_annulation_eteint_l_ecoute_immediatement(self):
        # Fermer la boîte doit rendre la main : sinon les appuis suivants
        # alimenteraient un formulaire qui n'est plus affiché.
        self.lec.armer_apprentissage(30)
        self.lec.desarmer_apprentissage()
        self.assertFalse(self.lec.apprentissage_actif())
        pousser(self.lec, trame_rps(ID_B))
        self.assertIn("event: inconnu", self.file.get_nowait())

    def test_la_fenetre_expiree_rend_la_main_au_mode_normal(self):
        self.lec.armer_apprentissage(-1)          # déjà expirée
        pousser(self.lec, trame_rps(ID_B))
        self.assertIn("event: inconnu", self.file.get_nowait())

    def test_le_dernier_appui_reste_consultable_apres_coup(self):
        # C'est le cas courant : on appuie, PUIS on ouvre la fenêtre d'ajout.
        pousser(self.lec, trame_rps(ID_B))
        dernier = self.lec.dernier_appui()
        self.assertEqual(dernier["code"], "FEFF1234")


class TestRegistre(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())

    def test_le_registre_survit_a_un_redemarrage(self):
        chemin = self.tmp / "b.json"
        Registre(chemin).ajouter(ID_A, "Station 4")
        self.assertEqual(Registre(chemin).get(ID_A).nom, "Station 4")

    def test_un_doublon_est_refuse_avec_le_nom_existant(self):
        reg = Registre(self.tmp / "b.json")
        reg.ajouter(ID_A, "Station 4")
        with self.assertRaises(ValueError) as ctx:
            reg.ajouter(ID_A, "Station 9")
        self.assertIn("Station 4", str(ctx.exception))

    def test_un_fichier_corrompu_n_empeche_pas_le_demarrage(self):
        chemin = self.tmp / "b.json"
        chemin.write_text("{ ceci n'est pas du JSON")
        self.assertEqual(Registre(chemin).tous(), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
