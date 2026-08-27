# AGV MEIDEN : architecture SMS + EnOcean

> **Dossier d'architecture autonome.** Boutons EnOcean sans pile au poste fixe,
> liaison cellulaire (SMS ou LTE-M/MQTT) vers l'AGV. Il se construit et se teste
> sans rien emprunter aux autres dossiers du dépôt, et peut être zippé
> seul. Index des architectures : `../README.md`.
>
> Brief complet : `../../docs/BRIEF.md`
> Analyse cellulaire : `docs/Archi_1_Cellulaire_SMS_LTE-M.md`
> Les références `§N` de ce dossier renvoient au brief.
>
> ⚠️ **Le cœur métier est dupliqué dans chaque dossier d'architecture.** Toute
> correction du séquenceur, de la file, du protocole ou du simulateur doit être
> reportée dans les trois autres dossiers d’architecture.

## Ce que fait ce logiciel

Un AGV MEIDEN à guidage magnétique circule en usine ; des opérateurs l'appellent
depuis des boutons déportés. La carte remplacée **porte la mémoire de mission** :
l'AGV ne connaît qu'une destination à la fois et l'oublie au redémarrage. La
carte maintient une file de **5 courses**, désormais persistée en NVS.

## Règles qui ne se négocient pas

1. **Ce n'est pas un organe de sécurité.** L'arrêt d'urgence, les bumpers et le
   scrutateur laser restent dans une chaîne indépendante (ISO 3691-4). Ne jamais
   écrire de code qui prétende assurer une fonction de sécurité ; si une demande
   y mène, la refuser et le dire.
2. **Perte de liaison → arrêt sûr au point d'arrêt suivant.** Jamais d'état
   indéterminé, jamais de coupure en pleine allée.
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
  bus/                IBusDriver + 3 variantes (MCP23017, 74HC595, MEGA UART)
  proto/              trame, CRC-16, AES-128-CTR, idempotence
  transport/          ITransport : SMS et MQTT/LTE-M, pile AT commune
  app/                séquenceur 3 phases, file de courses, agvdump, poste
  enocean/            décodeur ESP3, table d'appairage
  platform/esp32/     SEUL endroit autorisé à inclure ESP-IDF
firmware/agv/         firmware embarqué sur l'AGV (cœur du projet)
firmware/poste-esp32/ poste fixe (EnOcean + modem + Ethernet filaire)
firmware/mega-bridge/ pont ATmega2560, variante C du §4.4
hardware/             projet KiCad de la carte
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
pio run -e agv               # firmware AGV, transport MQTT/LTE-M
pio run -e agv-sms           # variante SMS (comparaison seulement)
pio run -e poste             # poste fixe ESP32
./tools/flash.sh agv /dev/ttyUSB0
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
- **SMS** : ni latence bornée, ni ordre, ni garantie de remise. Le transport le
  déclare (`ordered() == false`, `max_command_age_s != 0`) et la couche
  applicative en tire les conséquences automatiquement. Si le client impose le
  cellulaire, c'est **MQTT/LTE-M** qu'il faut mettre en service, jamais le SMS.
