#!/usr/bin/env bash
# Reconstitue une architecture AUTONOME dans un zip.
#
# Depuis que le matériel et le brief sont partagés, un dossier d'architecture
# n'est plus complet à lui seul. Ce script rassemble ce qu'il faut pour qu'un
# destinataire puisse lire, compiler et fabriquer sans le reste du dépôt.
set -euo pipefail

RACINE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCHI="${1:-}"

if [[ -z "$ARCHI" || ! -d "$RACINE/architectures/$ARCHI" ]]; then
  echo "usage : $(basename "$0") <architecture>" >&2
  echo "architectures disponibles :" >&2
  ls -1 "$RACINE/architectures" | grep -v '^README' | sed 's/^/  /' >&2
  exit 1
fi

# La carte dépend de l'architecture : A4 tourne sur la V5.0.1, A2 et A3 sur la V6.0.
case "$ARCHI" in
  A4_Wifi) CARTE="AIO_AGV_Control_V5.0.1" ;;
  A2_Hybride|A3_LoRa) CARTE="AIO_AGV_Control_V6.0" ;;
  *) CARTE="" ;;                     # A1 : carte neuve, pas encore de projet
esac

HORODATAGE="$(date +%F)"
SORTIE="$RACINE/${ARCHI}_${HORODATAGE}.zip"
TEMP="$(mktemp -d)"
trap 'rm -rf "$TEMP"' EXIT
DEST="$TEMP/$ARCHI"

mkdir -p "$DEST"
# --exclude : les artefacts de compilation et les caches n'ont rien à faire
# dans une livraison, et pèsent plus lourd que les sources.
rsync -a --exclude build/ --exclude __pycache__/ --exclude '*.pyc' \
      "$RACINE/architectures/$ARCHI/" "$DEST/"

mkdir -p "$DEST/docs"
cp "$RACINE/docs/BRIEF.md" "$DEST/docs/BRIEF.md"
cp "$RACINE/docs/COMPARAISON.md" "$DEST/docs/COMPARAISON.md"

if [[ -n "$CARTE" ]]; then
  mkdir -p "$DEST/materiel"
  rsync -a --exclude '*-backups/' --exclude '*.lck' \
        "$RACINE/materiel/$CARTE" "$DEST/materiel/"
fi

# Le destinataire doit savoir ce qu'il a entre les mains, et d'où ça vient.
cat > "$DEST/EXPORT.md" <<EOF
# Export autonome — $ARCHI

Extrait du dépôt AGV MEIDEN le $HORODATAGE$( [[ -n "$(git -C "$RACINE" rev-parse --short HEAD 2>/dev/null)" ]] && echo " (commit $(git -C "$RACINE" rev-parse --short HEAD))" ).

Cet ensemble contient tout le nécessaire :

- l'architecture complète — firmware, tests, nomenclature, déploiement ;
- \`docs/BRIEF.md\` : la référence unique du projet, où renvoient les \`§N\` ;
- \`docs/COMPARAISON.md\` : le comparatif des quatre architectures ;$( [[ -n "$CARTE" ]] && printf '\n- `materiel/%s` : le projet KiCad de la carte.' "$CARTE" )

Les chemins \`../../docs/\` et \`../../materiel/\` des documents renvoient ici
vers \`docs/\` et \`materiel/\`.

\`\`\`bash
make test        # tests unitaires, aucun matériel requis
\`\`\`
EOF

( cd "$TEMP" && zip -qr "$SORTIE" "$ARCHI" )
printf 'écrit : %s (%s)\n' "$SORTIE" "$(du -h "$SORTIE" | cut -f1)"
