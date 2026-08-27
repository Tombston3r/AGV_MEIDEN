#!/usr/bin/env python3
"""Essai de RÉCEPTION LoRa sur le poste fixe Linux.

Écoute en continu, décode les trames applicatives, mesure RSSI et SNR, et
acquitte. En face, faire tourner `test_tx.py` sur l'autre poste, ou
l'environnement `tx` du dossier `../esp32/`.

    ./test_rx.py                       # écoute et acquitte
    ./test_rx.py --no-ack --duration 60
    ./test_rx.py --survey               # relevé de portée : RSSI/SNR seuls

Le mode `--survey` sert au relevé de couverture : promener l'émetteur le long
du parcours et lire la marge. Un lien qui « passe » à −120 dBm ne passera plus
avec un chariot chargé entre les deux antennes.
"""

import argparse
import sys
import time
from collections import Counter

import agv_frame as proto
import sx1276


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--duration", type=float, default=0.0,
                    help="durée d'écoute en secondes (0 = sans limite)")
    ap.add_argument("--no-ack", action="store_true", help="écouter sans acquitter")
    ap.add_argument("--survey", action="store_true",
                    help="relevé de portée : n'affiche que RSSI/SNR")
    ap.add_argument("--node-id", type=lambda x: int(x, 0), default=0x0002,
                    help="identifiant de ce nœud dans les accusés")
    ap.add_argument("--sf", type=int, default=9)
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

    print(f"RFM95W détecté (RegVersion 0x{version:02X})")
    print(f"{cfg.frequency_hz / 1e6:.1f} MHz  SF{cfg.spreading_factor}  "
          f"BW{cfg.bandwidth_hz // 1000}  CR4/{cfg.coding_rate}  "
          f"sync 0x{cfg.sync_word:02X}")
    print("écoute… (Ctrl-C pour arrêter)\n")
    radio.listen()

    debut = time.monotonic()
    valides = rejetees = crc_radio = 0
    rssis, snrs = [], []
    motifs = Counter()
    vues = set()

    try:
        while args.duration <= 0 or time.monotonic() - debut < args.duration:
            paquet = radio.receive()
            if paquet is None:
                time.sleep(0.002)
                continue

            charge, rssi, snr = paquet
            if not charge:
                crc_radio += 1
                print("  CRC LoRa invalide : trame écartée par le modem")
                continue

            rssis.append(rssi)
            snrs.append(snr)

            if args.survey:
                print(f"  {len(charge):2} o   {rssi:5} dBm   SNR {snr:+5.1f} dB")
                continue

            try:
                trame = proto.decode(charge)
            except proto.BadFrame as exc:
                rejetees += 1
                motifs[str(exc).split(",")[0]] += 1
                print(f"  REJETÉE : {exc}   ({charge.hex(' ')})")
                continue

            valides += 1
            doublon = (trame.node_id, trame.seq) in vues
            vues.add((trame.node_id, trame.seq))
            marque = "  [DOUBLON]" if doublon else ""
            print(f"  {trame.type.name:9} node=0x{trame.node_id:04X} seq={trame.seq:3} "
                  f"station={trame.station:4} vitesse={trame.speed} "
                  f"{rssi:5} dBm  SNR {snr:+5.1f} dB{marque}")

            # Un doublon est RÉ-ACQUITTÉ sans être ré-exécuté : c'est la règle
            # d'idempotence du cœur métier. Sans elle, un accusé perdu déclenche
            # une course en double.
            if not args.no_ack and trame.type in (proto.FrameType.CMD_GOTO,
                                                  proto.FrameType.CMD_STOP,
                                                  proto.FrameType.PING):
                ack = proto.Frame(type=proto.FrameType.ACK, node_id=args.node_id,
                                  seq=trame.seq, station=trame.station)
                radio.transmit(ack.encode())
                radio.listen()

    except KeyboardInterrupt:
        print()

    ecoute = time.monotonic() - debut
    print(f"\n--- Bilan après {ecoute:.0f} s ---")
    print(f"trames valides       : {valides}")
    print(f"trames rejetées      : {rejetees}")
    for motif, n in motifs.most_common():
        print(f"    {motif} : {n}")
    print(f"CRC LoRa invalides   : {crc_radio}")
    if rssis:
        print(f"RSSI  min {min(rssis)} / moy {sum(rssis) / len(rssis):.0f} / max {max(rssis)} dBm")
        print(f"SNR   min {min(snrs):+.1f} / moy {sum(snrs) / len(snrs):+.1f} / max {max(snrs):+.1f} dB")
        if min(rssis) < -115:
            print("\n⚠️  RSSI sous −115 dBm : marge insuffisante. Un chariot chargé")
            print("    entre les deux antennes suffira à couper la liaison.")
    else:
        print("aucune trame reçue : vérifier fréquence, SF et mot de synchronisation")

    radio.close()
    return 0 if valides else 1


if __name__ == "__main__":
    sys.exit(main())
