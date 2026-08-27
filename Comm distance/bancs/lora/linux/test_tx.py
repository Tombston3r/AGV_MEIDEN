#!/usr/bin/env python3
"""Essai d'ÉMISSION LoRa depuis le poste fixe Linux.

Émet des trames applicatives réelles (`CmdGoto`) et attend l'accusé du pair.
En face, faire tourner `test_rx.py` sur l'autre poste, ou l'environnement
`rx` du dossier `../esp32/`.

    ./test_tx.py --count 10 --station 42
    ./test_tx.py --count 200 --interval 0.5 --no-ack   # essai d'endurance

Le budget de rapport cyclique est appliqué : le script REFUSE d'émettre
au-delà de 1 % sur une heure glissante. C'est une obligation réglementaire
(EN 300 220 / ERC 70-03), pas une précaution : un essai d'endurance est
justement le moment où l'on risque de la franchir sans s'en rendre compte.
"""

import argparse
import sys
import time
from collections import deque

import agv_frame as proto
import sx1276


class DutyCycle:
    """Budget 1 % sur une heure glissante, identique à `duty_cycle.cpp`."""

    def __init__(self, permille=10, window_s=3600.0):
        self.budget_s = window_s * permille / 1000.0
        self.window_s = window_s
        self._emissions = deque()   # (instant, durée_s)

    def _purge(self, now):
        while self._emissions and now - self._emissions[0][0] > self.window_s:
            self._emissions.popleft()

    def used_s(self, now):
        self._purge(now)
        return sum(d for _, d in self._emissions)

    def allows(self, duree_s, now):
        return self.used_s(now) + duree_s <= self.budget_s

    def record(self, duree_s, now):
        self._emissions.append((now, duree_s))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--count", type=int, default=10, help="nombre de trames (défaut 10)")
    ap.add_argument("--interval", type=float, default=2.0, help="secondes entre trames")
    ap.add_argument("--station", type=int, default=42, help="station visée, 0-1023")
    ap.add_argument("--node-id", type=lambda x: int(x, 0), default=0x0001)
    ap.add_argument("--speed", type=int, default=2, help="consigne de vitesse, 0-15")
    ap.add_argument("--no-ack", action="store_true", help="ne pas attendre l'accusé")
    ap.add_argument("--ack-timeout", type=float, default=0.4)
    ap.add_argument("--sf", type=int, default=9, help="facteur d'étalement, 7-12")
    ap.add_argument("--spi-bus", type=int, default=0)
    ap.add_argument("--spi-cs", type=int, default=0)
    ap.add_argument("--reset-pin", type=int, default=None)
    args = ap.parse_args()

    cfg = sx1276.LoraConfig(spreading_factor=args.sf)
    radio = sx1276.Sx1276(cfg, spi_bus=args.spi_bus, spi_cs=args.spi_cs,
                          reset_pin=args.reset_pin)

    try:
        version = radio.begin()
    except sx1276.RadioUnavailable as exc:
        print(f"[ÉCHEC] {exc}", file=sys.stderr)
        return 2

    attendu_ms = sx1276.airtime_ms(proto.BASE_SIZE, cfg)
    print(f"RFM95W détecté (RegVersion 0x{version:02X})")
    print(f"{cfg.frequency_hz / 1e6:.1f} MHz  SF{cfg.spreading_factor}  "
          f"BW{cfg.bandwidth_hz // 1000}  CR4/{cfg.coding_rate}  "
          f"sync 0x{cfg.sync_word:02X}  {cfg.tx_power_dbm} dBm")
    print(f"temps d'antenne calculé : {attendu_ms:.1f} ms par trame "
          f"→ {int(3600_000 * 0.01 / attendu_ms)} émissions/heure au maximum légal\n")

    duty = DutyCycle()
    envoyees = acquittees = refusees = 0
    latences = []

    for i in range(args.count):
        now = time.monotonic()
        if not duty.allows(attendu_ms / 1000.0, now):
            refusees += 1
            print(f"[{i:3}] REFUSÉ : budget de rapport cyclique épuisé "
                  f"({duty.used_s(now):.1f} s utilisées sur {duty.budget_s:.1f} s)")
            time.sleep(args.interval)
            continue

        trame = proto.Frame(type=proto.FrameType.CMD_GOTO, node_id=args.node_id,
                            seq=i & 0xFF, station=args.station, speed=args.speed)
        brut = trame.encode()

        mesure_ms = radio.transmit(brut)
        duty.record(mesure_ms / 1000.0, time.monotonic())
        envoyees += 1
        ligne = (f"[{i:3}] émise seq={trame.seq:3} station={trame.station:4} "
                 f"{len(brut)} o  {mesure_ms:6.1f} ms")

        if args.no_ack:
            print(ligne)
        else:
            radio.listen()
            limite = time.monotonic() + args.ack_timeout
            recu = None
            while time.monotonic() < limite:
                paquet = radio.receive()
                if paquet and paquet[0]:
                    recu = paquet
                    break
                time.sleep(0.002)

            if recu is None:
                print(ligne + "   → PAS D'ACCUSÉ")
            else:
                charge, rssi, snr = recu
                try:
                    ack = proto.decode(charge)
                except proto.BadFrame as exc:
                    print(ligne + f"   → réponse illisible : {exc}")
                else:
                    if ack.type == proto.FrameType.ACK and ack.seq == trame.seq:
                        latence = (args.ack_timeout - (limite - time.monotonic())) * 1000
                        latences.append(latence)
                        acquittees += 1
                        print(ligne + f"   → ACK  {rssi} dBm  SNR {snr:+.1f} dB")
                    else:
                        print(ligne + f"   → trame inattendue : {ack.type.name} seq={ack.seq}")

        time.sleep(args.interval)

    print(f"\n--- Bilan ---")
    print(f"émises      : {envoyees}")
    print(f"refusées    : {refusees}  (budget de rapport cyclique)")
    if not args.no_ack:
        taux = 100.0 * acquittees / envoyees if envoyees else 0.0
        print(f"acquittées  : {acquittees}  ({taux:.1f} %)")
        if latences:
            print(f"latence     : min {min(latences):.0f} ms  "
                  f"moy {sum(latences) / len(latences):.0f} ms  max {max(latences):.0f} ms")
    print(f"budget      : {duty.used_s(time.monotonic()):.1f} s utilisées "
          f"sur {duty.budget_s:.1f} s autorisées par heure")

    radio.close()
    return 0 if envoyees and (args.no_ack or acquittees) else 1


if __name__ == "__main__":
    sys.exit(main())
