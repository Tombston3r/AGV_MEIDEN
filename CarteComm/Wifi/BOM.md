# Nomenclature — A4 — Wi-Fi + EnOcean (carte V5.0.1 conservée)

## 💰 Total

| | HT | TTC | *Accessoires écartés* |
|---|---:|---:|---:|
| **A4 — Wi-Fi + EnOcean** | **550,65 €** | **660,78 €** | *+ 141,50 € HT* |

Ces totaux ne comptent que les **composants déterminants**.

Les accessoires arbitrables selon le budget — antennes, boîtiers,
coffrets, enveloppes murales, câbles — sont volontairement **hors
nomenclature** : ils se substituent librement d'un fournisseur à l'autre
et ne changent rien à la conception. La dernière colonne rappelle ce
qu'ils pèsent, pour que ce total ne soit jamais pris pour un coût
d'achat complet.

⚠️ [`../COMPARAISON.md`](../COMPARAISON.md) et le `README.md` racine comparent les architectures
**accessoires compris** — sans quoi le classement serait faussé. Leurs
chiffres sont donc plus élevés que ceux-ci, et c'est normal.

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

### **[A4]** Carte AGV — extraite du projet KiCad

**Nomenclature réelle**, extraite de
[`hardware/AIO_AGV_Control_V5.0.1/`](hardware/AIO_AGV_Control_V5.0.1/) :
57 composants placés au PCB. Ce n'est plus une estimation d'étude.

⚠️ Cette carte est **fabriquée**, pas réutilisée : elle reprend le couple
ATmega2560 + ESP32 de l'originale, sur supports, avec son propre étage de
sortie. La ligne « carte AGV à 0 € » des versions précédentes de ce
document était donc fausse.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU — carte Mega2560 Pro sur support | `Clone Mega2560 Pro (A1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Clone+Mega2560+Pro) · [Amazon](https://www.amazon.fr/s?k=Clone+Mega2560+Pro) | ☐ | ☐ | ☐ | *21,60 €* |
| Module Wi-Fi/BT sur support | `ESP32-DEVKITC-32D-F (U1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-DEVKITC-32D-F) | ☐ | ☐ | ☐ | *14,40 €* |
| **Étage de sortie** — MOSFET N canal TO-220 | `IRF520 (Vishay, T1–T24)` | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=IRF520) | ☐ | ☐ | ☐ | *16,56 €* |
| Résistances de grille des MOSFET | `1 kΩ THT 0411 (R1–R24)` | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=1+k%CE%A9+THT+0411) | ☐ | ☐ | ☐ | *1,38 €* |
| Diviseurs de mesure | `4,7 k / 2,2 k / 22 k / 220 k (R30, R31, R40, R41)` | 4 | [RS](https://fr.rs-online.com/web/c/?searchTerm=4%2C7+k+%2F+2%2C2+k+%2F+22+k+%2F+220+k) | ☐ | ☐ | ☐ | *0,24 €* |
| Régulateur 6 V — alimentation de l'ATmega | `L7806CV (LM1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=L7806CV) | ☐ | ☐ | ☐ | *1,08 €* |
| Convertisseur DC/DC **isolé** 24 V → 5 V, 5 W | `TDN 5-2411WISM (Traco, TDN1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TDN+5-2411WISM) | ☐ | ☐ | ☐ | *30,00 €* |
| Diode de protection DO-41 | `1N4007 ou équiv. (D1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=1N4007) | ☐ | ☐ | ☐ | *0,12 €* |
| Connecteur SUB-D 25 **mâle** coudé CI (entrées) | `Amphenol DB25P564CTXLF (J1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+DB25P564CTXLF) | ☐ | ☐ | ☐ | *7,20 €* |
| Connecteur SUB-D 25 **femelle** coudé CI (sorties) | `Amphenol DB25S564GTLF (J2)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+DB25S564GTLF) | ☐ | ☐ | ☐ | *7,20 €* |
| Supports et barrettes pour les deux modules | `barrettes tulipe 2,54 mm` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=barrettes+tulipe+2%2C54+mm) | ☐ | ☐ | ☐ | *3,60 €* |
| PCB ~150 × 100 mm (série de 5) | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *18,00 €* |
| **Sous-total** | | | | | | **☐** | ***121,38 €*** |

### **[A4]** Harnais de raccordement

La carte existe, mais son câblage vers l'automate est à refaire.
Détail : [`docs/subd25_atmega.md`](docs/subd25_atmega.md).

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Nappe 25 conducteurs, gaine souple, ~1 m | `3M 3365/25 ou équiv.` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=3M+3365%2F25) | ☐ | ☐ | ☐ | *14,40 €* |
| Connecteur IDC SUB-D 25 **mâle** (entrées) | `Amphenol L17D25P` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L17D25P) | ☐ | ☐ | ☐ | *4,80 €* |
| Connecteur IDC SUB-D 25 **femelle** (sorties) | `Amphenol L17D25S` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L17D25S) | ☐ | ☐ | ☐ | *4,80 €* |
| Capot métallisé SUB-D 25 avec serre-câble | `Amphenol 17E-1726-2` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+17E-1726-2) | ☐ | ☐ | ☐ | *8,40 €* |
| Cosses à sertir côté AGV (CN61 à CN64) | `selon bornier automate` | 50 | [RS](https://fr.rs-online.com/web/c/?searchTerm=selon+bornier+automate) | ☐ | ☐ | ☐ | *9,00 €* |
| **Sous-total** | | | | | | **☐** | ***41,40 €*** |

### **[A4]** Poste fixe — Unipi Gate G100

Le poste porte le récepteur EnOcean, le broker MQTT et l'interface de
supervision. **Le Gate G100 remplace l'E413 initialement prévu** : voir
la justification en fin de document.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Passerelle Linux DIN — Debian, 16 Go eMMC, 2× Ethernet, USB 3.0, RS485 | `Unipi Gate G100` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Unipi+Gate+G100) · [Unipi](https://www.unipi.technology/search?query=Unipi+Gate+G100) | ☐ | ☐ | ☐ | *240,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515) | ☐ | ☐ | ☐ | *33,60 €* |
| Adaptateur USB-série vers le TCM 515 | `FTDI FT232RL` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL) | ☐ | ☐ | ☐ | *9,60 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MEAN+WELL+HDR-15-24) | ☐ | ☐ | ☐ | *16,80 €* |
| **Sous-total** | | | | | | **☐** | ***300,00 €*** |

### **[A4]** Boutons d'appel EnOcean — 2 stations

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PTM+210) · [Mouser](https://www.mouser.fr/c/?q=PTM+210) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=PTM+210) | ☐ | ☐ | ☐ | *72,00 €* |
| **Sous-total** | | | | | | **☐** | ***72,00 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Programmateur ISP — **sauvegarde puis flash de l'ATmega** | `USBasp ou USBtinyISP` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=USBasp+ou+USBtinyISP) · [Amazon](https://www.amazon.fr/s?k=USBasp+ou+USBtinyISP) | ☐ | ☐ | ☐ | *9,60 €* |
| Adaptateur USB-série 3,3 V — ESP32 et liaison inter-MCU | `FTDI FT232RL ou CP2102` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL+ou+CP2102) · [Amazon](https://www.amazon.fr/s?k=FTDI+FT232RL+ou+CP2102) | ☐ | ☐ | ☐ | *7,20 €* |
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=clone+Saleae+24+MHz) · [Amazon](https://www.amazon.fr/s?k=clone+Saleae+24+MHz) | ☐ | ☐ | ☐ | *18,00 €* |
| Jeu de cosses + pince à sertir — confection du harnais | `Knipex ou Engineer PA-09` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Knipex+ou+Engineer+PA-09) | ☐ | ☐ | ☐ | *54,00 €* |
| Kit réseau : testeur RJ45, sertisseuse | `lot` | 1 | — | ☐ | ☐ | ☐ | *37,20 €* |
| **Sous-total** | | | | | | **☐** | ***126,00 €*** |

---

## Ce que la nomenclature KiCad apprend

L'extraction du projet KiCad ne fait pas que donner des prix : elle renseigne
deux points qui étaient ouverts.

### 1. L'étage de sortie est à MOSFET — `x_open_drain` doit être `false`

Les 22 voies X passent par **23 IRF520** (MOSFET N canal, TO-220) attaqués par
des résistances de grille de 1 kΩ. C'est un étage à **collecteur ouvert
matériel** : le MOSFET tire la ligne de l'automate à la masse, il ne sort jamais
de niveau haut.

Conséquence directe sur le firmware : le microcontrôleur pilote une **grille**,
pas la ligne de l'automate. Il doit donc être en **sortie poussée**
(`bus.x_open_drain: false`). Le mode collecteur ouvert côté microcontrôleur
laisserait la grille **flottante** à l'état actif — un MOSFET à grille flottante
peut conduire partiellement, ce qui est le pire état possible sur un étage de
puissance.

L'inversion est faite par le MOSFET : microcontrôleur à l'état haut → MOSFET
passant → ligne automate tirée à 0 V.

⚠️ **23 MOSFET pour 22 voies X** (T13 absent de la numérotation). À vérifier au
schéma : voie de réserve, ou signal supplémentaire non identifié.

### 2. Les 21 entrées Y n'ont aucune protection

La carte ne compte que **quatre résistances** hors étage de sortie — deux
diviseurs (`R30`/`R31` et `R40`/`R41`), vraisemblablement pour une mesure de
tension. **Aucun optocoupleur, aucun diviseur sur les 21 lignes `Y`** : elles
arrivent directement sur les broches du Mega.

Le point bloquant W1b reste donc entier, et il est maintenant confirmé par le
routage : si l'amplitude des lignes `Y` dépasse V_CC, rien ne protège le
microcontrôleur.

### 3. L'alimentation est isolée

Le `TDN 5-2411WISM` est un convertisseur **isolé 1,5 kV**, 5 W. C'est le poste
le plus cher de la carte, et il explique pourquoi la masse de l'AGV et celle de
la logique sont séparées.

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
| **Carte AGV (nomenclature KiCad)** | ☐ | *121,38 €* | *101,15 €* |
| Harnais de raccordement | ☐ | *41,40 €* | *34,50 €* |
| Poste fixe UniPi | ☐ | *300,00 €* | *250,00 €* |
| 2 boutons EnOcean | ☐ | *72,00 €* | *60,00 €* |
| Outillage | ☐ | *126,00 €* | *105,00 €* |
| **TOTAL** | **☐** | ***660,78 €*** | ***550,65 €*** |

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

### Pourquoi le Gate G100 et non l'E413

Le service du poste n'utilise, en tout et pour tout, **un port série et de
l'Ethernet** : le récepteur EnOcean d'un côté, le broker MQTT de l'autre. Les
boutons étant EnOcean, **aucune entrée TOR n'est utilisée** —
`agv_poste/io_backend.py` n'est même pas appelé par le service.

Payer un automate à entrées/sorties revient donc à financer du matériel qui ne
sert pas.

| | Unipi Gate G100 | Unipi E413 / Patron |
|---|---|---|
| Prix indicatif | **~200 €** | 375 à 479 € |
| Système | **Debian Linux** | à vérifier (§12.9) |
| Ethernet | **2 ports** (Gb + 100M) | 1 |
| USB | 1× USB 3.0 — pour le TCM 515 | selon modèle |
| RS485 | 1 (2 isolés sur G110) | selon modèle |
| Alimentation | 6–36 VDC | 24 V |
| Stockage | 16 Go eMMC + microSD | 8 Go eMMC |
| Entrées/sorties TOR | **aucune** | plusieurs — **inutilisées ici** |

Trois gains au-delà du prix :

1. **Le point ouvert §12.9 disparaît.** Le Gate est livré sous **Debian**, donc
   le service Python 3.11 et systemd fonctionnent sans question. L'incertitude
   « Mervis ou Linux ? » qui bloquait le dossier ne se pose plus.
2. **Deux ports Ethernet** permettent de séparer physiquement le VLAN OT du
   raccordement de maintenance — un argument de plus auprès du service
   informatique.
3. **16 Go d'eMMC** au lieu de 8 : de la marge pour le journal d'événements.

Ce raisonnement ne vaut que pour **cette** architecture : le poste LoRa, lui,
doit piloter une radio SX1276 en SPI, ce qu'un boîtier DIN fermé ne permet pas.
Voir [`../LoRa/BOM.md`](../LoRa/BOM.md), section « Pourquoi pas un Unipi Gate
pour ce poste ? ».

Le seul port USB est à surveiller : il est pris par l'adaptateur série du
TCM 515. Si un second périphérique USB devenait nécessaire, passer le TCM 515
en RS485 via un convertisseur TTL/RS485 (~10 €) libère le port.

⚠️ **Prix relevé en 2021 sur un article de presse, à confirmer.** Et la
référence « E413 » du document de planification n'a pas pu être retrouvée au
catalogue Unipi : vérifier qu'elle existe encore avant toute comparaison
formelle.

### Alternative encore moins chère

Si aucun historique long terme n'est attendu, un ESP32 avec module Ethernet
remplit la même fonction pour ~102,00 € au lieu de 300,00 € — soit
**198,00 € d'économie**. Le Gate ne se justifie que par la
persistance, l'hébergement du broker et le fait d'être un matériel industriel
référencé.

---

## Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| `UniPi E413` | 2 à 6 semaines | **Chemin critique matériel.** Vérifier la référence exacte et le runtime livré (§12.9) avant commande |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS : prévoir un distributeur EnOcean |
| PCB + assemblage de la carte AGV | 3 à 5 semaines | **Chemin critique matériel** — la carte est fabriquée, pas réutilisée |
| `TDN 5-2411WISM` | 1 à 3 semaines | Convertisseur isolé : poste le plus cher de la carte |
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

## Analyse critique de la carte routée

Trois observations issues du projet KiCad. Aucune ne remet en cause le routage,
mais deux méritent une décision avant fabrication.

### ⚠️ L'`IRF520` n'est pas un MOSFET « logic-level »

Sa tension de seuil est spécifiée **de 2 à 4 V**, et son `Rds(on)` est garanti à
**Vgs = 10 V**. Attaqué par une broche à 5 V, il conduit — mais hors des
conditions du constructeur, et avec une résistance à l'état passant très
supérieure à celle annoncée.

**Pour les courants en jeu, quelques milliampères sur une entrée d'automate,
cela fonctionne.** La chute de tension reste négligeable même à 10 Ω. Ce n'est
donc pas un défaut bloquant — c'est un choix hors spécification, qu'il faut
connaître avant de l'attribuer à autre chose le jour où une voie se comporte mal
en température.

Si le PCB n'est pas encore fabriqué, deux alternatives valent d'être pesées :

| Solution | Coût | Surface | Remarque |
|---|---:|---|---|
| `IRF520` ×23 (actuel) | ~16,56 € | **23 boîtiers TO-220** | Hors spec à 5 V, très encombrant |
| `IRL520` ×23 | ~30,36 € | idem | Version **logic-level** du même composant, brochage identique — **substitution sans reroutage** |
| `ULN2803A` ×3 | ~4,32 € | 3 boîtiers DIP-18 | Réseau Darlington à collecteur ouvert : même fonction, **résistances de grille supprimées**, surface divisée par dix |

L'`IRL520` est le changement le moins risqué : même boîtier, même brochage,
aucune modification du circuit. L'`ULN2803A` est le plus rationnel si le
routage peut encore bouger, mais il sature à ~1,1 V au lieu de ~0,1 V : **à
valider contre le seuil d'entrée de l'automate** avant de le retenir.

### ⚠️ Isolation : le convertisseur isolé et l'étage à MOSFET se contredisent

Le `TDN 5-2411WISM` est un convertisseur **isolé 1,5 kV** — le poste le plus
cher de la carte. Or les MOSFET de sortie tirent les lignes de l'automate vers
**la masse de la carte** : il y a donc bien une masse commune avec l'automate,
et l'isolation galvanique n'existe pas au niveau des signaux.

Deux lectures possibles, et il faut trancher :

- **l'isolation sert à découpler le 24 V de l'AGV du rail logique** (bruit,
  transitoires du chariot). Elle est alors justifiée, et le convertisseur reste ;
- **l'isolation était censée s'appliquer aux signaux.** Dans ce cas elle est
  inopérante, et un `TSR 1-2450` non isolé à ~8,40 € remplacerait le `TDN` — soit
  **21,60 € d'économie** sur la ligne la plus chère de la carte.

### ⚠️ Le 6 V du `L7806CV` alimente un microcontrôleur prévu pour 5,5 V

Rappel de [`docs/subd25_atmega.md`](docs/subd25_atmega.md) : 6,0 V est le
**maximum absolu** de l'ATmega2560. Le `TDN` fournit déjà un 5 V propre sur la
carte. Si le Mega peut être alimenté depuis ce 5 V, le `L7806CV` devient inutile
et le microcontrôleur revient dans sa plage recommandée.

À vérifier au schéma : le 6 V va-t-il sur `Vin` du module Mega — auquel cas il
est *trop bas* pour son régulateur — ou directement sur `V_CC` ?


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
