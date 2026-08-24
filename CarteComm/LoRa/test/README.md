# Essais radio LoRa — deux côtés, quatre combinaisons

Ces essais servent à répondre à une seule question avant tout déploiement :
**la liaison passe-t-elle, et les deux extrémités se comprennent-elles ?**

Ce sont deux questions distinctes, et la seconde est la plus traître. Une radio
qui porte et des trames qui divergent d'un bit donne un banc parfait et une
panne en atelier. C'est pourquoi les deux côtés utilisent **la trame
applicative du projet**, pas un message d'essai.

```
test/
├── native/     tests unitaires du transport (sans matériel)
├── esp32/      carte AGV ou poste ESP32 — PlatformIO / ESP-IDF
└── unipi/      poste fixe Linux — Python 3
```

## Ce qui tourne sans matériel

```bash
cd unipi && ./test_interop_trame.py
```

Compile le codec C++ du cœur métier, lui fait produire des vecteurs, et vérifie
que `agv_frame.py` encode **octet pour octet la même chose** — extension
d'horodatage comprise. C'est le contrôle qui garantit que les deux dossiers
d'essai parlent vraiment la même langue.

## Les quatre combinaisons

| Émetteur | Récepteur | Ce que ça éprouve |
|---|---|---|
| `esp32/ -e tx` | `esp32/ -e rx` | La radio et le pilote du projet, en boucle |
| `unipi/test_tx.py` | `unipi/test_rx.py` | Le pilote Python, en boucle |
| **`esp32/ -e tx`** | **`unipi/test_rx.py`** | **L'interopérabilité réelle : AGV → poste** |
| **`unipi/test_tx.py`** | **`esp32/ -e rx`** | **L'interopérabilité réelle : poste → AGV** |

Les deux dernières sont celles qui comptent. Les deux premières ne prouvent que
la cohérence d'une implémentation avec elle-même.

## Côté ESP32

```bash
cd esp32
pio run -e tx -t upload -t monitor      # émet et attend l'accusé
pio run -e rx -t upload -t monitor      # écoute et acquitte
```

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

## Côté UniPi

```bash
cd unipi
sudo apt install python3-spidev python3-libgpiod
./test_tx.py --count 10 --station 42
./test_rx.py --survey                  # relevé de portée : RSSI/SNR
```

⚠️ **Un `Unipi Gate G100` ne convient pas** : c'est un boîtier DIN fermé, sans
connecteur SPI ni GPIO. Ces scripts demandent une machine qui expose un bus
SPI — un modèle UniPi à base de Raspberry Pi, ou un adaptateur USB-SPI. C'est
la raison pour laquelle la nomenclature retient une carte ESP32 pour le poste
LoRa, et non une passerelle : voir `../BOM.md`, « Pourquoi pas un Unipi Gate
pour ce poste ? ».

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
C'est l'arbitrage décrit dans [`../docs/latence_lora.md`](../docs/latence_lora.md),
et ces essais sont faits pour le trancher sur le terrain plutôt qu'au bureau.
