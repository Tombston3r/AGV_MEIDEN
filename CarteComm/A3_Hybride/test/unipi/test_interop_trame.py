#!/usr/bin/env python3
"""Vérifie que `agv_frame.py` encode EXACTEMENT comme le C++ du projet.

Ne demande aucun matériel : c'est le seul essai de ce dossier qui tourne sur
un poste de développement. Il compile le codec C++ du cœur métier, lui fait
produire des vecteurs, et compare octet à octet.

C'est le contrôle qui compte : deux radios qui s'entendent mais dont les trames
divergent d'un bit donnent une panne indétectable au banc et évidente en
atelier.

    ./test_interop_trame.py
"""

import pathlib
import subprocess
import sys
import tempfile

import agv_frame as proto

RACINE = pathlib.Path(__file__).resolve().parents[3]
COMMON = RACINE / "Wifi" / "firmware" / "common"

VECTEURS = [
    # (type, node_id, seq, station, speed, flags, ts_s)
    (proto.FrameType.CMD_GOTO, 0x0001, 0, 0, 0, 0, 0),
    (proto.FrameType.CMD_GOTO, 0x1234, 7, 42, 3, 0, 0),
    (proto.FrameType.CMD_STOP, 0xFFFF, 255, 1023, 15, 0, 0),
    (proto.FrameType.ACK, 0x0002, 128, 512, 8, proto.FLAG_RETRY, 0),
    (proto.FrameType.TELEMETRY, 0x00AB, 64, 7, 1, proto.FLAG_TIMESTAMPED, 1_700_000_000),
    (proto.FrameType.PING, 0x0000, 1, 0, 0, proto.FLAG_PRIORITY, 0),
]

SOURCE_CPP = r"""
#include <cstdio>
#include "proto/frame.h"
int main() {
  struct V { int type; unsigned nid; unsigned seq; unsigned st; unsigned sp;
             unsigned fl; unsigned long ts; } v[] = {
%s
  };
  for (auto& x : v) {
    agv::Frame f;
    f.type = static_cast<agv::FrameType>(x.type);
    f.node_id = static_cast<uint16_t>(x.nid);
    f.seq = static_cast<uint8_t>(x.seq);
    f.station = static_cast<uint16_t>(x.st);
    f.speed = static_cast<uint8_t>(x.sp);
    f.flags = static_cast<uint8_t>(x.fl);
    f.ts_s = static_cast<uint32_t>(x.ts);
    uint8_t out[agv::kFrameMaxSize];
    size_t n = agv::encode_frame(f, out, sizeof(out));
    for (size_t i = 0; i < n; ++i) printf("%%02x", out[i]);
    printf("\n");
  }
  return 0;
}
"""


def main() -> int:
    lignes = ",\n".join(
        f"    {{{int(t)}, {nid}, {seq}, {st}, {sp}, {fl}, {ts}ul}}"
        for t, nid, seq, st, sp, fl, ts in VECTEURS
    )

    with tempfile.TemporaryDirectory() as tmp:
        src = pathlib.Path(tmp) / "vecteurs.cpp"
        exe = pathlib.Path(tmp) / "vecteurs"
        src.write_text(SOURCE_CPP % lignes)

        compil = subprocess.run(
            ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-O1",
             f"-I{COMMON}", str(src),
             str(COMMON / "proto" / "frame.cpp"), str(COMMON / "proto" / "crc16.cpp"),
             "-o", str(exe)],
            capture_output=True, text=True,
        )
        if compil.returncode != 0:
            print("[ÉCHEC] compilation du codec C++ :", file=sys.stderr)
            print(compil.stderr, file=sys.stderr)
            return 2

        attendu = subprocess.run([str(exe)], capture_output=True, text=True,
                                 check=True).stdout.split()

    ecarts = 0
    for (t, nid, seq, st, sp, fl, ts), ref in zip(VECTEURS, attendu):
        trame = proto.Frame(type=t, node_id=nid, seq=seq, station=st,
                            speed=sp, flags=fl, ts_s=ts)
        obtenu = trame.encode().hex()
        ok = obtenu == ref
        ecarts += not ok
        print(f"{'ok   ' if ok else 'ÉCART'} {t.name:9} seq={seq:3} {obtenu}")
        if not ok:
            print(f"      C++ attendait {ref}")

        # Le décodage doit refermer la boucle sur la sortie du C++.
        relu = proto.decode(bytes.fromhex(ref))
        if (relu.node_id, relu.seq, relu.station, relu.speed, relu.flags, relu.ts_s) != \
           (nid, seq, st, sp, fl, ts):
            ecarts += 1
            print(f"      décodage divergent : {relu}")

    print(f"\n{len(VECTEURS)} vecteurs, {ecarts} écart(s)")
    return 1 if ecarts else 0


if __name__ == "__main__":
    sys.exit(main())
