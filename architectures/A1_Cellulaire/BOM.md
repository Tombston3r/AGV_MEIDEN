# Nomenclature — A1 — Cellulaire + EnOcean (SMS ou LTE-M)

## 💰 Total

| | HT | TTC | *Accessoires écartés* |
|---|---:|---:|---:|
| **A1 — poste ESP32 (recommandé)** | **266,70 €** | **320,04 €** | *+ 139,50 € HT* |
| **A1 — poste Unipi Gate G100** | **428,70 €** | **514,44 €** | *+ 116,00 € HT* |
| **A1 — poste UniPi E413 LTE** | **564,70 €** | **677,64 €** | *+ 133,00 € HT* |

Ces totaux ne comptent que les **composants déterminants**.

Les accessoires arbitrables selon le budget — antennes, boîtiers,
coffrets, enveloppes murales, câbles — sont volontairement **hors
nomenclature** : ils se substituent librement d'un fournisseur à l'autre
et ne changent rien à la conception. La dernière colonne rappelle ce
qu'ils pèsent, pour que ce total ne soit jamais pris pour un coût
d'achat complet.

⚠️ [`../../docs/COMPARAISON.md`](../../docs/COMPARAISON.md) et le `README.md` racine comparent les architectures
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
[`../../docs/COMPARAISON.md`](../../docs/COMPARAISON.md) raisonne en HT. Diviser un total TTC par 1,20 donne le HT.

---

### **[A1]** Carte AGV — variante LTE-M

Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte**.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash | `ESP32-WROOM-32E-N8` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | ☐ | ☐ | ☐ | *6,00 €* |
| Modem LTE-M / NB-IoT, très basse consommation | `SIM7080G (SIMCom)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SIM7080G) · [Amazon](https://www.amazon.fr/s?k=SIM7080G) | ☐ | ☐ | ☐ | *21,60 €* |
| Support SIM nano, protection ESD | `Molex 785900001` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Molex+785900001) | ☐ | ☐ | ☐ | *1,80 €* |
| Optocoupleur quadruple — 43 voies | `PC847 (Sharp)` | 11 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PC847) | ☐ | ☐ | ☐ | *7,92 €* |
| Convertisseur DC/DC 24 V → 5 V 1 A | `TSR 1-2450 (Traco Power)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450) | ☐ | ☐ | ☐ | *8,40 €* |
| LDO 3,3 V 600 mA | `AP2112K-3.3TRG1 (Diodes)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=AP2112K-3.3TRG1) | ☐ | ☐ | ☐ | *0,72 €* |
| Diode TVS protection 24 V | `SMBJ33A (Littelfuse)` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SMBJ33A) | ☐ | ☐ | ☐ | *1,20 €* |
| **Réservoir capacitif** — pics d'émission modem (2 A) | `470 µF low-ESR 16 V` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=470+%C2%B5F+low-ESR+16+V) | ☐ | ☐ | ☐ | *3,60 €* |
| Résistances 1 %, découplages, LED d'état | `lot` | 1 | — | ☐ | ☐ | ☐ | *9,60 €* |
| ILS (reed) + aimant — Wi-Fi de maintenance | `Standex KSK-1A66 ou équiv.` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Standex+KSK-1A66) | ☐ | ☐ | ☐ | *2,40 €* |
| SUB-D 25 mâle et femelle, coudés CI | `Amphenol L717SDB25xA4CH4F` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L717SDB25xA4CH4F) | ☐ | ☐ | ☐ | *7,20 €* |
| PCB 4 couches ~120 × 100 mm (série de 5) | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *14,40 €* |
| **Sous-total** | | | | | | **☐** | ***84,84 €*** |

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

### **[A1]** Poste fixe — option A : ESP32 (recommandée)

Suffit dès lors que l'historique long terme n'est pas exigé.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash (LittleFS + pages web) | `ESP32-WROOM-32E-N8` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | ☐ | ☐ | ☐ | *6,00 €* |
| Modem LTE-M / NB-IoT | `SIM7080G (SIMCom)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SIM7080G) · [Amazon](https://www.amazon.fr/s?k=SIM7080G) | ☐ | ☐ | ☐ | *21,60 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515) | ☐ | ☐ | ☐ | *33,60 €* |
| Ethernet SPI + RJ45 — **liaison filaire** | `WIZnet WIZ850io (W5500)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=WIZnet+WIZ850io) | ☐ | ☐ | ☐ | *7,20 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MEAN+WELL+HDR-15-24) | ☐ | ☐ | ☐ | *16,80 €* |
| 24 V → 5 V → 3,3 V | `TSR 1-2450 + AP2112K-3.3` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450+%2B+AP2112K-3.3) | ☐ | ☐ | ☐ | *9,60 €* |
| PCB 2 couches ~100 × 80 mm | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *7,20 €* |
| Support SIM, passifs | `Molex 785900001 + lot` | 1 | — | ☐ | ☐ | ☐ | *3,60 €* |
| **Sous-total** | | | | | | **☐** | ***105,60 €*** |

### **[A1]** Poste fixe — option B : Unipi Gate G100 (**si Ethernet disponible**)

Le modem du poste ne sert à rien dès qu'une prise réseau est à portée :
le poste parle au broker par le fil. Cela **supprime une SIM sur deux** et
ouvre la gamme Gate, qui n'a pas de cellulaire. Debian d'origine.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Passerelle Linux DIN — Debian, 16 Go eMMC, 2× Ethernet, USB 3.0, RS485 | `Unipi Gate G100` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Unipi+Gate+G100) · [Unipi](https://www.unipi.technology/search?query=Unipi+Gate+G100) | ☐ | ☐ | ☐ | *240,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515) | ☐ | ☐ | ☐ | *33,60 €* |
| Adaptateur USB-série vers le TCM 515 | `FTDI FT232RL` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL) | ☐ | ☐ | ☐ | *9,60 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MEAN+WELL+HDR-15-24) | ☐ | ☐ | ☐ | *16,80 €* |
| **Sous-total** | | | | | | **☐** | ***300,00 €*** |

### **[A1]** Poste fixe — option C : UniPi E413 LTE (**seulement sans Ethernet**)

À ne retenir que si le poste est hors de portée d'une prise réseau. Le
modem intégré est alors la raison d'être du modèle — et son surcoût.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Automate compact Linux, E/S TOR, modem LTE intégré | `UniPi E413 (variante LTE)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=UniPi+E413) · [Unipi](https://www.unipi.technology/search?query=UniPi+E413) | ☐ | ☐ | ☐ | *420,00 €* |
| Récepteur EnOcean + antenne | `TCM 515 + ANT300` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515+%2B+ANT300) · [Mouser](https://www.mouser.fr/c/?q=TCM+515+%2B+ANT300) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515+%2B+ANT300) | ☐ | ☐ | ☐ | *43,20 €* |
| **Sous-total** | | | | | | **☐** | ***463,20 €*** |

### **[A1]** Boutons d'appel EnOcean — 2 stations

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PTM+210) · [Mouser](https://www.mouser.fr/c/?q=PTM+210) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=PTM+210) | ☐ | ☐ | ☐ | *72,00 €* |
| **Sous-total** | | | | | | **☐** | ***72,00 €*** |

### Outillage — non récurrent

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Analyseur logique 8 voies — chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=clone+Saleae+24+MHz) · [Amazon](https://www.amazon.fr/s?k=clone+Saleae+24+MHz) | ☐ | ☐ | ☐ | *18,00 €* |
| Adaptateur USB-série 3,3 V — mise au point pile AT | `FTDI FT232RL ou CP2102` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL+ou+CP2102) · [Amazon](https://www.amazon.fr/s?k=FTDI+FT232RL+ou+CP2102) | ☐ | ☐ | ☐ | *7,20 €* |
| Jeu de cosses, pince à sertir, consommables | `Knipex ou Engineer PA-09` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Knipex+ou+Engineer+PA-09) | ☐ | ☐ | ☐ | *28,80 €* |
| **Sous-total** | | | | | | **☐** | ***54,00 €*** |

---

## Interface bus

Les deux variantes d'interface bus de la section précédente s'appliquent aussi
ici : `shift595` est retenue par défaut. Sous-total carte AGV complète :
***88,44 €*** TTC (73,70 € HT).

## Récapitulatif — variante B (LTE-M / MQTT), poste ESP32

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV (variante `shift595`) | ☐ | *88,44 €* | *73,70 €* |
| Poste fixe ESP32 (option A) | ☐ | *105,60 €* | *88,00 €* |
| 2 boutons EnOcean | ☐ | *72,00 €* | *60,00 €* |
| Outillage | ☐ | *54,00 €* | *45,00 €* |
| **TOTAL** | **☐** | ***320,04 €*** | ***266,70 €*** |

### Avec un poste UniPi — laquelle des deux options ?

| Poste | *Repère TTC* | *Repère HT* | Récurrent |
|---|---:|---:|---|
| **B — Unipi Gate G100** *(si Ethernet)* | ***514,44 €*** | ***428,70 €*** | **1 SIM** |
| **C — UniPi E413 LTE** *(sans Ethernet)* | *677,64 €* | *564,70 €* | 2 SIM |

**163,20 € d'écart de matériel — et la
moitié du récurrent.** Le Gate n'a pas de modem cellulaire ; c'est précisément ce
qui le rend éligible ici, puisqu'un poste raccordé en Ethernet n'en a aucun
besoin. Il apporte au passage Debian d'origine, deux ports Ethernet et 16 Go
d'eMMC.

**La question à poser au client tient en une phrase : y a-t-il une prise réseau
là où le poste sera fixé ?** Si oui, l'option C n'a plus de justification.

⚠️ L'unique port USB du Gate est pris par l'adaptateur série du `TCM 515`.
Prévoir un concentrateur si un autre périphérique USB devient nécessaire.

Face à l'option A (ESP32, 105,60 €), l'écart restant est de
**194,40 €** : c'est le prix de
l'historique long terme et d'un matériel référencé.

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
| Matériel et outillage (TTC) | 320,04 € | 677,64 € |
| Récurrent sur 10 ans (TTC) | 1 152,00 € | 1 152,00 € |
| **Total 10 ans (TTC)** | **1 472,04 €** | **1 829,64 €** |
| *pour mémoire, en HT* | *1 226,70 €* | *1 524,70 €* |

---

## Variante A — SMS, pour mémoire

Chiffrée pour la comparaison. **Ne pas déployer** en liaison principale : ni
latence bornée, ni ordre de remise, ni garantie de remise.

⚠️ Ce tableau est chiffré **accessoires compris** — antennes, coffrets,
câblage — contrairement au reste du document. C'est la nomenclature d'étude
d'origine, conservée telle quelle : elle ne sert qu'à donner l'ordre de
grandeur du récurrent, qui est l'argument décisif.

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
| `Unipi Gate G100` (option B) | 2 à 6 semaines | Debian d'origine, pas de runtime à vérifier |
| `UniPi E413` (option C) | 2 à 6 semaines | Vérifier l'existence de la **variante LTE** au catalogue |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS |
| `ESP32-DEVKITC`, `Mega2560 Pro`, `IRF520` | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : un seul
point d'arrêt sous −110 dBm disqualifie l'architecture, et le choix de variante
d'interface bus conditionne le routage du PCB.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 6 à 9 jours-homme de mise en œuvre, hors développement.
- **Le contrôle de l'obsolescence 2G/3G** : le `SIM7080G` est LTE-M/NB-IoT, donc
  hors calendrier d'extinction. Un module 2G ne le serait pas.
- **La carte de rechange** : ~88,44 € pour un échange standard.
---

## Analyse critique de cette nomenclature

### ✅ Corrigé — le réservoir capacitif était dimensionné pour le mauvais modem

La nomenclature d'étude annonçait des pics de **2 A**. C'est la valeur d'un
`SIM7600E-H` (LTE Cat-1), retenu dans la **variante SMS**. Le `SIM7080G` de la
variante recommandée est un modem **LTE-M / NB-IoT** : ses pics d'émission sont
de l'ordre de **0,5 à 0,7 A**.

La ligne reste — un réservoir est nécessaire — mais son dimensionnement change,
et il ne faut pas surdimensionner l'alimentation pour un besoin qui n'existe
pas. À confirmer sur la fiche technique du modem effectivement commandé.

### ✅ Corrigé — le poste UniPi peut descendre en gamme

Même constat que dans l'architecture Wi-Fi : les boutons sont **EnOcean**, donc
`agv_poste/io_backend.py` n'est jamais appelé. Les entrées TOR de l'`E413` sont
payées et inutilisées.

L'`E413` était pourtant imposé par une contrainte réelle — son **modem
cellulaire intégré**. C'est cette contrainte qui tombe : si le poste est
raccordé en Ethernet, il n'a aucune raison de passer par le réseau de
l'opérateur, et le modem disparaît avec elle. La gamme **Gate** redevient
alors éligible.

D'où la scission en deux options : **B — Unipi Gate G100** dès qu'une prise
réseau existe, **C — E413 LTE** seulement sinon. L'option A (ESP32 à
105,60 €) reste la moins chère dans les deux cas.

### ⚠️ À vérifier — la limitation de courant des optocoupleurs

Comme pour l'architecture LoRa : 43 canaux de `PC847` demandent 43 résistances
de limitation dimensionnées pour la tension réelle des lignes (§12.1). À sortir
du forfait « passifs » une fois la mesure faite.

### ⚠️ Deux antennes cellulaires, deux abonnements

L'AGV **et** le poste portent chacun un `SIM7080G` et une SIM. C'est ce qui
double le coût récurrent. Si le poste dispose d'un raccordement Ethernet — ce
qui est le cas dès qu'il est dans un local technique — **son modem est inutile**
et il parle au broker par le réseau filaire : 39,60 € de matériel et la moitié
du récurrent en moins.

C'est probablement l'économie la plus simple de cette architecture, et elle n'a
aucune contrepartie technique.


---

## Mode d'emploi pour le service achats

1. Rechercher chaque `Réf. fabricant` sur [fr.rs-online.com](https://fr.rs-online.com).
2. Renseigner la référence RS (numéro de stock), le prix unitaire TTC et le total.
3. Si la référence est absente du catalogue RS : chercher sur Amazon, et
   **noter la source dans la colonne `Réf. catalogue`** — la traçabilité de
   l'origine compte autant que le prix.
4. Attention aux **conditionnements** : un IRF520 ou une résistance se vend
   souvent par 5, 10 ou 100. Le prix unitaire affiché peut correspondre à un
   lot entier.
5. Les lignes marquées `PCB` ne sont pas des articles de catalogue : elles
   demandent un devis chez un fabricant de circuits imprimés
   (JLCPCB, Eurocircuits, PCBWay) à partir des fichiers Gerber.
6. Reporter les sous-totaux dans le récapitulatif, puis **mettre à jour
   [`../../docs/COMPARAISON.md`](../../docs/COMPARAISON.md)** si les écarts changent le classement des architectures.

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

