"""Lecture du dongle EnOcean USB sur la UniPi.

Le dongle (USB 300 / TCM 515U) se présente comme un port série FTDI à
57 600 bauds. Deux réalisations derrière la même interface :

- `SerialDongle` : le vrai port, via `pyserial` ;
- `FakeDongle`   : injecte des octets, pour éprouver le banc sans matériel.

C'est ce qui permet de tester tout le reste sur un poste de développement.
"""

from __future__ import annotations

import glob
import logging
from typing import Iterable, Protocol

LOG = logging.getLogger(__name__)

BAUD = 57600
# Le dongle EnOcean se présente en FTDI ; sur une UniPi, /dev/ttyUSB* est
# occupé par d'autres périphériques, d'où la recherche par identifiant stable.
MOTIFS = (
    "/dev/serial/by-id/*EnOcean*",
    "/dev/serial/by-id/*FT232*",
    "/dev/ttyUSB*",
)


def detecter_port() -> str | None:
    """Premier port plausible, en préférant les identifiants stables.

    `/dev/ttyUSB0` change de numéro au gré des rebranchements ; un lien
    `by-id` ne bouge pas. On ne retient le nom court qu'en dernier recours.
    """
    for motif in MOTIFS:
        trouves = sorted(glob.glob(motif))
        if trouves:
            return trouves[0]
    return None


class Dongle(Protocol):
    def lire(self) -> bytes:
        """Octets disponibles, éventuellement vide. Ne doit pas bloquer longtemps."""
        ...

    def fermer(self) -> None: ...


class SerialDongle:
    def __init__(self, port: str | None = None, baud: int = BAUD) -> None:
        import serial  # dépendance optionnelle : absente sur un poste de dev

        self.port = port or detecter_port()
        if self.port is None:
            raise FileNotFoundError(
                "aucun dongle EnOcean détecté. Vérifier le branchement USB, "
                "puis `ls /dev/serial/by-id/`."
            )
        # timeout court : la boucle de lecture doit rester réactive à l'arrêt.
        self._ser = serial.Serial(self.port, baud, timeout=0.05)
        LOG.info("dongle EnOcean ouvert sur %s à %d bauds", self.port, baud)

    def lire(self) -> bytes:
        n = self._ser.in_waiting or 1
        return self._ser.read(n)

    def fermer(self) -> None:
        self._ser.close()


class FakeDongle:
    """Dongle de simulation : on lui pousse des octets, il les rend."""

    def __init__(self) -> None:
        self._tampon = bytearray()
        self.port = "fake"

    def injecter(self, octets: Iterable[int] | bytes) -> None:
        self._tampon.extend(bytes(octets))

    def lire(self) -> bytes:
        out = bytes(self._tampon)
        self._tampon.clear()
        return out

    def fermer(self) -> None:
        self._tampon.clear()


def trame_rps(sender_id: int, data: int = 0x30, status: int = 0x30,
              rssi_dbm: int = -60) -> bytes:
    """Construit une trame ESP3 RADIO_ERP1/RPS complète.

    Sert au `FakeDongle` et aux tests : c'est exactement ce qu'émet un PTM 210.
    """
    from .esp3 import crc8

    payload = bytes([0xF6, data]) + sender_id.to_bytes(4, "big") + bytes([status])
    opt = bytes([0x03]) + b"\xff\xff\xff\xff" + bytes([abs(rssi_dbm)]) + bytes([0x00])
    entete = len(payload).to_bytes(2, "big") + bytes([len(opt), 0x01])
    corps = payload + opt
    return bytes([0x55]) + entete + bytes([crc8(entete)]) + corps + bytes([crc8(corps)])
