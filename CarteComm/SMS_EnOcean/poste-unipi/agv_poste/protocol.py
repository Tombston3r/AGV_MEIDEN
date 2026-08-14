"""Trame applicative AGV — implémentation Python du protocole du brief §5.1.

Miroir strict de `firmware/common/proto/frame.h`. Toute évolution doit être
faite des deux côtés le même jour : le champ `ver` existe pour que la version
soit rejetée proprement plutôt que mal interprétée.

Disposition sur le fil (identique au C++) :
    octet 0      : ver (4 bits) | type (4 bits)
    octets 1-2   : node_id (16 bits, big endian)
    octet 3      : seq
    octets 4-5   : station (10 bits) | speed (4 bits) | 2 bits réservés
    octet 6      : flags
    [octets 7-10]: ts_s (32 bits), si flags & TIMESTAMPED
    2 derniers   : CRC-16/CCITT-FALSE
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

FRAME_BASE_SIZE = 9
FRAME_MAX_SIZE = 13


class FrameType(IntEnum):
    CMD_GOTO = 0
    CMD_STOP = 1
    ACK = 2
    TELEMETRY = 3
    PING = 4
    PAIR = 5


class Flag(IntEnum):
    PRIORITY = 0x01
    PURGE_QUEUE = 0x02
    TIMESTAMPED = 0x04
    NACK = 0x08
    RETRY = 0x10
    STATUS_MOVING = 0x20
    STATUS_IN_STATION = 0x40
    STATUS_FAULT = 0x80


class FrameError(Exception):
    """Trame illisible : CRC faux, longueur invalide, version inattendue."""


def crc16_ccitt(data: bytes, seed: int = 0xFFFF) -> int:
    """CRC-16/CCITT-FALSE, identique à `crc16_ccitt()` côté firmware."""
    crc = seed
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


@dataclass(slots=True)
class Frame:
    type: FrameType
    node_id: int
    seq: int
    station: int = 0
    speed: int = 0
    flags: int = 0
    ts_s: int = 0
    ver: int = 1

    @property
    def timestamped(self) -> bool:
        return bool(self.flags & Flag.TIMESTAMPED)

    def encode(self) -> bytes:
        if not 0 <= self.station <= 0x3FF:
            raise ValueError("station hors bornes (10 bits)")
        if not 0 <= self.speed <= 0x0F:
            raise ValueError("vitesse hors bornes (4 bits)")

        body = bytearray()
        body.append(((self.ver & 0x0F) << 4) | (int(self.type) & 0x0F))
        body += struct.pack(">H", self.node_id & 0xFFFF)
        body.append(self.seq & 0xFF)
        body += struct.pack(">H", ((self.station & 0x3FF) << 6) | ((self.speed & 0x0F) << 2))
        body.append(self.flags & 0xFF)
        if self.timestamped:
            body += struct.pack(">I", self.ts_s & 0xFFFFFFFF)
        body += struct.pack(">H", crc16_ccitt(bytes(body)))
        return bytes(body)

    @classmethod
    def decode(cls, data: bytes, expected_version: int | None = 1) -> Frame:
        if len(data) < FRAME_BASE_SIZE:
            raise FrameError("trame trop courte")
        flags = data[6]
        expected = FRAME_MAX_SIZE if flags & Flag.TIMESTAMPED else FRAME_BASE_SIZE
        if len(data) != expected:
            raise FrameError(f"longueur {len(data)} incohérente avec les flags (attendu {expected})")
        if crc16_ccitt(data[:-2]) != struct.unpack(">H", data[-2:])[0]:
            raise FrameError("CRC invalide")

        ver = data[0] >> 4
        if expected_version is not None and ver != expected_version:
            raise FrameError(f"version {ver} inattendue")
        raw_type = data[0] & 0x0F
        try:
            frame_type = FrameType(raw_type)
        except ValueError as exc:
            raise FrameError(f"type inconnu {raw_type}") from exc

        packed = struct.unpack(">H", data[4:6])[0]
        ts_s = struct.unpack(">I", data[7:11])[0] if flags & Flag.TIMESTAMPED else 0
        return cls(
            type=frame_type,
            node_id=struct.unpack(">H", data[1:3])[0],
            seq=data[3],
            station=(packed >> 6) & 0x3FF,
            speed=(packed >> 2) & 0x0F,
            flags=flags,
            ts_s=ts_s,
            ver=ver,
        )
