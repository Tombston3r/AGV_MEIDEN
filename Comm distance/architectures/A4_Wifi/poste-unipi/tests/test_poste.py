"""Tests du poste fixe UniPi : chaîne EnOcean -> MQTT (planification §3)."""

from __future__ import annotations

import json

from agv_poste.esp3 import Deduplicator, Esp3Decoder, crc8, parse_rps
from agv_poste.service import PosteService, ServiceConfig


def rps_frame(sender_id: int, data: int = 0x10, rssi: int = 60) -> bytes:
    """Construit une trame ESP3 RADIO_ERP1 portant un télégramme RPS."""
    payload = bytes([0xF6, data]) + sender_id.to_bytes(4, "big") + bytes([0x30])
    opt = bytes([0x03, 0xFF, 0xFF, 0xFF, 0xFF, rssi, 0x00])
    header = bytes([len(payload) >> 8, len(payload) & 0xFF, len(opt), 0x01])
    return bytes([0x55]) + header + bytes([crc8(header)]) + payload + opt + bytes(
        [crc8(payload + opt)]
    )


class Recorder:
    def __init__(self, fail: bool = False) -> None:
        self.messages: list[tuple[str, str, bool]] = []
        self.fail = fail

    def __call__(self, topic: str, payload: str, retain: bool) -> bool:
        if self.fail:
            return False
        self.messages.append((topic, payload, retain))
        return True

    def commands(self) -> list[dict]:
        return [json.loads(p) for t, p, _ in self.messages if t.endswith("/cmd")]


def make_service(**kwargs) -> tuple[PosteService, Recorder]:
    recorder = Recorder(fail=kwargs.pop("fail", False))
    return PosteService(ServiceConfig(**kwargs), recorder), recorder


# --- Décodeur ESP3 ---------------------------------------------------------

def test_crc8_vecteur_de_reference() -> None:
    assert crc8(b"123456789") == 0xF4


def test_decode_telegramme_rps_complet() -> None:
    decoder = Esp3Decoder()
    packet = None
    for byte in rps_frame(0x0189ABCD):
        packet = decoder.feed(byte) or packet
    assert packet is not None
    rps = parse_rps(packet)
    assert rps is not None
    assert rps.sender_id == 0x0189ABCD
    assert rps.pressed
    assert rps.rssi_dbm == -60


def test_crc_header_faux_est_rejete_puis_resynchronise() -> None:
    decoder = Esp3Decoder()
    corrupted = bytearray(rps_frame(0x11223344))
    corrupted[5] ^= 0xFF
    for byte in corrupted:
        decoder.feed(byte)
    assert decoder.crc_header_errors == 1

    packet = None
    for byte in rps_frame(0x11223344):
        packet = decoder.feed(byte) or packet
    assert packet is not None


def test_crc_data_faux_est_rejete() -> None:
    decoder = Esp3Decoder()
    corrupted = bytearray(rps_frame(0x55667788))
    corrupted[-1] ^= 0x5A
    packet = None
    for byte in corrupted:
        packet = decoder.feed(byte) or packet
    assert packet is None
    assert decoder.crc_data_errors == 1


def test_deduplication_des_trois_sous_telegrammes() -> None:
    dedup = Deduplicator(window_ms=100)
    assert dedup.accept(0x1234, 0x10, 1000)
    assert not dedup.accept(0x1234, 0x10, 1020)
    assert not dedup.accept(0x1234, 0x10, 1045)
    assert dedup.duplicates == 2
    assert dedup.accept(0x1234, 0x10, 1200)


# --- Service ---------------------------------------------------------------

def press(service: PosteService, sender_id: int, data: int = 0x10, now_ms: float = 0.0) -> None:
    for byte in rps_frame(sender_id, data):
        service.feed_enocean(byte, now_ms)


def test_appui_appaire_publie_une_commande_horodatee() -> None:
    service, recorder = make_service()
    service.start_pairing(7, 5)
    press(service, 0x0189ABCD, now_ms=0)
    assert service.stats.pairings_done == 1
    assert recorder.commands() == []  # l'appui d'appairage ne roule pas

    press(service, 0x0189ABCD, now_ms=5000)
    commands = recorder.commands()
    assert len(commands) == 1
    assert commands[0]["dest"] == 7
    assert commands[0]["speed"] == 5
    assert commands[0]["ts"] > 0  # horodatage obligatoire pour la péremption


def test_trois_sous_telegrammes_ne_donnent_qu_une_commande() -> None:
    service, recorder = make_service()
    service.start_pairing(3)
    press(service, 0xAA, now_ms=0)
    for i in range(3):
        press(service, 0xAA, now_ms=1000 + i * 20)
    assert len(recorder.commands()) == 1
    assert service.stats.duplicates == 2


def test_bouton_non_appaire_ne_declenche_rien() -> None:
    service, recorder = make_service()
    press(service, 0xDEADBEEF)
    assert recorder.commands() == []
    assert service.stats.unpaired == 1


def test_chaque_appui_est_trace() -> None:
    # Topic poste/1/button/<id> : traçabilité demandée par la planification §2.
    service, recorder = make_service()
    press(service, 0x12345678)
    topics = [t for t, _, _ in recorder.messages]
    assert "poste/1/button/12345678" in topics


def test_sequence_croissante_et_roulante() -> None:
    service, recorder = make_service()
    for _ in range(3):
        service.send_goto(1)
    seqs = [c["seq"] for c in recorder.commands()]
    assert seqs == [1, 2, 3]


def test_echec_de_publication_est_compte() -> None:
    service, _ = make_service(fail=True)
    assert not service.send_goto(4)
    assert service.stats.publish_failures == 1
    assert service.stats.commands_sent == 0


def test_etat_recu_puis_perime() -> None:
    # L'indicateur de fraîcheur est le plus important de l'IHM : un état figé
    # tromperait l'opérateur (planification §3.4).
    service, _ = make_service(state_stale_s=5.0)
    assert not service.state_is_fresh(now=100.0)

    service.on_mqtt_message("agv/1/state", json.dumps({"station": 4, "moving": True}), now=100.0)
    assert service.state_is_fresh(now=102.0)
    assert not service.state_is_fresh(now=110.0)
    assert service.snapshot(now=110.0)["fresh"] is False


def test_ack_et_nack_sont_distingues() -> None:
    service, _ = make_service()
    service.on_mqtt_message("agv/1/ack", json.dumps({"seq": 1, "ok": True}))
    service.on_mqtt_message("agv/1/ack", json.dumps({"seq": 2, "ok": False, "status": 3}))
    assert service.stats.acks == 1
    assert service.stats.nacks == 1


def test_charge_utile_illisible_est_ignoree() -> None:
    service, _ = make_service()
    service.on_mqtt_message("agv/1/state", "pas du json")
    assert service.stats.states_received == 0


def test_appairage_persiste_et_se_recharge(tmp_path) -> None:
    path = tmp_path / "pairings.json"
    service, _ = make_service(pairings_path=path)
    service.start_pairing(11, 6)
    press(service, 0x01020304)
    assert path.exists()

    reloaded, _ = make_service(pairings_path=path)
    assert len(reloaded.pairings) == 1
    assert reloaded.pairings[(0x01020304, 0)].station == 11


def test_table_d_appairage_corrompue_ne_charge_rien(tmp_path) -> None:
    path = tmp_path / "pairings.json"
    path.write_text("{ceci n'est pas du json")
    service, _ = make_service(pairings_path=path)
    assert service.pairings == {}


def test_stop_avec_purge() -> None:
    service, recorder = make_service()
    service.send_stop(purge=True)
    command = recorder.commands()[0]
    assert command["stop"] == 1
    assert command["purge"] == 1
