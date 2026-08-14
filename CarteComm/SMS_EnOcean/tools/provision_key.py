#!/usr/bin/env python3
"""Génère et provisionne la clé AES-128 partagée (brief §5.1).

La clé vit en NVS sur l'AGV et sur le poste fixe.
Sans elle, la liaison démarre EN CLAIR : le firmware le journalise bruyamment,
mais ne refuse pas de démarrer — un site sans clé doit rester diagnosticable.

    # Générer une clé et l'écrire dans un fichier de partition NVS
    python3 tools/provision_key.py generate --out build/nvs_agv.csv

    # Réutiliser une clé existante (tous les nœuds doivent partager la MÊME)
    python3 tools/provision_key.py generate --key 00112233...ff --out ...

Le fichier CSV produit se transforme en image NVS avec l'outil d'Espressif :

    python3 $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \\
            generate build/nvs_agv.csv build/nvs_agv.bin 0x6000

⚠ La clé ne doit pas être versionnée. Conservez-la dans le coffre du client :
la reperdre impose de reflasher TOUS les nœuds.
"""

from __future__ import annotations

import argparse
import secrets
import sys
from pathlib import Path

KEY_SIZE = 16


def build_csv(key: bytes, namespace: str) -> str:
    return (
        "key,type,encoding,value\n"
        f"{namespace},namespace,,\n"
        f"aes_key,data,hex2bin,{key.hex()}\n"
        "aes_nonce,data,hex2bin,00000000\n"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    gen = sub.add_parser("generate", help="produire un CSV de partition NVS")
    gen.add_argument("--key", help="clé existante en hexadécimal (32 caractères)")
    gen.add_argument("--namespace", default="agv", help="agv (défaut) ou poste")
    gen.add_argument("--out", type=Path, required=True)

    args = parser.parse_args(argv)

    if args.key:
        try:
            key = bytes.fromhex(args.key)
        except ValueError:
            print("clé hexadécimale invalide", file=sys.stderr)
            return 2
        if len(key) != KEY_SIZE:
            print(f"la clé doit faire exactement {KEY_SIZE} octets", file=sys.stderr)
            return 2
    else:
        key = secrets.token_bytes(KEY_SIZE)
        print(f"clé générée : {key.hex()}")
        print("→ à conserver hors du dépôt, et à provisionner sur TOUS les nœuds")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(build_csv(key, args.namespace))
    print(f"écrit : {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
