# Déploiement — architecture LoRa 868 MHz (carte neuve)

> ⚠️ **Ce dossier n'est pas encore un projet autonome.** Il contient les sources
> spécifiques à LoRa, pas le cœur métier. La **phase 0** ci-dessous le complète ;
> tant qu'elle n'est pas faite, aucune des commandes suivantes ne fonctionne.
>
> Contenu et recette de complétion : [`README.md`](README.md).
> Document de référence : [`docs/Archi_3_LoRa_P2P_homogene.md`](docs/Archi_3_LoRa_P2P_homogene.md).
> La variante à boutons sans pile vit dans [`../A2_Hybride/`](../A2_Hybride/).

**Charge estimée** : 1 jour pour la phase 0, puis 5 à 8 jours-homme hors
fabrication de carte.

---

## ⚠️ Trois choses à savoir avant de commencer

### 1. Deux architectures partagent ce dossier

| | A3 — LoRa homogène | A2 — Hybride EnOcean + LoRa |
|---|---|---|
| Boutons | sur pile, radio LoRa intégrée | **PTM 210 sans pile** |
| Retour visuel au bouton | **oui** — LED verte = ACK reçu | **non**, le TCM 515 est en réception seule |
| Poste fixe | facultatif | obligatoire (récepteur EnOcean → LoRa) |
| Ajout d'un bouton | flasher un `node_id` + une station | appairage depuis l'IHM, sans flash |

**A2 est l'architecture retenue**, sous réserve que le « sans-pile » soit une
exigence réelle. Si ce n'en est pas une, A3 est plus simple et rend le retour
visuel à l'opérateur — argument à ne pas perdre.

Les phases ci-dessous couvrent les deux ; les étapes propres à chacune sont
marquées **[A3]** ou **[A2]**.

### 2. Le rapport cyclique est une obligation réglementaire

EN 300 220 / ERC 70-03 imposent **1 % d'émission sur 1 h glissante** en 868 MHz.
Le firmware refuse d'émettre au-delà et remonte le refus en défaut visible. Ce
n'est pas un réglage de confort : le dépasser est une infraction.

Conséquence concrète : **le facteur d'étalement et la période de télémétrie se
choisissent ensemble**, sous ce budget. Voir la phase 4.

### 3. Le facteur d'étalement est un arbitrage, pas un défaut

| SF | Temps d'antenne (13 o.) | Aller-retour | Émissions max/heure |
|---:|---:|---:|---:|
| 7 | ~46 ms | ~92 ms | ~780 |
| **9** (valeur du profil) | **~165 ms** | **~330 ms** | **~218** |
| 12 | ~1 320 ms | ~2 640 ms | ~27 |

Le §6 du brief vise « ~200 ms typique, pire cas ~800 ms ». **SF9 ne tient pas
cette cible** : l'aller seul coûte 165 ms. SF7 la tient, au prix de la portée.
Détail et options dans [`docs/latence_lora.md`](docs/latence_lora.md).

---

## Phase 0 — Compléter le dossier ⚠️ PRÉALABLE

Le cœur métier (séquenceur, file, protocole, simulateur) vit dans les autres
dossiers d'architecture. Il faut en greffer une copie ici.

```bash
cd architectures/A3_LoRa
cp -rn ../A1_Cellulaire/{firmware,sim,test,tools,web,profiles,docs,Makefile,platformio.ini} .
cat profiles/lora_fragment.yaml >> profiles/default.yaml
```

Puis :

- [ ] Retirer `firmware/common/transport/{sms_transport,mqtt_lte_transport,at_engine}.*`
- [ ] Retirer `firmware/common/app/alert_gateway.*` si l'alerte hors site n'est pas retenue
- [ ] Rebrancher les deux `main.cpp` sur `LoraTransport` + `Sx1276Radio`
      (`LoraTransport` prend sa `LoraConfig` en **second argument**)
- [ ] Rétablir `FakeRadio` dans `test/native/fakes.h` (il est en tête de
      `test/native/test_lora.cpp`)
- [ ] **[A3]** Ajouter l'environnement `[env:bouton]` dans `platformio.ini`
      (framework `arduino`)
- [ ] Implémenter `radio_begin` / `radio_send` / `radio_wait_ack` du nœud
      bouton, déclarés `extern` dans `firmware/bouton-lora/src/main.cpp`

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] Tests verts, dont les **10 tests LoRa** (temps d'antenne, budget légal,
      half-duplex, retransmissions)

`LoraConfig` est volontairement séparé de `HardwareProfile` : le cœur métier ne
transporte aucun paramètre radio, ce qui rend cette greffe indolore.

---

## Phase 1 — Relevés éliminatoires ⚠️ BLOQUANTS

### 1.1 Couverture radio le long du parcours

À la hauteur réelle de l'antenne de l'AGV, machines en marche. Un relevé fait à
1,50 m avec un module de test posé sur un chariot ne vaut rien.

| Point d'arrêt | RSSI (dBm) | SNR (dB) | Verdict |
|---|---|---|---|
| | | | |

- [ ] RSSI minimum relevé : ______ dBm
- [ ] SNR minimum relevé : ______ dB
- [ ] Marge suffisante au SF envisagé

### 1.2 Occupation de la bande 868 MHz

- [ ] Analyseur de spectre ou récepteur SDR : autres émetteurs sur le site ?
- [ ] Canal retenu : ______ MHz
- [ ] `sync_word` retenu : ______ (**doit différer de 0x34**, réservé LoRaWAN)

### 1.3 Mesures sur le bus MEIDEN

Sur la carte d'origine, **avant sa dépose** — elle ne sera plus disponible
ensuite. Détail : `docs/procedures_essai.md` (présent après la phase 0).

- [ ] Amplitude d'une ligne `Y` (`Y05`) : ______ V
- [ ] Niveau au repos d'une sortie X : ☐ bas ☐ haut
- [ ] Écart dernier front d'adresse → `X93` : ______ µs
- [ ] Délais `X93`→`Y22` et `X82`→`Y05` : ______ ms / ______ ms
- [ ] Durée de course la plus longue : ______ s
- [ ] Sortie `agvdump` complète archivée

### 1.4 **[A2]** Portée EnOcean

- [ ] Marge RSSI ≥ 10 dB à chaque poste, **depuis son emplacement définitif**
- [ ] Répéteur nécessaire : ☐ non ☐ oui, aux emplacements ______

---

## Phase 2 — Fabrication et contrôle de la carte

Nomenclature : `docs/Archi_3_LoRa_P2P_homogene.md`. Schémas détaillés :
[`hardware/schema_detail_voies.svg`](hardware/schema_detail_voies.svg).

- [ ] Variante d'interface bus arrêtée (`bus.driver_variant`) — `shift595`
      recommandé : 3 µs, pose strictement simultanée par latch `RCLK` commun
- [ ] Carte fabriquée, assemblée, contrôlée
- [ ] Chaîne d'alimentation vérifiée à vide : 24 V → 5 V → 3,3 V, isolation
- [ ] **Antenne déportée** montée hors du châssis métallique, pigtail contrôlé
- [ ] Continuité SUB-D 25 conforme à `docs/signal_map.md`

⚠️ Ne jamais alimenter un module RFM95W sans antenne : l'étage de sortie ne
supporte pas la désadaptation.

---

## Phase 3 — Poste de développement

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
make test FILTER=lora           # temps d'antenne, budget légal, half-duplex
```

- [ ] Tests verts
- [ ] `budget_1_pourcent_refuse_au_dela` passe — c'est la garantie que le refus
      réglementaire est effectif

---

## Phase 4 — Renseigner le profil, et choisir le couple SF / télémétrie

Éditer `profiles/default.yaml` **et rien d'autre**.

| Section | Clés | Vient de |
|---|---|---|
| `bus` | `driver_variant`, `x_active_high`, `t_setup_us`, `y_debounce_us` | 1.3, 2 |
| `timeouts` | `y22_write_ack_ms`, `y05_start_ack_ms`, `y10_arrival_ms` | 1.3 |
| `lora` | `spreading_factor`, `frequency_hz`, `sync_word`, `tx_power_dbm` | 1.1, 1.2 |
| `lora` | `ack_timeout_ms`, `max_tries` | cohérents avec le SF retenu |

### Le calcul à faire, une fois pour toutes

Budget légal : **36 s d'antenne par heure**. À répartir entre commandes, ACK et
télémétrie.

| SF retenu | Télémétrie conseillée | Budget consommé |
|---:|---|---:|
| 7 | 10 s | ~17 % |
| **9** | **30 s** | **~55 %** |
| 12 | 300 s | ~44 % |

- [ ] SF retenu : ______
- [ ] Période de télémétrie retenue : ______ s
- [ ] Budget calculé, marge laissée aux commandes : ______ %

⚠️ Une télémétrie à 2 s en SF9 représente 1 800 émissions par heure : **très
au-delà du budget légal**. Le firmware refusera d'émettre, et les commandes
passeront avant la télémétrie — mais le service sera dégradé sans raison.

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] Tests verts avec les vraies valeurs
- [ ] Cases cochées dans `docs/questions_ouvertes.md`

---

## Phase 5 — Clé de chiffrement

Le chiffrement est **applicatif** (AES-128-CTR) : il n'y a pas de TLS sur une
liaison radio nue. Sans clé, n'importe qui avec un module à 10 € peut appeler
l'AGV.

```bash
python3 tools/provision_key.py generate --namespace agv   --out build/nvs_agv.csv
python3 tools/provision_key.py generate --namespace poste --out build/nvs_poste.csv \
        --key <la_meme_cle_hex>
```

- [ ] Clé générée, archivée **hors du dépôt**, remise au client
- [ ] NVS écrite sur l'AGV et sur le poste
- [ ] **[A3]** La même clé est inscrite dans le firmware de **chaque bouton**

La reperdre impose de reflasher tous les nœuds, boutons compris.

---

## Phase 6 — Banc et chronogrammes

```bash
make test FILTER=sequenceur
make test FILTER=point_12
```

- [ ] Course simple, file de 5 courses
- [ ] Perte de liaison → arrêt au point d'arrêt suivant
- [ ] Coupure d'alimentation → file restaurée depuis la NVS

À l'oscilloscope, carte alimentée, **automate débranché** :

- [ ] Simultanéité de la pose des 22 lignes conforme à la variante retenue
- [ ] `t_setup` mesuré : ______ µs

Sur la liaison radio, deux cartes en vis-à-vis :

- [ ] Aller-retour commande + ACK mesuré : ______ ms — cohérent avec le SF
- [ ] Après 3 tentatives sans réponse, la liaison est déclarée perdue

---

## Phase 7 — Flash

```bash
./tools/flash.sh agv   /dev/ttyUSB0     # firmware AGV
./tools/flash.sh poste /dev/ttyUSB1     # poste fixe + assets LittleFS
```

`flash.sh` régénère la configuration **et lance les tests** avant chaque flash.

### **[A3]** Un flash par bouton

Modifier avant chaque flash, dans `firmware/bouton-lora/src/main.cpp` :

```cpp
constexpr uint16_t kNodeId  = 0x0101;   // unique par bouton
constexpr uint16_t kStation = 2;        // station appelée
```

- [ ] Chaque bouton porte un `node_id` **unique** — un doublon casse
      l'idempotence et provoque des courses fantômes
- [ ] Table des `node_id` ↔ stations ↔ emplacements archivée
- [ ] Consommation vérifiée en sommeil profond : **< 2 µA**

Aucune modification côté AGV n'est nécessaire pour ajouter un bouton.

### **[A2]** Aucun flash de bouton

Les PTM 210 n'ont pas de firmware. L'appairage se fait à la phase 8.

---

## Phase 8 — Mise en service

### **[A2]** Appairage des boutons EnOcean

Pour chaque point d'appel, **depuis son emplacement définitif**.

- [ ] Mode appairage ouvert, station saisie, appui sur le bouton
- [ ] Second appui → commande émise vers l'AGV

⚠️ TCM 515 en réception seule : **aucun accusé ne revient au bouton**. Si un
retour opérateur est exigé, il faut un TCM 310 ou un voyant déporté câblé — à
trancher avant de figer l'IHM (§12.8).

### **[A3]** Vérification des boutons

- [ ] Appui → **LED verte fixe 2 s** = ACK reçu
- [ ] Bouton hors portée → **LED rouge clignotante** après 3 essais

C'est le retour visuel que la solution EnOcean pure ne sait pas rendre.

### Première course

- [ ] Appui → l'AGV rejoint la station attendue
- [ ] `/agvdump` cohérent côté poste et côté AGV
- [ ] **L'atelier confirme que le format `agvdump` reste exploitable**

---

## Phase 9 — Recette : essais de dégradation

| # | Essai | Attendu | OK |
|---|---|---|---|
| 1 | Éloigner l'AGV jusqu'à perte de liaison | Arrêt au point d'arrêt suivant ; LED `FAULT` | ☐ |
| 2 | Revenir à portée | L'AGV **ne repart pas seul** | ☐ |
| 3 | Émettre en rafale jusqu'à épuiser le budget légal | Émissions **refusées**, `tx_refused_duty` s'incrémente, refus visible dans l'IHM | ☐ |
| 4 | Attendre le glissement de la fenêtre d'une heure | Le budget se libère, les émissions reprennent | ☐ |
| 5 | Rejouer une commande déjà exécutée | Ré-acquittée, **pas ré-exécutée** | ☐ |
| 6 | Brouiller la trame (corruption volontaire) | Comptée en `rx_bad_crc`, ignorée | ☐ |
| 7 | Coupure d'alimentation AGV en pleine course | File restaurée depuis la NVS ; course de plus de 30 min écartée | ☐ |
| 8 | Empiler 6 courses | 5 acceptées, la 6ᵉ refusée | ☐ |
| 9 | **[A3]** Retirer la pile d'un bouton | Bouton muet ; absence détectable au poste | ☐ |
| 10 | **[A2]** Deux appuis très rapprochés | Une seule course (déduplication des sous-télégrammes) | ☐ |
| 11 | Parcours complet en production | Latence P50/P95/P99, taux d'appels perdus, `duty_used_permille` maximum relevé | ☐ |

Seuils à faire fixer **par le client, avant les essais** :

- latence appui → départ, P95 : ______ ms
- taux d'appels perdus : ______ %
- budget de rapport cyclique consommé en exploitation nominale : ______ %

---

## Retour arrière

| Situation | Action |
|---|---|
| Carte neuve défaillante | Reposer la carte V5.0.1 d'origine, conservée intacte |
| Couverture radio insuffisante | Repositionner l'antenne, réduire le SF, ajouter un relais — ou basculer sur `../A4_Wifi/` |
| Budget de rapport cyclique systématiquement atteint | Revoir le SF et la période de télémétrie (phase 4) avant tout |

La carte d'origine **n'est pas modifiée** par cette architecture : le retour
arrière consiste à la reposer.

---

## Diagnostic sur site

| Symptôme | Où regarder | Cause probable |
|---|---|---|
| Commandes refusées à l'émission | `tx_refused_duty`, `duty_used_permille` | Budget légal épuisé — revoir SF et télémétrie |
| Appels perdus en zone précise | `rssi_dbm`, `snr_db` | Trou de couverture (1.1) |
| Latence supérieure à l'attendu | SF retenu, `retries` | Retransmissions — voir `docs/latence_lora.md` |
| Trames reçues mais illisibles | `rx_bad_crc` | Clé AES différente entre nœuds, ou interférence |
| L'AGV ne part pas | `write_op_return`, `start_op_return` | Timeouts (1.3) ou câblage (2) |
| **[A3]** Bouton muet | pile, LED rouge | Pile morte ou hors portée |
| **[A2]** Appui sans effet | `enocean_unpaired` | Bouton non appairé ou hors portée |

---

## Fiche de recette

| Élément | Valeur relevée | Date | Visa |
|---|---|---|---|
| Architecture retenue | ☐ A3 ☐ A2 | | |
| RSSI / SNR minimum sur le parcours | | | |
| Facteur d'étalement retenu | | | |
| Période de télémétrie retenue | | | |
| Budget de rapport cyclique consommé | ______ % | | |
| `sync_word` retenu (≠ 0x34) | | | |
| `t_setup_us` mesuré | | | |
| Clé AES provisionnée sur tous les nœuds | ☐ | | |
| **[A3]** Table des `node_id` archivée | ☐ | | |
| Essais de dégradation | ☐ 11/11 | | |
| Format `agvdump` validé par l'atelier | ☐ | | |

**Rappel de sûreté** : cette carte est un organe de **commande**, pas un organe
de sécurité. L'arrêt d'urgence, les bumpers et le scrutateur laser restent dans
une chaîne indépendante conforme à l'ISO 3691-4. Aucun essai de cette procédure
ne valide la chaîne de sécurité, qui relève d'une recette distincte.
