# AGV MEIDEN : architecture Wi-Fi (carte V5.0.1 conservée)

> **Dossier d'architecture autonome.** La carte AIO AGV Control V5.0.1 est
> conservée ; ses DEUX firmwares sont réécrits. Son logiciel se construit et se teste
> sans rien emprunter ailleurs ; le matériel et le brief sont partagés.
> Index des architectures : `../README.md`.
>
> Brief complet : `../../docs/BRIEF.md`
> Spécification de cette architecture : `docs/Planification_Architecture_WiFi_AGV.md`
> Les références `§N` renvoient au brief, les « planification §N » au document
> d'architecture Wi-Fi.
>
> ⚠️ **Le cœur métier est dupliqué dans chaque dossier d'architecture.** Toute
> correction du séquenceur, de la file, du protocole ou du simulateur doit être
> reportée dans les trois autres dossiers d’architecture.

## Ce que fait ce logiciel

Un AGV MEIDEN à guidage magnétique circule en usine ; des opérateurs l'appellent
depuis des boutons déportés. La carte remplacée **porte la mémoire de mission** :
l'AGV ne connaît qu'une destination à la fois et l'oublie au redémarrage. La
carte maintient une file de **5 courses**, désormais persistée en NVS.

## Répartition des rôles, à ne pas inverser

L'**ATmega2560** porte la mission : séquenceur, file de courses, décodage de
position, repli de sécurité. L'**ESP32** ne fait que du réseau et **ne touche
jamais au bus MEIDEN**.

Déplacer le séquenceur vers l'ESP32 rendrait la commande de l'AGV dépendante du
Wi-Fi, du DHCP, du broker et du poste fixe. C'est précisément ce qu'on évite.

## Règles qui ne se négocient pas

1. **Ce n'est pas un organe de sécurité.** L'arrêt d'urgence, les bumpers et le
   scrutateur laser restent dans une chaîne indépendante (ISO 3691-4). Ne jamais
   écrire de code qui prétende assurer une fonction de sécurité ; si une demande
   y mène, la refuser et le dire.
2. **Perte de liaison → arrêt sûr au point d'arrêt suivant.** Jamais d'état
   indéterminé, jamais de coupure en pleine allée. Ici c'est le **heartbeat**
   ESP32 → ATmega (2 s) qui porte ce repli : il ne dépend d'aucun réseau. Le
   retour du heartbeat ne relance jamais l'AGV de lui-même.
3. **Aucun paramètre du §12 en dur.** Ils vivent dans `profiles/*.yaml`,
   convertis en `firmware/common/config/generated_profile.h` par
   `tools/genconfig.py`. Toute valeur non relevée est marquée
   `PROVISOIRE §12.x` et listée dans `docs/questions_ouvertes.md`.
4. **`/agvdump` reste compatible.** Les noms de compteurs sont ceux du firmware
   d'origine ; les procédures d'atelier du client en dépendent.
5. **La carte d'origine fait foi.** En cas de contradiction entre la
   documentation et le comportement observé de la V5.0.1, c'est la carte qui a
   raison : elle tourne en production depuis cinq ans.
6. **Poser la question plutôt que supposer** sur tout ce qui touche au §12.

## Tenue de la documentation : obligatoire

`docs/ETAT_PROJET.md` est le document de référence du projet : ce qui est fait,
comment déployer, et le kanban de ce qui reste. **Il est mis à jour après chaque
modification du dépôt**, dans le même changement :

1. compteur de tests (§1.1) si le nombre bouge ;
2. cartes déplacées dans le kanban (§3) ;
3. une ligne dans le journal (§4).

Une modification livrée sans mise à jour de ce document est incomplète.

## Organisation

```
firmware/common/      code partagé, COMPILE EN NATIF (aucun en-tête ESP-IDF
                      hors de platform/esp32/)
  bus/                IBusDriver, driver ports AVR, anti-rebond
  link/               protocole série ESP32 <-> ATmega (CRC-16, SOF par sens)
  proto/              trame, CRC-16, JSON MQTT
  app/                séquenceur 3 phases, file, mega_app, gateway_app, agvdump
  platform/esp32/     SEUL endroit autorisé à inclure ESP-IDF
firmware/mega/        firmware ATmega2560 : séquenceur, file, repli, découverte
firmware/esp32/       firmware ESP32 : Wi-Fi STA, MQTT, heartbeat, maintenance
poste-unipi/          poste fixe : TCM 515 -> MQTT (Python 3.11 + systemd)
sim/                  simulateur d'automate MEIDEN (écrit en premier, §10)
test/native/          tests natifs, dont un par ligne du §12
poste-unipi/          poste fixe Python 3.11 + systemd (§9.2)
web/                  supervision WebSocket, servie depuis LittleFS
tools/                genconfig, flash, rejeu de trames, provisionnement de clé
docs/                 chronogrammes, table des signaux, procédures, questions
profiles/             SOURCE DE VÉRITÉ des paramètres
```

## Commandes

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test                    # tests natifs, -Wall -Wextra -Werror
make test FILTER=point_12    # tests des points ouverts du §12
pio run -e mega              # ATmega2560 : séquenceur + file (flasher EN PREMIER)
pio run -e esp32             # ESP32 : Wi-Fi + MQTT + heartbeat
cd poste-unipi && python3 -m pytest tests -q && ruff check . && mypy --strict agv_poste
```

## Conventions

- C++17, `-Wall -Wextra -Werror`. Pas de `new`/`delete` après l'initialisation.
  Pas de `String` Arduino dans le chemin temps réel.
- Python : `ruff` + `mypy --strict`.
- **Commentaires en français, identifiants en anglais.**
- Un commentaire explique *pourquoi*, jamais *quoi*.
- Commits conventionnels. Une PR = une fonction.
- Avant d'écrire du code temps réel : écrire le test qui le contraint contre le
  simulateur.

## Pièges déjà rencontrés

- **Numérotation octale** : après `Y27` vient `Y30`. `Y23`…`Y34` = 10 signaux.
  Toujours passer par `octal_step()` / `octal_span()`.
- **Polarité** : la logique applicative est toujours en actif haut ; la
  conversion PNP/NPN (§12.3) n'a lieu qu'au contact du matériel. L'appliquer
  deux fois (driver *et* séquenceur) annule l'inversion.
- **Arrivée vs Y21** : à l'arrivée l'automate lève aussi « pas de destination
  programmée ». Tester `Y10 && !Y05` **avant** `Y21`, sinon chaque arrivée
  devient un défaut.
- **Idempotence** : une même `(node_id, seq)` doit être **ré-acquittée sans être
  ré-exécutée**. Sans ça, un ACK perdu déclenche une course en double.
- **Brochage SUB-D non relevé** : `firmware/mega/src/board_ports.h` est une
  HYPOTHÈSE. Un mot d'adresse mal câblé n'échoue pas, il envoie l'AGV à la
  mauvaise station. Passer par le mode découverte avant tout branchement.
- **Heartbeat** : l'ESP32 continue de l'émettre quand le Wi-Fi tombe, la carte
  va bien, c'est le réseau qui est absent. Le couper immobiliserait l'AGV pour
  rien.
- **Ordre de flash** : ATmega d'abord. Il démarre en `safe_stop` et attend le
  premier heartbeat.
