# Nomenclature — architecture LoRa 868 MHz (carte neuve)

## ⚠️ Feuille de sourcing — à compléter avant usage

Ce document est une **liste d'achat à remplir**, pas un devis. Les colonnes
`Réf. catalogue`, `PU TTC` et `Total TTC` sont vides : c'est le service achats
qui les renseigne depuis le catalogue.

| Colonne | Ce qu'elle contient |
|---|---|
| `Réf. fabricant` | **Référence exacte à rechercher** — c'est ce qui rend la ligne non ambiguë |
| `Source` | Où chercher en priorité. `RS` = [fr.rs-online.com](https://fr.rs-online.com), `Amazon` = référence non distribuée par RS, `PCB` = fabricant de circuits imprimés, `Spécialiste` = distributeur EnOcean ou UniPi |
| `Réf. catalogue` / `PU TTC` / `Total TTC` | **À remplir** |
| *`Repère TTC`* | Estimation de départ, **en italique** — voir l'avertissement ci-dessous |

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

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU Wi-Fi/BT, 8 Mo flash | `ESP32-WROOM-32E-N8` | 1 | RS | ☐ | ☐ | ☐ | *6,00 €* |
| Module LoRa SX1276 868 MHz | `RFM95W-868S2 (HopeRF)` | 1 | Amazon | ☐ | ☐ | ☐ | *12,00 €* |
| Pigtail U.FL → SMA femelle + passe-cloison | `Amphenol 336312-24-0100` | 1 | RS | ☐ | ☐ | ☐ | *3,60 €* |
| Antenne 868 MHz 1/4 onde 2 dBi, embase SMA | `Siretta ALPHA-1A ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| Optocoupleur quadruple — 43 voies | `PC847 (Sharp)` | 11 | RS | ☐ | ☐ | ☐ | *7,92 €* |
| Convertisseur DC/DC 24 V → 5 V 1 A | `TSR 1-2450 (Traco Power)` | 1 | RS | ☐ | ☐ | ☐ | *8,40 €* |
| LDO 3,3 V 600 mA | `AP2112K-3.3TRG1 (Diodes)` | 1 | RS | ☐ | ☐ | ☐ | *0,72 €* |
| Diode TVS protection 24 V | `SMBJ33A (Littelfuse)` | 2 | RS | ☐ | ☐ | ☐ | *1,20 €* |
| Résistances 1 %, découplages, LED d'état | `lot` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| ILS (reed) + aimant — Wi-Fi de maintenance | `Standex KSK-1A66 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *2,40 €* |
| SUB-D 25 mâle et femelle, coudés CI | `Amphenol L717SDB25xA4CH4F` | 2 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| PCB 4 couches ~120 × 100 mm (série de 5) | `Gerber projet` | 1 | PCB | ☐ | ☐ | ☐ | *14,40 €* |
| Boîtier, fixation, presse-étoupes, conn. de prog. | `Hammond 1590 ou Fibox` | 1 | RS | ☐ | ☐ | ☐ | *33,60 €* |
| **Sous-total** | | | | | | **☐** | ***114,24 €*** |

### Interface bus — variante `shift595` (recommandée)

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Registre à décalage sortie 8 bits | `SN74HC595N (TI)` | 3 | RS | ☐ | ☐ | ☐ | *1,80 €* |
| Registre à décalage entrée 8 bits | `SN74HC165N (TI)` | 3 | RS | ☐ | ☐ | ☐ | *1,80 €* |
| **Sous-total** | | | | | | **☐** | ***3,60 €*** |

### Interface bus — variante `mcp23017` (alternative)

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Expandeur I²C 16 GPIO | `MCP23017-E/SP (Microchip)` | 4 | RS | ☐ | ☐ | ☐ | *12,00 €* |
| **Sous-total** | | | | | | **☐** | ***12,00 €*** |

### **[A1]** Bouton d'appel sur pile — l'unité

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| MCU ultra-basse consommation | `STM32L071KBU6 (ST)` | 1 | RS | ☐ | ☐ | ☐ | *4,20 €* |
| Module LoRa 868 MHz | `RFM95W-868S2 (HopeRF)` | 1 | Amazon | ☐ | ☐ | ☐ | *12,00 €* |
| Antenne 868 MHz + embase SMA | `Siretta ALPHA-1A ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| Bouton poussoir Ø22 IP65 | `Schneider XB4BA31 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *14,40 €* |
| Pile Li-SOCl₂ 3,6 V 2,6 Ah + support | `ER14505 / Saft LS14500` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| Convertisseur buck ultra-basse conso | `TPS62740DSSR (TI)` | 1 | RS | ☐ | ☐ | ☐ | *2,40 €* |
| LED bicolore verte/rouge + résistances | `Kingbright L-59EGW` | 1 | RS | ☐ | ☐ | ☐ | *1,20 €* |
| PCB 2 couches ~50 × 50 mm | `Gerber projet` | 1 | PCB | ☐ | ☐ | ☐ | *3,60 €* |
| Boîtier IP65, presse-étoupe, embase antenne | `Fibox PC 095808 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *21,60 €* |
| **Sous-total** | | | | | | **☐** | ***73,80 €*** |

### **[A3]** Poste fixe EnOcean → LoRa

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash (LittleFS + pages web) | `ESP32-WROOM-32E-N8` | 1 | RS | ☐ | ☐ | ☐ | *6,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | Spécialiste | ☐ | ☐ | ☐ | *33,60 €* |
| Antenne EnOcean 868 MHz déportée | `EnOcean ANT300 ou équiv.` | 1 | Spécialiste | ☐ | ☐ | ☐ | *9,60 €* |
| Module LoRa SX1276 | `RFM95W-868S2 (HopeRF)` | 1 | Amazon | ☐ | ☐ | ☐ | *12,00 €* |
| Pigtail U.FL → SMA + antenne LoRa 2 dBi | `Amphenol + Siretta` | 1 | RS | ☐ | ☐ | ☐ | *10,80 €* |
| Contrôleur Ethernet SPI + RJ45 magnétique | `WIZnet WIZ850io (W5500)` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| LED d'accusé bicolore, LED de vie, résistances | `lot` | 1 | RS | ☐ | ☐ | ☐ | *1,80 €* |
| Bouton d'appairage + bouton reset | `Omron B3F-1000` | 2 | RS | ☐ | ☐ | ☐ | *2,40 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | RS | ☐ | ☐ | ☐ | *16,80 €* |
| 24 V → 5 V → 3,3 V | `TSR 1-2450 + AP2112K-3.3` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| PCB 2 couches ~100 × 80 mm | `Gerber projet` | 1 | PCB | ☐ | ☐ | ☐ | *7,20 €* |
| Boîtier mural IP54, presse-étoupes, embases SMA | `Fibox ou Hammond 1554` | 1 | RS | ☐ | ☐ | ☐ | *36,00 €* |
| **Sous-total** | | | | | | **☐** | ***153,00 €*** |

### **[A3]** Bouton EnOcean sans pile — l'unité

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 1 | Spécialiste | ☐ | ☐ | ☐ | *36,00 €* |
| Enveloppe / poussoir mural compatible PTM 210 | `Eltako, NodOn ou Trio2Sys` | 1 | Spécialiste | ☐ | ☐ | ☐ | *14,40 €* |
| Plaque de repérage station gravée | `sur mesure` | 1 | Amazon | ☐ | ☐ | ☐ | *4,80 €* |
| **Sous-total** | | | | | | **☐** | ***55,20 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Dongle RTL-SDR + antenne — occupation de la bande 868 MHz | `RTL-SDR Blog V4` | 1 | Amazon | ☐ | ☐ | ☐ | *36,00 €* |
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | Amazon | ☐ | ☐ | ☐ | *18,00 €* |
| Adaptateur USB-série 3,3 V | `FTDI FT232RL ou CP2102` | 1 | Amazon | ☐ | ☐ | ☐ | *7,20 €* |
| Mesure de courant µA — sommeil profond **[A1]** | `multimètre à faible burden voltage` | 1 | Amazon | ☐ | ☐ | ☐ | *10,80 €* |
| **Sous-total** | | | | | | **☐** | ***72,00 €*** |

---

## Récapitulatif — variante A1 (LoRa homogène)

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *117,84 €* | *98,20 €* |
| 2 boutons sur pile | ☐ | *147,60 €* | *123,00 €* |
| Outillage | ☐ | *72,00 €* | *60,00 €* |
| **TOTAL** | **☐** | ***337,44 €*** | ***281,20 €*** |

## Récapitulatif — variante A3 (EnOcean + LoRa)

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *117,84 €* | *98,20 €* |
| Poste fixe EnOcean → LoRa | ☐ | *153,00 €* | *127,50 €* |
| 2 boutons PTM 210 | ☐ | *110,40 €* | *92,00 €* |
| Outillage | ☐ | *72,00 €* | *60,00 €* |
| **TOTAL** | **☐** | ***453,24 €*** | ***377,70 €*** |

### Où se croisent les deux courbes

| Stations | A1 (TTC) | A3 (TTC) | Moins cher |
|---:|---:|---:|---|
| 2 | 337,44 € | 453,24 € | **A1** |
| 4 | 485,04 € | 563,64 € | **A1** |
| 6 | 632,64 € | 674,04 € | **A1** |
| 8 | 780,24 € | 784,44 € | **A1** |
| 12 | 1 075,44 € | 1 005,24 € | **A3** |

Le point de bascule est à **8 stations**. En dessous, A1 coûte moins **et**
rend un accusé visuel à l'opérateur. Au-delà, A3 prend l'avantage grâce à des
boutons à 55,20 € au lieu de 73,80 €, et supprime les piles.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| **[A1]** bouton sur pile | **73,80 €** | 61,50 € |
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
