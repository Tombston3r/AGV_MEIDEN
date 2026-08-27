# A3 : LoRa P2P 868 MHz, boutons sur pile

> **Le logiciel de ce dossier est autonome** : `make test` ne dépend d'aucun
> autre dossier. Le **matériel** et le **brief** sont en revanche partagés, une
> seule copie pour tout le dépôt.
> Pour l'envoyer à quelqu'un : `./outils/exporter_architecture.sh <archi>`
> reconstitue un ensemble complet.
> Index des architectures : [`../README.md`](../README.md).

> **Matériel** : le projet KiCad vit dans [`../../materiel/AIO_AGV_Control_V6.0/`](../../materiel/AIO_AGV_Control_V6.0/), une seule copie pour tout le dépôt.
> **Bancs** : [`../../bancs/lora/`](../../bancs/lora/) valide la liaison de cette architecture sur table.

## Ce que fait cette architecture

Des boutons d'appel muraux **sur pile** émettent directement en **LoRa
868 MHz** vers la carte embarquée sur le chariot. Pas de poste fixe, pas de
point d'accès, pas d'abonnement : deux nœuds radio qui se parlent.

C'est la seule des quatre architectures où **le bouton est authentifié de bout
en bout** (chiffrement AES-128-CTR et fenêtre anti-rejeu jusqu'à l'AGV) et la
seule qui rende un **accusé visuel à l'opérateur** : LED verte quand l'ordre est
acquitté, rouge sinon.

```
Bouton LoRa sur pile ──LoRa 868 MHz──▶ Carte V6.0 ──43 lignes──▶ automate MEIDEN
   AES + anti-rejeu          ACK                 SUB-D 25 × 2
```

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
