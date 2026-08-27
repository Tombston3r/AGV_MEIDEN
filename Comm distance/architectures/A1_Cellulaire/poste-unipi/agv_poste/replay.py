"""Idempotence, anti-désordre et fraîcheur : miroir de `replay_window.h`.

Sur transport SMS, ces trois contrôles sont la seule protection contre :
  - un ACK perdu qui déclencherait une course en double (idempotence) ;
  - un STOP arrivé avant le GOTO qu'il annule (désordre) ;
  - une commande sortie du SMSC trois minutes plus tard (péremption).
"""

from __future__ import annotations

from collections import deque
from enum import Enum


class Verdict(Enum):
    ACCEPT = "accept"
    DUPLICATE = "duplicate"
    OUT_OF_ORDER = "out_of_order"
    EXPIRED = "expired"


def seq_after(a: int, b: int) -> bool:
    """`a` est-il postérieur à `b` sur un compteur 8 bits roulant ?"""
    return ((a - b) & 0xFF) != 0 and ((a - b) & 0xFF) < 0x80


class ReplayWindow:
    def __init__(self, window: int = 16, ordered_transport: bool = True) -> None:
        self._seen: deque[tuple[int, int]] = deque(maxlen=window)
        self._last_seq: dict[int, int] = {}
        self._ordered = ordered_transport

    def classify(
        self,
        node_id: int,
        seq: int,
        *,
        ts_s: int | None = None,
        now_s: int = 0,
        max_age_s: int = 0,
    ) -> Verdict:
        # 1. Doublon : prioritaire, y compris sur une trame périmée, un
        #    ré-envoi tardif d'une commande déjà exécutée doit être ré-acquitté.
        if (node_id, seq) in self._seen:
            return Verdict.DUPLICATE

        if ts_s is not None and max_age_s:
            age = max(0, now_s - ts_s)
            if age > max_age_s:
                return Verdict.EXPIRED

        if not self._ordered:
            last = self._last_seq.get(node_id)
            if last is not None and not seq_after(seq, last):
                return Verdict.OUT_OF_ORDER

        return Verdict.ACCEPT

    def remember(self, node_id: int, seq: int) -> None:
        """À n'appeler qu'APRÈS exécution effective de la commande."""
        self._seen.append((node_id, seq))
        last = self._last_seq.get(node_id)
        if last is None or seq_after(seq, last):
            self._last_seq[node_id] = seq
