# Banc d'essai de la liaison LoRa

Ces essais servent à répondre à une seule question avant tout déploiement :
**la liaison passe-t-elle, et les deux extrémités se comprennent-elles ?**

Ce sont deux questions distinctes, et la seconde est la plus traître. Une radio
qui porte et des trames qui divergent d'un bit donne un banc parfait et une
panne en atelier. C'est pourquoi les deux côtés utilisent **la trame
applicative du projet**, pas un message d'essai.

```
bancs/lora/
├── esp32/      carte AGV ou poste ESP32 — PlatformIO / ESP-IDF
└── linux/      poste fixe Linux — Python 3
```

Les tests unitaires du transport LoRa, eux, restent dans
[`../../architectures/A3_LoRa/test/native/`](../../architectures/A3_LoRa/test/native/) :
ils n'ont besoin d'aucun matériel et tournent avec `make test`.

## Ce qui tourne sans matériel

```bash
cd linux && ./test_interop_trame.py
```

Compile le codec C++ du cœur métier, lui fait produire des vecteurs, et vérifie
que `agv_frame.py` encode **octet pour octet la même chose** — extension
d'horodatage comprise. C'est le contrôle qui garantit que les deux dossiers
d'essai parlent vraiment la même langue.

## Les quatre combinaisons

| Émetteur | Récepteur | Ce que ça éprouve |
|---|---|---|
| `esp32/ -e tx` | `esp32/ -e rx` | La radio et le pilote du projet, en boucle |
| `linux/test_tx.py` | `linux/test_rx.py` | Le pilote Python, en boucle |
| **`esp32/ -e tx`** | **`linux/test_rx.py`** | **L'interopérabilité réelle : AGV → poste** |
| **`linux/test_tx.py`** | **`esp32/ -e rx`** | **L'interopérabilité réelle : poste → AGV** |

Les deux dernières sont celles qui comptent. Les deux premières ne prouvent que
la cohérence d'une implémentation avec elle-même.

## Côté ESP32 — deux cartes possibles

```bash
cd esp32
pio run -e v6_tx    -t upload -t monitor    # carte V6.0, émission
pio run -e v6_rx    -t upload -t monitor    # carte V6.0, écoute
pio run -e tbeam_tx -t upload -t monitor    # LILYGO T-Beam, émission
pio run -e tbeam_rx -t upload -t monitor    # LILYGO T-Beam, écoute
```

Une **LILYGO T-Beam 868 MHz** porte le **même SX1276** que le `RFM95W` de la
carte V6.0 — le `RFM95W` n'est qu'un module HopeRF enveloppant cette puce. Le
pilote du projet fonctionne donc tel quel ; seul le brochage change, et il est
décrit dans `include/test_config.h`.

### Ce qu'elle apporte au banc

**Son écran affiche le relevé en direct.** C'est ce qui rend le relevé de portée
praticable : la carte tient dans une main, on marche le long du parcours et on
lit le niveau reçu, sans ordinateur au bout d'un câble USB.

```
Banc LoRa - ecoute

RSSI    -87 dBm
SNR      +9 dB
recues 42  rej 0
marge correcte
```

Sous −115 dBm, la dernière ligne passe à `!! MARGE FAIBLE`.

**Son bouton déclenche une émission à la demande.** Au relevé, on veut mesurer
*au point où l'on se trouve*, pas au rythme d'une boucle : l'environnement
`tbeam_tx` attend un appui avant chaque trame.

**Elle est sur batterie**, et affiche sa tension — un relevé qui s'arrête est
alors une liaison perdue ou une batterie vide, et on sait laquelle.

**Sa broche `RESET` est câblée**, contrairement à la V6.0 : un module figé s'y
récupère sans couper l'alimentation.

⚠️ **Sur T-Beam, la radio est alimentée par un gestionnaire AXP192**, pas
directement. Sans l'avoir activée, le SX1276 est hors tension et `RegVersion`
lit `0x00` : on cherche alors un défaut de câblage SPI qui n'existe pas. C'est
la cause n°1 des « la radio est morte » sur cette carte. `src/tbeam_power.cpp`
s'en charge et le dit à l'écran.

⚠️ **Le brochage change d'une révision à l'autre.** Le profil fourni est celui
de la **v1.1**, la plus répandue. Sur une v0.7 (sans AXP192) ou une v1.2
(AXP2101, registres différents), vérifier avant de flasher — le numéro est
sérigraphié près du connecteur USB.

Brochage par défaut — les broches **libres** relevées au projet KiCad de la
carte V5.0.1, dont l'`ESP32-DEVKITC` n'utilise que 4 broches sur 38 :

| RFM95W | ESP32 |
|---|---|
| SCK | `IO18` |
| MISO | `IO19` |
| MOSI | `IO23` |
| NSS | `IO5` |
| RESET | `IO14` |
| DIO0 | `IO26` |

⚠️ **Ne jamais utiliser `IO16` ni `IO17`** : elles portent la liaison série
vers le `Mega2560 Pro`. Un `static_assert` le rappelle dans `test_config.h`.

Ces essais compilent le **pilote du projet** (`Sx1276Radio`) et son
**budget de rapport cyclique** (`DutyCycleBudget`), pas une bibliothèque tierce.
Ils éprouvent donc le code qui partira en production.

## Côté Linux

```bash
cd linux
sudo apt install python3-spidev python3-libgpiod
./test_tx.py --count 10 --station 42
./test_rx.py --survey                  # relevé de portée : RSSI/SNR
```

⚠️ **Un `Unipi Gate G100` ne convient pas** : c'est un boîtier DIN fermé, sans
connecteur SPI ni GPIO. Ces scripts demandent une machine qui expose un bus
SPI — un modèle UniPi à base de Raspberry Pi, ou un adaptateur USB-SPI. C'est
la raison pour laquelle la nomenclature retient une carte ESP32 pour le poste
LoRa, et non une passerelle : voir `../../architectures/A2_Hybride/BOM.md`, « Pourquoi pas un Unipi
Gate pour ce poste ? ».

## Lire les résultats

**Le taux d'accusés** est la mesure utile. 100 % sur 20 trames à poste fixe ne
prouve rien ; c'est le relevé **le long du parcours**, chariot chargé, qui
compte.

**Le RSSI plancher** décide de la marge. Sous **−115 dBm**, les deux scripts
avertissent : un chariot chargé entre les deux antennes suffira à couper la
liaison. Un lien qui « passe tout juste » ne passe pas.

**Le budget de rapport cyclique** est appliqué des deux côtés, y compris sur
les accusés — un récepteur qui acquitte est un émetteur. À SF9, une trame de
9 octets occupe ~145 ms d'antenne, ce qui plafonne à **~248 émissions par
heure**, accusés compris. Les scripts refusent d'émettre au-delà et le disent.
Ce n'est pas un réglage : c'est l'EN 300 220 / ERC 70-03.

**`--sf 7` change tout** : ~41 ms au lieu de ~145, donc 3,5 fois plus
d'émissions possibles et une latence bien plus basse — au prix de la portée.
C'est l'arbitrage décrit dans [`../../architectures/A3_LoRa/docs/latence_lora.md`](../../architectures/A3_LoRa/docs/latence_lora.md),
et ces essais sont faits pour le trancher sur le terrain plutôt qu'au bureau.

## La T-Beam peut-elle servir de télécommande finale ?

**Comme prototype, oui et c'est même recommandé.** Elle permet d'éprouver tout
le comportement du bouton A3 — protocole, latence, accusé, budget légal — et de
le faire essayer par un opérateur pendant quelques semaines, **avant** d'engager
un circuit imprimé. C'est le meilleur moyen de dérisquer la carte définitive.

**En production, non.** La raison tient en une ligne :

| | Sommeil | Réserve | Autonomie |
|---|---:|---:|---:|
| T-Beam, sommeil soigné | 0,3 mA | 18650, 3 000 mAh | **~14 mois** |
| T-Beam, sommeil courant | 1,0 mA | 18650, 3 000 mAh | **~4 mois** |
| Bouton A3 (`STM32L071` + `ER14505`) | 0,002 mA | Li-SOCl₂, 2 600 mAh | **5 à 8 ans** |

L'écart n'est pas un défaut de réglage : un ESP32 avec son gestionnaire
d'alimentation, sa puce USB-série et son écran consomme au repos mille fois ce
que consomme un microcontrôleur de la famille L0. Il ne se rattrape pas.

Trois autres points, moins spectaculaires mais réels :

- **Ce n'est pas un bouton.** C'est une carte de développement avec un accu
  18650 qui dépasse. Il faudrait lui ajouter un boîtier IP65 et un poussoir Ø22
  câblé sur une entrée — soit l'essentiel de la nomenclature du bouton A3, sans
  en avoir l'autonomie.
- **Ce n'est pas du matériel industriel** : connecteur USB exposé, pas de
  vernis, qualité grand public, dans un atelier à poussière et vibrations.
- **Les révisions changent de brochage** et sortent du catalogue. Pour une
  installation prévue pour dix ans, c'est un risque d'approvisionnement.

En revanche, **rien ne s'oppose à ce qu'elle serve de poste fixe d'essai** dans
l'architecture A2 : sur secteur, la consommation n'a plus d'importance, et son
écran affiche l'état de la liaison.
