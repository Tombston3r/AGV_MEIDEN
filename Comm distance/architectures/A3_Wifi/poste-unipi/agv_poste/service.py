"""Service du poste fixe UniPi : architecture Wi-Fi (planification §3).

Chaîne : appui EnOcean -> déduplication -> table d'appairage -> publication
MQTT `agv/<id>/cmd` -> ESP32 de la carte V5.0.1 -> ATmega -> bus MEIDEN.

Le poste héberge aussi le broker Mosquitto et l'interface de supervision ;
ceux-ci sont des services distincts, ce module ne fait que produire les
commandes et consommer l'état.
"""

from __future__ import annotations

import json
import logging
import time
from dataclasses import dataclass, field
from pathlib import Path

from .esp3 import Deduplicator, Esp3Decoder, parse_rps

logger = logging.getLogger(__name__)


@dataclass(slots=True)
class Pairing:
    """Un bouton EnOcean (identifiant usine 32 bits) -> une station."""

    enocean_id: int
    rocker: int
    station: int
    speed: int = 4


@dataclass(slots=True)
class ServiceConfig:
    agv_id: str = "1"
    dedup_window_ms: int = 100
    pairing_timeout_s: int = 60
    # Péremption appliquée par l'ESP32 : on horodate donc TOUTES les commandes.
    command_ttl_s: int = 30
    # Au-delà, l'état publié par l'AGV est considéré périmé (publication à 1 s).
    state_stale_s: float = 5.0
    pairings_path: Path | None = None


@dataclass(slots=True)
class ServiceStats:
    telegrams: int = 0
    duplicates: int = 0
    unpaired: int = 0
    commands_sent: int = 0
    publish_failures: int = 0
    acks: int = 0
    nacks: int = 0
    pairings_done: int = 0
    states_received: int = 0


class PosteService:
    """Logique du poste, indépendante du transport MQTT et du matériel série."""

    def __init__(self, config: ServiceConfig, publish) -> None:
        # `publish(topic, payload, retain) -> bool` : injecté pour rester
        # testable sans broker.
        self._cfg = config
        self._publish = publish
        self._decoder = Esp3Decoder()
        self._dedup = Deduplicator(config.dedup_window_ms)
        self._pairings: dict[tuple[int, int], Pairing] = {}
        self._pairing_target: tuple[int, int] | None = None
        self._pairing_until: float = 0.0
        self._seq = 0
        self.stats = ServiceStats()
        self.last_state: dict[str, object] = {}
        self.last_state_at: float = 0.0
        if config.pairings_path is not None:
            self.load_pairings(config.pairings_path)

    # --- Appairage ---------------------------------------------------------

    def load_pairings(self, path: Path) -> int:
        if not path.exists():
            return 0
        try:
            raw = json.loads(path.read_text())
        except (OSError, ValueError):
            # Fichier illisible : on repart sans appairage plutôt que d'inventer
            # des associations. Un bouton muet se diagnostique ; un bouton qui
            # envoie l'AGV à la mauvaise station, beaucoup moins.
            logger.exception("table d'appairage illisible : %s", path)
            return 0
        self._pairings = {
            (int(e["id"]), int(e.get("rocker", 0))): Pairing(
                int(e["id"]), int(e.get("rocker", 0)), int(e["station"]), int(e.get("speed", 4))
            )
            for e in raw.get("pairings", [])
        }
        return len(self._pairings)

    def save_pairings(self, path: Path) -> None:
        payload = {
            "pairings": [
                {"id": p.enocean_id, "rocker": p.rocker, "station": p.station, "speed": p.speed}
                for p in self._pairings.values()
            ]
        }
        path.write_text(json.dumps(payload, indent=2))

    def start_pairing(self, station: int, speed: int = 4, now: float | None = None) -> None:
        """Mode appairage : « appuyez sur le bouton à associer »."""
        self._pairing_target = (station, speed)
        self._pairing_until = (now or time.monotonic()) + self._cfg.pairing_timeout_s

    def pairing_active(self, now: float | None = None) -> bool:
        return self._pairing_target is not None and (now or time.monotonic()) < self._pairing_until

    @property
    def pairings(self) -> dict[tuple[int, int], Pairing]:
        return dict(self._pairings)

    # --- Entrée EnOcean ----------------------------------------------------

    def feed_enocean(self, byte: int, now_ms: float | None = None) -> bool:
        """Injecte un octet du TCM 515. True si un appui a été traité."""
        packet = self._decoder.feed(byte)
        if packet is None:
            return False
        rps = parse_rps(packet)
        if rps is None or not rps.pressed:
            return False

        now_ms = now_ms if now_ms is not None else time.monotonic() * 1000.0
        if not self._dedup.accept(rps.sender_id, rps.data, now_ms):
            self.stats.duplicates += 1
            return False
        self.stats.telegrams += 1

        # Traçabilité des appuis (planification §2, topic poste/<id>/button).
        self._publish(
            f"poste/1/button/{rps.sender_id:08X}",
            json.dumps({"rocker": rps.rocker, "rssi": rps.rssi_dbm, "ts": int(time.time())}),
            False,
        )

        key = (rps.sender_id, rps.rocker)
        if self.pairing_active():
            assert self._pairing_target is not None
            station, speed = self._pairing_target
            self._pairings[key] = Pairing(rps.sender_id, rps.rocker, station, speed)
            self._pairing_target = None
            self.stats.pairings_done += 1
            if self._cfg.pairings_path is not None:
                self.save_pairings(self._cfg.pairings_path)
            logger.info("bouton %08X (bascule %d) appairé à la station %d",
                        rps.sender_id, rps.rocker, station)
            return True

        pairing = self._pairings.get(key)
        if pairing is None:
            # On ne devine JAMAIS une station. L'appui est compté pour que
            # l'exploitant voie qu'un bouton non appairé est utilisé.
            self.stats.unpaired += 1
            logger.warning("appui d'un bouton non appairé : %08X (bascule %d)",
                           rps.sender_id, rps.rocker)
            return False

        return self.send_goto(pairing.station, pairing.speed)

    # --- Sortie MQTT -------------------------------------------------------

    def send_goto(self, station: int, speed: int = 4, priority: bool = False) -> bool:
        self._seq = (self._seq + 1) & 0xFF
        payload = {
            "seq": self._seq,
            "dest": station,
            "speed": speed,
            # Horodatage obligatoire : c'est lui qui permet à l'ESP32 de
            # refuser une commande retardée par le réseau (planification §2).
            "ts": int(time.time()),
        }
        if priority:
            payload["priority"] = 1
        ok = self._publish(f"agv/{self._cfg.agv_id}/cmd", json.dumps(payload), False)
        if ok:
            self.stats.commands_sent += 1
        else:
            self.stats.publish_failures += 1
            logger.error("commande station=%d NON publiée", station)
        return ok

    def send_stop(self, purge: bool = False) -> bool:
        self._seq = (self._seq + 1) & 0xFF
        payload = {"seq": self._seq, "dest": 0, "stop": 1,
                   "purge": 1 if purge else 0, "ts": int(time.time())}
        return self._publish(f"agv/{self._cfg.agv_id}/cmd", json.dumps(payload), False)

    # --- Entrée MQTT -------------------------------------------------------

    def on_mqtt_message(self, topic: str, payload: str, now: float | None = None) -> None:
        try:
            data = json.loads(payload)
        except ValueError:
            logger.warning("charge utile MQTT illisible sur %s", topic)
            return

        if topic.endswith("/state"):
            self.last_state = data
            self.last_state_at = now or time.monotonic()
            self.stats.states_received += 1
        elif topic.endswith("/ack"):
            if data.get("ok"):
                self.stats.acks += 1
            else:
                self.stats.nacks += 1
                logger.warning("commande seq=%s refusée par l'AGV (status=%s)",
                               data.get("seq"), data.get("status"))

    # --- Supervision -------------------------------------------------------

    def state_is_fresh(self, now: float | None = None) -> bool:
        """L'état affiché représente-t-il encore la réalité ?

        C'est l'indicateur le plus important de l'IHM : un état figé qui
        continue d'afficher « en station » alors que la liaison est morte
        tromperait l'opérateur.
        """
        if not self.last_state:
            return False
        return ((now or time.monotonic()) - self.last_state_at) <= self._cfg.state_stale_s

    def snapshot(self, now: float | None = None) -> dict[str, object]:
        return {
            "state": self.last_state,
            "fresh": self.state_is_fresh(now),
            "age_s": round((now or time.monotonic()) - self.last_state_at, 1)
            if self.last_state
            else None,
            "pairings": len(self._pairings),
            "pairing_active": self.pairing_active(now),
            "stats": {
                "telegrams": self.stats.telegrams,
                "duplicates": self.stats.duplicates,
                "unpaired": self.stats.unpaired,
                "commands_sent": self.stats.commands_sent,
                "publish_failures": self.stats.publish_failures,
                "acks": self.stats.acks,
                "nacks": self.stats.nacks,
                "states_received": self.stats.states_received,
            },
        }
