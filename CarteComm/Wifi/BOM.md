# Nomenclature — architecture Wi-Fi (carte V5.0.1 conservée)

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

### Carte AGV — 0 €

**La carte AIO AGV Control V5.0.1 est conservée.** Aucun composant n'est
ajouté : seuls ses deux firmwares sont réécrits. C'est le seul intérêt
économique décisif de cette architecture.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Carte existante — ESP32-WROOM-32E + ATmega2560 | `AIO AGV Control V5.0.1` | 1 | — | ☐ | ☐ | ☐ | *0,00 €* |
| **Sous-total** | | | | | | **☐** | ***0,00 €*** |

### Harnais de raccordement

La carte existe, mais son câblage vers l'automate est à refaire.
Détail : [`docs/subd25_atmega.md`](docs/subd25_atmega.md).

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Nappe 25 conducteurs, gaine souple, ~1 m | `3M 3365/25 ou équiv.` | 2 | RS | ☐ | ☐ | ☐ | *14,40 €* |
| Connecteur IDC SUB-D 25 **mâle** (entrées) | `Amphenol L17D25P` | 1 | RS | ☐ | ☐ | ☐ | *4,80 €* |
| Connecteur IDC SUB-D 25 **femelle** (sorties) | `Amphenol L17D25S` | 1 | RS | ☐ | ☐ | ☐ | *4,80 €* |
| Capot métallisé SUB-D 25 avec serre-câble | `Amphenol 17E-1726-2` | 2 | RS | ☐ | ☐ | ☐ | *8,40 €* |
| Cosses à sertir côté AGV (CN61 à CN64) | `selon bornier automate` | 50 | RS | ☐ | ☐ | ☐ | *9,00 €* |
| Gaine tressée, colliers, repérage des fils | `lot` | 1 | RS | ☐ | ☐ | ☐ | *9,00 €* |
| **Sous-total** | | | | | | **☐** | ***50,40 €*** |

### Antenne Wi-Fi déportée

L'antenne d'origine émet depuis l'intérieur d'un châssis métallique.
⚠️ Vérifier au démontage que le module ESP32 dispose d'un connecteur U.FL.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Antenne 2,4 GHz 2 dBi, embase SMA, déportée | `Siretta DELTA-6A ou équiv.` | 1 | RS | ☐ | ☐ | ☐ | *21,60 €* |
| Pigtail U.FL → SMA femelle + passe-cloison | `Amphenol 336312-24-0100` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| Support de fixation, visserie | `lot` | 1 | RS | ☐ | ☐ | ☐ | *4,80 €* |
| **Sous-total** | | | | | | **☐** | ***36,00 €*** |

### Poste fixe UniPi

Le poste le plus lourd de cette architecture : récepteur EnOcean, broker
MQTT et interface de supervision.

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Automate compact Linux, 1 Go / 8 Go eMMC, E/S TOR | `UniPi E413` | 1 | Spécialiste | ☐ | ☐ | ☐ | *450,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | Spécialiste | ☐ | ☐ | ☐ | *33,60 €* |
| Antenne EnOcean 868 MHz déportée + pigtail | `EnOcean ANT300 ou équiv.` | 1 | Spécialiste | ☐ | ☐ | ☐ | *12,00 €* |
| Adaptateur USB-série vers le TCM 515 | `FTDI FT232RL` | 1 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | RS | ☐ | ☐ | ☐ | *16,80 €* |
| Coffret rail DIN, bornier, presse-étoupes | `Fibox ou Schneider` | 1 | RS | ☐ | ☐ | ☐ | *24,00 €* |
| Câble Ethernet blindé vers le réseau usine | `Cat 6 S/FTP, 5 m` | 1 | RS | ☐ | ☐ | ☐ | *7,20 €* |
| **Sous-total** | | | | | | **☐** | ***553,20 €*** |

### Boutons d'appel EnOcean — 2 stations

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 2 | Spécialiste | ☐ | ☐ | ☐ | *72,00 €* |
| Enveloppe / poussoir mural compatible PTM 210 | `Eltako, NodOn ou Trio2Sys` | 2 | Spécialiste | ☐ | ☐ | ☐ | *28,80 €* |
| Plaque de repérage station gravée | `sur mesure` | 2 | Amazon | ☐ | ☐ | ☐ | *9,60 €* |
| Fixation, visserie, adhésif industriel | `3M VHB ou équiv.` | 2 | RS | ☐ | ☐ | ☐ | *9,60 €* |
| **Sous-total** | | | | | | **☐** | ***120,00 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Programmateur ISP — **sauvegarde puis flash de l'ATmega** | `USBasp ou USBtinyISP` | 1 | Amazon | ☐ | ☐ | ☐ | *9,60 €* |
| Adaptateur USB-série 3,3 V — ESP32 et liaison inter-MCU | `FTDI FT232RL ou CP2102` | 1 | Amazon | ☐ | ☐ | ☐ | *7,20 €* |
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | Amazon | ☐ | ☐ | ☐ | *18,00 €* |
| Jeu de cosses + pince à sertir — confection du harnais | `Knipex ou Engineer PA-09` | 1 | RS | ☐ | ☐ | ☐ | *54,00 €* |
| Kit réseau : testeur RJ45, sertisseuse | `lot` | 1 | Amazon | ☐ | ☐ | ☐ | *37,20 €* |
| **Sous-total** | | | | | | **☐** | ***126,00 €*** |

---

## Adaptation de niveaux — CONDITIONNEL

⚠️ **Poste chiffré à titre conservatoire, en attente de mesure.**

Le relevé de câblage amène les 21 lignes `Y` **directement** sur des broches de
l'ATmega, sans protection. Leur amplitude n'est pas connue (point bloquant W1b,
phase 1 de [`DEPLOY.md`](DEPLOY.md)).

| Résultat de la mesure sur `Y05` | Matériel à ajouter | *Repère TTC* |
|---|---|---:|
| ≤ V_CC de l'ATmega | rien | **0 €** |
| Légèrement au-dessus | 21 diviseurs résistifs 1 % | *9,60 €* |
| 24 V ou incompatible | 6× `PC847` + résistances + carte fille | *54,00 €* |

Dans le troisième cas, il faut aussi **router une carte fille** : plusieurs
semaines de délai, pas seulement un coût.

## Récapitulatif

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| **Carte AGV (conservée)** | ☐ | *0,00 €* | *0,00 €* |
| Harnais de raccordement | ☐ | *50,40 €* | *42,00 €* |
| Antenne Wi-Fi déportée | ☐ | *36,00 €* | *30,00 €* |
| Poste fixe UniPi | ☐ | *553,20 €* | *461,00 €* |
| 2 boutons EnOcean | ☐ | *120,00 €* | *100,00 €* |
| Outillage | ☐ | *126,00 €* | *105,00 €* |
| **TOTAL** | **☐** | ***885,60 €*** | ***738,00 €*** |

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| Bouton PTM 210 complet | **60,00 €** | 50,00 € |
| Avec accusé EnOcean par station | 156,00 € | 130,00 € |

### Coûts récurrents

| Poste | Annuel |
|---|---:|
| Abonnement opérateur | **0 €** |
| Infrastructure Wi-Fi | à la charge du client (existante) |
| Piles des boutons | **0 €** — PTM 210 auto-alimentés |

### Alternative à considérer

La planification pose la question (§12.2) : **historique ou état instantané ?**

Si un historique sur plusieurs semaines n'est **pas** attendu, un ESP32 avec
module Ethernet remplit la même fonction pour ~102,00 € au lieu de
553,20 € — soit **451,20 € d'économie**. L'UniPi ne se
justifie que par la persistance, l'interface web riche et l'hébergement du
broker.

---

## Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| `UniPi E413` | 2 à 6 semaines | **Chemin critique matériel.** Vérifier la référence exacte et le runtime livré (§12.9) avant commande |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS : prévoir un distributeur EnOcean |
| Carte AGV | — | **Aucun : elle existe** |
| Adaptation de niveaux | 3 à 5 semaines **si nécessaire** | Conditionnel à la mesure W1b — d'où l'urgence de la faire |
| Points d'accès Wi-Fi additionnels | variable | À la charge du client, dépend du relevé 0.2 |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : le relevé de
couverture Wi-Fi peut disqualifier l'architecture, et la mesure d'amplitude
change la nomenclature.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 3 à 5 jours-homme de mise en œuvre, hors développement.
- **Les points d'accès Wi-Fi additionnels**, si le relevé en révèle le besoin.
  À la charge du client, et potentiellement le poste le plus lourd du projet.
- **La carte de rechange** : si la sauvegarde des firmwares d'origine échoue
  (phase 2 de `DEPLOY.md`), il n'y a plus de retour arrière. Prévoir une V5.0.1
  de rechange devient une assurance à chiffrer avec le client.


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
