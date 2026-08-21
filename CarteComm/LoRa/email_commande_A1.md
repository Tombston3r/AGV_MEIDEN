# Email de commande — architecture LoRa, variante A1 (LoRa homogène)

> Variante **A1**, deux points d'appel. Les quantités couvrent **une carte AGV
> et deux boutons**. Le PCB et l'outillage sont hors de cette demande : le
> circuit imprimé se commande chez un fabricant spécialisé
> ([`BOM.md`](BOM.md)), l'outillage est non récurrent et facultatif.
>
> ⚠️ Les prix n'étant pas encore relevés au catalogue, c'est une **demande de
> prix et de disponibilité** — elle devient un bon de commande dès le retour
> chiffré.

---

**Objet :** Demande de prix et disponibilité — composants liaison radio 868 MHz

Bonjour,

Nous réalisons le remplacement du système d'appel d'un chariot filoguidé (AGV)
en atelier.

Le principe est simple : **deux boutons muraux sur pile émettent en LoRa
868 MHz** vers une carte embarquée sur le chariot, qui traduit l'appel en ordre
de mission pour l'automate existant. Liaison radio directe, en bande libre :
ni point d'accès Wi-Fi, ni abonnement opérateur. Les boutons sont autonomes
pour plusieurs années sur pile lithium.

Merci de nous communiquer **prix unitaire, disponibilité et délai** pour les
références ci-dessous.

## Carte embarquée (1 ensemble)

| Réf. fabricant | Désignation | Qté |
|---|---|---:|
| `ESP32-WROOM-32E-N8` | Module MCU Wi-Fi/BT, 8 Mo flash | 1 |
| `RFM95W-868S2` (HopeRF) | Module LoRa SX1276 868 MHz | 3 |
| `Amphenol 336312-24-0100` | Pigtail U.FL → SMA femelle, passe-cloison | 1 |
| `Siretta ALPHA-1A` | Antenne 868 MHz 1/4 onde 2 dBi, embase SMA | 3 |
| `PC847` (Sharp) | Optocoupleur quadruple | 11 |
| `SN74HC595N` (TI) | Registre à décalage, sortie 8 bits, DIP-16 | 3 |
| `SN74HC165N` (TI) | Registre à décalage, entrée 8 bits, DIP-16 | 3 |
| `TSR 1-2450` (Traco Power) | Convertisseur DC/DC 24 V → 5 V 1 A | 1 |
| `AP2112K-3.3TRG1` (Diodes) | LDO 3,3 V 600 mA | 1 |
| `SMBJ33A` (Littelfuse) | Diode TVS 33 V | 2 |
| `Standex KSK-1A66` | Contact ILS + aimant | 1 |
| `L717SDB25xA4CH4F` (Amphenol) | SUB-D 25 coudé CI, mâle et femelle | 2 |
| `Hammond 1590` ou équiv. | Boîtier alu, presse-étoupes | 1 |

## Boutons d'appel (2 unités)

| Réf. fabricant | Désignation | Qté |
|---|---|---:|
| `STM32L071KBU6` (ST) | MCU ultra-basse consommation | 2 |
| `Schneider XB4BA31` | Bouton poussoir Ø22 IP65 | 2 |
| `ER14505` / `Saft LS14500` | Pile Li-SOCl₂ 3,6 V AA 2,6 Ah + support | 2 |
| `Kingbright L-59EGW` | LED bicolore verte/rouge | 2 |
| `BAT54` | Diode Schottky de protection | 2 |
| Tantale 220 µF + X7R 10 µF | Réservoir d'impulsion d'émission | 2 |
| `Fibox PC 095808` ou équiv. | Boîtier IP65, presse-étoupe, embase antenne | 2 |

Un lot de résistances 1 %, condensateurs de découplage et LED d'état complète
l'ensemble ; nous pouvons le préciser si vous souhaitez le chiffrer.

## Trois questions

1. **Les équivalents fonctionnels sont acceptés** sur les composants courants —
   `LTV-847` ou `TLP281-4` pour le `PC847`, `MC74HC595AN` pour le `SN74HC595N`,
   `LS14500` pour l'`ER14505`. Merci de proposer une substitution plutôt qu'un
   reliquat si une référence est en rupture.
2. Quel est le **conditionnement minimum** sur les optocoupleurs et les
   registres à décalage ?
3. Les modules `RFM95W-868S2` et `ESP32-WROOM-32E-N8` figurent-ils à votre
   catalogue ? Ils ne sont pas toujours distribués en France ; si tel est le
   cas, merci de nous l'indiquer, nous les sourcerons ailleurs.

Bien cordialement,
