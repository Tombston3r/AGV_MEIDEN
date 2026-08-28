# A2 : LoRa 868 MHz

> **Le logiciel de ce dossier est autonome** : `make test` ne dépend d'aucun
> autre dossier. Le **matériel** et le **brief** sont en revanche partagés, une
> seule copie pour tout le dépôt.
> Pour l'envoyer à quelqu'un : `./outils/exporter_architecture.sh <archi>`
> reconstitue un ensemble complet.
> Index des architectures : [`../README.md`](../README.md).

> **Matériel** : le projet KiCad vit dans [`../../materiel/AIO_AGV_Control_V6.0/`](../../materiel/AIO_AGV_Control_V6.0/), une seule copie pour tout le dépôt.
> **Bancs** : [`../../bancs/lora/`](../../bancs/lora/) valide la liaison de cette architecture sur table.


## La chaîne

```
Boutons d'appel (optionnels) -> Poste Central -> Carte AGV Control -> AGV
                                UniPi Lite 1.1     carte V6.0
```

Le **poste central** héberge l'API de planning quotidien et décide des départs.
Il parle à la carte AGV en **LoRa 868 MHz**, sur bande libre : ni abonnement, ni
point d'accès, ni dépendance au réseau d'entreprise.

Modèle commun aux trois architectures :
[`../../../docs/ARCHITECTURE_COMMUNE.md`](../../../docs/ARCHITECTURE_COMMUNE.md).

⚠️ **Fusion du 2026-08-28.** Ce dossier réunit les deux anciennes variantes
LoRa, qui ne se distinguaient que par la technologie du bouton. Les boutons
étant devenus optionnels, la distinction ne définissait plus une architecture.
Leurs deux documents de référence sont conservés dans `docs/`, sous les noms
`Archi_2a_` et `Archi_2b_`.

## Les boutons, si vous en voulez

Ils ne figurent pas dans la nomenclature : le planning seul fait rouler l'AGV.
Deux options restent disponibles, **indépendantes de l'architecture** :

| | Ce qu'elle apporte | Ce qu'elle coûte |
|---|---|---|
| **LoRa sur pile** | bouton **authentifié** AES, **accusé visuel** vert/rouge | pile tous les 5 à 8 ans, carte à fabriquer. Firmware : [`firmware/bouton-lora/`](firmware/bouton-lora/) |
| **EnOcean** `PTM 210` | aucune pile, jamais | télégramme en clair, aucun accusé. Dongle USB au poste, validé par [`../../bancs/enocean/`](../../bancs/enocean/) |

## Matériel

**Carte `AIO_AGV_Control_V6.0`** : la V5.0.1 augmentée d'un `RFM95W-868S2`
câblé sur le SPI libre de l'ESP32. Relevé du projet KiCad :

| RFM95W | ESP32 |
|---|---|
| `NSS` | `IO5` |
| `SCK` | `IO18` |
| `MISO` | `IO19` |
| `MOSI` | `IO23` |
| `DIO0` | `IO26` |

⚠️ **`RESET` n'est pas câblée.** Un module figé ne se récupère qu'en coupant
l'alimentation de la carte. Une GPIO libre suffirait à corriger cela sur une
révision ultérieure.

⚠️ **Ne jamais utiliser `IO16` ni `IO17`** : elles portent la liaison série vers
le `Mega2560 Pro` (`SoftwareSerial`, 38 400 bauds).

## Répartition des rôles, à ne pas inverser

L'**ATmega2560** porte la mission : séquenceur trois phases, file de 5 courses,
décodage de position, repli de sécurité sur perte du heartbeat. L'**ESP32** ne
fait que de la radio et **ne touche jamais au bus MEIDEN**.

## Commandes

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test                    # 119 tests natifs, -Wall -Wextra -Werror
pio run -e mega              # ATmega2560 : séquenceur + file (flasher EN PREMIER)
pio run -e esp32             # ESP32 : radio LoRa + heartbeat
```

## Points à ne pas perdre de vue

- Le **budget de rapport cyclique est une obligation réglementaire** (EN 300 220
  / ERC 70-03), pas une option : ~145 ms d'antenne par trame à SF9 plafonnent à
  **~248 émissions par heure**, accusés compris.
- Le **sync word doit différer de 0x34**, réservé LoRaWAN.
- L'arbitrage **SF7 / SF9** conditionne latence et portée : voir
  [`docs/latence_lora.md`](docs/latence_lora.md).
- **Autonomie des boutons** : 5 à 8 ans annoncés sur `ER14505`, à mesurer au
  banc. Un courant de repos de 20 µA au lieu de 2 divise l'autonomie par cinq.
