# Déploiement — architecture SMS + EnOcean (carte neuve)

> Procédure opérationnelle complète : de la première mesure au procès-verbal de
> recette. À dérouler **dans l'ordre** — chaque phase suppose la précédente
> validée.
>
> État du projet et kanban : [`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md).
> Analyse de l'architecture : [`docs/Archi_2_Cellulaire_SMS_LTE-M.md`](docs/Archi_2_Cellulaire_SMS_LTE-M.md).

**Charge estimée** : 6 à 9 jours-homme hors fabrication de carte et hors délais
externes (souscription des SIM, disponibilité de l'AGV). La fabrication de la
carte AGV est le chemin critique matériel.

---

## ⚠️ Quatre décisions à prendre avant de commencer

### 1. MQTT/LTE-M, pas SMS

Le SMS n'offre **ni latence bornée, ni ordre de remise, ni garantie de remise,
ni protection contre les doublons**. Pour piloter un engin mobile, c'est
disqualifiant : un `STOP` peut arriver avant le `GOTO` qu'il annule.

Cette procédure déploie donc **`MqttLteTransport`** (environnements `agv` et
`poste`). Les environnements `agv-sms` et `poste-sms` existent pour la
comparaison chiffrée demandée par le client — **pas pour la mise en service**.

Le SMS garde un usage légitime : `AlertGateway`, quelques messages par mois vers
un technicien hors site. C'est la phase 9.

### 2. Coût récurrent

| Variante | Récurrent annuel | Sur 10 ans |
|---|---:|---:|
| SMS (2 SIM + volume) | ~1 500 € | ~15 000 € |
| LTE-M/MQTT (2 SIM data + VPS) | ~100 € | ~1 000 € |
| Architectures sans opérateur | 0 € | 0 € |

À faire acter par le client avant la commande des SIM.

### 3. La couverture cellulaire n'est pas garantie en intérieur

Une usine est une structure métallique. Contrairement à un réseau privé, **on ne
peut pas corriger la couverture en ajoutant un répéteur**. La phase 1 est
éliminatoire : un seul point d'arrêt sous −110 dBm disqualifie l'architecture.

### 4. La variante d'interface bus n'est pas tranchée

`profiles/default.yaml` → `bus.driver_variant` : `mcp23017`, `shift595` ou
`mega_uart`. Ce choix conditionne le routage de la carte. Le logiciel supporte
les trois, mais **le PCB doit être dessiné pour un seul**.

Recommandation : `shift595` — 3 µs de pose, strictement simultanée grâce au
latch `RCLK` commun. Les MCP23017 posent en ~150 µs avec un décalage résiduel
entre GPIOA et GPIOB.

---

## Phase 0 — Ce qu'il faut avoir en main

### Matériel

- [ ] Carte AGV fabriquée et assemblée (nomenclature : `docs/Archi_2_Cellulaire_SMS_LTE-M.md` §6)
- [ ] ESP32-WROOM-32E, modem **SIM7080G** (LTE-M/NB-IoT), antenne déportée
- [ ] Expandeurs selon la variante retenue (4× MCP23017, ou 3× 74HC595 + 3× 74HC165)
- [ ] Optocoupleurs pour les 43 voies, SUB-D 25 mâle et femelle
- [ ] Poste fixe : ESP32 (A), **Unipi Gate G100** (B, si une prise réseau existe
      sur place) ou UniPi E413 LTE (C, seulement sinon) — voir `BOM.md`
- [ ] Récepteur EnOcean TCM 515, boutons PTM 210
- [ ] 2 SIM M2M avec accès data LTE-M
- [ ] Multimètre, oscilloscope ou analyseur logique
- [ ] Smartphone en mode ingénieur ou testeur de couverture

### Logiciel

```bash
pip install platformio
cd CarteComm/SMS_EnOcean
```

### Informations à obtenir

| Information | Auprès de | Phase |
|---|---|---|
| APN, code PIN des SIM | Opérateur | 4 |
| Hôte, port, identifiants et CA du broker MQTT | Client ou hébergeur | 4, 7 |
| Numéro du technicien d'astreinte (`alert_msisdn`) | Client | 9 |
| Liste des points d'appel et numéro de station | Exploitation | 8 |
| Runtime réel du poste commandé (§12.9) | Fournisseur — **sans objet si Gate G100** : Debian d'origine | 6 |
| Sortie réelle de `agvdump` de la carte d'origine | Atelier | 3 |

---

## Phase 1 — Relevés éliminatoires ⚠️ BLOQUANTS

Durée : 1 à 2 jours. **Aucune commande de matériel avant ces résultats.**

### 1.1 Couverture cellulaire le long du parcours

Aux heures de production, machines en marche, à la hauteur réelle de l'antenne
de l'AGV.

| Point d'arrêt | RSRP (dBm) | RSRQ (dB) | Verdict |
|---|---|---|---|
| | | | |

- [ ] Relevé effectué à **tous** les points d'arrêt et le long du parcours
- [ ] RSRP minimum relevé : ______ dBm

**Un seul point sous −110 dBm disqualifie l'architecture.** Arrêter ici et
basculer sur `../LoRa/` ou `../Wifi/`.

### 1.2 Essai de latence réel

200 aller-retours en conditions de production, avec une SIM de prêt.

- [ ] Latence P50 : ______ s
- [ ] Latence P95 : ______ s
- [ ] Latence **P99** : ______ s ← c'est celle qui compte
- [ ] Taux de perte : ______ %

### 1.3 Calendrier d'extinction 2G/3G

- [ ] Vérifié auprès des opérateurs, à jour au moment de l'étude
- [ ] Modem retenu compatible LTE-M/Cat-M1 ou Cat-1 bis : ☐ oui

### 1.4 Mesures sur le bus MEIDEN

Sur la carte d'origine, **avant sa dépose** — elle ne sera plus disponible
ensuite. Détail : [`docs/procedures_essai.md`](docs/procedures_essai.md) §4.

- [ ] Amplitude d'une ligne `Y` (`Y05`) : ______ V → dimensionnement des optocoupleurs
- [ ] Niveau au repos d'une sortie X : ☐ bas (`x_active_high: true`) ☐ haut (`false`)
- [ ] Écart dernier front d'adresse → `X93` : ______ µs → `t_setup_us`
- [ ] Délai `X93` → `Y22` : typique ______ ms, max ______ ms
- [ ] Délai `X82` → `Y05` : typique ______ ms, max ______ ms
- [ ] Durée de course la plus longue : ______ s
- [ ] Sortie `agvdump` complète archivée

### 1.5 Portée EnOcean

Depuis l'emplacement définitif de chaque bouton, pas depuis l'établi.

- [ ] Marge RSSI à chaque poste ≥ 10 dB au-dessus de la sensibilité
- [ ] Répéteur nécessaire : ☐ non ☐ oui, aux emplacements ______

---

## Phase 2 — Fabrication et contrôle de la carte

- [ ] Variante d'interface bus arrêtée : ☐ `shift595` ☐ `mcp23017` ☐ `mega_uart`
- [ ] Schéma et routage figés selon ce choix
- [ ] Carte fabriquée, assemblée, contrôlée visuellement
- [ ] Alimentation vérifiée à vide : 24 V → 5 V → 3,3 V
- [ ] **Réservoir capacitif présent** pour les pics d'émission du modem (2 A)
- [ ] Continuité SUB-D 25 relevée et conforme à
      [`docs/signal_map.md`](docs/signal_map.md)

---

## Phase 3 — Poste de développement

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] `112 tests, 373 assertions, 0 échecs`

```bash
cd poste-unipi && pip install -e '.[dev,mqtt]'
python3 -m pytest tests -q && ruff check . && mypy --strict agv_poste && cd ..
```

- [ ] 16 tests Python au vert, lint et typage propres

---

## Phase 4 — Renseigner le profil

Éditer `profiles/default.yaml` **et rien d'autre**.

| Section | Clés | Vient de |
|---|---|---|
| `bus` | `driver_variant`, `x_active_high`, `y_active_high`, `t_setup_us`, `y_debounce_us` | 1.4, 2 |
| `timeouts` | `y22_write_ack_ms`, `y05_start_ack_ms`, `y10_arrival_ms` | 1.4 |
| `cellular` | `apn`, `sim_pin`, `mqtt_host`, `mqtt_port`, `mqtt_client_id` | Opérateur, client |
| `cellular` | `alert_msisdn`, `alerts_per_day_max` | Client (phase 9) |
| `enocean` | `dedup_window_ms`, `rx_only` | Récepteur retenu |

Prendre les valeurs **maximales** observées pour les timeouts, avec marge : un
timeout trop court provoque des réessais intermittents, difficiles à
diagnostiquer sur site.

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] Tests verts **avec les vraies valeurs**
- [ ] Lignes `PROVISOIRE` correspondantes retirées
- [ ] Cases cochées dans [`docs/questions_ouvertes.md`](docs/questions_ouvertes.md)

---

## Phase 5 — Clé de chiffrement et banc

### 5.1 Provisionner la clé AES

Contrairement à une liaison TLS, le chiffrement est ici **applicatif**
(AES-128-CTR, §5.1). La même clé va sur l'AGV et sur le poste.

```bash
python3 tools/provision_key.py generate --namespace agv   --out build/nvs_agv.csv
python3 tools/provision_key.py generate --namespace poste --out build/nvs_poste.csv \
        --key <la_meme_cle_hex>

python3 $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
        generate build/nvs_agv.csv build/nvs_agv.bin 0x6000
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 build/nvs_agv.bin
```

- [ ] Clé générée, archivée **hors du dépôt**, remise au client
- [ ] NVS écrite sur l'AGV et sur le poste

Sans clé, le firmware démarre **en clair** et le journal le signale bruyamment.
C'est délibéré : un site sans clé doit rester diagnosticable.

### 5.2 Banc, sans AGV

```bash
make test FILTER=sequenceur     # les 4 phases, tous les chemins de timeout
make test FILTER=point_12       # les paramètres non figés
```

- [ ] Course simple aboutie, file de 5 courses enchaînée
- [ ] Perte de liaison → arrêt au point d'arrêt suivant
- [ ] Coupure d'alimentation → file restaurée depuis la NVS au redémarrage

### 5.3 Chronogrammes à l'oscilloscope

Carte alimentée, **automate débranché**.

- [ ] Simultanéité de la pose des 22 lignes conforme à la variante retenue
- [ ] `t_setup` mesuré entre le dernier front d'adresse et `X93` : ______ µs
- [ ] Sur variante `mcp23017` : décalage GPIOA/GPIOB mesuré ______ µs, **très
      inférieur** à `t_setup_us`

---

## Phase 6 — Broker MQTT et poste fixe

### 6.1 Mosquitto

Sur le VPS ou le serveur usine.

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd agv1
sudo mosquitto_passwd    /etc/mosquitto/passwd poste
```

```
listener 8883
certfile /etc/mosquitto/certs/server.crt
keyfile  /etc/mosquitto/certs/server.key
cafile   /etc/mosquitto/certs/ca.crt
allow_anonymous false
password_file /etc/mosquitto/passwd
acl_file /etc/mosquitto/aclfile
```

```
# /etc/mosquitto/aclfile — l'AGV ne publie que sur ce qui le concerne
user agv1
topic write agv/1/ack
topic write agv/1/telemetry
topic write agv/1/status
topic read  agv/1/cmd

user poste
topic write agv/1/cmd
topic read  agv/1/#
```

- [ ] Connexion TLS acceptée, client anonyme refusé
- [ ] Un client `agv1` ne peut **pas** publier sur `agv/1/cmd`
- [ ] **Last Will and Testament** vérifié : couper brutalement l'AGV fait
      apparaître `offline` sur `agv/1/status`

### 6.2 Poste fixe UniPi

⚠️ §12.9 : si la référence commandée tourne sous **Mervis**, le service Python
est sans objet — l'intégration passe par Modbus TCP depuis un autre hôte.
Vérifier **avant** de dérouler cette étape.

```bash
sudo useradd -r -s /usr/sbin/nologin agv
sudo mkdir -p /opt/agv /etc/agv
sudo python3.11 -m venv /opt/agv/venv
sudo /opt/agv/venv/bin/pip install './poste-unipi[mqtt]'
sudo cp poste-unipi/poste.example.toml /etc/agv/poste.toml
sudo $EDITOR /etc/agv/poste.toml     # io.kind, transport.kind = "mqtt", boutons
sudo cp poste-unipi/systemd/agv-poste.service /etc/systemd/system/
sudo systemctl enable --now agv-poste
journalctl -u agv-poste -f
```

- [ ] `io.kind` renseigné explicitement : ☐ `evok` ☐ `modbus` ☐ `simulated`
- [ ] `transport.kind = "mqtt"` — **pas `sms`**
- [ ] Service démarré sans erreur

### 6.3 Poste fixe ESP32 (variante A3)

Si le poste est un ESP32 avec TCM 515 et Ethernet filaire :

```bash
./tools/flash.sh poste /dev/ttyUSB1
```

- [ ] Assets web envoyés dans LittleFS
- [ ] `http://agv.local` accessible en Ethernet **filaire** (aucune émission
      2,4 GHz permanente)

---

## Phase 7 — Flash de l'AGV

```bash
./tools/flash.sh agv /dev/ttyUSB0
```

`flash.sh` régénère la configuration **et lance les tests** avant chaque flash :
un test rouge interrompt le flash.

- [ ] Firmware AGV flashé en variante **MQTT/LTE-M** (env `agv`)
- [ ] Modem attaché au réseau : `+CEREG` à 1 ou 5
- [ ] `agv/1/status` publie `online` sur le broker
- [ ] Télémétrie visible sur `agv/1/telemetry`

**Période de télémétrie** : la régler selon le forfait data. Une publication par
seconde sur un abonnement M2M facturé au volume coûte cher pour rien.

---

## Phase 8 — Appairage et première course

### 8.1 Appairage des boutons EnOcean

Pour chaque point d'appel, **depuis son emplacement définitif**.

- [ ] Mode appairage ouvert (`/api/pair` ou bouton du poste), station saisie
- [ ] Appui sur le bouton physique → journal d'appairage
- [ ] Second appui → une commande part vers l'AGV

⚠️ Le TCM 515 est en **réception seule** : aucun accusé ne revient vers le
bouton. Si un retour opérateur est exigé, il faut un voyant câblé au poste — à
trancher avant de figer l'IHM (§12.8).

### 8.2 Première course

- [ ] Appui bouton → commande publiée → ACK reçu
- [ ] L'AGV rejoint la station attendue
- [ ] `/agvdump` du poste et de l'AGV cohérents
- [ ] **L'atelier confirme que le format `agvdump` reste exploitable**

---

## Phase 9 — Passerelle d'alerte SMS

Usage résiduel légitime du cellulaire : prévenir un technicien **hors site** en
cas de défaut bloquant. Quelques messages par mois, ~10 €/an.

- [ ] `cellular.alert_msisdn` renseigné
- [ ] `cellular.alerts_per_day_max` réglé (défaut 6)
- [ ] Test : provoquer un défaut bloquant → un SMS arrive au technicien
- [ ] Test de quota : au-delà de la limite, les alertes sont supprimées et
      comptées, pas envoyées

Ce module est **architecturalement incapable de piloter la chaîne de commande** :
il n'a accès ni au séquenceur ni à la file. Le vérifier en revue, pas seulement
en essai.

---

## Phase 10 — Recette : essais de dégradation

| # | Essai | Attendu | OK |
|---|---|---|---|
| 1 | Couper la liaison cellulaire en pleine course | La course va au bout ; arrêt au point d'arrêt suivant ; LED `FAULT` | ☐ |
| 2 | Rétablir | Reconnexion, `status` repasse `online`, l'AGV **ne repart pas seul** | ☐ |
| 3 | Retirer la SIM à chaud | Détecté, journalisé, aucune commande fantôme | ☐ |
| 4 | Modem muet (débrancher l'UART) | Cycle d'alimentation automatique après `modem_mute_timeout_ms` | ☐ |
| 5 | Redémarrer le broker | Reconnexion automatique, LWT observé puis effacé | ☐ |
| 6 | Coupure d'alimentation AGV en pleine course | File **restaurée** depuis la NVS ; course de plus de 30 min écartée | ☐ |
| 7 | Rejouer une commande déjà exécutée | Ré-acquittée, **pas ré-exécutée** (`cmd_duplicate` s'incrémente) | ☐ |
| 8 | Injecter une commande horodatée d'il y a 3 minutes | Refusée (`cmd_expired`) | ☐ |
| 9 | Deux appuis très rapprochés sur un bouton | Une seule course (déduplication) | ☐ |
| 10 | Empiler 6 courses | 5 acceptées, la 6ᵉ refusée | ☐ |
| 11 | Bouton hors portée EnOcean | Aucune commande ; absence visible au journal | ☐ |
| 12 | Campagne en production | Latence P50/P95/P99, taux d'appels perdus, coût data relevé | ☐ |

Seuils à faire fixer **par le client, avant les essais** :

- latence appui → départ, P95 : ______ s
- taux d'appels perdus : ______ %
- coût data mensuel constaté : ______ €

---

## Retour arrière

| Situation | Action |
|---|---|
| Carte neuve défaillante | Reposer la carte V5.0.1 d'origine, conservée intacte |
| Couverture cellulaire insuffisante | Basculer sur `../LoRa/` (aucun opérateur) ou `../Wifi/` |
| Coût récurrent refusé | Idem |

La carte d'origine **n'est pas modifiée** par cette architecture : le retour
arrière consiste à la reposer. C'est un avantage réel par rapport à l'architecture
`../Wifi/`, qui réécrit ses firmwares.

---

## Diagnostic sur site

| Symptôme | Où regarder | Cause probable |
|---|---|---|
| L'AGV ne part pas | `write_op_return`, `start_op_return`, `fault` | Timeouts (1.4) ou câblage (2) |
| Commandes ignorées | `cmd_expired` | Latence réseau > `max_command_age_s`, ou horloge décalée |
| Courses en double | `cmd_duplicate` à 0 malgré des rejeux | Idempotence à investiguer |
| Appuis sans effet | `enocean_unpaired` | Bouton non appairé ou hors portée |
| Liaison qui tombe | `rssi_dbm`, `rx_bad_crc`, `telemetry_age_ms` | Couverture (1.1) |
| Modem qui se relance | `recoveries` | Modem muet, alimentation insuffisante aux pics 2 A |
| Facture data anormale | période de télémétrie | Publication trop fréquente |

Décoder une trame relevée :

```bash
python3 tools/replay_frames.py decode <hexa>
```

---

## Fiche de recette

| Élément | Valeur relevée | Date | Visa |
|---|---|---|---|
| RSRP minimum sur le parcours | | | |
| Latence P99 sur 200 échanges | | | |
| Variante d'interface bus retenue | | | |
| `t_setup_us` mesuré | | | |
| `y22_write_ack_ms` / `y05_start_ack_ms` | | | |
| Décalage de pose du bus | | | |
| Clé AES provisionnée et remise | ☐ | | |
| ACL du broker vérifiées | ☐ | | |
| Essais de dégradation | ☐ 12/12 | | |
| Format `agvdump` validé par l'atelier | ☐ | | |
| Coût récurrent acté par le client | ☐ | | |

**Rappel de sûreté** : cette carte est un organe de **commande**, pas un organe
de sécurité. L'arrêt d'urgence, les bumpers et le scrutateur laser restent dans
une chaîne indépendante conforme à l'ISO 3691-4. Aucun essai de cette procédure
ne valide la chaîne de sécurité, qui relève d'une recette distincte.
