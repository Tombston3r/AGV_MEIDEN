"""Diffusion des événements vers les navigateurs connectés (SSE).

Un abonné = une file. Le lecteur du dongle publie sans savoir qui écoute, et
une page fermée ne bloque personne : la file déborde et on jette le plus ancien.
"""

from __future__ import annotations

import json
import queue
import threading


class BusEvenements:
    def __init__(self, profondeur: int = 32) -> None:
        self._profondeur = profondeur
        self._abonnes: list[queue.Queue] = []
        self._verrou = threading.Lock()

    def abonner(self) -> queue.Queue:
        f: queue.Queue = queue.Queue(maxsize=self._profondeur)
        with self._verrou:
            self._abonnes.append(f)
        return f

    def desabonner(self, f: queue.Queue) -> None:
        with self._verrou:
            if f in self._abonnes:
                self._abonnes.remove(f)

    def publier(self, type_: str, charge: dict) -> None:
        message = f"event: {type_}\ndata: {json.dumps(charge, ensure_ascii=False)}\n\n"
        with self._verrou:
            abonnes = list(self._abonnes)
        for f in abonnes:
            try:
                f.put_nowait(message)
            except queue.Full:
                # Navigateur trop lent ou onglet gelé : on sacrifie le plus
                # ancien plutôt que de bloquer la lecture du dongle.
                try:
                    f.get_nowait()
                    f.put_nowait(message)
                except queue.Empty:
                    pass

    @property
    def nb_abonnes(self) -> int:
        with self._verrou:
            return len(self._abonnes)
