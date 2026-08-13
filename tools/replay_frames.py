#!/usr/bin/env python3
"""Décodage et rejeu de trames applicatives AGV.

Trois usages :

    # Décoder une trame relevée à l'analyseur ou dans un SMS
    python3 tools/replay_frames.py decode 1012342afffc010492

    # Fabriquer une trame (mise au point, injection sur banc)
    python3 tools/replay_frames.py encode --type CMD_GOTO --node 2 --seq 7 \\
            --station 12 --speed 4

    # Rejouer un journal de trames hexadécimales (une par ligne)
    python3 tools/replay_frames.py replay releve.txt

Le décodage est volontairement tolérant : une trame illisible est signalée avec
sa raison, jamais silencieusement ignorée — c'est tout l'intérêt en diagnostic.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "poste-unipi"))

from agv_poste.protocol import Flag, Frame, FrameError, FrameType  # noqa: E402


def describe(frame: Frame) -> str:
    flags = [flag.name for flag in Flag if frame.flags & flag]
    lines = [
        f"  type           : {frame.type.name}",
        f"  version        : {frame.ver}",
        f"  node_id        : {frame.node_id} (0x{frame.node_id:04X})",
        f"  seq            : {frame.seq}",
        f"  station        : {frame.station}",
        f"  speed          : {frame.speed}",
        f"  flags          : 0x{frame.flags:02X} {flags}",
    ]
    if frame.timestamped:
        lines.append(f"  ts_s           : {frame.ts_s}")
    return "\n".join(lines)


def cmd_decode(args: argparse.Namespace) -> int:
    try:
        raw = bytes.fromhex(args.hex.replace(" ", ""))
    except ValueError:
        print("hexadécimal invalide", file=sys.stderr)
        return 2
    try:
        frame = Frame.decode(raw, expected_version=None)
    except FrameError as exc:
        print(f"trame REJETÉE : {exc}", file=sys.stderr)
        return 1
    print(f"{raw.hex().upper()} ({len(raw)} octets)")
    print(describe(frame))
    return 0


def cmd_encode(args: argparse.Namespace) -> int:
    flags = 0
    if args.priority:
        flags |= Flag.PRIORITY
    if args.purge:
        flags |= Flag.PURGE_QUEUE
    if args.ts:
        flags |= Flag.TIMESTAMPED
    frame = Frame(
        type=FrameType[args.type],
        node_id=args.node,
        seq=args.seq,
        station=args.station,
        speed=args.speed,
        flags=flags,
        ts_s=args.ts or 0,
    )
    print(frame.encode().hex().upper())
    return 0


def cmd_replay(args: argparse.Namespace) -> int:
    ok = 0
    bad = 0
    for lineno, line in enumerate(args.file.read_text().splitlines(), start=1):
        text = line.split("#", 1)[0].strip().replace(" ", "")
        if not text:
            continue
        try:
            frame = Frame.decode(bytes.fromhex(text), expected_version=None)
        except (FrameError, ValueError) as exc:
            bad += 1
            print(f"ligne {lineno}: REJETÉE — {exc}")
            continue
        ok += 1
        print(f"ligne {lineno}: {frame.type.name} node={frame.node_id} seq={frame.seq} "
              f"station={frame.station} speed={frame.speed}")
    print(f"\n{ok} trame(s) valide(s), {bad} rejetée(s)")
    return 0 if bad == 0 else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p_decode = sub.add_parser("decode", help="décoder une trame hexadécimale")
    p_decode.add_argument("hex")
    p_decode.set_defaults(func=cmd_decode)

    p_encode = sub.add_parser("encode", help="fabriquer une trame")
    p_encode.add_argument("--type", default="CMD_GOTO", choices=[t.name for t in FrameType])
    p_encode.add_argument("--node", type=int, default=1)
    p_encode.add_argument("--seq", type=int, default=1)
    p_encode.add_argument("--station", type=int, default=0)
    p_encode.add_argument("--speed", type=int, default=0)
    p_encode.add_argument("--priority", action="store_true")
    p_encode.add_argument("--purge", action="store_true")
    p_encode.add_argument("--ts", type=int, default=0, help="horodatage unix (0 = absent)")
    p_encode.set_defaults(func=cmd_encode)

    p_replay = sub.add_parser("replay", help="rejouer un journal de trames")
    p_replay.add_argument("file", type=Path)
    p_replay.set_defaults(func=cmd_replay)

    args = parser.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    sys.exit(main())
