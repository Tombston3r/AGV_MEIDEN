"""Trame applicative AGV — encodage, décodage, CRC.

Portage fidèle de `firmware/common/proto/frame.cpp` et `crc16.cpp` du cœur
métier. C'est ce qui donne son intérêt aux essais : le poste et l'AGV ne
« s'entendent » vraiment que s'ils parlent la MÊME trame, pas seulement la même
modulation. Une radio qui porte et une trame qui ne décode pas, c'est une panne
qu'on ne veut pas découvrir en atelier.

Disposition sur le fil (brief §5.1) :
    octet 0      ver (4 bits) | type (4 bits)
    octets 1-2   node_id, gros-boutiste
    octet 3      seq
    octets 4-5   station (10 bits) | speed (4 bits) | 2 bits réservés
    octet 6      flags
    [7-10]       ts_s, seulement si flags & TIMESTAMPED
    2 derniers   CRC-16/CCITT sur tout ce qui précède
"""

from dataclasses import dataclass
from enum import IntEnum

BASE_SIZE = 9
MAX_SIZE = 13


class FrameType(IntEnum):
    CMD_GOTO = 0
    CMD_STOP = 1
    ACK = 2
    TELEMETRY = 3
    PING = 4
    PAIR = 5


FLAG_PRIORITY = 0x01
FLAG_PURGE_QUEUE = 0x02
FLAG_TIMESTAMPED = 0x04
FLAG_NACK = 0x08
FLAG_RETRY = 0x10


def crc16_ccitt(data: bytes, seed: int = 0xFFFF) -> int:
    crc = seed
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


@dataclass
class Frame:
    ver: int = 1
    type: FrameType = FrameType.PING
    node_id: int = 0
    seq: int = 0
    station: int = 0
    speed: int = 0
    flags: int = 0
    ts_s: int = 0

    @property
    def timestamped(self) -> bool:
        return bool(self.flags & FLAG_TIMESTAMPED)

    def encode(self) -> bytes:
        out = bytearray()
        out.append(((self.ver & 0x0F) << 4) | (int(self.type) & 0x0F))
        out += self.node_id.to_bytes(2, "big")
        out.append(self.seq & 0xFF)
        packed = ((self.station & 0x03FF) << 6) | ((self.speed & 0x0F) << 2)
        out += packed.to_bytes(2, "big")
        out.append(self.flags & 0xFF)
        if self.timestamped:
            out += self.ts_s.to_bytes(4, "big")
        out += crc16_ccitt(bytes(out)).to_bytes(2, "big")
        return bytes(out)


class BadFrame(ValueError):
    """Trame rejetée : longueur, version ou CRC."""


def decode(raw: bytes) -> Frame:
    if len(raw) not in (BASE_SIZE, MAX_SIZE):
        raise BadFrame(f"longueur {len(raw)}, attendu {BASE_SIZE} ou {MAX_SIZE}")

    recu = int.from_bytes(raw[-2:], "big")
    calcule = crc16_ccitt(raw[:-2])
    if recu != calcule:
        raise BadFrame(f"CRC {recu:04X}, calculé {calcule:04X}")

    flags = raw[6]
    ts_attendu = bool(flags & FLAG_TIMESTAMPED)
    # La longueur DOIT concorder avec le drapeau : sans ce contrôle, une trame
    # tronquée dont le CRC tombe juste passerait pour valide.
    if ts_attendu != (len(raw) == MAX_SIZE):
        raise BadFrame("drapeau d'horodatage incohérent avec la longueur")

    packed = int.from_bytes(raw[4:6], "big")
    return Frame(
        ver=raw[0] >> 4,
        type=FrameType(raw[0] & 0x0F),
        node_id=int.from_bytes(raw[1:3], "big"),
        seq=raw[3],
        station=(packed >> 6) & 0x03FF,
        speed=(packed >> 2) & 0x0F,
        flags=flags,
        ts_s=int.from_bytes(raw[7:11], "big") if ts_attendu else 0,
    )
