# Email de commande — architecture LoRa, variante A3 (hybride EnOcean + LoRa)

> Variante **A3**, deux points d'appel. Les quantités couvrent **une carte AGV,
> un poste fixe et deux boutons**. Le PCB et l'outillage sont hors de cette
> demande : les circuits imprimés se commandent chez un fabricant spécialisé
> ([`BOM.md`](BOM.md)), l'outillage est non récurrent et facultatif.
>
> ⚠️ Les prix n'étant pas encore relevés au catalogue, c'est une **demande de
> prix et de disponibilité** — elle devient un bon de commande dès le retour
> chiffré.
>
> Variante concurrente : [`email_commande_A1.md`](email_commande_A1.md).

---

**Objet :** Demande de prix et disponibilité — composants radio 868 MHz (EnOcean + LoRa)

Bonjour,

Nous réalisons le remplacement du système d'appel d'un chariot filoguidé (AGV)
en atelier.

Le principe : **deux boutons muraux EnOcean, sans pile ni câblage**, émettent
vers un petit poste fixe qui relaie l'appel en **LoRa 868 MHz** vers une carte
embarquée sur le chariot. Celle-ci traduit l'appel en ordre de mission pour
l'automate existant. Ni point d'accès Wi-Fi, ni abonnement opérateur.

L'intérêt des boutons EnOcean est qu'ils **récupèrent leur énergie dans l'appui
lui-même** : aucune pile à remplacer sur la durée de vie de l'installation.

Merci de nous communiquer **prix unitaire, disponibilité et délai** pour les
références ci-dessous.

## 1. Carte embarquée sur le chariot (1 ensemble)

| Réf. fabricant | Désignation | Qté |
|---|---|---:|
| `ESP32-WROOM-32E-N8` | Module MCU Wi-Fi/BT, 8 Mo flash | 2 |
| `RFM95W-868S2` (HopeRF) | Module LoRa SX1276 868 MHz | 2 |
| `Amphenol 336312-24-0100` | Pigtail U.FL → SMA femelle, passe-cloison | 2 |
| `Siretta ALPHA-1A` | Antenne LoRa 868 MHz 2 dBi, embase SMA | 2 |
| `PC847` (Sharp) | Optocoupleur quadruple | 11 |
| `SN74HC595N` (TI) | Registre à décalage, sortie 8 bits, DIP-16 | 3 |
| `SN74HC165N` (TI) | Registre à décalage, entrée 8 bits, DIP-16 | 3 |
| `TSR 1-2450` (Traco Power) | Convertisseur DC/DC 24 V → 5 V 1 A | 2 |
| `AP2112K-3.3TRG1` (Diodes) | LDO 3,3 V 600 mA | 2 |
| `SMBJ33A` (Littelfuse) | Diode TVS 33 V | 2 |
| `Standex KSK-1A66` | Contact ILS + aimant | 1 |
| `L717SDB25xA4CH4F` (Amphenol) | SUB-D 25 coudé CI, mâle et femelle | 2 |
| `Hammond 1590` ou équiv. | Boîtier alu, presse-étoupes | 1 |

> Les quantités **2** de cette première liste couvrent la carte embarquée **et**
> le poste fixe, qui partagent la même base électronique.

## 2. Poste fixe — spécifique (1 ensemble)

| Réf. fabricant | Désignation | Qté |
|---|---|---:|
| `TCM 515` (EnOcean, EU 868 MHz) | Récepteur EnOcean, interface série ESP3 | 1 |
| `ANT300` (EnOcean) ou équiv. | Antenne EnOcean 868 MHz déportée | 1 |
| `WIZ850io` (WIZnet, W5500) | Contrôleur Ethernet SPI + RJ45 magnétique | 1 |
| `Omron B3F-1000` | Bouton tactile — appairage et reset | 2 |
| `MEAN WELL HDR-15-24` | Alimentation rail DIN 230 V → 24 V 15 W | 1 |
| `Fibox` / `Hammond 1554` ou équiv. | Boîtier mural IP54, presse-étoupes, embases SMA | 1 |

## 3. Boutons d'appel EnOcean (2 unités)

| Réf. fabricant | Désignation | Qté |
|---|---|---:|
| `PTM 210` (EnOcean, **EU 868 MHz**) | Émetteur auto-alimenté, sans pile | 2 |
| Enveloppe murale compatible `PTM 210` | Eltako, NodOn ou Trio2Sys | 2 |

Un lot de résistances 1 %, condensateurs de découplage et LED d'état complète
l'ensemble ; nous pouvons le préciser si vous souhaitez le chiffrer. Les plaques
de repérage gravées sont commandées séparément.

## Quatre questions

1. **La version EU 868 MHz du `PTM 210` est impérative** — les versions 902 MHz
   et 928 MHz ne sont pas utilisables en France. Merci de confirmer la référence
   exacte proposée.
2. Distribuez-vous la gamme **EnOcean** (`PTM 210`, `TCM 515`, `ANT300`) ? Si
   non, merci de nous l'indiquer, nous passerons par un distributeur spécialisé.
3. **Les équivalents fonctionnels sont acceptés** sur les composants courants —
   `LTV-847` ou `TLP281-4` pour le `PC847`, `MC74HC595AN` pour le `SN74HC595N`,
   `OKI-78SR-5/1.5-W36-C` pour le `TSR 1-2450`. Merci de proposer une
   substitution plutôt qu'un reliquat en cas de rupture.
4. Quel est le **conditionnement minimum** sur les optocoupleurs et les
   registres à décalage ?

Bien cordialement,
