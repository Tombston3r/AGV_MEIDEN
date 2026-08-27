#!/usr/bin/env bash
# Déploie ou met à jour le banc sur la machine cible.
#
#   ./deployer.sh                    # sur la UniPi elle-même
#   ./deployer.sh unipi@10.0.0.42    # depuis un poste de développement
#
# Copier les fichiers À LA MAIN a déjà fait perdre du temps : un correctif
# présent dans le dépôt mais absent de /opt donne exactement les symptômes du
# défaut qu'il corrige. Ce script rend la mise à jour reproductible.
set -euo pipefail

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CIBLE="${1:-}"

if [[ -n "$CIBLE" ]]; then
  echo "→ envoi vers $CIBLE"
  rsync -az --delete --exclude __pycache__/ --exclude '*.pyc' \
        "$SOURCE/" "$CIBLE:~/banc-enocean-src/"
  # shellcheck disable=SC2029
  ssh -t "$CIBLE" 'cd ~/banc-enocean-src && ./deployer.sh'
  exit 0
fi

VERSION="$(sed -n 's/^__version__ = "\(.*\)"/\1/p' "$SOURCE/banc_enocean/__init__.py")"
echo "→ installation du banc EnOcean v$VERSION dans /opt/banc-enocean"

sudo mkdir -p /opt/banc-enocean
# --delete : un fichier retiré du dépôt doit disparaître de la cible, sinon un
# ancien module continue d'être importé.
sudo rsync -a --delete --exclude __pycache__/ --exclude '*.pyc' \
     "$SOURCE/banc_enocean" "$SOURCE/web" /opt/banc-enocean/

if ! id banc &>/dev/null; then
  echo "→ création du compte de service"
  sudo useradd --system --no-create-home --shell /usr/sbin/nologin -G dialout banc
fi

sudo cp "$SOURCE/systemd/banc-enocean.service" /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now banc-enocean
sudo systemctl restart banc-enocean

sleep 1
if sudo systemctl is-active --quiet banc-enocean; then
  SERVIE="$(curl -fsS localhost:8080/api/etat 2>/dev/null \
            | sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' || true)"
  echo "✓ service actif, version servie : ${SERVIE:-inconnue}, attendue : $VERSION"
  [[ -n "$SERVIE" && "$SERVIE" != "$VERSION" ]] && echo "⚠ ÉCART DE VERSION" >&2
  curl -fsS localhost:8080/api/etat 2>/dev/null | sed 's/^/  /'
  echo
else
  echo "✗ le service n'a pas démarré :" >&2
  sudo journalctl -u banc-enocean -n 15 --no-pager >&2
  exit 1
fi
