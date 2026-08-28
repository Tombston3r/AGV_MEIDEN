# Déploiement : banc API sur l'ESP32 de la carte

**État au 2026-08-28 : non déployé. Le code est écrit et n'a jamais été
compilé pour ESP32.** Deux obstacles, dans cet ordre.

## 1. Flasher est irréversible tant que le firmware d'origine n'est pas sauvegardé

C'est le même avertissement que `Comm distance/architectures/A3_Wifi/DEPLOY.md`
§1, et il s'applique tel quel ici.

L'ESP32 branché sur `/dev/ttyUSB0` est celui de la carte AGV Control V5.0.1, qui
tourne en production depuis cinq ans. Le firmware d'origine n'existe **ni en
sources, ni en sauvegarde connue** : une recherche dans le dépôt ne trouve aucun
`esp32_origine.bin`. Dès la première écriture en flash, l'ancien système est
perdu.

La sauvegarde doit donc précéder le flash, et c'est elle qui rend l'opération
réversible :

```
esptool.py --port /dev/ttyUSB0 --baud 460800 read_flash 0 0x400000 esp32_origine.bin
```

Puis vérifier que le fichier n'est pas vide ni uniformément à `0xFF` (flash
protégée en lecture). **Si les bits de protection interdisent la lecture, il n'y
a pas de retour arrière** : c'est une décision à faire acter par le client, par
écrit, avant de continuer.

Restauration, le cas échéant :

```
esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash 0 esp32_origine.bin
```

## 2. Aucune chaîne de compilation ESP32 sur la machine de développement

Vérifié le 2026-08-28 : `pio`, `platformio`, `idf.py`, `xtensa-esp32-elf-gcc`,
`esptool`, `esptool.py` et `arduino-cli` sont tous absents, ainsi que les
modules Python `esptool` et `pyserial`. `/dev/ttyUSB0` existe bien
(`crw-rw---- root dialout 188, 0`).

Rien ne peut être construit ni flashé avant d'en installer une :

```
python3 -m venv ~/.venv-pio && ~/.venv-pio/bin/pip install platformio
```

Vérifier aussi l'appartenance au groupe `dialout`, sans quoi le port est
inaccessible :

```
id -nG | tr ' ' '\n' | grep -x dialout || sudo usermod -aG dialout "$USER"
```

(déconnexion/reconnexion nécessaire après `usermod`)

## Procédure, une fois les deux points levés

| # | Étape | Commande |
| --- | --- | --- |
| 1 | Sauvegarder le firmware d'origine | voir point 1 |
| 2 | Vérifier la sauvegarde | taille 4 Mio, pas uniformément `0xFF` |
| 3 | Faire acter la décision par le client | par écrit |
| 4 | Régénérer les pages embarquées | `python3 ../../../outils/embarquer_web.py` |
| 5 | Construire | `pio run -d .` |
| 6 | Flasher | `pio run -d . -t upload` |
| 7 | Observer le démarrage | `pio device monitor -b 115200` |
| 8 | Se connecter au Wi-Fi `agv-atelier` | mot de passe dans `README.md` |
| 9 | Ouvrir `http://192.168.4.1/` | l'interface agvschedule doit s'afficher |
| 10 | Poser l'heure | panneau Simulation, ou `POST /api/sim/heure` |

Tant que l'étape 10 n'est pas faite, le moteur reste gelé et aucun départ ne
sort : c'est le comportement attendu, pas une panne.

## Ce qui a été vérifié sans carte

Les branches `ESP_PLATFORM` de `banc_api/serveur.cpp` compilent proprement sur
l'hôte, en `-Wall -Wextra -Werror` : les en-têtes lwIP portent les mêmes noms
que POSIX, et les appels utilisés (`socket`, `bind`, `listen`, `accept`,
`poll`, `read`, `write`, `close`, `setsockopt`, `getsockname`) sont tous
fournis par lwIP.

```
g++ -std=gnu++17 -Wall -Wextra -Werror -DESP_PLATFORM -I. -Iatelier \
    -Ibanc_api/esp32/src -fsyntax-only banc_api/serveur.cpp
```

Cela ne vaut **pas** une compilation ESP-IDF : `src/main.cpp` (esp_wifi,
FreeRTOS, NVS) n'est couvert par aucune vérification, et la tenue mémoire du
serveur sur 320 Ko de RAM reste entièrement à démontrer. La pile de la tâche
serveur est fixée à 16 Kio par prudence, valeur à confirmer au premier essai
(`uxTaskGetStackHighWaterMark`).

Le banc hôte, lui, reste vert : 48 tests, dont les 3 de fidélité `agvdump`.
