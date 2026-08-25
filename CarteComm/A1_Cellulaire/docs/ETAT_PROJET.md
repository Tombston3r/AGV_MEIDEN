# État du projet — architecture SMS + EnOcean

> **Document vivant.** Il est mis à jour à chaque modification du dépôt : toute
> évolution du code, des profils ou des relevés matériels doit se refléter ici
> le même jour. Journal des mises à jour en fin de document.
>
> Dernière mise à jour : **2026-08-20** — passage à une organisation par
> dossier d'architecture ; extraction des sources LoRa vers `CarteComm/A3_LoRa/`.
>
> **Périmètre de ce document** : le dossier `CarteComm/A1_Cellulaire/`, projet
> autonome et zippable (boutons EnOcean au poste + liaison cellulaire vers
> l'AGV). Index des architectures : [`../../README.md`](../../README.md).

---

## 1. Ce qui est fait

### 1.1 Vue d'ensemble

| Indicateur | Valeur |
|---|---|
| Tests natifs | **112 tests, 373 assertions, 0 échec** |
| Tests Python (poste UniPi) | 16 tests écrits — *non exécutés ici, `pytest` absent* |
| Compilation | `-std=c++17 -Wall -Wextra -Werror`, sans avertissement |
| Matériel nécessaire pour tout ce qui précède | **aucun** |
| Lignes de code (hors docs) | ~10 600 |

Commande de vérification complète :

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

### 1.2 Le cœur : séquenceur du bus MEIDEN

Machine à états explicite (`firmware/common/app/sequencer.cpp`), sans `delay()`
ni blocage, identique dans les trois architectures et totalement découplée du
transport.

- **12 états** : `BOOT`, `IDLE`, `WRITE_SETUP`, `WRITE_STROBE`, `WRITE_RELEASE`,
  `START_PULSE`, `START_RELEASE`, `TRANSIT`, `ARRIVED`, `STOP_PULSE`,
  `SAFE_STOP`, `FAULT`.
- Les 4 phases du §4.3 sont reproduites front par front — voir
  [`chronogrammes.md`](chronogrammes.md).
- Compteurs nommés **littéralement** comme la section `AGV STATE` de `agvdump` :
  `write_tries`, `write_op_return`, `start_tries`, `start_op_return`,
  `stop_tries`, `stop_op_return`, `current_station`, `nb_courses_programmed`,
  `programmed_courses[5]`.
- Chaque timeout est instrumenté (`y22_timeouts`, `y05_timeouts`,
  `y10_timeouts`) et chaque transition est journalisable via un observateur.
- **Perte de liaison → arrêt sûr au point d'arrêt suivant** : la course engagée
  va au bout, aucune course supplémentaire n'est lancée, sorties au repos.

### 1.3 File de courses — le point fonctionnel central

`firmware/common/app/course_queue.cpp` : jusqu'à 5 courses, priorité en tête de
file, purge, et **persistance NVS** avec restauration au boot (amélioration par
rapport à la V5.0.1, qui perdait tout à chaque coupure).

- Blob versionné + CRC-16 : un blob corrompu donne une file **vide**, jamais une
  course inventée.
- Politique de validité : une course plus vieille que `course_validity_min`
  (30 min par défaut) est écartée au boot.
- Sans horloge murale sûre (`now_s == 0`), la politique se déclare inopérante et
  restaure tel quel plutôt que d'écarter à tort.

### 1.4 Protocole applicatif

`firmware/common/proto/` — conforme au §5.1, avec une extension documentée.

| Élément | État |
|---|---|
| Trame 9 octets (ver, type, node_id, seq, station, speed, flags, CRC-16) | ✅ |
| Extension horodatage (+4 octets, signalée par un flag) | ✅ — indispensable au §8.1 |
| CRC-16/CCITT-FALSE | ✅ vecteur `0x29B1` vérifié |
| AES-128-CTR | ✅ vecteur FIPS-197 vérifié |
| AES-CMAC tronqué (option hors spéc.) | ✅ désactivé par défaut |
| Idempotence `(node_id, seq)`, fenêtre 16 | ✅ ré-acquittement sans ré-exécution |
| Anti-désordre (transports non ordonnés) | ✅ |
| Refus des commandes périmées | ✅ |
| Compatibilité binaire C++ ↔ Python | ✅ vecteur figé des deux côtés |

### 1.5 Transports (§5) — deux implémentations derrière `ITransport`

L'implémentation LoRa vit dans le dossier d'architecture `CarteComm/A3_LoRa/`.

| Transport | État | Particularités implémentées |
|---|---|---|
| `MqttLteTransport` | ✅ | **Transport recommandé de cette architecture.** SIM7080G, topics `agv/<id>/{cmd,ack,telemetry,status}`, QoS 1, **Last Will and Testament** |
| `SmsTransport` | ✅ | Pile AT en machine à états, `+CMTI` → `CMGR` → `CMGD`, PWRKEY 1 000/2 500 ms, détection de modem muet + cycle d'alimentation, chien de garde matériel TPL5010. Déclare `ordered() == false` et impose l'horodatage |

### 1.6 Interface bus (§4.4) — trois variantes interchangeables

| Variante | État | Propriété vérifiée par test |
|---|---|---|
| 4× MCP23017 (I²C) | ✅ | GPIOA/GPIOB écrits en **une seule transaction** ; décalage A/B < `t_setup_us` ; NACK compté |
| 3× 74HC595 + 3× 74HC165 (SPI) | ✅ | **Un seul front `RCLK`** pour les 22 lignes ; `OE` validé seulement après mise à zéro |
| ATmega2560 conservé (UART) | ✅ | Protocole inter-MCU défini + firmware du pont ; MEGA muet détecté sans blocage |
| Simulateur | ✅ | Sert de quatrième implémentation, utilisée par tous les tests |

### 1.7 EnOcean (architecture retenue, §7)

- Décodeur **ESP3** complet : sync `0x55`, CRC8 header, CRC8 data,
  resynchronisation après corruption, extraction du RSSI des OptData.
- Télégrammes **RPS / PTM 210** : identifiant 32 bits, bascule, énergie.
- **Déduplication des 3 sous-télégrammes** (fenêtre 100 ms) — sans elle, un
  appui déclencherait trois courses.
- **Table d'appairage** `enocean_id → station` persistée + **mode appairage**
  (« appuyez sur le bouton à associer »), deux bascules par bouton.
- Un bouton non appairé est **compté, jamais deviné**.

### 1.8 Applications

- `AgvApp` : traitement des trames, ACK/NACK, chien de garde de liaison,
  télémétrie, restauration de la file, rendu `/agvdump`.
- `PosteApp` : chaîne EnOcean → appairage → cellulaire, instantané de supervision,
  fraîcheur de télémétrie, statistiques.
- `AlertGateway` (§8.3) : SMS bas volume vers un technicien hors site, quota
  journalier, **architecturalement incapable de piloter la chaîne de commande**.
- Wi-Fi de maintenance AGV : **désactivé par défaut**, ouvert sur contact ILS,
  extinction automatique après 10 min, sert `/agvdump`.

### 1.9 Supervision web et poste UniPi

- `web/` : page unique, WebSocket avec repli en interrogation périodique
  **signalé**, mise en avant de la fraîcheur de liaison et du budget
  d'émission, refus de commande affiché explicitement.
- `poste-unipi/` : paquet Python 3.11 + service systemd, protocole réimplémenté
  à l'identique, backends d'E/S **explicites** (aucun choix implicite, §12.9).

### 1.10 Outillage

| Outil | Rôle |
|---|---|
| `tools/genconfig.py` | YAML → en-tête C++, **source de vérité unique** |
| `tools/pio_genconfig.py` | Régénération automatique avant chaque build PlatformIO |
| `tools/flash.sh` | Régénère, teste, puis flashe — refuse de flasher si un test échoue |
| `tools/replay_frames.py` | Décodage / fabrication / rejeu de trames |
| `tools/provision_key.py` | Génération et provisionnement de la clé AES |

### 1.11 Couverture de test par domaine

| Fichier | Tests | Couvre |
|---|---:|---|
| `test_proto.cpp` | 19 | CRC, trame, AES, idempotence, péremption |
| `test_sequencer.cpp` | 16 | 4 phases, tous les chemins de timeout, dégradations |
| `test_transport.cpp` | 6 | Pile AT, URC `+CMTI`, modem muet, refus sans horodatage |
| `test_app.cpp` | 16 | Persistance NVS, chien de garde, `agvdump` |
| `test_poste.cpp` | 13 | Chaîne EnOcean → commande complète |
| `test_open_points_12.cpp` | 12 | **Un test par ligne du §12** |
| `test_enocean.cpp` | 11 | ESP3, déduplication, appairage |
| `test_bus_drivers.cpp` | 10 | Les trois variantes matérielles |
| `test_bus_octal.cpp` | 9 | Numérotation octale Meiden, anti-rebond |

### 1.12 Ce qui n'a **pas** pu être vérifié ici

- **Le code ESP-IDF n'a jamais été compilé** : PlatformIO et l'IDF ne sont pas
  installés sur le poste de développement. Cela concerne
  `firmware/common/platform/esp32/`, les trois `main.cpp` et `web_server.cpp`.
  → Première tâche de la colonne « À faire ».
- **Les tests Python n'ont pas été exécutés** (`pytest` absent, Python 3.10 au
  lieu de 3.11). Les modules s'importent et le vecteur de trame partagé est
  vérifié manuellement.
- **Aucun essai matériel** : ni banc HIL, ni AGV réel, ni relevé radio.

---

## 2. Déploiement en conditions réelles

> ⚠️ **Ne pas déployer avant d'avoir fait les relevés du §12.** Les timings
> livrés sont des valeurs par défaut arbitraires. Un `t_setup_us` faux ne se
> voit pas en atelier : il se traduit par des écritures perdues intermittentes
> sur l'AGV en production.

### Étape 0 — Prérequis bloquants

| Prérequis | Pourquoi | Où reporter |
|---|---|---|
| Relevé oscilloscope de la V5.0.1 **avant dépose** | La carte ne sera plus disponible ensuite | `profiles/default.yaml` |
| Sortie `agvdump` réelle de la V5.0.1 | Recaler le format servi (§12.6) | `app/agvdump.cpp` |
| ~~Table de câblage SUB-D 25~~ | **RELEVÉE** : CN61 à CN64, voir `Wifi/docs/subd25_atmega.md` | fait |
| Polarité automate PNP/NPN | Inverse les 22 voies X (§12.3) | `bus.x_active_high` |
| Variante d'interface bus choisie | §12.10 | `bus.driver_variant` |
| **Relevé RSRP/RSRQ en tous points du parcours** | Un point sous −110 dBm disqualifie l'architecture (Archi_2 §7.1) | prérequis, aucun paramètre |

Procédure de relevé détaillée : [`procedures_essai.md`](procedures_essai.md) §4.

### Étape 1 — Poste de développement

```bash
# Outils
pip install platformio            # ou pipx install platformio
cd poste-unipi && pip install -e '.[dev,mqtt]' && cd ..

# Vérification que tout est vert AVANT de toucher au matériel
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
cd poste-unipi && python3 -m pytest tests -q && cd ..
```

### Étape 2 — Renseigner le profil avec les valeurs relevées

Éditer `profiles/default.yaml` — **et rien d'autre**. Chaque valeur relevée
remplace une ligne marquée `PROVISOIRE §12.x`. Puis :

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test          # les tests doivent rester verts avec les vraies valeurs
```

Cocher la ligne correspondante dans
[`questions_ouvertes.md`](questions_ouvertes.md) et **mettre à jour ce
document** (§4, journal).

### Étape 3 — Provisionner la clé AES

La même clé doit être installée sur l'AGV **et** sur le poste fixe.

```bash
python3 tools/provision_key.py generate --namespace agv   --out build/nvs_agv.csv
python3 tools/provision_key.py generate --namespace poste --out build/nvs_poste.csv \
        --key <la_meme_cle_hex>

python3 $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
        generate build/nvs_agv.csv build/nvs_agv.bin 0x6000
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 build/nvs_agv.bin
```

> Sans clé, le firmware démarre **en clair** et le journal le signale
> bruyamment. C'est délibéré : un site sans clé doit rester diagnosticable.
> La clé n'est jamais versionnée (`.gitignore`) — la perdre impose de reflasher
> tous les nœuds, boutons compris.

### Étape 4 — Banc HIL, avant tout branchement sur l'AGV

```bash
python3 tools/genconfig.py profiles/hil.yaml \
        firmware/common/config/generated_profile.h
./tools/flash.sh agv /dev/ttyUSB0 profiles/hil.yaml
```

Vérifications à l'oscilloscope (détail dans
[`procedures_essai.md`](procedures_essai.md) §3) :

1. Simultanéité de la pose des 22 lignes.
2. Écart réel entre le dernier front d'adresse et `X93` → **valide `t_setup_us`**.
3. Injection de timeouts `Y22`/`Y05`/`Y10` → vérifier les compteurs dans
   `/agvdump`.
4. Coupure de liaison → arrêt au point d'arrêt suivant.
5. Coupure d'alimentation en cours de course → file restaurée au redémarrage.

### Étape 5 — Flash des cibles

```bash
./tools/flash.sh agv          /dev/ttyUSB0    # firmware AGV (MQTT/LTE-M)
./tools/flash.sh poste        /dev/ttyUSB1    # poste fixe + assets LittleFS
./tools/flash.sh mega-bridge  /dev/ttyACM0    # seulement en variante C
```

`flash.sh` régénère la configuration **et lance les tests** avant chaque flash :
un test rouge interrompt le flash.

Les boutons de cette architecture sont des **PTM 210 EnOcean sans pile** : ils
ne se flashent pas, ils s'appairent depuis l'IHM web (étape 7).

### Étape 6 — Poste UniPi (uniquement en architecture 2)

> ⚠️ §12.9 non tranché : si la référence commandée tourne sous **Mervis**, ce
> service Python est sans objet et l'intégration passe par Modbus TCP depuis un
> autre hôte. **Vérifier avant de commander le matériel.**

```bash
sudo useradd -r -s /usr/sbin/nologin agv
sudo mkdir -p /opt/agv /etc/agv
sudo python3.11 -m venv /opt/agv/venv
sudo /opt/agv/venv/bin/pip install ./poste-unipi[mqtt]
sudo cp poste-unipi/poste.example.toml /etc/agv/poste.toml
sudo $EDITOR /etc/agv/poste.toml        # io.kind, transport.kind, boutons
sudo cp poste-unipi/systemd/agv-poste.service /etc/systemd/system/
sudo systemctl enable --now agv-poste
journalctl -u agv-poste -f
```

### Étape 7 — Mise en service

1. Raccorder l'Ethernet **filaire** du poste (choix délibéré : aucune émission
   2,4 GHz permanente).
2. Ouvrir `http://agv.local` — vérifier que la fraîcheur de liaison est verte.
3. Appairer chaque bouton EnOcean : bouton « Ouvrir le mode appairage », saisir
   la station, puis appuyer sur le bouton physique.
4. Vérifier `/agvdump` **côté poste** et **côté AGV** (fenêtre de maintenance :
   approcher l'aimant du contact ILS, 10 min).
5. Faire valider par l'atelier que le format `/agvdump` reste exploitable avec
   leurs procédures existantes.
6. Essai de bout en bout : appel depuis chaque bouton, file de 5 courses,
   coupure de liaison volontaire, coupure d'alimentation volontaire.

### Diagnostic sur site

| Symptôme | Où regarder |
|---|---|
| L'AGV ne part pas | `/agvdump` → `write_op_return`, `start_op_return`, `fault` |
| Courses en double | `cmd_duplicate` — si nul, l'idempotence ne voit pas les rejeux |
| Commandes refusées | `tx_refused_duty`, `commands_refused` → modem occupé ou indisponible |
| Appuis sans effet | `enocean_unpaired` → bouton non appairé |
| Liaison qui tombe | `rssi_dbm` (RSRP), `rx_bad_crc`, `telemetry_age_ms` |
| Modem qui se relance | `recoveries` de `SmsTransport` |

---

## 3. Kanban

### 🔴 Bloqué — en attente de relevés ou d'une décision client

| # | Tâche | Débloqué par |
|---|---|---|
| B1 | Renseigner `t_setup_us`, timeouts `Y22`/`Y05`/`Y10` | Relevé oscilloscope V5.0.1 (§12.4, §12.5) |
| ~~B2~~ | ~~Figer la table de brochage SUB-D 25~~ | **FAIT** — relevé client, CN61 à CN64 |
| B3 | Figer la polarité PNP/NPN | Mesure sur l'automate (§12.3) |
| B4 | Recaler le format `/agvdump` | Sortie réelle de la V5.0.1 (§12.6, §3.3) |
| B5 | Choisir la variante d'interface bus | Décision matérielle (§12.10) |
| B6 | Trancher TCM 515 vs TCM 310 | Besoin d'accusé opérateur ? (§12.8) |
| B7 | Écrire l'accès aux E/S UniPi | Runtime réel de la référence commandée (§12.9) — **sans objet** si poste ESP32 ou Gate G100 |
| B7b | **Y a-t-il une prise réseau là où le poste sera fixé ?** | Décide entre l'option B (Gate G100, 1 SIM) et l'option C (E413 LTE, 2 SIM) — 153 € et la moitié du récurrent |
| B8 | Valider la couverture cellulaire du parcours | Relevé RSRP/RSRQ, essai de latence sur 200 aller-retours (Archi_2 §7) |
| B9 | Décider du sort de l'app mobile « AIO AGV Remote » | Décision client (§12.7) |

### 🟡 À faire — prêt à démarrer, sans dépendance externe

| # | Tâche | Effort | Pourquoi |
|---|---|---|---|
| T1 | **Compiler les cibles ESP-IDF** (`pio run -e agv`, `-e poste`, `-e bouton`, `-e mega-bridge`) | M | Jamais compilé ici ; corriger les inévitables erreurs d'API IDF |
| T2 | Exécuter les tests Python sous 3.11 (`pytest`, `ruff`, `mypy --strict`) | S | Jamais exécutés ici |
| T4 | Intégration continue (GitHub Actions : `make test` + build PlatformIO + lint Python) | S | Empêcher toute régression silencieuse |
| T5 | Journalisation structurée à niveaux + tampon circulaire consultable via `/agvdump` | M | Exigé au §13, actuellement `ESP_LOG` seul |
| T6 | Mesure de tension batterie (ADC AGV) et remontée dans la télémétrie | S | Champ prévu dans `agvdump`, non alimenté |
| T7 | Persistance du compteur de nonce AES à chaque émission | S | Aujourd'hui restauré au boot mais jamais réécrit → risque de réutilisation de flux après reset |
| T8 | Diffusion WebSocket sur changement de **défaut** (aujourd'hui : station et mouvement seulement) | S | Un défaut apparu entre deux changements de station tarde à s'afficher |
| T9 | Rendre la période de télémétrie configurable dans le profil | S | Aujourd'hui codée dans `AgvApp` (2 s) ; le budget légal impose de l'adapter au SF |
| T10 | Banc HIL : firmware du second ESP32 jouant l'automate sur les 43 lignes | L | Niveau 2 du §10, pas encore écrit |
| T11 | Faire de `CarteComm/A3_LoRa/` un dossier d'architecture autonome | M | Aujourd'hui simples sources extraites ; recette dans son README |
| T12 | Vérifier la non-divergence du cœur entre dossiers d'architecture | S | Conséquence directe de l'organisation par dossier zippable |

### 🔵 Backlog — utile, non prioritaire

| # | Tâche | Pourquoi |
|---|---|---|
| S1 | Mise à jour OTA du firmware AGV pendant la fenêtre de maintenance | Éviter de démonter la carte à chaque correctif |
| S2 | Export CSV des courses et des défauts depuis l'IHM | Demande probable de l'exploitation |
| S3 | Activer AES-CMAC si le client accepte 4 octets par trame | CTR seul est malléable |
| S4 | Repli automatique MQTT → SMS en cas de perte d'attachement | Seulement si le relevé montre des zones sans couverture data |
| S5 | Page de configuration du profil depuis l'IHM web | Aujourd'hui, tout changement passe par un reflash |
| S6 | Traduction de l'IHM (si opérateurs non francophones) | À confirmer |

### ✅ Fait

| Livrable | Détail |
|---|---|
| Simulateur d'automate MEIDEN | Temps virtuel, profils YAML, injection de défauts |
| Séquenceur trois phases | 12 états, tous les timeouts instrumentés |
| File de 5 courses + persistance NVS | Restauration au boot, politique de validité |
| Protocole complet | Trame, CRC-16, AES-128-CTR, idempotence, anti-rejeu, péremption |
| Trois variantes d'interface bus | MCP23017, 74HC595+165, ATmega2560 + pont |
| `SmsTransport` / `MqttLteTransport` | Pile AT, récupération de modem muet, LWT |
| Organisation par dossier d'architecture | `A1_Cellulaire/` autonome et zippable |
| Chaîne EnOcean | ESP3, déduplication, appairage persisté |
| `AlertGateway` | SMS bas volume, quota, isolé de la chaîne de commande |
| Supervision web + `/agvdump` | WebSocket + repli signalé |
| Poste UniPi | Paquet Python + systemd, backends explicites |
| Wi-Fi de maintenance | Désactivé par défaut, fenêtre de 10 min |
| Outillage | genconfig, flash, rejeu de trames, provisionnement de clé |
| Documentation | Chronogrammes, signaux, protocole MEGA, essais, questions ouvertes |
| **Nomenclature chiffrée** | `BOM.md` : matériel, outillage, récurrent, coût sur 10 ans |
| **Runbook de déploiement** | `DEPLOY.md` : 10 phases, relevés éliminatoires, recette |
| 112 tests natifs | Dont un par ligne du §12 |

---

## 4. Journal des mises à jour

| Date | Modification | Impact sur ce document |
|---|---|---|
| 2026-08-25 | **La passerelle MQTT est remplacée par une passerelle LoRa.** `GatewayApp` publiait sur des topics et dépendait d'`IMqttPublisher` : il n'avait rien à faire dans une architecture sans broker. `LoraGatewayApp` le remplace — trames applicatives de 9 octets sur `ITransport`, accusé par la même voie, idempotence `(node_id, seq)` en table locale, et le **budget de rapport cyclique appliqué aux accusés** puisqu'un récepteur qui acquitte est un émetteur. Le firmware ESP32 est réécrit : plus de Wi-Fi STA ni de client MQTT, mais SPI + `Sx1276Radio` + `LoraTransport`, avec le brochage relevé au KiCad dans `board_pins.h`. `MqttConfig` et `WifiConfig` quittent le profil, la section `lora:` y entre, `aes_enabled` passe à **true** — sans TLS, le chiffrement applicatif est la seule protection de la liaison. Le champ `mqtt_up` d'`/agvdump` est **conservé** : c'est un contrat d'atelier (§3.3), il porte désormais l'état radio. **Kanban LG1 soldé.** | firmware, profils, tests |
| 2026-08-25 | ⚠️ **Deux défauts trouvés en écrivant les tests.** (1) La détection de silence de l'ATmega utilisait `0` comme sentinelle « jamais reçu », alors que c'est un instant valide au démarrage : elle était inopérante pendant les premières millisecondes. Corrigé par un drapeau explicite. (2) Le `Makefile` ne suivait **pas les dépendances d'en-têtes** : modifier une classe ne recompilait pas ses utilisateurs, et l'on reliait des objets compilés contre deux versions de la même classe — un test échouait sans qu'aucune erreur ne soit signalée. `-MMD -MP` ajouté aux **quatre** Makefile. | firmware, Makefile |
| 2026-08-25 | **La carte LoRa est la `AIO_AGV_Control_V6.0`, et elle est FABRIQUÉE.** Le diff des deux projets KiCad est sans ambiguïté : 58 empreintes contre 57, **un seul écart, le `RFM95W-868S2`** — tout le reste est identique au composant près. La radio est intégrée, câblée sur le SPI libre de l'ESP32 : ni carte fille, ni câblage volant. Les nomenclatures chiffraient jusqu'ici une V5.0.1 **réutilisée à 0 €** augmentée d'une carte fille à 18 € ; c'était une hypothèse de travail, remplacée par la nomenclature réelle à **111,15 € HT**. Conséquence assumée : **A3 passe de 208 à 329 €** et **A2 de 307 à 428 €** accessoires compris. Trois analyses devenues fausses sont réécrites — « réutiliser la carte existante », la question des optocoupleurs (la V6.0 a des MOSFET, pas de `PC847`) et le choix `shift595`. La table d'équivalences, jusqu'ici partagée, devient propre à chaque architecture : elle listait des composants absents de la carte concernée. | BOM, README, COMPARAISON |
| 2026-08-25 | **A2 et A3 permutés** : l'hybride EnOcean + LoRa devient **A2**, le LoRa pur devient **A3** — l'hybride a été étudié avant. Les dossiers suivent : `A3_Hybride/` → `A2_Hybride/` et `A2_LoRa/` → `A3_LoRa/`, ainsi que les documents de référence. Chaque dossier ne garde désormais **que sa propre spécification** : la copie étrangère héritée de la duplication est retirée. Le brief et l'index racine sont réordonnés pour se lire A1, A2, A3, A4. **Les variables du générateur cessent d'encoder un numéro d'architecture** — `bouton_a1` devient `bouton_pile`, `poste_a3` devient `poste_enocean`, `a1_ht` devient `ht_lora_pur` : c'est ce qui faisait revenir ce travail à chaque permutation. `subd25_atmega.md` reste exclu, `A2`/`A3` y désignant des broches de connecteur. | arborescence, BOM, brief, README |
| 2026-08-25 | **Réorganisation : un dossier par ARCHITECTURE et non plus par mode de communication.** `SMS_EnOcean/` → `A1_Cellulaire/`, `Wifi/` → `A4_Wifi/`, `LoRa/` → `A3_LoRa/`, et **`A2_Hybride/` est créé**. A3 et A2 étaient jusqu'ici un dossier de sources non autonome : ils reçoivent le cœur métier de A4 — c'est lui qui porte le duo MEGA + ESP32 de la carte V6.0, pas le firmware mono-ESP32 de A1 — plus le transport LoRa, et A2 y ajoute le décodage ESP3 et le poste fixe. **Les quatre dossiers compilent et testent seuls** : 112, 119, 130 et 109 tests. Côté matériel, `AIO_AGV_Control_V6.0` quitte A4 pour A3 et A2 : le relevé KiCad montre que c'est la V5.0.1 **plus un `RFM95W-868S2`** (58 empreintes contre 57), câblé `NSS`→`IO5`, `SCK`→`IO18`, `MISO`→`IO19`, `MOSI`→`IO23`, `DIO0`→`IO26` — exactement le brochage supposé. ⚠️ **`RESET` n'est pas câblée**, `kPinReset` passe à `0xFF` et une carte de kanban est ouverte. ⚠️ Dette assumée : le firmware ESP32 de A3 et A2 porte encore la passerelle MQTT héritée de A4, à remplacer par un `LoraGatewayApp` (kanban LG1). | arborescence, BOM, README, journaux |
| 2026-08-25 | **A1 et A3 permutés** : le cellulaire devient **A1**, le LoRa homogène devient **A3** — la numérotation suit désormais l'ordre chronologique d'étude, le SMS ayant précédé le LoRa. A2 (hybride) et A4 (Wi-Fi) sont inchangés. Permutation appliquée à 10 fichiers, dont **le brief**, dont la table des architectures est aussi réordonnée ; les deux copies restent identiques. Les documents de référence sont renommés — `Archi_1_Cellulaire_SMS_LTE-M.md` et `Archi_3_LoRa_P2P_homogene.md` — et leurs six renvois suivis. **Deux faux positifs protégés** : le repère KiCad `Mega2560 Pro (A1)`, et l'intégralité de `subd25_atmega.md`, où `A1`/`A3` désignent des broches de connecteur et non des architectures. | brief, BOM, README, COMPARAISON, DEPLOY |
| 2026-08-25 | **Numérotation A3–A4 rendue explicite dans les nomenclatures.** Le retrait des accessoires couvrait déjà les quatre architectures (42 lignes), mais seules A3 et A2 portaient leur code : les titres, les sections et les blocs Total nomment désormais **A1** (cellulaire) et **A4** (Wi-Fi) au même titre. `README.md` gagne une colonne `Code` et `COMPARAISON.md` complète ses libellés. `tools/prix_a_completer.md` est aligné : 7 lignes d'accessoires retirées, section « Boîtiers » supprimée, section 2 renommée « Radio », et une **enveloppe de sourcing** (~1 630 € HT) ajoutée — explicitement présentée comme une borne d'effort de recherche et **non un budget**, puisqu'elle additionne des alternatives qui s'excluent. | BOM, README, COMPARAISON, feuille de sourcing |
| 2026-08-25 | **Nomenclatures resserrées sur les composants déterminants.** Les accessoires arbitrables selon le budget — antennes, boîtiers, coffrets, enveloppes murales, câbles, plaques gravées — sont retirés de l'affichage et des sous-totaux : 36 lignes, qui se substituent librement d'un fournisseur à l'autre et n'engagent aucun choix de conception. Chaque `BOM.md` s'ouvre désormais sur un **bloc Total** rappelant, par variante, le montant des accessoires écartés — sans quoi un total amputé se lirait comme un coût d'achat complet. Les définitions restent dans `tools/generer_bom.py`, marquées `core=False`. ⚠️ `README.md` et `COMPARAISON.md` continuent de raisonner **accessoires compris** : comparer des architectures sans leurs boîtiers fausserait le classement. Le point de bascule A3/A2 est d'ailleurs devenu **calculé** au lieu d'être asséné — il passe de 8 à 9 stations, et deux coûts par station encore codés en dur ont été branchés sur les sections. | BOM, README, COMPARAISON |
| 2026-08-21 | Troisième variante d'interface bus chiffrée pour l'architecture LoRa : **`avr_port`**, alignée sur la topologie de la V5.0.1. Un `Mega2560 Pro` porte les 43 lignes sur ses broches, les 11 `PC847` disparaissent, et `avr_port_bus.cpp` — écrit et testé ici — se réutilise avec son relevé de câblage. 12 € HT de plus, une conception matérielle au lieu de deux. **Contrepartie : W1b devient strictement bloquant** — un optocoupleur encaisse 24 V, une broche d'ATmega non. La V5.0.1 relie pourtant ses 21 entrées Y directement depuis cinq ans, ce qui rend l'hypothèse « lignes en logique » très probable sans la démontrer. | BOM LoRa, W1b |
| 2026-08-21 | Colonne **`Lien d'achat`** ajoutée aux nomenclatures et à `tools/prix_a_completer.md`, en remplacement de la colonne `Source` qu'elle rend redondante. **RS est mis en tête partout**, y compris sur les références EnOcean et Unipi qu'il ne distribue habituellement pas : le catalogue évolue et une commande groupée chez un fournisseur référencé vaut souvent le surcoût unitaire. Ce sont des liens de **recherche sur la référence fabricant**, pas des fiches produit — le site de RS bloque l'accès automatisé, aucun numéro de stock n'a donc pu être vérifié. Des numéros inventés auraient produit des liens crédibles menant à la mauvaise pièce. | BOM, feuille de sourcing |
| 2026-08-20 | **Analyse critique** ajoutée à `BOM.md`. Le réservoir capacitif était dimensionné pour des pics de 2 A — valeur d'un `SIM7600E-H` (LTE Cat-1) retenu dans la variante SMS, alors que le `SIM7080G` de la variante recommandée est un modem LTE-M dont les pics sont de 0,5 à 0,7 A. Surtout : l'AGV **et** le poste portent chacun un modem et une SIM, ce qui double le récurrent. Dès que le poste est dans un local technique raccordé en Ethernet, **son modem est inutile** — ~40 € de matériel et la moitié de l'abonnement en moins, sans contrepartie technique. Conséquence directe : le poste UniPi peut descendre en gamme. L'`E413` n'était imposé que par son modem intégré ; sans ce besoin, la gamme **Gate** redevient éligible. Le poste se scinde donc en option **B — Unipi Gate G100** (si Ethernet, 545 € HT au total) et option **C — E413 LTE** (sinon, 698 €), soit 153 € et une SIM d'écart. | §3 kanban, BOM |
| 2026-08-18 | Ajout de [`../BOM.md`](../BOM.md) : nomenclature complète des deux variantes. Écart de ~14 000 € sur dix ans entre SMS et LTE-M, chiffré poste par poste. Ajoute une option de poste ESP32 à 148 € en regard des 439 € de l'UniPi, et le coût comparé des trois variantes d'interface bus — `shift595` est à la fois la moins chère et la plus rapide. | §3 « Fait », journal |
| 2026-08-13 | Création du monorepo complet (§14 étapes 1 à 8), 122 tests verts | Document initial |
| 2026-08-18 | Ajout de [`../DEPLOY.md`](../DEPLOY.md) : procédure de déploiement propre à cette architecture, en 10 phases. Met en tête les quatre décisions préalables — déployer MQTT/LTE-M et non le SMS, faire acter le coût récurrent (~1 500 €/an en SMS contre ~100 € en LTE-M), la couverture cellulaire éliminatoire, et le choix de variante d'interface bus qui conditionne le routage du PCB. | §3 « Fait », journal |
| 2026-08-18 | Rangement : `Archi_1_Cellulaire_SMS_LTE-M.md` rejoint `docs/`, pour que tous les dossiers d'architecture aient la même disposition (documents de référence dans `docs/`, matériel dans `hardware/`). | Liens, journal |
| 2026-08-14 | Relevé de câblage SUB-D 25 fourni par le client : §12.2 et §12.6 (ordre des bits) passent à « relevé ». La table côté AGV vaut pour toutes les architectures ; elle est documentée dans `Wifi/docs/subd25_atmega.md`. | `signal_map.md`, `questions_ouvertes.md`, kanban B2 |
| 2026-08-13 | Réorganisation en un dossier autonome par architecture. Le projet entier passe sous `CarteComm/A1_Cellulaire/` ; les sources LoRa (transport, budget ERC 70-03, SX1276, bouton sur pile, 10 tests) sont extraites vers `CarteComm/A3_LoRa/`. Les deux `main.cpp` passent sur le transport cellulaire. | Périmètre, compteurs (122 → 112 tests), transports, kanban (T3 retiré, T11/T12 ajoutés), déploiement (plus de flash de bouton) |

### Règle de tenue

À chaque modification du dépôt :

1. Mettre à jour le **compteur de tests** (§1.1) si le nombre change.
2. Déplacer la ou les cartes concernées dans le **Kanban** (§3).
3. Ajouter une ligne au **journal** ci-dessus.
4. Si un point du §12 est relevé : mettre à jour `profiles/*.yaml`, cocher
   [`questions_ouvertes.md`](questions_ouvertes.md), et retirer la carte
   correspondante de la colonne « Bloqué ».
5. **Si la modification touche le cœur métier** (séquenceur, file, protocole,
   simulateur), la reporter dans les autres dossiers d'architecture de
   `CarteComm/` — ils en portent une copie.
