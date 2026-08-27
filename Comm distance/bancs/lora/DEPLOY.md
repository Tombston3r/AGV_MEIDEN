# Mise en œuvre du banc LoRa

Ce banc éprouve **la liaison**, pas l'AGV. Il se monte sur une table, puis se
promène le long du parcours pour le relevé de portée.

## 0. Ce qu'il faut avoir

- **deux** modules `RFM95W-868S2` avec antenne 868 MHz et embase SMA ;
- côté embarqué : un ESP32 (ou une carte `AIO_AGV_Control_V6.0`) ;
- côté poste : un second ESP32, ou une machine Linux exposant un bus SPI.

⚠️ **Ne jamais émettre sans antenne.** L'étage de puissance du SX1276 se dégrade
en quelques secondes s'il travaille à vide.

⚠️ Un `Unipi Gate G100` **ne convient pas** : boîtier fermé, ni SPI ni GPIO. Il
faut un modèle à base de Raspberry Pi, ou un adaptateur USB-SPI.

## 1. Vérifier la trame avant la radio

```bash
cd linux && ./test_interop_trame.py
```

**Attendu** : six vecteurs, `0 écart(s)`. Cela prouve que les deux côtés
encodent la trame applicative **octet pour octet à l'identique**. C'est le seul
essai qui tourne sans matériel, et le premier à passer : une radio qui porte
avec des trames divergentes donne un banc parfait et une panne en atelier.

## 1 bis. Cas d'une LILYGO T-Beam

Rien à câbler : la radio et l'antenne sont déjà sur la carte. Deux points
propres à cette carte, à connaître avant de conclure à une panne.

**La radio est hors tension au démarrage.** Elle passe par un gestionnaire
AXP192 sur l'I²C. Le firmware l'active et l'annonce :

```
alimentation : AXP192 initialise, radio sous tension
LILYGO T-Beam v1.1 — SX1276 detecte (RegVersion 0x12)
```

Si la première ligne dit `aucun AXP192 a l'adresse 0x34`, la carte n'est pas
une v1.0/v1.1 : une v0.7 n'a pas de gestionnaire, une v1.2 embarque un AXP2101
dont les registres diffèrent.

**Le GPS est coupé** par le firmware : il ne sert à rien pour un essai de
portée et consomme plusieurs dizaines de milliampères sur batterie.

**L'écran et le bouton sont utilisés.** En réception, l'écran affiche RSSI, SNR,
compteurs et tension batterie ; en émission, chaque appui sur le bouton de la
carte envoie **une** trame. C'est ce qui rend le relevé de l'étape 5 possible
sans ordinateur.

## 2. Câbler

| RFM95W | ESP32 |
|---|---|
| `SCK` | `IO18` |
| `MISO` | `IO19` |
| `MOSI` | `IO23` |
| `NSS` | `IO5` |
| `DIO0` | `IO26` |
| `3V3`, `GND` | idem |

⚠️ **`IO16` et `IO17` sont interdites** : elles portent la liaison série vers le
`Mega2560 Pro` sur la carte V6.0.

## 3. Détecter le module

Au démarrage, chaque côté lit `RegVersion`.

**Attendu** : `RFM95W détecté (RegVersion 0x12)`.
Toute autre valeur signale un câblage SPI, une alimentation 3,3 V ou un `NSS`
en défaut — inutile d'aller plus loin.

## 4. Recette, sur table d'abord

```bash
# poste
cd linux && ./test_rx.py
# embarqué
cd esp32 && pio run -e tx -t upload -t monitor
```

| # | Attendu |
|---|---|
| 1 | Les deux côtés annoncent `868.1 MHz SF9 BW125 CR4/5 sync 0x12` |
| 2 | Chaque trame émise reçoit un `ACK` — **taux d'accusé 100 %** à un mètre |
| 3 | Le RSSI à un mètre est **meilleur que −60 dBm** |
| 4 | Le temps d'antenne annoncé est d'environ **145 ms** par trame |
| 5 | Aucune trame `REJETÉE` : ni CRC, ni longueur, ni doublon |

Un taux d'accusé inférieur à 100 % **sur table** ne se debug pas dans l'atelier :
c'est un problème d'antenne, d'alimentation ou de paramètres.

## 5. Relevé de portée — l'essai qui décide

Promener l'émetteur le long du parcours, chariot **chargé**, en lisant le RSSI.

```bash
cd linux && ./test_rx.py --survey        # côté Linux, avec un ordinateur
pio run -e tbeam_rx -t upload            # côté T-Beam, lecture sur l'écran
```

Avec deux T-Beam, le relevé se fait à deux personnes et sans matériel
informatique : l'une appuie sur le bouton au point à mesurer, l'autre lit le
niveau reçu sur l'écran.

| RSSI | Verdict |
|---|---|
| meilleur que −100 dBm | marge confortable |
| −100 à −115 dBm | acceptable, à revérifier en charge |
| **sous −115 dBm** | **marge insuffisante** — un chariot chargé entre les antennes coupera la liaison |

Les deux scripts avertissent seuls sous −115 dBm. **Un lien qui passe tout
juste ne passe pas.**

## 6. Budget d'émission — obligation réglementaire

À SF9, une trame de 9 octets occupe ~145 ms d'antenne, ce qui plafonne à
**~248 émissions par heure**, accusés compris. Les deux côtés refusent d'émettre
au-delà et le disent.

Ce n'est pas un réglage : c'est l'**EN 300 220 / ERC 70-03**, 1 % sur une heure
glissante.

`--sf 7` descend à ~41 ms, donc 3,5 fois plus d'émissions et une latence bien
plus basse — au prix de la portée. C'est l'arbitrage à trancher **sur le
terrain**, avec ce banc, plutôt qu'au bureau.

## En cas de panne

| Symptôme | Cause probable |
|---|---|
| `RegVersion` ≠ `0x12` | câblage SPI, 3,3 V, ou `NSS` |
| Aucune trame reçue | fréquence, facteur d'étalement ou **mot de synchronisation** différents |
| Trames reçues, toutes `REJETÉES` | les deux côtés ne parlent pas la même trame — refaire l'étape 1 |
| Émissions refusées | budget de rapport cyclique épuisé : attendre, ou passer en SF7 |
