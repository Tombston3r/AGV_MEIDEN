#!/usr/bin/env python3
"""Génère l'en-tête C++ de configuration à partir d'un profil YAML.

Une seule source de vérité (brief §13) : `profiles/*.yaml`. Le header produit
est inclus par `firmware/common/config/hardware_profile.h`.

Usage :
    python3 tools/genconfig.py profiles/default.yaml \
            firmware/common/config/generated_profile.h

Volontairement sans dépendance : un sous-ensemble de YAML (mappings imbriqués,
scalaires, commentaires) suffit et évite d'imposer PyYAML sur le poste de build.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Any

# Valeurs à re-marquer PROVISOIRE dans le header généré (brief §12).
PROVISIONAL_KEYS = {
    "bus.x_active_high": "§12.3",
    "bus.y_active_high": "§12.3",
    "bus.t_setup_us": "§12.4",
    "bus.t_strobe_us": "§12.4",
    "bus.t_hold_us": "§12.4",
    "bus.y_debounce_us": "§12.1",
    "bus.driver_variant": "§12.10",
    "bus.mcp_ab_skew_us": "§12.10",
    "timeouts.y22_write_ack_ms": "§12.5",
    "timeouts.y05_start_ack_ms": "§12.5",
    "timeouts.y10_arrival_ms": "§12.5",
    "timeouts.write_max_tries": "§12.5",
    "timeouts.start_max_tries": "§12.5",
    "timeouts.stop_max_tries": "§12.5",
}

DRIVER_VARIANTS = {"sim": 0, "mcp23017": 1, "shift595": 2, "mega_uart": 3}


class YamlError(RuntimeError):
    pass


def parse_yaml_subset(text: str) -> dict[str, Any]:
    """Analyse un sous-ensemble de YAML : mappings imbriqués et scalaires."""
    root: dict[str, Any] = {}
    stack: list[tuple[int, dict[str, Any]]] = [(-1, root)]

    for lineno, raw in enumerate(text.splitlines(), start=1):
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        indent = len(line) - len(line.lstrip(" "))
        stripped = line.strip()
        if ":" not in stripped:
            raise YamlError(f"ligne {lineno}: mapping attendu -> {raw!r}")

        key, _, value = stripped.partition(":")
        key = key.strip()
        value = value.strip()

        while stack and indent <= stack[-1][0]:
            stack.pop()
        if not stack:
            raise YamlError(f"ligne {lineno}: indentation incohérente")
        parent = stack[-1][1]

        if value == "":
            child: dict[str, Any] = {}
            parent[key] = child
            stack.append((indent, child))
        else:
            parent[key] = _scalar(value)
    return root


def _scalar(value: str) -> Any:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        return value[1:-1]
    low = value.lower()
    if low in ("true", "false"):
        return low == "true"
    if re.fullmatch(r"0[xX][0-9a-fA-F]+", value):
        return int(value, 16)
    if re.fullmatch(r"[+-]?\d+", value):
        return int(value)
    if re.fullmatch(r"[+-]?\d*\.\d+", value):
        return float(value)
    return value


def flatten(node: dict[str, Any], prefix: str = "") -> list[tuple[str, Any]]:
    out: list[tuple[str, Any]] = []
    for key, value in node.items():
        path = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict):
            out.extend(flatten(value, path))
        else:
            out.append((path, value))
    return out


def macro_name(path: str) -> str:
    return "CFG_" + re.sub(r"[^A-Za-z0-9]", "_", path).upper()


def literal(path: str, value: Any) -> str:
    if path == "bus.driver_variant":
        if value not in DRIVER_VARIANTS:
            raise YamlError(f"bus.driver_variant inconnu : {value!r}")
        return str(DRIVER_VARIANTS[value])
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int):
        return f"{value}u" if value >= 0 else str(value)
    if isinstance(value, float):
        return repr(value) + "f"
    return '"' + str(value).replace("\\", "\\\\").replace('"', '\\"') + '"'


def render(profile: dict[str, Any], source: Path) -> str:
    lines: list[str] = [
        "// ===========================================================================",
        "//  FICHIER GÉNÉRÉ : NE PAS ÉDITER À LA MAIN.",
        f"//  Source : {source.as_posix()}",
        "//  Régénération : python3 tools/genconfig.py <profil.yaml> <sortie.h>",
        "// ===========================================================================",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "// Variantes d'interface bus (§12.10).",
        "#define CFG_DRIVER_SIM       0",
        "#define CFG_DRIVER_MCP23017  1",
        "#define CFG_DRIVER_SHIFT595  2",
        "#define CFG_DRIVER_MEGA_UART 3",
        "",
    ]

    pinmap_x: list[tuple[str, int]] = []
    pinmap_y: list[tuple[str, int]] = []

    for path, value in flatten(profile):
        if path.startswith("pinmap.x."):
            pinmap_x.append((path.rsplit(".", 1)[1], int(value)))
            continue
        if path.startswith("pinmap.y."):
            pinmap_y.append((path.rsplit(".", 1)[1], int(value)))
            continue
        note = PROVISIONAL_KEYS.get(path)
        if note:
            lines.append(f"// PROVISOIRE {note} : valeur non relevée sur la V5.0.1.")
        lines.append(f"#define {macro_name(path)} {literal(path, value)}")

    lines.append("")
    lines.append("// --- Brochage (§12.2) : mapping signal -> position dans le mot bus. ---")
    lines.append("// PROVISOIRE §12.2 : divergence CN61/62/63 vs CN62/63/64 non tranchée.")
    for name, bit in sorted(pinmap_x, key=lambda kv: kv[1]):
        lines.append(f"#define CFG_PIN_{name} {bit}u")
    for name, bit in sorted(pinmap_y, key=lambda kv: kv[1]):
        lines.append(f"#define CFG_PIN_{name} {bit}u")

    lines.append("")
    lines.append(f"#define CFG_PINMAP_X_COUNT {len(pinmap_x)}u")
    lines.append(f"#define CFG_PINMAP_Y_COUNT {len(pinmap_y)}u")
    lines.append("")
    return "\n".join(lines) + "\n"


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    src, dst = Path(argv[1]), Path(argv[2])
    profile = parse_yaml_subset(src.read_text(encoding="utf-8"))
    if len(profile.get("pinmap", {}).get("x", {})) != 22:
        raise YamlError("pinmap.x doit décrire exactement 22 sorties (brief §4.1)")
    if len(profile.get("pinmap", {}).get("y", {})) != 21:
        raise YamlError("pinmap.y doit décrire exactement 21 entrées (brief §4.2)")
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text(render(profile, src), encoding="utf-8")
    print(f"[genconfig] {src} -> {dst}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
