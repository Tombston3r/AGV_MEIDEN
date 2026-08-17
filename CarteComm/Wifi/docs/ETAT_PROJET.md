# État du projet — architecture Wi-Fi (carte V5.0.1 conservée)

> **Document vivant.** Mis à jour à chaque modification du dossier.
>
> Dernière mise à jour : **2026-08-14** — ajout du runbook de déploiement
> [`../DEPLOY.md`](../DEPLOY.md).
>
> **Périmètre** : `CarteComm/Wifi/`, projet autonome et zippable. La carte AIO
> AGV Control V5.0.1 est **conservée** ; ses **deux firmwares sont réécrits**.
> Index des architectures : [`../../README.md`](../../README.md).
> Spécification suivie : [`../Planification_Architecture_WiFi_AGV.md`](../Planification_Architecture_WiFi_AGV.md).

---

## 1. Ce qui est fait

### 1.1 Vue d'ensemble

| Indicateur | Valeur |
|---|---|
| Tests natifs C++ | **108 tests, 536 assertions, 0 échec** |
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
| Étage de sortie **collecteur ouvert** (rail 6 V automate) | ✅ testé |
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
- **Les niveaux du bus sont entièrement inconnus** : le L7806CV s'étant révélé
  être l'alimentation de l'ATmega, rien ne renseigne sur les signaux.
  L'amplitude des lignes Y (W1b) est la mesure la plus urgente ; la topologie
  des entrées de l'automate (W1d) décide du mode de sortie.
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
| B1 | Renseigner `t_setup_us` et les timeouts `Y22`/`Y05`/`Y10` | Relevé à l'analyseur logique (0.4, §12.4-12.5) |
| B2 | ⚠️ **Qualifier l'amplitude des lignes Y** — protection à ajouter si > V_CC | Mesure sur `Y05` (W1b) |
| B2c | Confirmer la topologie de l'étage de sortie (`x_open_drain`) | Mesure du tirage côté automate (W1d) |
| B2d | Vérifier la tension V_CC de l'ATmega | 6 V = maximum absolu du datasheet (W1e) |
| B2b | Trancher les pull-ups internes sur les Y | Sorties automate à collecteur ouvert ou poussées ? (W1c) |
| B3 | Figer la polarité PNP/NPN | Mesure sur l'automate (§12.3) |
| B4 | Recaler le format `/agvdump` | Sortie réelle de la V5.0.1 (§12.6, §3.3) |
| B5 | Paramètres réseau et MQTT | Accord du service informatique (0.1, W5, W6) |
| B6 | Confirmer l'UART ESP32 ↔ ATmega | Relevé de continuité (W2) |
| B7 | Décider de la bande 2,4 GHz vs bi-bande | Arbitrage 0.7 |
| B8 | Runtime de l'UniPi | Référence commandée (§12.9) |
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
| **Runbook de déploiement** | `DEPLOY.md` : 11 phases, checklists, recette, retour arrière |

---

## 4. Journal des mises à jour

| Date | Modification | Impact |
|---|---|---|
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
   `CarteComm/` — ils en portent une copie.
