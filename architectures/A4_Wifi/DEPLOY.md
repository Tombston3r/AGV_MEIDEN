# Déploiement — architecture Wi-Fi (carte V5.0.1 conservée)

> Procédure opérationnelle complète : de la première mesure au procès-verbal de
> recette. À dérouler **dans l'ordre** — chaque phase suppose la précédente
> validée.
>
> État du projet et kanban : [`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md).
> Ce que fait le logiciel : [`README.md`](README.md).

**Charge estimée** : 3 à 5 jours-homme hors délais externes (accord du service
informatique, disponibilité de l'AGV). Le chemin critique n'est pas technique :
c'est l'accord IT et la fenêtre d'arrêt de production.

---

## ⚠️ Trois choses à savoir avant de commencer

### 1. Flasher est irréversible si le firmware d'origine n'est pas sauvegardé

L'AGV tourne en production depuis cinq ans. Le firmware d'origine n'existe ni en
sources ni en sauvegarde connue. **Dès que vous écrivez dans la flash de
l'ATmega ou de l'ESP32, l'ancien système est perdu** — sauf si la phase 2
réussit à le relire.

Si les bits de protection interdisent la lecture, le retour arrière n'existe
plus. C'est une décision à faire acter par le client, par écrit, avant la
phase 8.

### 2. Les niveaux électriques du bus ne sont pas mesurés

Les 21 lignes `Y` arrivent **directement** sur des broches de l'ATmega. Leur
amplitude est inconnue. Au-delà de V_CC + 0,5 V, l'entrée est détruite.
La phase 1 n'est pas une formalité.

### 3. L'accord du service informatique conditionne tout le reste

Cette architecture met un équipement OT sur le réseau d'entreprise. Sans VLAN,
adressage et règles de pare-feu accordés, rien ne fonctionne — et ce délai ne
dépend pas de l'équipe projet. **À lancer le jour 1**, en parallèle de tout le
reste.

---

## Phase 0 — Ce qu'il faut avoir en main

### Matériel

- [ ] AGV MEIDEN accessible, avec fenêtre d'arrêt de production planifiée
- [ ] Carte AIO AGV Control V5.0.1 déposable
- [ ] Programmateur ISP pour l'ATmega2560 (USBasp, USBtinyISP ou Arduino en ISP)
- [ ] Câble USB-série pour l'ESP32, ou adaptateur 3,3 V si le connecteur est nu
- [ ] Multimètre
- [ ] Oscilloscope ou analyseur logique (phase 1 et phase 5)
- [ ] UniPi E413 avec son alimentation
- [ ] Récepteur EnOcean TCM 515 raccordé à l'UniPi
- [ ] Boutons EnOcean PTM 210, un par point d'appel
- [ ] Adaptateur USB-série pour espionner la liaison inter-MCU (diagnostic)

### Logiciel sur le poste de développement

```bash
pip install platformio esptool
sudo apt install avrdude          # ou dnf install avrdude
git clone <url-du-depot> && cd architectures/A4_Wifi
```

### Informations à obtenir du client

| Information | Auprès de | Phase qui en dépend |
|---|---|---|
| SSID, méthode d'authentification, mot de passe ou certificats 802.1X | Service informatique | 4, 7 |
| Adresse IP réservée, passerelle, masque, VLAN | Service informatique | 4, 7 |
| Autorisation d'un équipement OT sur le réseau | Service informatique | 7 |
| Liste des points d'appel et numéro de station de chacun | Exploitation | 10 |
| Seuil de latence acceptable pour un opérateur | Exploitation | 11 |
| Sortie réelle de `agvdump` de la carte d'origine | Atelier | 4 |

---

## Phase 1 — Mesures préalables ⚠️ BLOQUANTES

**Aucune de ces mesures ne peut être sautée.** Elles décident du câblage et de
paramètres qui, mal réglés, provoquent des pannes intermittentes ou détruisent
du matériel.

Durée : une demi-journée. Carte d'origine **encore en place et fonctionnelle**.

### 1.1 Amplitude des lignes Y

Automate sous tension, AGV en fonctionnement normal.

```
Sonde sur Y05 (CN62 A2) — voir docs/subd25_atmega.md
```

- [ ] Amplitude mesurée au niveau haut : ______ V
- [ ] Amplitude mesurée au niveau bas : ______ V

**Si la tension dépasse V_CC + 0,5 V de l'ATmega** : il faut interposer des
optocoupleurs ou des diviseurs. C'est du matériel à ajouter — arrêter ici et
traiter ce point avant d'aller plus loin.

### 1.2 Tension d'alimentation de l'ATmega

Le L7806CV abaisse le 24 V de CN64 A6/B6 à 6 V.

- [ ] Tension mesurée en sortie du L7806CV : ______ V
- [ ] Ce 6 V arrive sur : ☐ V_CC directement ☐ un second régulateur 5 V ☐ `Vin` d'une carte Arduino
- [ ] Tension mesurée sur la broche V_CC de l'ATmega : ______ V

**6,0 V est le maximum absolu du datasheet de l'ATmega2560** (plage recommandée
4,5–5,5 V à 16 MHz). Si V_CC dépasse 5,5 V, le signaler au client : ça
fonctionne, mais hors spécification.

### 1.3 Topologie des entrées de l'automate

Automate sous tension, **carte débranchée**.

- [ ] Tension à vide sur une entrée d'automate (ex. `X93`, CN61 A4) : ______ V

| Résultat | Conclusion | Paramètre |
|---|---|---|
| Tension présente (tirage) | La carte doit **tirer à la masse** | `bus.x_open_drain: true` |
| 0 V, entrée flottante | L'automate attend un courant fourni | `bus.x_open_drain: false` |

### 1.4 Polarité et chronogrammes

Analyseur logique sur la carte d'origine, en fonctionnement.

- [ ] Niveau au repos d'une sortie X : ☐ bas (`x_active_high: true`) ☐ haut (`false`)
- [ ] Écart entre le dernier front d'adresse et le front de `X93` : ______ µs → `t_setup_us`
- [ ] Délai `X93` → `Y22` : typique ______ ms, maximum ______ ms → `y22_write_ack_ms`
- [ ] Délai `X82` → `Y05` : typique ______ ms, maximum ______ ms → `y05_start_ack_ms`
- [ ] Durée de course la plus longue observée : ______ s → `y10_arrival_ms`
- [ ] Sorties de l'automate (lignes Y) : ☐ collecteur ouvert (`y_pullups: true`) ☐ poussées (`false`)

### 1.5 Sortie `agvdump` de référence

- [ ] Copie d'une sortie complète de `agvdump` de la carte d'origine, archivée

Elle sert à recaler le format servi par le nouveau firmware, dont dépendent les
procédures d'atelier du client.

---

## Phase 2 — Sauvegarde des firmwares d'origine

**C'est la seule chance de pouvoir revenir en arrière.**

```bash
mkdir -p sauvegarde-origine && cd sauvegarde-origine

# ESP32 — 4 Mo de flash, ajuster si le module diffère
esptool.py --port /dev/ttyUSB0 --baud 460800 \
           read_flash 0 0x400000 esp32_origine.bin

# ATmega2560 via ISP — flash, EEPROM et fusibles
avrdude -c usbasp -p m2560 -U flash:r:mega_origine.hex:i
avrdude -c usbasp -p m2560 -U eeprom:r:mega_eeprom.hex:i
avrdude -c usbasp -p m2560 -U lfuse:r:lfuse.txt:h \
                            -U hfuse:r:hfuse.txt:h \
                            -U efuse:r:efuse.txt:h
```

- [ ] `esp32_origine.bin` obtenu et de taille cohérente
- [ ] `mega_origine.hex` obtenu et **non vide** (une flash protégée renvoie des `0xFF`)
- [ ] Fusibles relevés
- [ ] Sauvegardes archivées hors du poste de développement

**Si la lecture échoue ou ne renvoie que des `0xFF`** : la flash est protégée.
Le retour arrière devient impossible. Faire acter la décision de poursuivre par
le client, par écrit, avant la phase 8.

---

## Phase 3 — Poste de développement

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] `108 tests, 536 assertions, 0 échecs`

```bash
cd poste-unipi && pip install -e '.[dev,mqtt,enocean_serial]'
python3 -m pytest tests -q && ruff check . && mypy --strict agv_poste && cd ..
```

- [ ] 17 tests Python au vert, lint et typage propres

---

## Phase 4 — Renseigner le profil

Éditer `profiles/default.yaml` **et rien d'autre** : c'est la source de vérité
unique. Reporter les mesures de la phase 1.

| Section | Clés à renseigner | Vient de |
|---|---|---|
| `bus` | `x_active_high`, `y_active_high`, `t_setup_us`, `x_open_drain`, `y_pullups` | 1.3, 1.4 |
| `timeouts` | `y22_write_ack_ms`, `y05_start_ack_ms`, `y10_arrival_ms` | 1.4 |
| `wifi` | `ssid`, `password`, `static_ip`, `gateway`, `netmask` | Service informatique |
| `mqtt` | `host`, `username`, `password`, `agv_id` | Phase 7 |

Prendre les valeurs **maximales** observées pour les timeouts, avec une marge :
un timeout trop court provoque des réessais intermittents, très difficiles à
diagnostiquer sur site.

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

- [ ] Les tests restent verts **avec les vraies valeurs**
- [ ] Lignes `PROVISOIRE` correspondantes retirées du profil
- [ ] Cases cochées dans [`docs/questions_ouvertes.md`](docs/questions_ouvertes.md)

---

## Phase 5 — Validation au banc, sans AGV

Le simulateur d'automate rejoue les quatre phases du séquenceur avec les timings
relevés.

```bash
make test FILTER=sequenceur     # les 4 phases, tous les chemins de timeout
make test FILTER=heartbeat      # le repli de sécurité
make test FILTER=point_12       # les paramètres non figés
```

- [ ] Course simple aboutie
- [ ] File de 5 courses enchaînée dans l'ordre
- [ ] Perte de heartbeat → arrêt au point d'arrêt suivant, course suivante non lancée
- [ ] Retour du heartbeat → l'AGV ne repart pas tout seul

---

## Phase 6 — Contrôle du câblage ⚠️ AUTOMATE DÉBRANCHÉ

La table de câblage est relevée, mais une nappe peut être sertie à l'envers. Une
erreur ici **n'échoue pas** : elle envoie l'AGV à la mauvaise station.

**Les deux SUB-D 25 doivent être déconnectés de l'automate.**

```bash
pio run -e mega -t upload
pio device monitor -b 115200
# puis envoyer 'd' pour entrer en mode découverte
```

Le firmware active une sortie à la fois et annonce l'index sur la console.

- [ ] Pour chacun des 22 index, la broche SUB-D qui bouge correspond à
      [`docs/subd25_atmega.md`](docs/subd25_atmega.md)
- [ ] Envoyer `n` pour revenir en mode normal

Pour les entrées : forcer un signal `Y` depuis l'automate en mode manuel, lire
l'état renvoyé par `GetState` sur la liaison série.

```bash
python3 tools/decode_link.py --file capture.hex
```

- [ ] Chaque signal `Y` forcé apparaît au bon index

Toute divergence se corrige dans `firmware/mega/src/board_ports.h`, puis
`make test`.

---

## Phase 7 — Infrastructure réseau

Prérequis : accord du service informatique obtenu.

1. **VLAN et adressage** — côté client : VLAN OT, réservation de l'IP statique
   de l'AGV, règles de pare-feu autorisant l'AGV à joindre le broker.
2. **Mosquitto sur l'UniPi** :

```bash
sudo apt install mosquitto mosquitto-clients
sudo mosquitto_passwd -c /etc/mosquitto/passwd agv1
sudo mosquitto_passwd    /etc/mosquitto/passwd poste
```

   Configuration minimale à durcir — **les ACL par topic ne sont pas
   optionnelles** : l'AGV ne doit pouvoir publier que sur ce qui le concerne.

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
# /etc/mosquitto/aclfile
user agv1
topic write agv/1/state
topic write agv/1/ack
topic write agv/1/status
topic read  agv/1/cmd

user poste
topic write agv/1/cmd
topic read  agv/1/#
topic write poste/1/#
```

3. **Vérification depuis le poste de développement** :

```bash
mosquitto_sub -h <unipi> -p 8883 --cafile ca.crt -u poste -P <mdp> -t 'agv/1/#' -v
```

- [ ] Connexion TLS acceptée
- [ ] Un client mal authentifié est refusé
- [ ] Un client `agv1` ne peut **pas** publier sur `agv/1/cmd`

4. **Couverture Wi-Fi** — relevé RSSI à la hauteur réelle de l'antenne AGV, sur
   le trajet complet, en heures de production.

- [ ] RSSI minimum relevé sur le parcours : ______ dBm
- [ ] Nombre de points d'accès couvrant le trajet : ______
- [ ] Si plusieurs AP : temps de reconnexion mesuré au handover : ______ ms

Sous −75 dBm (`wifi.rssi_warn_dbm`), la liaison décroche en mouvement. Traiter
la couverture **avant** la mise en service, pas après les premiers appels
perdus.

---

## Phase 8 — Flash des deux firmwares

⚠️ Ne pas commencer sans la phase 2 (sauvegarde) et la décision écrite du client
si la sauvegarde a échoué.

**L'ordre compte.** L'ATmega démarre en `safe_stop` et refuse toute course tant
qu'il n'a pas reçu de heartbeat. Le flasher en premier garantit qu'il n'y a
jamais de fenêtre où l'ESP32 commande un ATmega dans un état inconnu.

```bash
pio run -e mega  -t upload     # 1. séquenceur, file, repli de sécurité
pio run -e esp32 -t upload     # 2. Wi-Fi, MQTT, heartbeat
```

- [ ] ATmega flashé, console affiche `# ATmega2560 AGV MEIDEN - firmware reecrit, pose en 5 ecritures de port`
- [ ] ESP32 flashé, journal affiche l'association Wi-Fi et `MQTT connecté`
- [ ] `agv/1/status` publie `online` sur le broker

---

## Phase 9 — Poste fixe UniPi

```bash
sudo useradd -r -s /usr/sbin/nologin agv
sudo mkdir -p /opt/agv /etc/agv /var/lib/agv
sudo chown agv:agv /var/lib/agv
sudo python3.11 -m venv /opt/agv/venv
sudo /opt/agv/venv/bin/pip install './poste-unipi[mqtt,enocean_serial]'
sudo cp poste-unipi/poste.example.toml /etc/agv/poste.toml
sudo $EDITOR /etc/agv/poste.toml      # port du TCM 515, broker, identifiants
sudo cp poste-unipi/systemd/agv-poste.service /etc/systemd/system/
sudo systemctl enable --now agv-poste
journalctl -u agv-poste -f
```

- [ ] Service démarré, aucune erreur au journal
- [ ] `poste fixe démarré (agv_id=1, N appairage(s))` visible

---

## Phase 10 — Appairage et première course

### 10.1 Appairage des boutons

Pour chaque point d'appel, **depuis son emplacement définitif** — la portée se
teste là où le bouton sera posé, pas sur l'établi.

- [ ] Mode appairage ouvert, station saisie
- [ ] Appui sur le bouton physique
- [ ] Journal : `bouton XXXXXXXX (bascule N) appairé à la station M`
- [ ] Second appui → une commande `agv/1/cmd` est publiée

Répéter pour chaque bouton. Un bouton hors portée échoue **silencieusement** :
vérifier chaque appairage au journal, un par un.

### 10.2 Première course, AGV à l'arrêt

- [ ] Appui bouton → commande publiée → `ack` reçu avec `ok: true`
- [ ] L'AGV démarre et rejoint la station attendue
- [ ] `agv/1/state` publie la position correcte pendant le trajet

### 10.3 Diagnostic atelier

- [ ] Approcher l'aimant du contact ILS → point d'accès `AGV-MAINT` visible
- [ ] `/agvdump` accessible et lisible
- [ ] **L'atelier confirme que le format reste exploitable avec ses procédures**
- [ ] Le point d'accès se referme seul après 10 minutes

---

## Phase 11 — Recette : essais de dégradation

C'est cette phase qui distingue une installation qui marche d'une installation
qui tient. À faire valider par le client.

| # | Essai | Attendu | OK |
|---|---|---|---|
| 1 | Couper le point d'accès Wi-Fi en pleine course | La course va au bout ; `state` cesse d'être publié ; le poste signale l'état périmé | ☐ |
| 2 | Rétablir le Wi-Fi | Reconnexion < 10 s, `status` repasse `online`, l'AGV **ne repart pas seul** | ☐ |
| 3 | **Débrancher la liaison série ESP32 ↔ ATmega en pleine course** | La course va jusqu'au point d'arrêt suivant, `safe_stop` remonte, toute nouvelle commande est refusée | ☐ |
| 4 | Rebrancher | L'AGV **ne repart pas seul** ; une nouvelle commande est de nouveau acceptée | ☐ |
| 5 | Redémarrer l'UniPi | Le broker et le service repartent ; les appairages sont conservés | ☐ |
| 6 | Couper l'alimentation de l'AGV en pleine course | Au redémarrage : `safe_stop`, file vide (elle est en RAM), aucune course résiduelle | ☐ |
| 7 | Appuyer deux fois très vite sur un bouton | Une seule course (déduplication des sous-télégrammes) | ☐ |
| 8 | Empiler 6 courses | Les 5 premières acceptées, la 6ᵉ refusée avec `QueueFull` | ☐ |
| 9 | Bouton hors portée | Aucune commande émise ; l'absence est visible au journal | ☐ |
| 10 | Campagne en production | Latence P50/P95/P99, taux d'appels perdus, nombre de handovers | ☐ |

Seuils à faire fixer **par le client, avant les essais** :

- latence appui → départ, P95 : ______ s
- taux d'appels perdus : ______ %
- temps de reprise après coupure d'AP : ______ s

---

## Retour arrière

| Situation | Action |
|---|---|
| Problème logiciel, sauvegardes disponibles | Reflasher `mega_origine.hex` et `esp32_origine.bin`, restaurer les fusibles |
| Problème logiciel, sauvegardes indisponibles | **Aucun retour possible.** Prévoir une carte de rechange avant la phase 8 |
| Réseau d'entreprise indisponible durablement | L'AGV reste en `safe_stop` : sûr, mais inexploitable. Repli documenté : architectures `../A3_LoRa/` ou `../A1_Cellulaire/`, sans dépendance à l'infrastructure du client |

```bash
avrdude -c usbasp -p m2560 -U flash:w:sauvegarde-origine/mega_origine.hex:i
esptool.py --port /dev/ttyUSB0 write_flash 0 sauvegarde-origine/esp32_origine.bin
```

---

## Diagnostic sur site

| Symptôme | Où regarder | Cause probable |
|---|---|---|
| L'AGV refuse toutes les courses | `safe_stop` dans `/agvdump` ou `agv/1/state` | Heartbeat perdu : ESP32 planté ou liaison série coupée |
| L'AGV accepte mais ne part pas | `write_op_return`, `start_op_return`, `fault` | Timeouts mal réglés (phase 1.4) ou câblage (phase 6) |
| L'AGV va à la mauvaise station | Phase 6 non faite ou nappe sertie à l'envers | Câblage |
| Appels perdus par moments | `rssi_dbm`, nombre de handovers | Couverture Wi-Fi insuffisante (phase 7.4) |
| Commandes ignorées | `cmd_expired` | Horloge du poste décalée, ou latence réseau > 30 s |
| Courses en double | `cmd_duplicate` à 0 alors qu'il y a des rejeux | L'idempotence ne voit pas les doublons : à investiguer |
| Appuis sans effet | `unpaired` côté poste | Bouton non appairé, ou hors portée |
| État figé dans l'IHM | Indicateur de fraîcheur, `agv/1/status` | LWT : l'AGV a disparu sans se déconnecter |

Espionner la liaison inter-MCU :

```bash
python3 tools/decode_link.py --file capture.hex
```

---

## Fiche de recette

À joindre au procès-verbal.

| Élément | Valeur relevée | Date | Visa |
|---|---|---|---|
| Amplitude ligne `Y05` | | | |
| Tension V_CC ATmega | | | |
| Topologie entrées automate | | | |
| Polarité X | | | |
| `t_setup_us` | | | |
| `y22_write_ack_ms` | | | |
| `y05_start_ack_ms` | | | |
| `y10_arrival_ms` | | | |
| RSSI minimum sur le parcours | | | |
| Sauvegarde firmware d'origine | ☐ réussie ☐ **impossible** | | |
| Contrôle du câblage (phase 6) | ☐ conforme | | |
| Essais de dégradation (phase 11) | ☐ 10/10 | | |
| Format `agvdump` validé par l'atelier | ☐ | | |

**Rappel de sûreté** : cette carte est un organe de **commande**, pas un organe
de sécurité. L'arrêt d'urgence, les bumpers et le scrutateur laser restent dans
une chaîne indépendante conforme à l'ISO 3691-4. Aucun essai de cette procédure
ne valide la chaîne de sécurité, qui relève d'une recette distincte.
