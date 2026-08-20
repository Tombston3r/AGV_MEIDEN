# Nomenclature — architecture SMS + EnOcean (carte neuve)

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

### Carte AGV — variante LTE-M

Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte**.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash | `ESP32-WROOM-32E-N8` | 1 | RS | ☐ | ☐ | ☐ | *6,00 €* |
| Modem LTE-M / NB-IoT, très basse consommation | `SIM7080G (SIMCom)` | 1 | Amazon | ☐ | ☐ | ☐ | *21,60 €* |
| Antenne LTE 4 dBi déportée + pigtail U.FL → SMA | `Siretta ECHO-9 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *14,40 €* |
| Support SIM nano, protection ESD | `Molex 785900001` | 1 | RS | ☐ | ☐ | ☐ | *1,80 €* |
| Optocoupleur quadruple — 43 voies | `PC847 (Sharp)` | 11 | RS | ☐ | ☐ | ☐ | *7,92 €* |
| Convertisseur DC/DC 24 V → 5 V 1 A | `TSR 1-2450 (Traco Power)` | 1 | RS | ☐ | ☐ | ☐ | *8,40 €* |
| LDO 3,3 V 600 mA | `AP2112K-3.3TRG1 (Diodes)` | 1 | RS | ☐ | ☐ | ☐ | *0,72 €* |
| Diode TVS protection 24 V | `SMBJ33A (Littelfuse)` | 2 | RS | ☐ | ☐ | ☐ | *1,20 €* |
| **Réservoir capacitif** — pics d'émission modem (2 A) | `470 µF low-ESR 16 V` | 1 | RS | ☐ | ☐ | ☐ | *3,60 €* |
| Résistances 1 %, découplages, LED d'état | `lot` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| ILS (reed) + aimant — Wi-Fi de maintenance | `Standex KSK-1A66 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *2,40 €* |
| SUB-D 25 mâle et femelle, coudés CI | `Amphenol L717SDB25xA4CH4F` | 2 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| PCB 4 couches ~120 × 100 mm (série de 5) | `Gerber projet` | 1 | PCB | ☐ | ☐ | ☐ | *14,40 €* |
| Boîtier, fixation, presse-étoupes, conn. de prog. | `Hammond 1590 ou Fibox` | 1 | RS | ☐ | ☐ | ☐ | *33,60 €* |
| **Sous-total** | | | | | | **☐** | ***132,84 €*** |

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

### Poste fixe — option A : ESP32 (recommandée)

Suffit dès lors que l'historique long terme n'est pas exigé.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash (LittleFS + pages web) | `ESP32-WROOM-32E-N8` | 1 | RS | ☐ | ☐ | ☐ | *6,00 €* |
| Modem LTE-M / NB-IoT | `SIM7080G (SIMCom)` | 1 | Amazon | ☐ | ☐ | ☐ | *21,60 €* |
| Antenne LTE déportée + pigtail | `Siretta ECHO-9 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *14,40 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | Spécialiste | ☐ | ☐ | ☐ | *33,60 €* |
| Antenne EnOcean déportée | `EnOcean ANT300 ou équiv.` | 1 | Spécialiste | ☐ | ☐ | ☐ | *9,60 €* |
| Ethernet SPI + RJ45 — **liaison filaire** | `WIZnet WIZ850io (W5500)` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| LED d'accusé, LED de vie, boutons appairage/reset | `lot` | 1 | RS | ☐ | ☐ | ☐ | *4,20 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | RS | ☐ | ☐ | ☐ | *16,80 €* |
| 24 V → 5 V → 3,3 V | `TSR 1-2450 + AP2112K-3.3` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| PCB 2 couches ~100 × 80 mm | `Gerber projet` | 1 | PCB | ☐ | ☐ | ☐ | *7,20 €* |
| Boîtier mural IP54, presse-étoupes, embases SMA | `Fibox ou Hammond 1554` | 1 | RS | ☐ | ☐ | ☐ | *36,00 €* |
| Support SIM, passifs | `Molex 785900001 + lot` | 1 | RS | ☐ | ☐ | ☐ | *3,60 €* |
| Câble Ethernet blindé | `Cat 6 S/FTP, 5 m` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| **Sous-total** | | | | | | **☐** | ***177,00 €*** |

### Poste fixe — option B : UniPi E413

À retenir seulement si un **historique sur plusieurs semaines** est demandé.
⚠️ §12.9 : vérifier le runtime livré avant commande.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Automate compact Linux, E/S TOR, modem LTE intégré | `UniPi E413 (variante LTE)` | 1 | Spécialiste | ☐ | ☐ | ☐ | *420,00 €* |
| Antenne LTE externe déportée | `Siretta ECHO-9 ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *18,00 €* |
| Récepteur EnOcean + antenne | `TCM 515 + ANT300` | 1 | Spécialiste | ☐ | ☐ | ☐ | *43,20 €* |
| Coffret rail DIN, alimentation, bornier | `Fibox + MEAN WELL HDR-15-24` | 1 | RS | ☐ | ☐ | ☐ | *40,80 €* |
| Câble Ethernet blindé | `Cat 6 S/FTP, 5 m` | 1 | RS | ☐ | ☐ | ☐ | *4,80 €* |
| **Sous-total** | | | | | | **☐** | ***526,80 €*** |

### Boutons d'appel EnOcean — 2 stations

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 2 | Spécialiste | ☐ | ☐ | ☐ | *72,00 €* |
| Enveloppe / poussoir mural compatible PTM 210 | `Eltako, NodOn ou Trio2Sys` | 2 | Spécialiste | ☐ | ☐ | ☐ | *28,80 €* |
| Plaque de repérage station gravée | `sur mesure` | 2 | Amazon | ☐ | ☐ | ☐ | *9,60 €* |
| Fixation, visserie | `lot` | 2 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| **Sous-total** | | | | | | **☐** | ***120,00 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | Amazon | ☐ | ☐ | ☐ | *18,00 €* |
| Adaptateur USB-série 3,3 V — mise au point pile AT | `FTDI FT232RL ou CP2102` | 1 | Amazon | ☐ | ☐ | ☐ | *7,20 €* |
| Jeu de cosses, pince à sertir, consommables | `Knipex ou Engineer PA-09` | 1 | RS | ☐ | ☐ | ☐ | *28,80 €* |
| **Sous-total** | | | | | | **☐** | ***54,00 €*** |

---

## Interface bus

Les deux variantes d'interface bus de la section précédente s'appliquent aussi
ici : `shift595` est retenue par défaut. Sous-total carte AGV complète :
***136,44 €*** TTC (113,70 € HT).

## Récapitulatif — variante B (LTE-M / MQTT), poste ESP32

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *136,44 €* | *113,70 €* |
| Poste fixe ESP32 (option A) | ☐ | *177,00 €* | *147,50 €* |
| 2 boutons EnOcean | ☐ | *120,00 €* | *100,00 €* |
| Outillage | ☐ | *54,00 €* | *45,00 €* |
| **TOTAL** | **☐** | ***487,44 €*** | ***406,20 €*** |

### Avec le poste UniPi (option B)

| Poste | *Repère TTC* | *Repère HT* |
|---|---:|---:|
| Carte AGV | *136,44 €* | *113,70 €* |
| Poste UniPi E413 | *526,80 €* | *439,00 €* |
| 2 boutons EnOcean | *120,00 €* | *100,00 €* |
| Outillage | *54,00 €* | *45,00 €* |
| **TOTAL** | ***837,24 €*** | ***697,70 €*** |

L'écart entre les deux postes est de **349,80 €** :
c'est le prix de l'historique long terme et d'un automate référencé.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| Bouton PTM 210 complet | **60,00 €** | 50,00 € |

---

## Abonnements et infrastructure

| Poste | HT | TTC |
|---|---:|---:|
| 2 SIM M2M data LTE-M, ~1,50 €/mois | 36 €/an | 43,20 €/an |
| Broker MQTT — VPS mutualisé | 60 €/an | 72,00 €/an |
| *Alternative* : Mosquitto sur un serveur usine existant | 0 €/an | 0 €/an |
| **Total récurrent** | **96,00 €/an** | **115,20 €/an** |

Héberger le broker sur un serveur du client supprime le poste VPS **et** la
dépendance à un tiers. À proposer systématiquement.

### Coût sur 10 ans — variante B

| | Poste ESP32 | Poste UniPi |
|---|---:|---:|
| Matériel et outillage (TTC) | 487,44 € | 837,24 € |
| Récurrent sur 10 ans (TTC) | 1 152,00 € | 1 152,00 € |
| **Total 10 ans (TTC)** | **1 639,44 €** | **1 989,24 €** |
| *pour mémoire, en HT* | *1 366,20 €* | *1 657,70 €* |

---

## Variante A — SMS, pour mémoire

Chiffrée pour la comparaison. **Ne pas déployer** en liaison principale : ni
latence bornée, ni ordre de remise, ni garantie de remise.

| Poste | *Repère TTC* | *Repère HT* |
|---|---:|---:|
| Poste UniPi E413 (LTE) + antenne + boutons filaires + câblage | *526,80 €* | *439,00 €* |
| Carte AGV avec modem `SIM7600E-H` (Cat-1, plus gourmand) | *169,20 €* | *141,00 €* |
| Outillage | *54,00 €* | *45,00 €* |
| **Total matériel** | ***750,00 €*** | ***625,00 €*** |

| Récurrent | HT | TTC |
|---|---:|---:|
| 2 abonnements SIM M2M | 120 à 240 €/an | 144,00 € à 288,00 €/an |
| Volume SMS (appels + accusés + télémétrie dégradée) | 1 000 à 1 300 €/an | 1 200,00 € à 1 560,00 €/an |
| **Total** | **~1 500 €/an** | **~1 800,00 €/an** |
| **Sur 10 ans, tout compris** | **~15 625,00 €** | **~18 750,00 €** |

Le surcoût sur dix ans face à la variante B est de **~14 258,80 € HT**,
pour un service strictement inférieur. C'est l'argument chiffré à opposer si le
SMS est demandé.

---

## Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| PCB 4 couches + assemblage | 3 à 5 semaines | **Chemin critique matériel** |
| `SIM7080G` | 2 à 4 semaines | Tensions récurrentes sur les modules cellulaires |
| SIM M2M data LTE-M | 1 à 3 semaines | Contractuel, pas technique |
| `UniPi E413` (si option B) | 2 à 6 semaines | Vérifier la référence **et le runtime** |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS |
| `ESP32`, `SN74HC595N`, `PC847` | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : un seul
point d'arrêt sous −110 dBm disqualifie l'architecture, et le choix de variante
d'interface bus conditionne le routage du PCB.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 6 à 9 jours-homme de mise en œuvre, hors développement.
- **Le contrôle de l'obsolescence 2G/3G** : le `SIM7080G` est LTE-M/NB-IoT, donc
  hors calendrier d'extinction. Un module 2G ne le serait pas.
- **La carte de rechange** : ~136,44 € pour un échange standard.


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
