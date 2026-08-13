#!/usr/bin/env bash
# Flash d'une cible du monorepo.
#
#   ./tools/flash.sh agv            [/dev/ttyUSB0] [profiles/default.yaml]
#   ./tools/flash.sh poste          [/dev/ttyUSB0]
#   ./tools/flash.sh bouton         [/dev/ttyUSB0]
#   ./tools/flash.sh mega-bridge    [/dev/ttyACM0]
#
# Le profil est régénéré AVANT le build : jamais de binaire flashé avec un
# generated_profile.h périmé — ce serait flasher des timings qui ne sont pas
# ceux du profil relevé.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-}"
PORT="${2:-}"
PROFILE="${3:-$ROOT/profiles/default.yaml}"

if [[ -z "$TARGET" ]]; then
    grep -E '^#( |$)' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 2
fi

case "$TARGET" in
    agv|agv-lte|poste|bouton|mega-bridge) ;;
    *) echo "cible inconnue : $TARGET" >&2; exit 2 ;;
esac

echo "== Régénération de la configuration depuis $PROFILE"
python3 "$ROOT/tools/genconfig.py" "$PROFILE" \
        "$ROOT/firmware/common/config/generated_profile.h"

echo "== Tests natifs (garde-fou avant tout flash)"
make -C "$ROOT" test

PIO_ARGS=(run -e "$TARGET" -t upload -d "$ROOT")
[[ -n "$PORT" ]] && PIO_ARGS+=(--upload-port "$PORT")

echo "== Build et flash de $TARGET"
pio "${PIO_ARGS[@]}"

if [[ "$TARGET" == "poste" ]]; then
    echo "== Envoi des assets web dans LittleFS"
    pio run -e poste -t uploadfs -d "$ROOT" ${PORT:+--upload-port "$PORT"}
fi

echo "== Terminé. Rappel : provisionner la clé AES (tools/provision_key.py)."
