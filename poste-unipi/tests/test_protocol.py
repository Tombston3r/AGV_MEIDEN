"""Tests du poste UniPi : compatibilité binaire avec le firmware et dégradations.

Le test le plus important est `test_vecteur_partage_avec_le_firmware` : il fige
un octet-à-octet identique à celui produit par le C++. Si les deux
implémentations divergent, l'AGV cesse d'obéir au poste — silencieusement.
"""

from __future__ import annotations

import time

import pytest

from agv_poste.io_backend import SimulatedBackend, UnsupportedBackend, make_backend
from agv_poste.protocol import FRAME_BASE_SIZE, Flag, Frame, FrameError, FrameType, crc16_ccitt
from agv_poste.replay import ReplayWindow, Verdict, seq_after
from agv_poste.service import ButtonBinding, PosteService, ServiceConfig
from agv_poste.transport import LoopbackTransport


def test_crc16_vecteur_de_reference() -> None:
    assert crc16_ccitt(b"123456789") == 0x29B1


def test_vecteur_partage_avec_le_firmware() -> None:
    # Même trame que `frame_aller_retour_sans_horodatage` côté natif.
    frame = Frame(
        type=FrameType.CMD_GOTO, node_id=0x1234, seq=42, station=1023, speed=15,
        flags=Flag.PRIORITY,
    )
    encoded = frame.encode()
    assert len(encoded) == FRAME_BASE_SIZE
    assert encoded[0] == 0x10  # ver=1, type=CMD_GOTO
    assert encoded[1:3] == b"\x12\x34"
    assert encoded[3] == 42
    # station 1023 (10 bits) << 6 | speed 15 << 2 = 0xFFFC
    assert encoded[4:6] == b"\xff\xfc"
    assert encoded[6] == Flag.PRIORITY

    decoded = Frame.decode(encoded)
    assert (decoded.station, decoded.speed, decoded.node_id) == (1023, 15, 0x1234)


def test_horodatage_rallonge_la_trame() -> None:
    frame = Frame(
        type=FrameType.CMD_GOTO, node_id=7, seq=1, station=5,
        flags=Flag.TIMESTAMPED, ts_s=1_700_000_000,
    )
    encoded = frame.encode()
    assert len(encoded) == 13
    assert Frame.decode(encoded).ts_s == 1_700_000_000


def test_crc_faux_rejete() -> None:
    encoded = bytearray(Frame(type=FrameType.PING, node_id=1, seq=1).encode())
    encoded[2] ^= 0x01
    with pytest.raises(FrameError, match="CRC"):
        Frame.decode(bytes(encoded))


def test_version_inattendue_rejetee() -> None:
    encoded = Frame(type=FrameType.PING, node_id=1, seq=1, ver=2).encode()
    with pytest.raises(FrameError, match="version"):
        Frame.decode(encoded, expected_version=1)
    assert Frame.decode(encoded, expected_version=2).ver == 2


def test_station_hors_bornes_refusee() -> None:
    with pytest.raises(ValueError):
        Frame(type=FrameType.CMD_GOTO, node_id=1, seq=1, station=1024).encode()


def test_seq_roulant() -> None:
    assert seq_after(1, 0)
    assert seq_after(0, 255)      # bouclage
    assert not seq_after(255, 0)
    assert not seq_after(5, 5)


def test_idempotence_et_desordre() -> None:
    window = ReplayWindow(ordered_transport=False)
    assert window.classify(1, 10) is Verdict.ACCEPT
    window.remember(1, 10)
    assert window.classify(1, 10) is Verdict.DUPLICATE
    # Un STOP arrivé après coup avec une séquence antérieure est rejeté : c'est
    # la seule protection contre un STOP qui doublerait le GOTO qu'il annule.
    assert window.classify(1, 9) is Verdict.OUT_OF_ORDER
    assert window.classify(1, 11) is Verdict.ACCEPT


def test_commande_perimee_rejetee() -> None:
    window = ReplayWindow(ordered_transport=False)
    now = 1_700_000_000
    assert window.classify(1, 1, ts_s=now - 5, now_s=now, max_age_s=15) is Verdict.ACCEPT
    assert window.classify(1, 1, ts_s=now - 180, now_s=now, max_age_s=15) is Verdict.EXPIRED


def test_appui_bouton_emet_une_commande_horodatee() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    service = PosteService(
        ServiceConfig(buttons=[ButtonBinding("1_01", station=7, speed=5)]),
        inputs,
        transport,
    )

    service.poll_once()
    assert transport.sent == []          # aucun appui

    inputs.values["1_01"] = True
    service.poll_once()
    assert len(transport.sent) == 1
    frame = Frame.decode(transport.sent[0])
    assert frame.type is FrameType.CMD_GOTO
    assert (frame.station, frame.speed) == (7, 5)
    assert frame.timestamped and frame.ts_s > 0

    # Maintien du bouton : pas de nouvelle commande (front, pas niveau).
    service.poll_once()
    assert len(transport.sent) == 1


def test_ack_ferme_la_transaction() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    service = PosteService(
        ServiceConfig(buttons=[ButtonBinding("1_01", station=3)], ack_timeout_s=0.0),
        inputs,
        transport,
    )
    inputs.values["1_01"] = True
    service.poll_once()
    sent = Frame.decode(transport.sent[0])

    transport.inbox.append(
        Frame(type=FrameType.ACK, node_id=1, seq=sent.seq).encode()
    )
    service.poll_once()
    assert service.stats.acks == 1
    # Plus rien en attente : aucune retransmission ne doit partir.
    before = len(transport.sent)
    service.poll_once()
    assert len(transport.sent) == before


def test_sans_ack_le_poste_retransmet_a_sequence_identique() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    service = PosteService(
        ServiceConfig(buttons=[ButtonBinding("1_01", station=3)], ack_timeout_s=0.0),
        inputs,
        transport,
    )
    inputs.values["1_01"] = True
    service.poll_once()
    first = Frame.decode(transport.sent[0])

    time.sleep(0.01)
    service.poll_once()
    assert service.stats.resends >= 1
    resent = Frame.decode(transport.sent[-1])
    # Même seq : côté AGV, l'idempotence évite la course en double.
    assert resent.seq == first.seq
    assert resent.flags & Flag.RETRY


def test_nack_est_compte_separement() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    service = PosteService(ServiceConfig(), inputs, transport)
    service.send_goto(4, 4)
    sent = Frame.decode(transport.sent[0])
    transport.inbox.append(
        Frame(type=FrameType.ACK, node_id=1, seq=sent.seq, flags=Flag.NACK).encode()
    )
    service.poll_once()
    assert service.stats.nacks == 1
    assert service.stats.acks == 0


def test_transport_refusant_ne_compte_pas_la_commande() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    transport.refuse = True
    service = PosteService(ServiceConfig(), inputs, transport)
    assert not service.send_goto(2, 4)
    assert service.stats.commands_sent == 0


def test_trame_illisible_est_comptee_et_ignoree() -> None:
    inputs = SimulatedBackend()
    transport = LoopbackTransport()
    service = PosteService(ServiceConfig(), inputs, transport)
    transport.inbox.append(b"\x00\x01\x02")
    service.poll_once()
    assert service.stats.frames_invalid == 1


def test_backend_inconnu_refuse_de_demarrer() -> None:
    # §12.9 : ne jamais deviner le runtime de l'UniPi.
    with pytest.raises(UnsupportedBackend):
        make_backend("mervis-peut-etre")
