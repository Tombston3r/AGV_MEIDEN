# A2 — Hybride EnOcean + LoRa

> **Dossier autonome.** Il se construit et se teste sans rien emprunter aux
> autres dossiers de `CarteComm/`, et peut être zippé seul.
> Index des architectures : [`../README.md`](../README.md).

## Ce que fait cette architecture

Des boutons **EnOcean sans pile** — l'énergie vient de l'appui lui-même —
émettent vers un **poste fixe** qui relaie l'appel en **LoRa 868 MHz** vers la
carte embarquée. Aucune pile à remplacer sur la durée de vie de l'installation.

```
Bouton PTM 210 ──EnOcean──▶ Poste fixe ──LoRa 868 MHz──▶ Carte V6.0 ──▶ automate
   sans pile, 30 m         ESP32 relais        AES + anti-rejeu
```

C'est **l'architecture retenue**, sous réserve que l'exigence « sans pile » soit
réelle : elle coûte 90 € de plus que [A3](../A3_LoRa/) à deux stations, et le
devient moins cher au-delà de **9 stations**.

## Le compromis à connaître

Le sans-pile se paie sur deux points, et ils sont documentés plutôt qu'esquivés :

- ⚠️ **Le bouton n'est pas authentifié.** Un télégramme `PTM 210` est en clair
  et rejouable dans un rayon d'une trentaine de mètres. Le chiffrement AES ne
  commence qu'au poste fixe. C'est le maillon faible de toutes les variantes
  EnOcean — A2 comme [A4](../A4_Wifi/).
- **Pas d'accusé visuel** à l'opérateur : le `PTM 210` n'a pas de LED, il ne
  peut rien afficher. L'opérateur appuie sans savoir si l'ordre est passé.

## Matériel

**Carte `AIO_AGV_Control_V6.0`** — identique à [A3](../A3_LoRa/) : la V5.0.1
augmentée d'un `RFM95W-868S2` sur le SPI libre de l'ESP32 (`NSS`→`IO5`,
`SCK`→`IO18`, `MISO`→`IO19`, `MOSI`→`IO23`, `DIO0`→`IO26`, `RESET` **non
câblée**).

**Poste fixe** — ESP32 portant un `TCM 515` (réception EnOcean) et un `RFM95W`
(émission LoRa).

⚠️ **Deux antennes 868 MHz sur le même boîtier.** Les espacer d'au moins 20 cm,
ou en déporter une : une désensibilisation du récepteur EnOcean par l'émetteur
LoRa se traduirait par des appuis perdus, **silencieusement**.

## Commandes

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test                    # 130 tests natifs, -Wall -Wextra -Werror
pio run -e mega              # ATmega2560 : séquenceur + file (flasher EN PREMIER)
pio run -e esp32             # ESP32 embarqué : radio LoRa + heartbeat
pio run -e poste             # poste fixe : TCM 515 -> LoRa
```

## ⚠️ Ce qui reste à faire

Comme [A3](../A3_LoRa/), le firmware ESP32 hérité de A4 porte encore la
**passerelle MQTT** (`firmware/common/app/gateway_app.{h,cpp}`). Il faut lui
substituer un `LoraGatewayApp` bâti sur `LoraTransport`. Le transport, le pilote
radio, le décodage ESP3 et le budget légal sont là et testés ; c'est la couche
d'assemblage qui manque.

Voir [`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md), kanban.

## Points à ne pas perdre de vue

- **Déduplication PTM 210** : chaque appui émet **3 sous-télégrammes**. Sans
  déduplication, un appui devient trois courses.
- Le **budget de rapport cyclique** vaut aussi pour le poste : un relais qui
  acquitte est un émetteur.
- Le **sync word doit différer de 0x34**, réservé LoRaWAN.
- **Portée EnOcean ~30 m** : c'est le maillon court de la chaîne, pas le LoRa.
