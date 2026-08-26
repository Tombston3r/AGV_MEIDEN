# État du projet — architecture Wi-Fi (carte V5.0.1 conservée)

> **Document vivant.** Mis à jour à chaque modification du dossier.
>
> Dernière mise à jour : **2026-08-18** — poste fixe ramené à une passerelle
> Unipi Gate G100, ce qui lève le point ouvert §12.9.
>
> **Périmètre** : `architectures/A4_Wifi/`, projet autonome et zippable. La carte AIO
> AGV Control V5.0.1 est **conservée** ; ses **deux firmwares sont réécrits**.
> Index des architectures : [`../../README.md`](../../README.md).
> Spécification suivie : [`Planification_Architecture_WiFi_AGV.md`](Planification_Architecture_WiFi_AGV.md).

---

## 1. Ce qui est fait

### 1.1 Vue d'ensemble

| Indicateur | Valeur |
|---|---|
| Tests natifs C++ | **109 tests, 537 assertions, 0 échec** |
| Tests Python (poste UniPi) | **17 tests, 0 échec** (rejoués à la main, `pytest` absent du poste) |
| Compilation | `-std=c++17 -Wall -Wextra -Werror`, sans avertissement |
| Matériel nécessaire | **aucun** |
| Firmwares réécrits | ESP32 **et** ATmega2560 |

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

### 1.2 Répartition des rôles — la décision structurante

Détail et justification : [`carte_v5_architecture.md`](carte_v5_architecture.md).

| Microcontrôleur | Porte | Ne porte pas |
|---|---|---|
| **ATmega2560** | séquenceur trois phases, file de 5 courses, décodage position 10 bits, repli de sécurité | rien de réseau |
| **ESP32** | client Wi-Fi STA, client MQTT, heartbeat, AP de maintenance, `/agvdump` | **jamais le bus MEIDEN** |

Motif : chaque maillon réseau (Wi-Fi, DHCP, handover, TLS, broker, poste) peut
tomber pour des raisons hors projet. Le séquenceur ne doit dépendre d'aucun.

### 1.3 Repli de sécurité — le mécanisme central

L'ESP32 émet un heartbeat toutes les 500 ms. Sans heartbeat pendant 2 s,
l'ATmega termine la course engagée **jusqu'au point d'arrêt suivant**, refuse
toute nouvelle course, et le signale (`safe_stop`).

Vérifié par 6 tests dédiés, dont :

- refus de toute course tant qu'aucun heartbeat n'a été reçu (état au boot) ;
- perte en pleine course → la course va au bout, la suivante n'est pas lancée ;
- **le retour du heartbeat ne relance rien tout seul** ;
- un heartbeat manqué isolément n'immobilise pas l'AGV.

### 1.4 Firmware ATmega2560 (réécrit)

| Élément | État |
|---|---|
| Séquenceur trois phases, 12 états, tous timeouts instrumentés | ✅ |
| File de 5 courses, priorité, purge | ✅ (RAM, planification §2.3) |
| Décodage position 10 bits `Y23`…`Y34` et vitesse `Y11`…`Y14` | ✅ |
| Pose des 22 lignes en **une seule section critique** (5 écritures de port) | ✅ testé |
| Masquage des ports mixtes PA/PB/PG (DDR et données) | ✅ testé |
| Étage de sortie **poussé** — la broche attaque une grille de MOSFET | ✅ testé |
| Table de câblage réelle CN61→CN64 | ✅ relevée |
| Idempotence : même séquence ré-acquittée sans ré-exécution | ✅ |
| Repli heartbeat | ✅ |
| **Mode découverte du brochage SUB-D** | ✅ (contrôle, plus relevé) |

### 1.5 Firmware ESP32 (réécrit)

| Élément | État |
|---|---|
| Client Wi-Fi **STA** (plus de point d'accès permanent) | ✅ |
| IP statique pour supprimer le délai DHCP au handover | ✅ |
| Client MQTT : `state` (retained, 1 s), `cmd`, `ack`, LWT sur `status` | ✅ |
| Heartbeat vers l'ATmega, **maintenu même sans Wi-Fi** | ✅ testé |
| Péremption des commandes (30 s), doublons, désordre | ✅ testé |
| AP de maintenance à la demande (ILS, 10 min) en **APSTA** | ✅ |
| `/agvdump` au format atelier | ✅ testé |

### 1.6 Liaison inter-MCU

Protocole documenté dans `link/link_protocol.h` : SOF distinct par sens,
CRC-16/CCITT, resynchronisation automatique, longueur invalide rejetée.
7 tests dédiés.

### 1.7 Poste fixe UniPi

| Élément | État |
|---|---|
| Décodeur ESP3 (TCM 515) en Python | ✅ |
| Déduplication des 3 sous-télégrammes PTM 210 | ✅ |
| Table d'appairage persistée + mode appairage | ✅ |
| Publication MQTT horodatée, séquence roulante | ✅ |
| Traçabilité des appuis (`poste/1/button/<id>`) | ✅ |
| Indicateur de fraîcheur de l'état | ✅ |
| Service systemd | ✅ |

### 1.8 Ce qui n'a **pas** pu être vérifié ici

- **Aucun des deux firmwares n'a été compilé pour sa cible** : ni PlatformIO,
  ni ESP-IDF, ni avr-gcc sur ce poste. Le cœur métier compile en natif avec
  `-Werror`, ce qui couvre la logique, pas les API ESP-IDF/Arduino.
- **`pytest` absent** : les 17 tests Python ont été rejoués par un harnais
  maison, pas par pytest.
- **Aucun essai matériel** : ni carte, ni banc, ni AGV, ni relevé réseau.
- **L'amplitude des lignes Y reste inconnue** (W1b), et la nomenclature KiCad
  confirme qu'**aucune protection n'est prévue** : la carte ne compte que quatre
  résistances hors étage de sortie, deux diviseurs de mesure. Les 21 lignes `Y`
  arrivent directement sur les broches du Mega. C'est la mesure la plus urgente.
- **La tension V_CC réelle de l'ATmega n'est pas mesurée** (W1e) : 6 V est le
  maximum absolu du datasheet.

---

## 2. Déploiement en conditions réelles

> 📖 **Procédure opérationnelle détaillée : [`../DEPLOY.md`](../DEPLOY.md)** —
> checklists, commandes, essais de recette et fiche à viser. Ce qui suit en est
> le résumé ; en cas d'écart, c'est `DEPLOY.md` qui fait foi.
>
> ⚠️ **Ordre non négociable.** Les étapes 0 à 2 conditionnent tout le reste.

### Étape 0 — Prérequis bloquants (planification §3)

| # | Tâche | Pourquoi bloquant |
|---|---|---|
| 0.1 | **Accord du service informatique** : VLAN OT, IP, pare-feu, 802.1X, notification de changement | Chemin critique du projet, délai hors de votre contrôle. À lancer le jour 1 |
| 0.2 | **Relevé de couverture Wi-Fi à hauteur d'antenne AGV**, trajet complet, en production | Un relevé fait à 1,50 m avec un portable ne vaut rien |
| 0.3 | Test de portée EnOcean bouton → poste, à chaque marqueur | Un bouton hors portée échoue **silencieusement** |
| 0.4a | ⚠️ **Mesurer l'amplitude d'une ligne Y** (`Y05`) et la tension V_CC de l'ATmega | Les Y arrivent directement sur des broches ; au-delà de V_CC + 0,5 V l'entrée est détruite. **Avant tout branchement** |
| 0.4c | Mesurer le tirage des entrées de l'automate | Décide entre collecteur ouvert et sortie poussée |
| 0.4b | Chronogramme X/Y, `t_setup`, PNP/NPN | Sans ça, les timings du profil sont des devinettes |
| 0.5 | Tentative de lecture des flash existantes (`esptool`, `avrdude`/ICSP) | Peut confirmer ou infirmer les hypothèses de câblage |
| 0.7 | **Décision de bande** : 2,4 GHz, ESP32-C5 bi-bande, ou bridge industriel | La saturation 2,4 GHz est le problème d'origine |

### Étape 1 — Poste de développement

```bash
pip install platformio
cd poste-unipi && pip install -e '.[dev,mqtt,enocean_serial]' && cd ..

python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
cd poste-unipi && python3 -m pytest tests -q && cd ..
```

### Étape 2 — Contrôle du brochage SUB-D, automate débranché

La table est **relevée** ([`subd25_atmega.md`](subd25_atmega.md)) ; il reste à
vérifier qu'aucune nappe n'est sertie à l'envers. Une erreur de sertissage
n'échoue pas : elle envoie l'AGV à la mauvaise station.

```bash
pio run -e mega -t upload        # via l'ICSP ou le bootloader, selon la carte
pio device monitor -b 115200     # puis envoyer 'd'
```

Reporter le relevé dans `board_ports.h` et `profiles/default.yaml`, régénérer,
relancer `make test`.

### Étape 3 — Renseigner le profil

Éditer `profiles/default.yaml` **et rien d'autre** : timings relevés (§12.4,
§12.5), polarité (§12.3), paramètres réseau (W5) et MQTT (W6).

### Étape 4 — Infrastructure (planification §4)

1. VLAN, adressage, pare-feu côté client.
2. Mosquitto sur l'UniPi : TLS, authentification par utilisateur, **ACL par
   topic** — l'AGV ne doit pouvoir publier que sur `agv/1/#`.
3. Validation du positionnement des AP si le relevé 0.2 montre des trous.
4. Mesure du temps de reconnexion au handover, IP statique activée.

### Étape 5 — Flash des deux firmwares

```bash
pio run -e mega  -t upload       # ATmega2560 : séquenceur + file
pio run -e esp32 -t upload       # ESP32 : Wi-Fi + MQTT + heartbeat
```

⚠️ Flasher l'ATmega **en premier** : au démarrage il est en `safe_stop` et
refuse toute course tant que l'ESP32 ne bat pas. L'ordre inverse laisse un ESP32
qui parle dans le vide.

### Étape 6 — Poste fixe UniPi

```bash
sudo useradd -r -s /usr/sbin/nologin agv
sudo mkdir -p /opt/agv /etc/agv /var/lib/agv
sudo python3.11 -m venv /opt/agv/venv
sudo /opt/agv/venv/bin/pip install './poste-unipi[mqtt,enocean_serial]'
sudo cp poste-unipi/poste.example.toml /etc/agv/poste.toml
sudo $EDITOR /etc/agv/poste.toml       # port TCM 515, broker, identifiants
sudo cp poste-unipi/systemd/agv-poste.service /etc/systemd/system/
sudo systemctl enable --now agv-poste
journalctl -u agv-poste -f
```

### Étape 7 — Mise en service

1. Appairer chaque bouton EnOcean depuis l'IHM, puis **vérifier chaque bouton
   depuis son emplacement définitif** (la portée se teste là où le bouton sera
   posé, pas sur l'établi).
2. Vérifier `/agvdump` : approcher l'aimant du contact ILS, 10 minutes.
3. Essais de dégradation, dans cet ordre : coupure AP, redémarrage UniPi,
   **débranchement de la liaison série ESP32↔ATmega en pleine course**, bouton
   hors portée.
4. Campagne en heures de production : latence P50/P95/P99, taux de perte,
   handovers (planification §7.4).

### Diagnostic sur site

| Symptôme | Où regarder |
|---|---|
| L'AGV refuse toutes les courses | `safe_stop` dans `/agvdump` ou MQTT → heartbeat perdu, donc ESP32 ou liaison série |
| L'AGV ne part pas mais accepte | `write_op_return`, `start_op_return`, `fault` |
| Appels perdus par moments | `rssi_dbm`, relevé 0.2 insuffisant, handover |
| Commandes ignorées | `cmd_expired` → horloge du poste ou latence réseau |
| Courses en double | `cmd_duplicate` — si nul, l'idempotence ne voit pas les rejeux |
| Appuis sans effet | `unpaired` côté poste → bouton non appairé, ou hors portée |
| État figé dans l'IHM | indicateur de fraîcheur : LWT `offline` sur `agv/1/status` |

---

## 3. Kanban

### 🔴 Bloqué — relevé ou décision client

| # | Tâche | Débloqué par |
|---|---|---|
| LG2 | Câbler une GPIO vers `RESET` du RFM95W sur une révision V6.1 | Relevé V6.0 : la broche n'est pas connectée |
| B1 | Renseigner `t_setup_us` et les timeouts `Y22`/`Y05`/`Y10` | Relevé à l'analyseur logique (0.4, §12.4-12.5) |
| ~~B2~~ | ~~Qualifier l'amplitude des lignes Y~~ | ✅ **vérifié par le client le 2026-08-21** |
| B2c | Confirmer la topologie de l'étage de sortie (`x_open_drain`) | Mesure du tirage côté automate (W1d) |
| B2d | Vérifier la tension V_CC de l'ATmega | 6 V = maximum absolu du datasheet (W1e) |
| B2b | Trancher les pull-ups internes sur les Y | Sorties automate à collecteur ouvert ou poussées ? (W1c) |
| B3 | Figer la polarité PNP/NPN | Mesure sur l'automate (§12.3) |
| B4 | Recaler le format `/agvdump` | Sortie réelle de la V5.0.1 (§12.6, §3.3) |
| B5 | Paramètres réseau et MQTT | Accord du service informatique (0.1, W5, W6) |
| ~~B6~~ | ~~Confirmer l'UART ESP32 ↔ ATmega~~ | ✅ **relevé au KiCad** : SoftwareSerial D52/D53, 38 400 bd |
| B6b | Vérifier au banc la tenue de `SoftwareSerial` à 38 400 bauds sous charge du séquenceur | L'émission logicielle masque les interruptions pendant chaque octet |
| B7 | Décider de la bande 2,4 GHz vs bi-bande | Arbitrage 0.7 |
| B2e | ⚠️ **Trancher l'étage de sortie** — `IRF520` (hors spec à 5 V), `IRL520` (logic-level, même brochage) ou `ULN2803A` | Le PCB est-il déjà fabriqué ? (analyse `BOM.md`) |
| B2f | Décider si l'isolation galvanique est requise — le `TDN 5-2411WISM` isolé cohabite avec des MOSFET à masse commune | Arbitrage client ; ~22 € en jeu (analyse `BOM.md`) |
| ~~B8~~ | ~~Runtime de l'UniPi~~ | **LEVÉ** si le Gate G100 est retenu : livré sous Debian |
| B9 | Voyant d'accusé opérateur | Décision client (planification §3.6) |

### 🟡 À faire — sans dépendance externe

| # | Tâche | Effort |
|---|---|---|
| T1 | **Compiler les deux firmwares** (`pio run -e esp32`, `-e mega`) et corriger les erreurs d'API | M |
| T2 | Vérifier l'empreinte RAM/flash de l'ATmega (8 Ko de SRAM) | S |
| T3 | Exécuter les tests Python sous 3.11 avec `pytest`, `ruff`, `mypy --strict` | S |
| T4 | Interface web de supervision servie par le poste (WebSocket) | M |
| T5 | Journal d'événements côté poste (planification §3.5) | M |
| T6 | Intégration continue (`make test` + build PlatformIO + lint Python) | S |
| T7 | Persistance EEPROM de la file sur l'ATmega, en option | S |
| T8 | Surveiller une ligne de heartbeat matérielle si le relevé en révèle une | S |
| T9 | Banc HIL : firmware du simulateur d'automate sur les 43 lignes | L |
| T10 | Durcissement Mosquitto : ACL par topic, rotation des certificats | S |

### 🔵 Backlog

| # | Tâche |
|---|---|
| S1 | Mise à jour OTA de l'ESP32 pendant la fenêtre de maintenance |
| S2 | Reflash de l'ATmega depuis l'ESP32 (l'ESP32 en programmateur ISP) |
| S3 | Historique long terme et export CSV côté poste |
| S4 | Repli automatique sur une bande ISM si le Wi-Fi est durablement perdu |

### ✅ Fait

| Livrable | Détail |
|---|---|
| Firmware ATmega2560 réécrit | Séquenceur, file, repli heartbeat, mode découverte |
| Firmware ESP32 réécrit | Wi-Fi STA, MQTT, heartbeat, AP de maintenance, agvdump |
| Protocole inter-MCU | CRC-16, SOF par sens, resynchronisation |
| JSON MQTT | Sérialiseur et analyseur minimaux, testés |
| Driver de bus AVR | Pose des 22 lignes en une section critique |
| Mode découverte du brochage | Une ligne à la fois, pour le relevé au multimètre |
| Poste fixe UniPi | ESP3, déduplication, appairage, MQTT, fraîcheur |
| Simulateur d'automate | Repris du cœur commun |
| 101 tests C++ + 17 tests Python | Dont 6 sur le repli heartbeat |
| Documentation | Architecture de la carte, brochage SUB-D, essais, questions |
| Matériel | Projet KiCad de la carte V5.0.1 et photo d'assemblage dans `hardware/` |
| **Nomenclature chiffrée** | `BOM.md` : matériel, outillage, récurrent, coût sur 10 ans |
| **Runbook de déploiement** | `DEPLOY.md` : 11 phases, checklists, recette, retour arrière |

---

## 4. Journal des mises à jour

| Date | Modification | Impact |
|---|---|---|
| 2026-08-26 | **Réorganisation du dépôt par NATURE d'objet.** `architectures/` (les 4 solutions), `bancs/` (les essais matériels), `materiel/` (les projets KiCad), `docs/` (brief et comparatif), `outils/` (scripts). Le dossier `CarteComm/` disparaît. **Le matériel n'est plus dupliqué** : une carte, un projet, un seul endroit — trois copies divergentes de la V6.0 avaient déjà coexisté, dont une que KiCad avait recréée à un chemin renommé, et il a fallu comparer les schémas pour trouver la vivante (celle du 26 août, avec l'embase J3 et les Gerbers). Le **brief** passe de 4 copies à 1. Les **bancs LoRa sortent de `A3_LoRa/test/`** pour rejoindre `bancs/lora/`, à la même forme que le banc EnOcean ; `test/native/` ne garde que les tests unitaires. Chaque banc a désormais son `README.md` **et** son `DEPLOY.md` avec une recette et les résultats attendus. `outils/exporter_architecture.sh` reconstitue une architecture autonome dans un zip — la zippabilité est payée à l'export, plus tous les jours. Le générateur de nomenclatures n'a plus de chemin absolu. **2 Mo de redondance supprimés.** | arborescence, tous les renvois |
| 2026-08-25 | **La passerelle MQTT est remplacée par une passerelle LoRa.** `GatewayApp` publiait sur des topics et dépendait d'`IMqttPublisher` : il n'avait rien à faire dans une architecture sans broker. `LoraGatewayApp` le remplace — trames applicatives de 9 octets sur `ITransport`, accusé par la même voie, idempotence `(node_id, seq)` en table locale, et le **budget de rapport cyclique appliqué aux accusés** puisqu'un récepteur qui acquitte est un émetteur. Le firmware ESP32 est réécrit : plus de Wi-Fi STA ni de client MQTT, mais SPI + `Sx1276Radio` + `LoraTransport`, avec le brochage relevé au KiCad dans `board_pins.h`. `MqttConfig` et `WifiConfig` quittent le profil, la section `lora:` y entre, `aes_enabled` passe à **true** — sans TLS, le chiffrement applicatif est la seule protection de la liaison. Le champ `mqtt_up` d'`/agvdump` est **conservé** : c'est un contrat d'atelier (§3.3), il porte désormais l'état radio. **Kanban LG1 soldé.** | firmware, profils, tests |
| 2026-08-25 | ⚠️ **Deux défauts trouvés en écrivant les tests.** (1) La détection de silence de l'ATmega utilisait `0` comme sentinelle « jamais reçu », alors que c'est un instant valide au démarrage : elle était inopérante pendant les premières millisecondes. Corrigé par un drapeau explicite. (2) Le `Makefile` ne suivait **pas les dépendances d'en-têtes** : modifier une classe ne recompilait pas ses utilisateurs, et l'on reliait des objets compilés contre deux versions de la même classe — un test échouait sans qu'aucune erreur ne soit signalée. `-MMD -MP` ajouté aux **quatre** Makefile. | firmware, Makefile |
| 2026-08-25 | **La carte LoRa est la `AIO_AGV_Control_V6.0`, et elle est FABRIQUÉE.** Le diff des deux projets KiCad est sans ambiguïté : 58 empreintes contre 57, **un seul écart, le `RFM95W-868S2`** — tout le reste est identique au composant près. La radio est intégrée, câblée sur le SPI libre de l'ESP32 : ni carte fille, ni câblage volant. Les nomenclatures chiffraient jusqu'ici une V5.0.1 **réutilisée à 0 €** augmentée d'une carte fille à 18 € ; c'était une hypothèse de travail, remplacée par la nomenclature réelle à **111,15 € HT**. Conséquence assumée : **A3 passe de 208 à 329 €** et **A2 de 307 à 428 €** accessoires compris. Trois analyses devenues fausses sont réécrites — « réutiliser la carte existante », la question des optocoupleurs (la V6.0 a des MOSFET, pas de `PC847`) et le choix `shift595`. La table d'équivalences, jusqu'ici partagée, devient propre à chaque architecture : elle listait des composants absents de la carte concernée. | BOM, README, COMPARAISON |
| 2026-08-25 | **A2 et A3 permutés** : l'hybride EnOcean + LoRa devient **A2**, le LoRa pur devient **A3** — l'hybride a été étudié avant. Les dossiers suivent : `A3_Hybride/` → `A2_Hybride/` et `A2_LoRa/` → `A3_LoRa/`, ainsi que les documents de référence. Chaque dossier ne garde désormais **que sa propre spécification** : la copie étrangère héritée de la duplication est retirée. Le brief et l'index racine sont réordonnés pour se lire A1, A2, A3, A4. **Les variables du générateur cessent d'encoder un numéro d'architecture** — `bouton_a1` devient `bouton_pile`, `poste_a3` devient `poste_enocean`, `a1_ht` devient `ht_lora_pur` : c'est ce qui faisait revenir ce travail à chaque permutation. `subd25_atmega.md` reste exclu, `A2`/`A3` y désignant des broches de connecteur. | arborescence, BOM, brief, README |
| 2026-08-25 | **Réorganisation : un dossier par ARCHITECTURE et non plus par mode de communication.** `SMS_EnOcean/` → `A1_Cellulaire/`, `Wifi/` → `A4_Wifi/`, `LoRa/` → `A3_LoRa/`, et **`A2_Hybride/` est créé**. A3 et A2 étaient jusqu'ici un dossier de sources non autonome : ils reçoivent le cœur métier de A4 — c'est lui qui porte le duo MEGA + ESP32 de la carte V6.0, pas le firmware mono-ESP32 de A1 — plus le transport LoRa, et A2 y ajoute le décodage ESP3 et le poste fixe. **Les quatre dossiers compilent et testent seuls** : 112, 119, 130 et 109 tests. Côté matériel, `AIO_AGV_Control_V6.0` quitte A4 pour A3 et A2 : le relevé KiCad montre que c'est la V5.0.1 **plus un `RFM95W-868S2`** (58 empreintes contre 57), câblé `NSS`→`IO5`, `SCK`→`IO18`, `MISO`→`IO19`, `MOSI`→`IO23`, `DIO0`→`IO26` — exactement le brochage supposé. ⚠️ **`RESET` n'est pas câblée**, `kPinReset` passe à `0xFF` et une carte de kanban est ouverte. ⚠️ Dette assumée : le firmware ESP32 de A3 et A2 porte encore la passerelle MQTT héritée de A4, à remplacer par un `LoraGatewayApp` (kanban LG1). | arborescence, BOM, README, journaux |
| 2026-08-25 | **A1 et A3 permutés** : le cellulaire devient **A1**, le LoRa homogène devient **A3** — la numérotation suit désormais l'ordre chronologique d'étude, le SMS ayant précédé le LoRa. A2 (hybride) et A4 (Wi-Fi) sont inchangés. Permutation appliquée à 10 fichiers, dont **le brief**, dont la table des architectures est aussi réordonnée ; les deux copies restent identiques. Les documents de référence sont renommés — `Archi_1_Cellulaire_SMS_LTE-M.md` et `Archi_3_LoRa_P2P_homogene.md` — et leurs six renvois suivis. **Deux faux positifs protégés** : le repère KiCad `Mega2560 Pro (A1)`, et l'intégralité de `subd25_atmega.md`, où `A1`/`A3` désignent des broches de connecteur et non des architectures. | brief, BOM, README, COMPARAISON, DEPLOY |
| 2026-08-25 | **Numérotation A3–A4 rendue explicite dans les nomenclatures.** Le retrait des accessoires couvrait déjà les quatre architectures (42 lignes), mais seules A3 et A2 portaient leur code : les titres, les sections et les blocs Total nomment désormais **A1** (cellulaire) et **A4** (Wi-Fi) au même titre. `README.md` gagne une colonne `Code` et `COMPARAISON.md` complète ses libellés. `tools/prix_a_completer.md` est aligné : 7 lignes d'accessoires retirées, section « Boîtiers » supprimée, section 2 renommée « Radio », et une **enveloppe de sourcing** (~1 630 € HT) ajoutée — explicitement présentée comme une borne d'effort de recherche et **non un budget**, puisqu'elle additionne des alternatives qui s'excluent. | BOM, README, COMPARAISON, feuille de sourcing |
| 2026-08-25 | **Nomenclatures resserrées sur les composants déterminants.** Les accessoires arbitrables selon le budget — antennes, boîtiers, coffrets, enveloppes murales, câbles, plaques gravées — sont retirés de l'affichage et des sous-totaux : 36 lignes, qui se substituent librement d'un fournisseur à l'autre et n'engagent aucun choix de conception. Chaque `BOM.md` s'ouvre désormais sur un **bloc Total** rappelant, par variante, le montant des accessoires écartés — sans quoi un total amputé se lirait comme un coût d'achat complet. Les définitions restent dans `tools/generer_bom.py`, marquées `core=False`. ⚠️ `README.md` et `COMPARAISON.md` continuent de raisonner **accessoires compris** : comparer des architectures sans leurs boîtiers fausserait le classement. Le point de bascule A3/A2 est d'ailleurs devenu **calculé** au lieu d'être asséné — il passe de 8 à 9 stations, et deux coûts par station encore codés en dur ont été branchés sur les sections. | BOM, README, COMPARAISON |
| 2026-08-22 | **Essais radio LoRa** ajoutés dans `../../A3_LoRa/test/`, en deux sous-dossiers `esp32/` et `unipi/`, chacun avec un essai d'émission et un de réception. Les quatre combinaisons sont possibles ; les deux croisées sont celles qui comptent, car elles seules éprouvent l'interopérabilité. Les deux côtés parlent la **trame applicative du projet**, et `test_interop_trame.py` — le seul essai qui tourne sans matériel — compile le codec C++ du cœur pour vérifier que le portage Python encode **octet pour octet la même chose** : 6 vecteurs, extension d'horodatage comprise, 0 écart. Le budget de rapport cyclique est appliqué des deux côtés, accusés compris. | `LoRa/test/`, journal |
| 2026-08-21 | ⚠️ **Correction de firmware — la liaison inter-MCU n'est pas un UART matériel.** Le relevé du projet KiCad montre `ESP32 IO17` → `MEGA D52` en direct, et `MEGA D53` → pont 2,2 k/4,7 k → `ESP32 IO16`. Or D52/D53 ne sont pas des broches d'UART sur le MEGA, et **ses trois UART sont inutilisables** : leurs broches de réception portent `Y13` (D19/PD2), `Y11` (D17/PH0) et `Y05` (D15/PJ0). Le `Serial1.begin()` que contenait `firmware/mega/src/main.cpp` aurait mis **Y13 en sortie contre la sortie de l'automate**. Passage en `SoftwareSerial` sur D52/D53 et **abaissement à 38 400 bauds**, 115 200 n'étant pas tenable en émulation logicielle sur AVR. **W2 est clos.** | §1.6, W2, `profiles/default.yaml`, 109 tests toujours verts |
| 2026-08-21 | **W1b clos** : le client a vérifié l'amplitude des lignes Y, la connexion directe sur broches d'ATmega est confirmée compatible. La valeur mesurée reste à consigner dans `questions_ouvertes.md` pour la traçabilité. | W1b, kanban B2 |
| 2026-08-21 | Troisième variante d'interface bus chiffrée pour l'architecture LoRa : **`avr_port`**, alignée sur la topologie de la V5.0.1. Un `Mega2560 Pro` porte les 43 lignes sur ses broches, les 11 `PC847` disparaissent, et `avr_port_bus.cpp` — écrit et testé ici — se réutilise avec son relevé de câblage. 12 € HT de plus, une conception matérielle au lieu de deux. **Contrepartie : W1b devient strictement bloquant** — un optocoupleur encaisse 24 V, une broche d'ATmega non. La V5.0.1 relie pourtant ses 21 entrées Y directement depuis cinq ans, ce qui rend l'hypothèse « lignes en logique » très probable sans la démontrer. | BOM LoRa, W1b |
| 2026-08-21 | Colonne **`Lien d'achat`** ajoutée aux nomenclatures et à `tools/prix_a_completer.md`, en remplacement de la colonne `Source` qu'elle rend redondante. **RS est mis en tête partout**, y compris sur les références EnOcean et Unipi qu'il ne distribue habituellement pas : le catalogue évolue et une commande groupée chez un fournisseur référencé vaut souvent le surcoût unitaire. Ce sont des liens de **recherche sur la référence fabricant**, pas des fiches produit — le site de RS bloque l'accès automatisé, aucun numéro de stock n'a donc pu être vérifié. Des numéros inventés auraient produit des liens crédibles menant à la mauvaise pièce. | BOM, feuille de sourcing |
| 2026-08-20 | **Analyse critique de la carte routée** ajoutée à `BOM.md`. Trois points relevés sur le projet KiCad. (1) L'`IRF520` n'est **pas** un MOSFET logic-level : seuil spécifié de 2 à 4 V, `Rds(on)` garanti à Vgs = 10 V. Il conduit sous 5 V et convient aux quelques milliampères d'une entrée d'automate, mais hors conditions constructeur ; l'`IRL520` est une substitution sans reroutage, l'`ULN2803A` divise la surface par dix. (2) Le convertisseur **isolé** `TDN 5-2411WISM` — poste le plus cher de la carte — cohabite avec un étage de sortie à **masse commune** : l'isolation ne protège pas les signaux, et si elle ne visait qu'eux, un `TSR 1-2450` économise ~22 €. (3) Le 6 V du `L7806CV` reste au maximum absolu de l'ATmega alors qu'un 5 V propre existe déjà sur la carte. Aucun changement de nomenclature : ces trois points sont des **décisions**, pas des corrections. | §3 kanban B2e/B2f, BOM |
| 2026-08-18 | Poste fixe : l'automate UniPi est remplacé par une passerelle **Unipi Gate G100** (~200 € au lieu de 375 €). Le service n'utilise qu'un port série et de l'Ethernet — les boutons étant EnOcean, aucune entrée TOR n'est employée, `io_backend.py` n'est même pas appelé. Trois gains au-delà du prix : le Gate est livré sous **Debian**, ce qui **lève le point ouvert §12.9** (Mervis ou Linux) ; il offre **deux ports Ethernet**, donc une séparation physique OT/IT ; et 16 Go d'eMMC au lieu de 8. Total de l'architecture : 692 € au lieu de 867 € HT. | §2 étape 0.6, §3 kanban B8, BOM, comparatif |
| 2026-08-18 | Nomenclature de la carte AGV **extraite du projet KiCad** (57 composants placés) au lieu d'être estimée. Deux découvertes : l'étage de sortie est à **23 MOSFET IRF520** avec résistances de grille — le collecteur ouvert est fait par le matériel, donc le microcontrôleur doit être en **sortie poussée** (`x_open_drain: false`, corrigé ; une grille flottante mettrait le MOSFET dans un état indéterminé) — et **aucune protection n'existe sur les 21 entrées Y**, ce qui confirme W1b. La carte est fabriquée et non réutilisée : le total passe de 738 € à 867 € HT. | §1.1 (109 tests), §1.4, §1.8, BOM, comparatif |
| 2026-08-18 | Ajout de [`../BOM.md`](../BOM.md) : nomenclature complète. La carte AGV coûte 0 € — elle est conservée — mais deux postes propres à cette architecture apparaissent : le harnais de raccordement (42 €) et une adaptation de niveaux conservatoire (0 à 45 €) tant que W1b n'est pas mesuré. Total ~738 € sur 10 ans, dont 461 € pour le seul poste UniPi. | §3 « Fait », journal |
| 2026-08-18 | Rangement du dossier : le projet KiCad de la carte d'origine est sous `../../materiel/AIO_AGV_Control_V5.0.1/`, la photo d'assemblage sous `hardware/photos/`, et `Planification_Architecture_WiFi_AGV.md` rejoint `docs/` pour aligner tous les dossiers d'architecture sur la même disposition. Les sauvegardes automatiques KiCad (35 archives, 3,6 Mo) sortent du suivi Git : elles sont régénérées à chaque ouverture du projet. | Liens, §3 « Fait », journal |
| 2026-08-14 | Ajout de [`../DEPLOY.md`](../DEPLOY.md) : procédure de déploiement en 11 phases, des mesures préalables au procès-verbal de recette. Met en tête les trois points irréversibles ou bloquants — sauvegarde des firmwares d'origine (sans laquelle il n'y a aucun retour arrière), niveaux du bus non mesurés, accord du service informatique. Inclut les commandes de sauvegarde `esptool`/`avrdude`, la configuration durcie de Mosquitto avec ACL par topic, dix essais de dégradation et une fiche de recette à viser. | §2 renvoie au runbook, §3 « Fait », journal |
| 2026-08-14 | Correction (analyse PCB) : **le L7806CV est l'ALIMENTATION de l'ATmega** — 24 V venant de CN64 A6/B6, abaissés à 6 V. Ce n'est donc ni un étage de sortie, ni un niveau de signal. Conséquence : les niveaux du bus redeviennent entièrement inconnus, l'amplitude des lignes Y (W1b) repasse BLOQUANT, et une question nouvelle apparaît (W1e) — 6 V est le maximum absolu du datasheet de l'ATmega2560, à vérifier sur V_CC. Le mode collecteur ouvert est CONSERVÉ, avec une justification corrigée : il ne peut rien détruire, à défaut de connaître la topologie. Aucun changement de code, uniquement les justifications. | §1.8, kanban B2/B2c/B2d, étapes 0.4a et 0.4c |
| 2026-08-14 | ~~Correction : le rail 6 V (LM7806) serait sur l'étage de SORTIE~~ — **infirmé le jour même par l'analyse PCB**, voir la ligne ci-dessus. Reste de cette étape : l'ajout du mode collecteur ouvert, qui garde sa valeur., pas sur les entrées. L'automate attend du 6 V là où l'ATmega sort 5 V. Ajout d'un mode **collecteur ouvert** (`bus.x_open_drain`, actif par défaut) : la broche tire à la masse ou passe en haute impédance, elle ne sort jamais de niveau haut — ce qui évite de remonter du courant dans la diode de protection si l'automate tire ses entrées à 6 V. W1b requalifié : ce n'est plus un risque de destruction des entrées, c'est un choix de topologie de sortie à confirmer par la mesure. | Compteurs (105 → 108 tests), §1.4, §1.8, kanban B2, étape 0.4a |
| 2026-08-14 | Intégration du **relevé de câblage SUB-D 25** fourni par le client. Le driver de bus AVR est réécrit : table bit à bit sur 11 ports au lieu de 3 ports contigus supposés, masquage obligatoire des 3 ports mixtes (PA/PB/PG), pull-ups paramétrables. §12.2, §12.6 et W1 passent à « relevé » ; W1b (amplitude des Y face aux entrées 5 V) devient le point bloquant le plus urgent. | Compteurs (101 → 105 tests), §1.4, §1.8, kanban B2/B2b, étapes 0.4a et 2 |
| 2026-08-14 | Création du dossier d'architecture Wi-Fi. Cœur métier repris du dossier SMS_EnOcean ; réécriture des deux firmwares de la carte V5.0.1 selon `Planification_Architecture_WiFi_AGV.md` : séquenceur et file déplacés sur l'ATmega, ESP32 en client Wi-Fi/MQTT, heartbeat de repli, protocole inter-MCU, JSON MQTT, driver de bus AVR, poste UniPi EnOcean→MQTT. | Document initial — 101 tests C++, 17 tests Python |

### Règle de tenue

À chaque modification du dossier :

1. compteur de tests (§1.1) si le nombre bouge ;
2. cartes déplacées dans le kanban (§3) ;
3. une ligne datée dans le journal ;
4. si un point du §12 ou un point W est relevé : mettre à jour
   `profiles/*.yaml` ou `board_ports.h`, cocher
   [`questions_ouvertes.md`](questions_ouvertes.md), sortir la carte de
   « Bloqué » ;
5. **si la modification touche le cœur métier** (séquenceur, file, protocole,
   simulateur), la reporter dans les autres dossiers d'architecture de
   `` — ils en portent une copie.
