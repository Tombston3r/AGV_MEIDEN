# Nomenclature — architecture LoRa 868 MHz (carte neuve)

## ⚠️ Feuille de sourcing — à compléter avant usage

Ce document est une **liste d'achat à remplir**, pas un devis. Les colonnes
`Réf. catalogue`, `PU TTC` et `Total TTC` sont vides : c'est le service achats
qui les renseigne depuis le catalogue.

| Colonne | Ce qu'elle contient |
|---|---|
| `Réf. fabricant` | **Référence exacte à rechercher** — c'est ce qui rend la ligne non ambiguë |
| `Lien d'achat` | **Recherche pré-remplie chez le distributeur**, RS en premier chaque fois que c'est plausible. Un second lien n'apparaît que lorsque RS ne distribue probablement pas la référence |
| `Réf. catalogue` / `PU TTC` / `Total TTC` | **À remplir** |
| *`Repère TTC`* | Estimation de départ, **en italique** — voir l'avertissement ci-dessous |

### ⚠️ Ce sont des liens de RECHERCHE, pas des fiches produit

Chaque lien lance une **recherche sur la référence fabricant** chez le
distributeur. Aucun numéro de stock n'a été vérifié : le site de RS bloque
l'accès automatisé, il n'était donc pas possible de confirmer qu'une fiche
produit existe ni à quel prix. Inventer des numéros de stock aurait produit des
liens crédibles menant à la mauvaise pièce — le pire résultat possible sur une
liste d'achat.

C'est au service achats de retenir la bonne fiche et d'en reporter la référence
dans la colonne `Réf. catalogue`. Si une recherche RS ne donne rien, la ligne
suit un second lien vers Mouser, Digi-Key ou Amazon.

**RS est mis en tête partout**, y compris sur les références EnOcean et Unipi
qu'il ne distribue habituellement pas : le catalogue évolue, et une commande
groupée chez un fournisseur déjà référencé vaut souvent le surcoût unitaire.

### ⚠️ Les repères ne sont PAS des prix RS

Les repères viennent des nomenclatures d'étude, établies sur des prix de
composants génériques, puis converties en TTC (TVA 20 %). **Chez un
distributeur industriel comme RS, les mêmes références coûtent couramment 2 à
4 fois plus.** Un module ESP32 y est plutôt à 15–25 € qu'à 6 €.

Ces repères servent à deux choses, et à rien d'autre :

- vérifier qu'une ligne relevée au catalogue n'est pas aberrante (erreur de
  référence, conditionnement par 100, prix pour un lot) ;
- garder un ordre de grandeur tant que la feuille n'est pas remplie.

**Le total d'une feuille complétée sera nettement supérieur au total des
repères.** C'est normal, et c'est le prix de la traçabilité d'un distributeur
référencé — disponibilité, garantie, facture, fiches techniques.

### Note pour l'arbitrage

Les prix sont demandés en TTC. Pour une entreprise, **la TVA est récupérable** :
la comparaison entre architectures reste donc pertinente en HT, et
[`../COMPARAISON.md`](../COMPARAISON.md) raisonne en HT. Diviser un total TTC par 1,20 donne le HT.

---

### Carte AGV — commune à A1 et A3

Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte** :
c'est le retour arrière de cette architecture.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU Wi-Fi/BT, 8 Mo flash | `ESP32-WROOM-32E-N8` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | ☐ | ☐ | ☐ | *6,00 €* |
| Module LoRa SX1276 868 MHz | `RFM95W-868S2 (HopeRF)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | ☐ | ☐ | ☐ | *12,00 €* |
| Pigtail U.FL → SMA femelle + passe-cloison | `Amphenol 336312-24-0100` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+336312-24-0100) | ☐ | ☐ | ☐ | *3,60 €* |
| Antenne 868 MHz 1/4 onde 2 dBi, embase SMA | `Siretta ALPHA-1A ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Siretta+ALPHA-1A) | ☐ | ☐ | ☐ | *7,20 €* |
| Optocoupleur quadruple — 43 voies | `PC847 (Sharp)` | 11 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PC847) | ☐ | ☐ | ☐ | *7,92 €* |
| Convertisseur DC/DC 24 V → 5 V 1 A | `TSR 1-2450 (Traco Power)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450) | ☐ | ☐ | ☐ | *8,40 €* |
| LDO 3,3 V 600 mA | `AP2112K-3.3TRG1 (Diodes)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=AP2112K-3.3TRG1) | ☐ | ☐ | ☐ | *0,72 €* |
| Diode TVS protection 24 V | `SMBJ33A (Littelfuse)` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SMBJ33A) | ☐ | ☐ | ☐ | *1,20 €* |
| Résistances 1 %, découplages, LED d'état | `lot` | 1 | — | ☐ | ☐ | ☐ | *9,60 €* |
| ILS (reed) + aimant — Wi-Fi de maintenance | `Standex KSK-1A66 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Standex+KSK-1A66) | ☐ | ☐ | ☐ | *2,40 €* |
| SUB-D 25 mâle et femelle, coudés CI | `Amphenol L717SDB25xA4CH4F` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L717SDB25xA4CH4F) | ☐ | ☐ | ☐ | *7,20 €* |
| PCB 4 couches ~120 × 100 mm (série de 5) | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *14,40 €* |
| Boîtier, fixation, presse-étoupes, conn. de prog. | `Hammond 1590 ou Fibox` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Hammond+1590+ou+Fibox) | ☐ | ☐ | ☐ | *33,60 €* |
| **Sous-total** | | | | | | **☐** | ***114,24 €*** |

### Interface bus — variante `shift595` (recommandée)

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Registre à décalage sortie 8 bits | `SN74HC595N (TI)` | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SN74HC595N) | ☐ | ☐ | ☐ | *1,80 €* |
| Registre à décalage entrée 8 bits | `SN74HC165N (TI)` | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SN74HC165N) | ☐ | ☐ | ☐ | *1,80 €* |
| **Sous-total** | | | | | | **☐** | ***3,60 €*** |

### Interface bus — variante `mcp23017` (alternative)

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Expandeur I²C 16 GPIO | `MCP23017-E/SP (Microchip)` | 4 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MCP23017-E%2FSP) | ☐ | ☐ | ☐ | *12,00 €* |
| **Sous-total** | | | | | | **☐** | ***12,00 €*** |

### Interface bus — variante `avr_port` (alignée sur la V5.0.1)

⚠️ Cette variante **retire les 11 `PC847`** de la carte AGV ci-dessus :
les 43 lignes arrivent directement sur les broches de l'ATmega. Ne pas
additionner les deux. Voir « Peut-on se passer des optocoupleurs ? ».

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU 5 V, 70 E/S — porte les 43 lignes du bus | `Mega2560 Pro (format compact)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Mega2560+Pro) · [Amazon](https://www.amazon.fr/s?k=Mega2560+Pro) | ☐ | ☐ | ☐ | *21,60 €* |
| Réseau Darlington collecteur ouvert — étage des 22 sorties X | `ULN2803A (TI)` | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ULN2803A) | ☐ | ☐ | ☐ | *4,32 €* |
| **Sous-total** | | | | | | **☐** | ***25,92 €*** |

### **[A1]** Bouton d'appel sur pile — l'unité

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| MCU ultra-basse consommation | `STM32L071KBU6 (ST)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=STM32L071KBU6) | ☐ | ☐ | ☐ | *4,20 €* |
| Module LoRa 868 MHz | `RFM95W-868S2 (HopeRF)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | ☐ | ☐ | ☐ | *12,00 €* |
| Antenne 868 MHz + embase SMA | `Siretta ALPHA-1A ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Siretta+ALPHA-1A) | ☐ | ☐ | ☐ | *7,20 €* |
| Bouton poussoir Ø22 IP65 | `Schneider XB4BA31 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Schneider+XB4BA31) | ☐ | ☐ | ☐ | *14,40 €* |
| Pile Li-SOCl₂ 3,6 V 2,6 Ah + support | `ER14505 / Saft LS14500` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ER14505+%2F+Saft+LS14500) | ☐ | ☐ | ☐ | *7,20 €* |
| Réservoir d'impulsion pour l'émission LoRa | `220 µF tantale + 10 µF X7R` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=220+%C2%B5F+tantale+%2B+10+%C2%B5F+X7R) | ☐ | ☐ | ☐ | *0,72 €* |
| LED bicolore verte/rouge + résistances | `Kingbright L-59EGW` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Kingbright+L-59EGW) | ☐ | ☐ | ☐ | *1,20 €* |
| Diode Schottky de protection pile | `BAT54 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=BAT54) | ☐ | ☐ | ☐ | *0,24 €* |
| PCB 2 couches ~50 × 50 mm | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *3,60 €* |
| Boîtier IP65, presse-étoupe, embase antenne | `Fibox PC 095808 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Fibox+PC+095808) | ☐ | ☐ | ☐ | *21,60 €* |
| **Sous-total** | | | | | | **☐** | ***72,36 €*** |

### **[A3]** Poste fixe EnOcean → LoRa

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash (LittleFS + pages web) | `ESP32-WROOM-32E-N8` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | ☐ | ☐ | ☐ | *6,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515) | ☐ | ☐ | ☐ | *33,60 €* |
| Antenne EnOcean 868 MHz déportée | `EnOcean ANT300 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=EnOcean+ANT300) · [Mouser](https://www.mouser.fr/c/?q=EnOcean+ANT300) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=EnOcean+ANT300) | ☐ | ☐ | ☐ | *9,60 €* |
| Module LoRa SX1276 | `RFM95W-868S2 (HopeRF)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | ☐ | ☐ | ☐ | *12,00 €* |
| Pigtail U.FL → SMA + antenne LoRa 2 dBi | `Amphenol + Siretta` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+%2B+Siretta) | ☐ | ☐ | ☐ | *10,80 €* |
| Contrôleur Ethernet SPI + RJ45 magnétique | `WIZnet WIZ850io (W5500)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=WIZnet+WIZ850io) | ☐ | ☐ | ☐ | *7,20 €* |
| LED d'accusé bicolore, LED de vie, résistances | `lot` | 1 | — | ☐ | ☐ | ☐ | *1,80 €* |
| Bouton d'appairage + bouton reset | `Omron B3F-1000` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Omron+B3F-1000) | ☐ | ☐ | ☐ | *2,40 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MEAN+WELL+HDR-15-24) | ☐ | ☐ | ☐ | *16,80 €* |
| 24 V → 5 V → 3,3 V | `TSR 1-2450 + AP2112K-3.3` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450+%2B+AP2112K-3.3) | ☐ | ☐ | ☐ | *9,60 €* |
| PCB 2 couches ~100 × 80 mm | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *7,20 €* |
| Boîtier mural IP54, presse-étoupes, embases SMA | `Fibox ou Hammond 1554` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Fibox+ou+Hammond+1554) | ☐ | ☐ | ☐ | *36,00 €* |
| **Sous-total** | | | | | | **☐** | ***153,00 €*** |

### **[A3]** Bouton EnOcean sans pile — l'unité

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PTM+210) · [Mouser](https://www.mouser.fr/c/?q=PTM+210) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=PTM+210) | ☐ | ☐ | ☐ | *36,00 €* |
| Enveloppe / poussoir mural compatible PTM 210 | `Eltako, NodOn ou Trio2Sys` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Eltako%2C+NodOn+ou+Trio2Sys) · [Mouser](https://www.mouser.fr/c/?q=Eltako%2C+NodOn+ou+Trio2Sys) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=Eltako%2C+NodOn+ou+Trio2Sys) | ☐ | ☐ | ☐ | *14,40 €* |
| Plaque de repérage station gravée | `sur mesure` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=sur+mesure) · [Amazon](https://www.amazon.fr/s?k=sur+mesure) | ☐ | ☐ | ☐ | *4,80 €* |
| **Sous-total** | | | | | | **☐** | ***55,20 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Dongle RTL-SDR + antenne — occupation de la bande 868 MHz | `RTL-SDR Blog V4` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RTL-SDR+Blog+V4) · [Amazon](https://www.amazon.fr/s?k=RTL-SDR+Blog+V4) | ☐ | ☐ | ☐ | *36,00 €* |
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=clone+Saleae+24+MHz) · [Amazon](https://www.amazon.fr/s?k=clone+Saleae+24+MHz) | ☐ | ☐ | ☐ | *18,00 €* |
| Adaptateur USB-série 3,3 V | `FTDI FT232RL ou CP2102` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL+ou+CP2102) · [Amazon](https://www.amazon.fr/s?k=FTDI+FT232RL+ou+CP2102) | ☐ | ☐ | ☐ | *7,20 €* |
| Mesure de courant µA — sommeil profond **[A1]** | `multimètre à faible burden voltage` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=multim%C3%A8tre+%C3%A0+faible+burden+voltage) · [Amazon](https://www.amazon.fr/s?k=multim%C3%A8tre+%C3%A0+faible+burden+voltage) | ☐ | ☐ | ☐ | *10,80 €* |
| **Sous-total** | | | | | | **☐** | ***72,00 €*** |

---

## Récapitulatif — variante A1 (LoRa homogène)

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *117,84 €* | *98,20 €* |
| 2 boutons sur pile | ☐ | *144,72 €* | *120,60 €* |
| Outillage | ☐ | *72,00 €* | *60,00 €* |
| **TOTAL** | **☐** | ***334,56 €*** | ***278,80 €*** |

## Récapitulatif — variante A3 (EnOcean + LoRa)

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *117,84 €* | *98,20 €* |
| Poste fixe EnOcean → LoRa | ☐ | *153,00 €* | *127,50 €* |
| 2 boutons PTM 210 | ☐ | *110,40 €* | *92,00 €* |
| Outillage | ☐ | *72,00 €* | *60,00 €* |
| **TOTAL** | **☐** | ***453,24 €*** | ***377,70 €*** |

## Demande de prix prête à envoyer

Les deux variantes ont leur courrier, en quantités réelles et non plus en
sous-totaux unitaires, avec l'explication du système en trois phrases et les
substitutions acceptées :

| Courrier | Ce qu'il commande |
|---|---|
| [`email_commande_A1.md`](email_commande_A1.md) | Une carte embarquée, **deux boutons sur pile** |
| [`email_commande_A3.md`](email_commande_A3.md) | Une carte embarquée, **un poste fixe**, deux boutons **EnOcean sans pile** |

Le second signale que la version **EU 868 MHz** du `PTM 210` est impérative :
les déclinaisons 902 et 928 MHz ne sont pas utilisables en France, et rien dans
la désignation ne les distingue au premier coup d'œil.

### Peut-on se passer des optocoupleurs ?

Oui — et c'est même ce que fait la carte d'origine. Mais l'échange n'est pas
celui qu'on croit : **on ne retire pas 11 boîtiers, on change de topologie.**

Un `ESP32` seul n'a qu'une trentaine d'E/S pour 43 signaux. C'est précisément
pour ça que la carte porte des optocoupleurs et des registres à décalage. Les
supprimer suppose donc d'ajouter un microcontrôleur qui, lui, a les broches :
un **`Mega2560 Pro`**, exactement comme la V5.0.1.

| | `shift595` (actuelle) | `avr_port` (V5.0.1) |
|---|---|---|
| Microcontrôleurs | `ESP32` seul | `ESP32` + `Mega2560 Pro` |
| Entrées Y | 11× `PC847` | **directement sur broches** |
| Sorties X | 11× `PC847` + registres | `ULN2803A` ×3 |
| Isolation galvanique | oui | **non** |
| Pose du bus | chaînes de registres | **5 écritures de port, ~0,3 µs** |
| Coût de la carte | 98,20 € HT | 110,20 € HT |

**12,00 € HT de plus, et beaucoup moins de logiciel.**

### Ce que cette variante fait gagner

Le driver existe déjà, écrit et testé pour l'architecture Wi-Fi :
`firmware/common/bus/avr_port_bus.cpp`, plus le **relevé de câblage complet**
dans `firmware/mega/src/board_ports.h` — 43 lignes réparties sur 11 ports, avec
les masques calculés à l'initialisation. Il n'y a rien à écrire.

Elle apporte aussi le **repli de sécurité par heartbeat** de l'architecture
Wi-Fi, que la carte à `ESP32` seul n'a pas : la mission vit sur un
microcontrôleur sans pile réseau, qui décide seul de l'arrêt sûr.

Et surtout : **une seule conception matérielle pour deux architectures.** La
carte LoRa devient la V5.0.1 avec une radio différente.

### Ce qu'elle coûte — et la condition qui la conditionne

L'isolation galvanique disparaît. Ce n'est **pas une régression** : la V5.0.1
n'en a pas non plus, ses MOSFET de sortie tirent les lignes vers la masse de la
carte. Cinq ans de production le valident.

Deux points ne se négocient pas :

1. **Les sorties gardent un étage de puissance.** On ne pilote pas une entrée
   d'automate depuis une broche de microcontrôleur. Les `ULN2803A` remplacent
   les MOSFET de la V5.0.1 — trois boîtiers DIP-18 au lieu de vingt-trois
   TO-220. À valider contre le seuil d'entrée de l'automate : ils saturent à
   ~1,1 V au lieu de ~0,1 V.
2. ⚠️ **§12.1 devient bloquant, et ne l'était pas.** Un `PC847` avec sa
   résistance de limitation encaisse des lignes à 24 V ; une broche d'ATmega
   les détruit. Aujourd'hui, l'amplitude des lignes Y **n'est pas mesurée**.

Le faisceau d'indices est pourtant très favorable : la V5.0.1 relie ses 21
entrées Y **directement aux broches de l'ATmega**, sans la moindre protection,
et tourne depuis cinq ans. Une ligne à 24 V sur une broche d'ATmega ne dure pas
cinq ans, elle dure quelques secondes.

**Mais un faisceau d'indices n'est pas une mesure**, et c'est un multimètre sur
`Y05` pendant trente secondes — le point W1b, déjà au kanban. Avec les
optocoupleurs, se tromper coûte une résistance ; sans eux, cela coûte la carte.

**Recommandation : retenir `avr_port`, et faire la mesure avant de lancer le
PCB.** Le gain logiciel est réel et immédiat ; le risque se referme en une
demi-minute d'atelier.

### Pourquoi pas un Unipi Gate pour ce poste ?

La question se pose puisque le poste de l'architecture Wi-Fi a été ramené à une
passerelle Unipi Gate G100. **Ici, non — et pour une raison de fond, pas de
prix.**

Le Gate est un boîtier DIN fermé : Ethernet, RS485, un port USB. **Aucun
connecteur SPI, aucun GPIO, aucune embase d'antenne.** Or un RFM95W est un
composant SPI qu'il faut piloter au niveau du PHY.

Les contournements existent, et ils coûtent tous plus cher que la carte
spécifiée :

| Contournement | Coût | Ce qu'on y perd |
|---|---:|---|
| Dongle LoRa USB | ~30,00 € + hub | L'unique port USB est déjà pris par le TCM 515 |
| Modem LoRa UART/RS485 (`E32-868T20D`, `RAK3172`) | ~18,00 € | **Le module gère le PHY lui-même** : il faudrait réécrire `LoraTransport`, et surtout **abandonner le contrôle du budget de rapport cyclique** que le firmware applique et teste aujourd'hui. C'est une obligation réglementaire, pas un réglage |
| Gate + carte ESP32 en frontal radio | ~393,00 € | On paie les deux |

**Le poste LoRa n'a d'ailleurs pas besoin de Linux.** Son travail est une
traduction de protocole : EnOcean entre, LoRa sort. Il n'héberge pas de broker,
et l'AGV lui parle directement.

L'asymétrie avec l'architecture Wi-Fi est donc logique :

| | Poste Wi-Fi | Poste LoRa |
|---|---|---|
| Doit héberger un broker MQTT | **oui** | non |
| Doit piloter une radio au niveau PHY | non — le réseau est Ethernet | **oui** — SX1276 sur SPI |
| Matériel qui en découle | boîtier Linux industriel | microcontrôleur avec SPI |
| Retenu | Unipi Gate G100 (~240,00 € TTC) | carte ESP32 (~153,00 € TTC) |

⚠️ **Deux antennes 868 MHz sur le même boîtier** — EnOcean et LoRa. Les espacer
d'au moins 20 cm, ou en déporter une. Une désensibilisation du récepteur EnOcean
par l'émetteur LoRa se traduirait par des appuis perdus, silencieusement.

### Où se croisent les deux courbes

| Stations | A1 (TTC) | A3 (TTC) | Moins cher |
|---:|---:|---:|---|
| 2 | 334,56 € | 453,24 € | **A1** |
| 4 | 479,28 € | 563,64 € | **A1** |
| 6 | 624,00 € | 674,04 € | **A1** |
| 8 | 768,72 € | 784,44 € | **A1** |
| 12 | 1 058,16 € | 1 005,24 € | **A3** |

Le point de bascule est à **8 stations**. En dessous, A1 coûte moins **et**
rend un accusé visuel à l'opérateur. Au-delà, A3 prend l'avantage grâce à des
boutons à 55,20 € au lieu de 72,36 €, et supprime les piles.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| **[A1]** bouton sur pile | **72,36 €** | 60,30 € |
| **[A3]** bouton PTM 210 | **55,20 €** | 46,00 € |

### Coûts récurrents

| Poste | Annuel |
|---|---:|
| Abonnement opérateur | **0 €** — bande ISM libre |
| Infrastructure | **0 €** — aucune |
| **[A1]** Remplacement des piles | ~7,20 € par bouton tous les 5 à 8 ans |
| **[A3]** Piles | **0 €** — PTM 210 auto-alimentés |

**C'est l'architecture la moins chère des trois sur dix ans.**

---

## Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| PCB 4 couches + assemblage | 3 à 5 semaines | **Chemin critique matériel** |
| **[A1]** PCB bouton + boîtiers IP65 | 3 à 5 semaines | En parallèle de la carte AGV |
| `RFM95W-868S2` | 1 à 3 semaines | **Contrefaçons fréquentes** — acheter chez un distributeur référencé, pas sur une place de marché |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS : prévoir un distributeur EnOcean |
| `ER14505` Li-SOCl₂ | 1 à 2 semaines | **Restrictions de transport aérien** sur le lithium |
| `ESP32`, `SN74HC595N`, `PC847` | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : le relevé de
couverture radio et l'arbitrage du facteur d'étalement peuvent changer
l'antenne, la puissance d'émission et le nombre de nœuds.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 5 à 8 jours-homme après la phase 0 qui rend ce dossier
  autonome, hors développement.
- **La conformité RED** : si les cartes sont produites en série et mises sur le
  marché, dossier technique, marquage CE et déclaration UE de conformité à
  conserver 10 ans. Compter 3 à 5 jours-homme. Sans objet en usage interne.
- **La carte de rechange** : ~117,84 € pour un échange standard. Recommandé.
- **Un éventuel relais LoRa** si le relevé révèle une zone morte : ~48,00 €,
  mais surtout une complexité applicative hors périmètre actuel.
---

## Analyse critique de cette nomenclature

### ✅ Corrigé — le convertisseur du bouton était inutile

La nomenclature d'étude prévoyait un `TPS62740` (buck ultra-basse consommation)
entre la pile et l'électronique. Il n'a pas lieu d'être : une pile Li-SOCl₂
délivre **3,6 V**, et les deux consommateurs l'acceptent directement —
`STM32L071` de 1,65 à 3,6 V, `RFM95W` de 1,8 à 3,7 V. Le convertisseur ajoutait
un composant, un courant de repos et un mode de panne, pour rien.

Le vrai besoin est ailleurs : une cellule Li-SOCl₂ a une **impédance interne
élevée**, et l'émission LoRa tire ~120 mA pendant quelques centaines de
millisecondes. Sans réservoir, la tension s'effondre et le microcontrôleur
redémarre — panne classique, et intermittente, donc pénible à diagnostiquer.

`TPS62740` remplacé par **220 µF tantale + 10 µF X7R** : 1,68 € d'économie, un
composant de moins, et le problème réellement traité.

Une diode Schottky est ajoutée en protection de la pile — 0,24 € — contre une
inversion au remplacement.

### ⚠️ À vérifier — la limitation de courant des optocoupleurs

Les 43 voies passent par 11 `PC847`. Chaque canal a besoin d'une **résistance de
limitation** dimensionnée pour la tension réelle des lignes — inconnue tant que
§12.1 n'est pas mesuré. Elles sont pour l'instant noyées dans la ligne
« résistances, découplages » : à sortir en ligne explicite une fois la tension
connue, car 43 résistances de valeur précise, ce n'est plus un forfait.

### ⚠️ À vérifier — l'autonomie annoncée

5 à 8 ans sur une `ER14505` de 2,6 Ah suppose un sommeil profond sous 2 µA et
quelques appuis par jour. **À mesurer au banc** (phase 7 de `DEPLOY.md`) : un
courant de repos de 20 µA au lieu de 2 divise l'autonomie par cinq, et
transforme une maintenance décennale en corvée annuelle.

### ✅ Confirmé — la variante `shift595` reste le bon choix

3 € contre 10 € pour les `MCP23017`, et une pose strictement simultanée par
latch commun. Aucune raison de payer plus cher pour un résultat temporel
inférieur.


---

## Mode d'emploi pour le service achats

1. Rechercher chaque `Réf. fabricant` sur [fr.rs-online.com](https://fr.rs-online.com).
2. Renseigner la référence RS (numéro de stock), le prix unitaire TTC et le total.
3. Si la référence est absente du catalogue RS : chercher sur Amazon, et
   **noter la source dans la colonne `Réf. catalogue`** — la traçabilité de
   l'origine compte autant que le prix.
4. Attention aux **conditionnements** : un PC847 ou un SN74HC595N se vend
   souvent par 5, 10 ou 100. Le prix unitaire affiché peut correspondre à un
   lot entier.
5. Les lignes marquées `PCB` ne sont pas des articles de catalogue : elles
   demandent un devis chez un fabricant de circuits imprimés
   (JLCPCB, Eurocircuits, PCBWay) à partir des fichiers Gerber.
6. Reporter les sous-totaux dans le récapitulatif, puis **mettre à jour
   [`../COMPARAISON.md`](../COMPARAISON.md)** si les écarts changent le classement des architectures.

## Équivalences acceptables

Ces substitutions ne changent rien au fonctionnement, et peuvent débloquer une
rupture de stock :

| Référence | Équivalents |
|---|---|
| `PC847` | `LTV-847`, `TLP281-4`, tout optocoupleur quadruple à sortie transistor |
| `SN74HC595N` | `MC74HC595AN`, `CD74HC595E` — boîtier DIP-16 |
| `SN74HC165N` | `MC74HC165AN`, `CD74HC165E` |
| `TSR 1-2450` | `OKI-78SR-5/1.5-W36-C` (Murata), même brochage |
| `AP2112K-3.3TRG1` | `MCP1700T-3302E`, `XC6206P332MR` |
| `SMBJ33A` | `SMBJ33CA` (bidirectionnelle), `P6SMB33A` |
| `ER14505` | `LS14500` (Saft), `SL-360` (Tadiran) — Li-SOCl₂ 3,6 V AA |
