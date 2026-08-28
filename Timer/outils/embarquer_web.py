#!/usr/bin/env python3
"""Transforme les fichiers de `web/` en tableaux C++, pour les embarquer.

Un banc sur ESP32 n'a pas de système de fichiers à monter : les pages voyagent
dans le binaire. Le fichier produit est GÉNÉRÉ, comme `generated_profile.h` :
ne pas l'éditer, relancer ce script.
"""
import sys
from pathlib import Path

racine = Path(__file__).resolve().parent.parent
web = racine / "banc_api/web"
sortie = racine / "banc_api/esp32/src/web_embarque.h"

def symbole(nom: str) -> str:
    return "k" + "".join(c if c.isalnum() else "_" for c in nom).title().replace("_", "")

lignes = ["// GÉNÉRÉ par outils_embarquer_web.py : ne pas éditer.",
          "//",
          "// Les pages du banc, embarquées dans le binaire ESP32. Sur une cible sans",
          "// système de fichiers, c'est le moyen le plus simple de les servir, et il",
          "// évite une partition SPIFFS à créer, monter et tenir à jour.",
          "#pragma once", "", "#include <cstddef>", "",
          "namespace banc_web {", "",
          "struct Fichier {", "  const char* nom;", "  const char* type;",
          "  const unsigned char* octets;", "  size_t taille;", "};", ""]

TYPES = {".html": "text/html; charset=utf-8",
         ".css": "text/css; charset=utf-8",
         ".js": "application/javascript; charset=utf-8"}

fichiers = sorted(p for p in web.iterdir() if p.suffix in TYPES)
for f in fichiers:
    data = f.read_bytes()
    lignes.append(f"// {f.name} : {len(data)} octets")
    lignes.append(f"inline const unsigned char {symbole(f.stem)}{f.suffix[1:].title()}[] = {{")
    for i in range(0, len(data), 16):
        lignes.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i+16]) + ",")
    lignes.append("};")
    lignes.append("")

lignes.append("inline const Fichier kFichiers[] = {")
for f in fichiers:
    sym = f"{symbole(f.stem)}{f.suffix[1:].title()}"
    lignes.append(f'    {{"{f.name}", "{TYPES[f.suffix]}", {sym}, sizeof({sym})}},')
lignes.append("};")
lignes.append("inline constexpr size_t kNbFichiers = sizeof(kFichiers) / sizeof(kFichiers[0]);")
lignes.append("")
lignes.append("}  // namespace banc_web")

sortie.parent.mkdir(parents=True, exist_ok=True)
sortie.write_text("\n".join(lignes) + "\n")
total = sum(f.stat().st_size for f in fichiers)
print(f"{len(fichiers)} fichiers, {total} octets embarqués -> {sortie.relative_to(racine)}")
