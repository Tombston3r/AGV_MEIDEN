"""Accès aux entrées TOR de l'UniPi E413.

⚠ POINT OUVERT §12.9 — LE RUNTIME RÉELLEMENT DISPONIBLE SUR LA RÉFÉRENCE
COMMANDÉE N'EST PAS CONNU. Deux mondes incompatibles coexistent :

  * Mervis IDE (OS propriétaire) : pas d'exécution Python arbitraire ;
    l'intégration passerait par Modbus TCP exposé par l'automate.
  * Linux + API E/S (Evok, ou sysfs Unipi) : service systemd Python possible.

Aucun de ces chemins n'est présupposé. Le backend est choisi explicitement par
la configuration ; si aucun n'est déclaré, le service refuse de démarrer plutôt
que de deviner — un poste d'appel qui « croit » lire un bouton est pire qu'un
poste qui ne démarre pas.

La question à poser au client AVANT d'écrire le code d'accès aux E/S est dans
docs/questions_ouvertes.md.
"""

from __future__ import annotations

import abc
import logging
from typing import Protocol

logger = logging.getLogger(__name__)


class DigitalInputs(Protocol):
    """Lecture des entrées TOR (boutons d'appel câblés)."""

    def read(self, channel: str) -> bool: ...
    def close(self) -> None: ...


class UnsupportedBackend(RuntimeError):
    """Runtime non déterminé : voir §12.9."""


class EvokBackend:
    """UniPi sous Linux avec Evok (API REST locale)."""

    def __init__(self, base_url: str = "http://127.0.0.1:8080") -> None:
        import requests  # importé tardivement : dépendance optionnelle

        self._session = requests.Session()
        self._base = base_url.rstrip("/")

    def read(self, channel: str) -> bool:
        response = self._session.get(f"{self._base}/rest/di/{channel}", timeout=1.0)
        response.raise_for_status()
        return bool(response.json().get("value"))

    def close(self) -> None:
        self._session.close()


class ModbusBackend:
    """UniPi sous Mervis : les E/S sont exposées en Modbus TCP."""

    def __init__(self, host: str, port: int = 502, unit: int = 1) -> None:
        from pymodbus.client import ModbusTcpClient  # dépendance optionnelle

        self._client = ModbusTcpClient(host, port=port)
        self._unit = unit
        if not self._client.connect():
            raise UnsupportedBackend(f"Modbus TCP injoignable sur {host}:{port}")

    def read(self, channel: str) -> bool:
        address = int(channel)
        result = self._client.read_discrete_inputs(address, count=1, slave=self._unit)
        if result.isError():
            raise UnsupportedBackend(f"lecture Modbus refusée sur l'entrée {address}")
        return bool(result.bits[0])

    def close(self) -> None:
        self._client.close()


class SimulatedBackend:
    """Backend de mise au point : les entrées sont pilotées par le test."""

    def __init__(self) -> None:
        self.values: dict[str, bool] = {}

    def read(self, channel: str) -> bool:
        return self.values.get(channel, False)

    def close(self) -> None:
        self.values.clear()


def make_backend(kind: str, **kwargs: object) -> DigitalInputs:
    """Fabrique le backend déclaré. Aucun choix implicite (§12.9)."""
    if kind == "evok":
        return EvokBackend(**kwargs)  # type: ignore[arg-type]
    if kind == "modbus":
        return ModbusBackend(**kwargs)  # type: ignore[arg-type]
    if kind == "simulated":
        return SimulatedBackend()
    raise UnsupportedBackend(
        f"backend d'E/S inconnu : {kind!r}. Le runtime de l'UniPi E413 commandé "
        "n'est pas tranché (§12.9) — poser la question avant de configurer."
    )


class _Unused(abc.ABC):
    """Réservé : ne pas utiliser."""
