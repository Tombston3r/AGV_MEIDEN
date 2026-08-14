"""Décodeur EnOcean Serial Protocol 3 (ESP3) — récepteur TCM 515.

Miroir Python du décodeur C++ du dossier SMS_EnOcean. Ici il tourne sur le
poste fixe UniPi, seul endroit de l'architecture Wi-Fi où de l'EnOcean circule.

Trame ESP3 :
    0x55 | DataLen(2, BE) | OptLen(1) | PacketType(1) | CRC8H |
    Data[DataLen] | OptData[OptLen] | CRC8D

⚠ Le TCM 515 est RÉCEPTION SEULE : aucun retour n'est possible vers le bouton.
L'accusé opérateur passe donc soit par un voyant câblé au poste, soit par un
actionneur EnOcean distinct (planification §3.6 et registre des risques).
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum

SYNC = 0x55
TYPE_RADIO_ERP1 = 0x01
RORG_RPS = 0xF6
DATA_MAX = 64
OPT_MAX = 16


def crc8(data: bytes, seed: int = 0x00) -> int:
    """CRC8 EnOcean, polynôme 0x07."""
    crc = seed
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


@dataclass(slots=True)
class Packet:
    packet_type: int
    data: bytes
    opt: bytes
    rssi_dbm: int = 0


@dataclass(slots=True)
class RpsTelegram:
    sender_id: int      # identifiant 32 bits gravé en usine
    data: int
    status: int
    rssi_dbm: int
    pressed: bool       # bit 4 : energy bow armé
    rocker: int         # bits 7..5 : bascule actionnée


class _State(IntEnum):
    SYNC = 0
    HEADER = 1
    CRC_HEADER = 2
    PAYLOAD = 3
    CRC_DATA = 4


class Esp3Decoder:
    """Décodeur incrémental : un octet à la fois, resynchronisation automatique."""

    def __init__(self) -> None:
        self._state = _State.SYNC
        self._header = bytearray()
        self._payload = bytearray()
        self._data_len = 0
        self._opt_len = 0
        self._packet_type = 0
        self.crc_header_errors = 0
        self.crc_data_errors = 0
        self.packets_ok = 0

    def feed(self, byte: int) -> Packet | None:
        if self._state is _State.SYNC:
            if byte == SYNC:
                self._header.clear()
                self._state = _State.HEADER
            return None

        if self._state is _State.HEADER:
            self._header.append(byte)
            if len(self._header) == 4:
                self._state = _State.CRC_HEADER
            return None

        if self._state is _State.CRC_HEADER:
            if crc8(bytes(self._header)) != byte:
                self.crc_header_errors += 1
                self._state = _State.SYNC
                return None
            self._data_len = (self._header[0] << 8) | self._header[1]
            self._opt_len = self._header[2]
            self._packet_type = self._header[3]
            if self._data_len > DATA_MAX or self._opt_len > OPT_MAX:
                self._state = _State.SYNC
                return None
            self._payload.clear()
            self._state = _State.PAYLOAD if (self._data_len + self._opt_len) else _State.CRC_DATA
            return None

        if self._state is _State.PAYLOAD:
            self._payload.append(byte)
            if len(self._payload) == self._data_len + self._opt_len:
                self._state = _State.CRC_DATA
            return None

        # _State.CRC_DATA
        self._state = _State.SYNC
        if crc8(bytes(self._payload)) != byte:
            self.crc_data_errors += 1
            return None

        data = bytes(self._payload[: self._data_len])
        opt = bytes(self._payload[self._data_len:])
        # OptData d'un RADIO_ERP1 : [subTelNum][destinationID×4][dBm][security]
        rssi = -opt[5] if self._packet_type == TYPE_RADIO_ERP1 and len(opt) >= 6 else 0
        self.packets_ok += 1
        return Packet(self._packet_type, data, opt, rssi)


def parse_rps(packet: Packet) -> RpsTelegram | None:
    """Extrait un télégramme RPS (PTM 210). None si ce n'en est pas un."""
    if packet.packet_type != TYPE_RADIO_ERP1:
        return None
    if len(packet.data) < 7 or packet.data[0] != RORG_RPS:
        return None
    return RpsTelegram(
        sender_id=int.from_bytes(packet.data[2:6], "big"),
        data=packet.data[1],
        status=packet.data[6],
        rssi_dbm=packet.rssi_dbm,
        pressed=bool(packet.data[1] & 0x10),
        rocker=(packet.data[1] >> 5) & 0x03,
    )


class Deduplicator:
    """Le PTM 210 émet 3 sous-télégrammes identiques par appui.

    Sans ce filtre, un appui déclencherait trois courses.
    """

    def __init__(self, window_ms: int = 100, slots: int = 8) -> None:
        self._window_ms = window_ms
        self._slots: list[tuple[int, int, float]] = []
        self._max = slots
        self.duplicates = 0

    def accept(self, sender_id: int, data: int, now_ms: float) -> bool:
        for slot_id, slot_data, at in self._slots:
            if slot_id == sender_id and slot_data == data and (now_ms - at) <= self._window_ms:
                self.duplicates += 1
                return False
        self._slots.append((sender_id, data, now_ms))
        if len(self._slots) > self._max:
            self._slots.pop(0)
        return True
