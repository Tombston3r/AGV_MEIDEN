#!/usr/bin/env bash
# Propage le thème AIO, ou signale les copies qui ont dérivé.
#
#   outils/theme.sh --verifier    (défaut) liste les copies non conformes
#   outils/theme.sh --appliquer   repropage docs/theme/theme.css partout
#
# Le thème vit une seule fois dans docs/theme/, mais chaque interface en porte
# une copie : les dossiers sont livrables seuls (docs/ORGANISATION.md). Une
# copie sans contrôle est une divergence en sursis : d'où ce script, et sa
# place dans les recettes de déploiement.
set -euo pipefail

RACINE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$RACINE/docs/theme/theme.css"
MODE="${1:---verifier}"

[[ -f "$SOURCE" ]] || { echo "introuvable : $SOURCE" >&2; exit 1; }

mapfile -t CIBLES < <(find "$RACINE" -type d -name web \
  -not -path "*/.git/*" -not -path "*/build/*" | sort)

ecarts=0
for dossier in "${CIBLES[@]}"; do
  copie="$dossier/theme.css"
  court="${dossier#"$RACINE"/}"
  if [[ "$MODE" == "--appliquer" ]]; then
    cp "$SOURCE" "$copie"
    echo "  posé   $court/theme.css"
  elif [[ ! -f "$copie" ]]; then
    echo "  ABSENT $court/theme.css" >&2
    ecarts=$((ecarts + 1))
  elif ! cmp -s "$SOURCE" "$copie"; then
    echo "  DÉRIVÉ $court/theme.css" >&2
    ecarts=$((ecarts + 1))
  else
    echo "  ok     $court/theme.css"
  fi
done

if [[ "$MODE" == "--appliquer" ]]; then
  echo "thème propagé dans ${#CIBLES[@]} interface(s)"
  exit 0
fi
if (( ecarts )); then
  echo >&2
  echo "$ecarts copie(s) non conforme(s). Reporter la correction dans" >&2
  echo "docs/theme/theme.css, puis : outils/theme.sh --appliquer" >&2
  exit 1
fi
echo "${#CIBLES[@]} interface(s) conformes au thème"
