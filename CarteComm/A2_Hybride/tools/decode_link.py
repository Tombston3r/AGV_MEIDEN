#!/usr/bin/env python3
"""Décodeur de trames de la liaison série ESP32 <-> ATmega2560.

Sert au diagnostic sur site : on branche un adaptateur USB-série sur la liaison
inter-MCU, on capture, et on relit ici.

    # Décoder une capture hexadécimale
    python3 tools/decode_link.py A502050100050400C0DE

    # Décoder un fichier de capture (une trame par ligne, ou flot continu)
    python3 tools/decode_link.py --file capture.hex

Format : SOF | cmd | len | payload | crc16 (CCITT-FALSE sur cmd|len|payload).
SOF 0xA5 = ESP32 -> MEGA, 0x5A = MEGA -> ESP32.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

SOF_TO_MEGA = 0xA5
SOF_TO_ESP = 0x5A

CMDS = {
    0x01: "Heartbeat", 0x02: "Goto", 0x03: "Stop", 0x04: "GetState",
    0x05: "ClearFault", 0x06: "Ping", 0x81: "Ack", 0x82: "State", 0x86: "Pong",
}
RESULTS = {
    0: "Accepted", 1: "Duplicate", 2: "QueueFull", 3: "SafeStopActive",
    4: "Fault", 5: "BadPayload",
}
SEQ_STATES = ["BOOT", "IDLE", "WRITE_SETUP", "WRITE_STROBE", "WRITE_RELEASE", "START_PULSE",
              "START_RELEASE", "TRANSIT", "ARRIVED", "STOP_PULSE", "SAFE_STOP", "FAULT"]
STATE_FLAGS = [(0x01, "moving"), (0x02, "in_station"), (0x04, "plc_fault"),
               (0x08, "no_destination"), (0x10, "SAFE_STOP"), (0x20, "heartbeat_ok")]


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def describe(cmd: int, payload: bytes) -> str:
    name = CMDS.get(cmd, f"0x{cmd:02X}")
    if cmd == 0x02 and len(payload) >= 5:  # Goto
        station = (payload[1] << 8) | payload[2]
        return f"{name} seq={payload[0]} station={station} speed={payload[3]} flags=0x{payload[4]:02X}"
    if cmd == 0x81 and len(payload) >= 2:  # Ack
        return f"{name} seq={payload[0]} result={RESULTS.get(payload[1], payload[1])}"
    if cmd == 0x82 and len(payload) >= 11:  # State
        station = (payload[0] << 8) | payload[1]
        flags = payload[5]
        active = [label for mask, label in STATE_FLAGS if flags & mask]
        state = SEQ_STATES[payload[3]] if payload[3] < len(SEQ_STATES) else payload[3]
        return (f"{name} station={station} speed={payload[2]} state={state} "
                f"fault={payload[4]} flags={'|'.join(active) or '-'} queue={payload[6]} "
                f"tries={payload[7]}/{payload[8]}/{payload[9]} last_seq={payload[10]}")
    return f"{name} payload={payload.hex().upper() or '-'}"


def decode(data: bytes) -> int:
    index = 0
    ok = bad = 0
    while index < len(data):
        if data[index] not in (SOF_TO_MEGA, SOF_TO_ESP):
            index += 1
            continue
        if index + 3 > len(data):
            break
        sof, cmd, length = data[index], data[index + 1], data[index + 2]
        total = length + 5
        if index + total > len(data):
            break
        payload = data[index + 3: index + 3 + length]
        crc_recv = (data[index + total - 2] << 8) | data[index + total - 1]
        direction = "ESP32->MEGA" if sof == SOF_TO_MEGA else "MEGA->ESP32"
        if crc16_ccitt(data[index + 1: index + total - 2]) != crc_recv:
            print(f"  CRC INVALIDE  {direction}  {data[index:index + total].hex().upper()}")
            bad += 1
            index += 1
            continue
        print(f"  {direction}  {describe(cmd, payload)}")
        ok += 1
        index += total
    print(f"\n{ok} trame(s) valide(s), {bad} rejetée(s)")
    return 0 if bad == 0 else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("hex", nargs="?", help="capture hexadécimale")
    parser.add_argument("--file", type=Path, help="fichier de capture hexadécimale")
    args = parser.parse_args(argv)

    if args.file is not None:
        text = "".join(line.split("#", 1)[0] for line in args.file.read_text().splitlines())
    elif args.hex:
        text = args.hex
    else:
        parser.print_help()
        return 2

    try:
        data = bytes.fromhex(text.replace(" ", "").replace(":", ""))
    except ValueError:
        print("hexadécimal invalide", file=sys.stderr)
        return 2
    return decode(data)


if __name__ == "__main__":
    sys.exit(main())
