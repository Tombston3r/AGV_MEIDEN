# Matériel : projets KiCad

**Une carte, un projet, un seul endroit.** Même quand deux architectures
partagent une carte, elle n'est pas dupliquée : trois copies divergentes de la
V6.0 ont déjà coexisté dans ce dépôt, et il a fallu comparer les schémas pour
savoir laquelle portait le travail réel. Un firmware dupliqué se compare par
`diff` ; deux schémas KiCad divergents, non.

| Projet | Sert à | Empreintes | État |
|---|---|---:|---|
| [`AIO_AGV_Control_V5.0.1/`](AIO_AGV_Control_V5.0.1/) | **A3** : Wi-Fi | 57 | carte d'origine, **en service sur le chariot** |
| [`AIO_AGV_Control_V6.0/`](AIO_AGV_Control_V6.0/) | **A2** et **A3** : LoRa | 59 | Gerbers exportés |

## Ce qui distingue la V6.0

Le diff des deux projets ne montre **aucun écart** hors la radio : mêmes
`Mega2560 Pro` et `ESP32-DEVKITC`, mêmes 23 `IRF520` et résistances de grille,
même `L7806CV`, même `TDN 5-2411WISM`, mêmes SUB-D 25.

S'y ajoutent un **`RFM95W-868S2` (U2)** et son **embase coaxiale (J3)**.

### Brochage relevé

| RFM95W | ESP32 |
|---|---|
| `NSS` | `IO5` |
| `SCK` | `IO18` |
| `MISO` | `IO19` |
| `MOSI` | `IO23` |
| `DIO0` | `IO26` |

⚠️ **`RESET` (pad 6) n'est pas connectée.** Aucun reset logiciel n'est possible :
un module figé ne se récupérera qu'en coupant l'alimentation de la carte.
**`IO27` est libre** et conviendrait, sans fonction de strapping au démarrage,
contrairement à `IO0`, `IO2`, `IO12` ou `IO15`.

⚠️ **`IO16` et `IO17` sont réservées** : elles portent la liaison série vers le
`Mega2560 Pro`, via un pont diviseur 2,2 k / 4,7 k dans le sens 5 V → 3,3 V.

## Points relevés sur la V5.0.1, valables pour les deux

- **Les trois UART matériels du MEGA sont inutilisables** : leurs broches de
  réception portent `Y13` (D19), `Y11` (D17) et `Y05` (D15). La liaison
  inter-microcontrôleurs est en `SoftwareSerial` sur D52/D53, à 38 400 bauds.
- **L'`IRF520` n'est pas un MOSFET logic-level** : seuil spécifié de 2 à 4 V,
  `Rds(on)` garanti à Vgs = 10 V. Attaqué en 5 V il conduit, mais hors des
  conditions du constructeur. L'`IRL520N` est la version logic-level **au même
  brochage**.
- **Les 21 entrées Y arrivent directement sur les broches de l'ATmega**, sans
  protection. Le client a vérifié que les niveaux le permettent.

Le relevé complet du câblage SUB-D vit dans
[`../architectures/A3_Wifi/docs/subd25_atmega.md`](../architectures/A3_Wifi/docs/subd25_atmega.md).
