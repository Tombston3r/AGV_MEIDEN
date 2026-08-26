"""Service du poste fixe UniPi E413 — architecture 2 (brief §9.2).

Boucle : lecture des boutons TOR -> émission d'une commande -> attente d'ACK
applicatif -> voyant.

RAPPEL PORTÉ PAR LE CODE : sur transport SMS, la latence n'est pas bornée et
l'ordre de remise n'est pas garanti. Le service refuse donc toute commande
qu'il ne peut pas horodater, et rejette à la réception toute trame plus ancienne
que `max_command_age_s`. Voir Archi_1_Cellulaire_SMS_LTE-M.md §3.2.
"""

from __future__ import annotations

import logging
import time
from dataclasses import dataclass, field

from .io_backend import DigitalInputs
from .protocol import Flag, Frame, FrameError, FrameType
from .replay import ReplayWindow, Verdict
from .transport import Transport

logger = logging.getLogger(__name__)


@dataclass(slots=True)
class ButtonBinding:
    """Un bouton câblé -> une station."""

    channel: str
    station: int
    speed: int = 4


@dataclass(slots=True)
class ServiceConfig:
    node_id: int = 2
    poll_period_s: float = 0.05
    ack_timeout_s: float = 30.0        # SMS : générosité obligatoire
    max_command_age_s: int = 15        # §8.1
    resend_max: int = 3
    buttons: list[ButtonBinding] = field(default_factory=list)
    ordered_transport: bool = False    # SMS : aucun ordre garanti


@dataclass(slots=True)
class ServiceStats:
    commands_sent: int = 0
    acks: int = 0
    nacks: int = 0
    resends: int = 0
    expired_rx: int = 0
    out_of_order_rx: int = 0
    duplicates_rx: int = 0
    frames_invalid: int = 0


class PosteService:
    def __init__(self, config: ServiceConfig, inputs: DigitalInputs, transport: Transport) -> None:
        self._cfg = config
        self._inputs = inputs
        self._transport = transport
        self._replay = ReplayWindow(ordered_transport=config.ordered_transport)
        self._seq = 0
        self._previous: dict[str, bool] = {}
        self._pending: tuple[int, float, Frame] | None = None
        self.stats = ServiceStats()

    # --- Émission ----------------------------------------------------------

    def _next_seq(self) -> int:
        self._seq = (self._seq + 1) & 0xFF
        return self._seq

    def send_goto(self, station: int, speed: int, now_s: int | None = None) -> bool:
        now = int(time.time()) if now_s is None else now_s
        frame = Frame(
            type=FrameType.CMD_GOTO,
            node_id=self._cfg.node_id,
            seq=self._next_seq(),
            station=station,
            speed=speed,
            flags=Flag.TIMESTAMPED,
            ts_s=now,
        )
        if not self._transport.send(frame.encode()):
            logger.error("commande station=%d NON transmise", station)
            return False
        self.stats.commands_sent += 1
        self._pending = (frame.seq, time.monotonic(), frame)
        return True

    # --- Réception ---------------------------------------------------------

    def _handle(self, payload: bytes) -> None:
        try:
            frame = Frame.decode(payload)
        except FrameError as exc:
            self.stats.frames_invalid += 1
            logger.warning("trame rejetée : %s", exc)
            return

        if frame.type is FrameType.ACK:
            if self._pending is not None and frame.seq == self._pending[0]:
                self._pending = None
            if frame.flags & Flag.NACK:
                self.stats.nacks += 1
                logger.warning("commande seq=%d REFUSÉE par l'AGV", frame.seq)
            else:
                self.stats.acks += 1
            return

        verdict = self._replay.classify(
            frame.node_id,
            frame.seq,
            ts_s=frame.ts_s if frame.timestamped else None,
            now_s=int(time.time()),
            max_age_s=self._cfg.max_command_age_s,
        )
        if verdict is Verdict.EXPIRED:
            self.stats.expired_rx += 1
            logger.warning("trame périmée ignorée (seq=%d, ts=%d)", frame.seq, frame.ts_s)
            return
        if verdict is Verdict.OUT_OF_ORDER:
            self.stats.out_of_order_rx += 1
            logger.warning("trame désordonnée ignorée (seq=%d)", frame.seq)
            return
        if verdict is Verdict.DUPLICATE:
            self.stats.duplicates_rx += 1
            return
        self._replay.remember(frame.node_id, frame.seq)

    # --- Boucle ------------------------------------------------------------

    def poll_once(self) -> None:
        for button in self._cfg.buttons:
            level = self._inputs.read(button.channel)
            if level and not self._previous.get(button.channel, False):
                logger.info("appui bouton %s -> station %d", button.channel, button.station)
                self.send_goto(button.station, button.speed)
            self._previous[button.channel] = level

        for payload in self._transport.receive():
            self._handle(payload)

        self._check_pending()

    def _check_pending(self) -> None:
        if self._pending is None:
            return
        seq, sent_at, frame = self._pending
        if (time.monotonic() - sent_at) < self._cfg.ack_timeout_s:
            return
        if (frame.flags & Flag.RETRY) and self.stats.resends >= self._cfg.resend_max:
            logger.error("commande seq=%d abandonnée après %d tentatives", seq,
                         self._cfg.resend_max)
            self._pending = None
            return
        # Retransmission à SÉQUENCE IDENTIQUE : c'est l'idempotence côté AGV qui
        # empêche la course en double si l'ACK d'origine était simplement perdu.
        frame.flags |= Flag.RETRY
        frame.ts_s = int(time.time())
        if self._transport.send(frame.encode()):
            self.stats.resends += 1
            self._pending = (seq, time.monotonic(), frame)

    def run_forever(self) -> None:
        logger.info("service poste UniPi démarré (node_id=%d)", self._cfg.node_id)
        try:
            while True:
                self.poll_once()
                time.sleep(self._cfg.poll_period_s)
        finally:
            self._inputs.close()
            self._transport.close()
