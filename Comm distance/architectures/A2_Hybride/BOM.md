# Nomenclature A2 : Hybride EnOcean + LoRa

## 💰 Total

| | HT | TTC | *Accessoires écartés* |
|---|---:|---:|---:|
| **A2 : EnOcean + LoRa, 2 boutons sans pile** | **310,15 €** | **372,18 €** | *+ 117,50 € HT* |

Ces totaux ne comptent que les **composants déterminants**.

Les accessoires arbitrables selon le budget : antennes, boîtiers,
coffrets, enveloppes murales, câbles : sont volontairement **hors
nomenclature** : ils se substituent librement d'un fournisseur à l'autre
et ne changent rien à la conception. La dernière colonne rappelle ce
qu'ils pèsent, pour que ce total ne soit jamais pris pour un coût
d'achat complet.

⚠️ [`../../docs/COMPARAISON.md`](../../docs/COMPARAISON.md) et le `README.md` racine comparent les architectures
**accessoires compris**, sans quoi le classement serait faussé. Leurs
chiffres sont donc plus élevés que ceux-ci, et c'est normal.

## ⚠️ Feuille de sourcing, à compléter avant usage

Ce document est une **liste d'achat à remplir**, pas un devis. Les colonnes
`Réf. catalogue`, `PU TTC` et `Total TTC` sont vides : c'est le service achats
qui les renseigne depuis le catalogue.

| Colonne | Ce qu'elle contient |
|---|---|
| `Réf. fabricant` | **Référence exacte à rechercher** : c'est ce qui rend la ligne non ambiguë |
| `Lien d'achat` | **Recherche pré-remplie chez le distributeur**, RS en premier chaque fois que c'est plausible. Un second lien n'apparaît que lorsque RS ne distribue probablement pas la référence |
| `Réf. catalogue` / `PU TTC` / `Total TTC` | **À remplir** |
| *`Repère TTC`* | Estimation de départ, **en italique** : voir l'avertissement ci-dessous |

### ⚠️ Ce sont des liens de RECHERCHE, pas des fiches produit

Chaque lien lance une **recherche sur la référence fabricant** chez le
distributeur. Aucun numéro de stock n'a été vérifié : le site de RS bloque
l'accès automatisé, il n'était donc pas possible de confirmer qu'une fiche
produit existe ni à quel prix. Inventer des numéros de stock aurait produit des
liens crédibles menant à la mauvaise pièce : le pire résultat possible sur une
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
référencé : disponibilité, garantie, facture, fiches techniques.

### Note pour l'arbitrage

Les prix sont demandés en TTC. Pour une entreprise, **la TVA est récupérable** :
la comparaison entre architectures reste donc pertinente en HT, et
[`../../docs/COMPARAISON.md`](../../docs/COMPARAISON.md) raisonne en HT. Diviser un total TTC par 1,20 donne le HT.

---

### Carte AGV `AIO_AGV_Control_V6.0`

**Nomenclature réelle**, extraite de
[`../../materiel/AIO_AGV_Control_V6.0/`](../../materiel/AIO_AGV_Control_V6.0/) :
58 composants placés au PCB.

C'est la **V5.0.1 au composant près, plus un `RFM95W-868S2`** : le
diff des deux projets KiCad ne montre aucun autre écart. La radio est
**intégrée à la carte**, câblée sur le SPI libre de l'ESP32 : il n'y a
ni carte fille ni câblage volant.

⚠️ Cette carte est **fabriquée**. La V5.0.1 d'origine reste intacte sur
le chariot : c'est le retour arrière de cette architecture.

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU : carte Mega2560 Pro sur support | `Clone Mega2560 Pro (A1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Clone+Mega2560+Pro) · [Amazon](https://www.amazon.fr/s?k=Clone+Mega2560+Pro) | ☐ | ☐ | ☐ | *21,60 €* |
| Module Wi-Fi/BT sur support | `ESP32-DEVKITC-32D-F (U1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-DEVKITC-32D-F) | ☐ | ☐ | ☐ | *14,40 €* |
| **Étage de sortie** : MOSFET N canal TO-220 | `IRF520 (Vishay, T1–T24)` | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=IRF520) | ☐ | ☐ | ☐ | *16,56 €* |
| Résistances de grille des MOSFET | `1 kΩ THT 0411 (R1–R24)` | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=1+k%CE%A9+THT+0411) | ☐ | ☐ | ☐ | *1,38 €* |
| Diviseurs de mesure | `4,7 k / 2,2 k / 22 k / 220 k (R30, R31, R40, R41)` | 4 | [RS](https://fr.rs-online.com/web/c/?searchTerm=4%2C7+k+%2F+2%2C2+k+%2F+22+k+%2F+220+k) | ☐ | ☐ | ☐ | *0,24 €* |
| Régulateur 6 V : alimentation de l'ATmega | `L7806CV (LM1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=L7806CV) | ☐ | ☐ | ☐ | *1,08 €* |
| Convertisseur DC/DC **isolé** 24 V → 5 V, 5 W | `TDN 5-2411WISM (Traco, TDN1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TDN+5-2411WISM) | ☐ | ☐ | ☐ | *30,00 €* |
| Diode de protection DO-41 | `1N4007 ou équiv. (D1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=1N4007) | ☐ | ☐ | ☐ | *0,12 €* |
| Connecteur SUB-D 25 **mâle** coudé CI (entrées) | `Amphenol DB25P564CTXLF (J1)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+DB25P564CTXLF) | ☐ | ☐ | ☐ | *7,20 €* |
| Connecteur SUB-D 25 **femelle** coudé CI (sorties) | `Amphenol DB25S564GTLF (J2)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+DB25S564GTLF) | ☐ | ☐ | ☐ | *7,20 €* |
| Supports et barrettes pour les deux modules | `barrettes tulipe 2,54 mm` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=barrettes+tulipe+2%2C54+mm) | ☐ | ☐ | ☐ | *3,60 €* |
| PCB ~150 × 100 mm (série de 5) | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *18,00 €* |
| **Module LoRa SX1276 868 MHz** : l'unique écart avec la V5.0.1 | `RFM95W-868S2 (HopeRF, U2)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | ☐ | ☐ | ☐ | *12,00 €* |
| **Sous-total** | | | | | | **☐** | ***133,38 €*** |

### **[A2]** Poste fixe EnOcean → LoRa

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module MCU, 8 Mo flash (LittleFS + pages web) | `ESP32-WROOM-32E-N8` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | ☐ | ☐ | ☐ | *6,00 €* |
| Récepteur EnOcean 868 MHz, UART ESP3 | `TCM 515 (EnOcean)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=TCM+515) | ☐ | ☐ | ☐ | *33,60 €* |
| Module LoRa SX1276 | `RFM95W-868S2 (HopeRF)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | ☐ | ☐ | ☐ | *12,00 €* |
| Contrôleur Ethernet SPI + RJ45 magnétique | `WIZnet WIZ850io (W5500)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=WIZnet+WIZ850io) | ☐ | ☐ | ☐ | *7,20 €* |
| Bouton d'appairage + bouton reset | `Omron B3F-1000` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Omron+B3F-1000) | ☐ | ☐ | ☐ | *2,40 €* |
| Alimentation rail DIN 230 V → 24 V 15 W | `MEAN WELL HDR-15-24` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MEAN+WELL+HDR-15-24) | ☐ | ☐ | ☐ | *16,80 €* |
| 24 V → 5 V → 3,3 V | `TSR 1-2450 + AP2112K-3.3` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450+%2B+AP2112K-3.3) | ☐ | ☐ | ☐ | *9,60 €* |
| PCB 2 couches ~100 × 80 mm | `Gerber projet` | 1 | [JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx) | ☐ | ☐ | ☐ | *7,20 €* |
| **Sous-total** | | | | | | **☐** | ***94,80 €*** |

### **[A2]** Bouton EnOcean sans pile : l'unité

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Module émetteur auto-alimenté, **sans pile** | `PTM 210 (EnOcean, EU 868)` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PTM+210) · [Mouser](https://www.mouser.fr/c/?q=PTM+210) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords=PTM+210) | ☐ | ☐ | ☐ | *36,00 €* |
| **Sous-total** | | | | | | **☐** | ***36,00 €*** |

### Outillage : non récurrent

| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |
|---|---|---:|---|---|---:|---:|---:|
| Dongle RTL-SDR + antenne : occupation de la bande 868 MHz | `RTL-SDR Blog V4` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RTL-SDR+Blog+V4) · [Amazon](https://www.amazon.fr/s?k=RTL-SDR+Blog+V4) | ☐ | ☐ | ☐ | *36,00 €* |
| Analyseur logique 8 voies : chronogrammes X/Y | `clone Saleae 24 MHz` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=clone+Saleae+24+MHz) · [Amazon](https://www.amazon.fr/s?k=clone+Saleae+24+MHz) | ☐ | ☐ | ☐ | *18,00 €* |
| Adaptateur USB-série 3,3 V | `FTDI FT232RL ou CP2102` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FTDI+FT232RL+ou+CP2102) · [Amazon](https://www.amazon.fr/s?k=FTDI+FT232RL+ou+CP2102) | ☐ | ☐ | ☐ | *7,20 €* |
| Mesure de courant µA : sommeil profond **[A3]** | `multimètre à faible burden voltage` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=multim%C3%A8tre+%C3%A0+faible+burden+voltage) · [Amazon](https://www.amazon.fr/s?k=multim%C3%A8tre+%C3%A0+faible+burden+voltage) | ☐ | ☐ | ☐ | *10,80 €* |
| **Sous-total** | | | | | | **☐** | ***72,00 €*** |

---

## Récapitulatif : variante A2 (EnOcean + LoRa)

| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |
|---|---:|---:|---:|
| Carte AGV `V6.0` (nomenclature KiCad) | ☐ | *133,38 €* | *111,15 €* |
| Poste fixe EnOcean → LoRa | ☐ | *94,80 €* | *79,00 €* |
| 2 boutons PTM 210 | ☐ | *72,00 €* | *60,00 €* |
| Outillage | ☐ | *72,00 €* | *60,00 €* |
| **TOTAL** | **☐** | ***372,18 €*** | ***310,15 €*** |

⚠️ La version **EU 868 MHz** du `PTM 210` est impérative : les déclinaisons
902 et 928 MHz ne sont pas utilisables en France, et rien dans la désignation
courante ne les distingue au premier coup d'œil.

### La V6.0, c'est la V5.0.1 plus une radio

Le diff des deux projets KiCad est sans ambiguïté : **58 empreintes contre 57,
un seul écart, le `RFM95W-868S2`.** Tout le reste : `Mega2560 Pro`,
`ESP32-DEVKITC`, les 23 `IRF520` et leurs résistances de grille, le `L7806CV`,
le `TDN 5-2411WISM`, les deux SUB-D 25 : est strictement identique.

La radio est **intégrée à la carte**, câblée sur le SPI que l'ESP32 laissait
libre. Le relevé confirme le brochage :

| RFM95W | ESP32 |
|---|---|
| `NSS` | `IO5` |
| `SCK` | `IO18` |
| `MISO` | `IO19` |
| `MOSI` | `IO23` |
| `DIO0` | `IO26` |

⚠️ **`RESET` n'est pas câblée.** Un module figé ne se récupérera qu'en coupant
l'alimentation de la carte : aucun reset logiciel n'est possible. Une GPIO libre
suffirait à corriger cela sur une V6.1 : il en reste largement.

⚠️ **`IO16` et `IO17` sont interdites** : elles portent la liaison série vers le
`Mega2560 Pro`.

### Ce que cela change par rapport aux versions précédentes de ce document

Les nomenclatures antérieures chiffraient la carte LoRa comme une **V5.0.1
réutilisée** à 0 €, augmentée d'une carte fille portant le `RFM95W`. C'était une
hypothèse de travail ; la V6.0 la remplace, et **elle est plus chère** :

| | Hypothèse précédente | **V6.0 réelle** |
|---|---:|---:|
| Carte AGV | 18,00 € HT *(greffe)* | **111,15 € HT** *(fabriquée)* |

La carte est à produire, comme celle de l'architecture A4. En contrepartie il
n'y a **ni carte fille, ni câblage volant dans un chariot qui vibre** : ce qui,
sur un équipement destiné à durer, vaut largement l'écart.

La V5.0.1 d'origine **reste intacte sur le chariot** : c'est le retour arrière
de cette architecture.

### La liaison entre les deux microcontrôleurs n'est pas un UART

Le relevé KiCad corrige une hypothèse qui était fausse, et le firmware a été
corrigé en conséquence.

```
MEGA D53 (PB0) ──[2,2 k]──┬── ESP32 IO16 (U2RXD)     5 V ramenés à 3,4 V
                          │
                       [4,7 k]
                          │
                         GND

ESP32 IO17 (U2TXD) ───────── MEGA D52 (PB1)          3,3 V lus en logique 5 V
```

Côté ESP32 c'est bien `UART2`. **Côté MEGA, D52 et D53 ne sont pas des broches
d'UART matériel**, et les trois UART du MEGA sont inutilisables, car leurs
broches de réception portent des signaux du bus : `D19`/`PD2` = `Y13`,
`D17`/`PH0` = `Y11`, `D15`/`PJ0` = **`Y05`**, le drapeau de déplacement.

Un `Serial1.begin()` aurait mis `Y13` en sortie **contre la sortie de
l'automate**. Le firmware passe donc en `SoftwareSerial` sur D52/D53, à
**38 400 bauds** : 115 200 n'est pas tenable en émulation logicielle sur AVR.

### Pourquoi pas un Unipi Gate pour ce poste ?

La question se pose puisque le poste de l'architecture Wi-Fi a été ramené à une
passerelle Unipi Gate G100. **Ici, non, et pour une raison de fond, pas de
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
| Gate + carte ESP32 en frontal radio | ~334,80 € | On paie les deux |

**Le poste LoRa n'a d'ailleurs pas besoin de Linux.** Son travail est une
traduction de protocole : EnOcean entre, LoRa sort. Il n'héberge pas de broker,
et l'AGV lui parle directement.

L'asymétrie avec l'architecture Wi-Fi est donc logique :

| | Poste Wi-Fi | Poste LoRa |
|---|---|---|
| Doit héberger un broker MQTT | **oui** | non |
| Doit piloter une radio au niveau PHY | non (le réseau est Ethernet | **oui**) SX1276 sur SPI |
| Matériel qui en découle | boîtier Linux industriel | microcontrôleur avec SPI |
| Retenu | Unipi Gate G100 (~240,00 € TTC) | carte ESP32 (~94,80 € TTC) |

⚠️ **Deux antennes 868 MHz sur le même boîtier** : EnOcean et LoRa. Les espacer
d'au moins 20 cm, ou en déporter une. Une désensibilisation du récepteur EnOcean
par l'émetteur LoRa se traduirait par des appuis perdus, silencieusement.

### Où se croisent les deux courbes

| Stations | A3 (TTC) | A2 (TTC) | Moins cher |
|---:|---:|---:|---|
| 2 | 350,10 € | 468,78 € | **A3** |
| 4 | 494,82 € | 579,18 € | **A3** |
| 6 | 639,54 € | 689,58 € | **A3** |
| 8 | 784,26 € | 799,98 € | **A3** |
| 12 | 1 073,70 € | 1 020,78 € | **A2** |

Le point de bascule est à **9 stations**. En dessous, A3 coûte moins
**et** rend un accusé visuel à l'opérateur. Au-delà, A2 prend l'avantage grâce
à des boutons à 55,20 € au lieu de
72,36 €, et supprime les piles.

⚠️ Ce tableau raisonne **accessoires compris** : boîtier IP65 et poussoir Ø22
du bouton A3, enveloppe murale du bouton A2. C'est une comparaison économique,
pas une liste d'achat : les retirer inverserait artificiellement le classement,
puisque c'est précisément l'enveloppe du bouton A3 qui le rend cher.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| **[A3]** bouton sur pile | **27,96 €** | 23,30 € |
| **[A2]** bouton PTM 210 | **36,00 €** | 30,00 € |

### Coûts récurrents

| Poste | Annuel |
|---|---:|
| Abonnement opérateur | **0 €** : bande ISM libre |
| Infrastructure | **0 €** : aucune |
| **[A3]** Remplacement des piles | ~7,20 € par bouton tous les 5 à 8 ans |
| **[A2]** Piles | **0 €** : PTM 210 auto-alimentés |

**C'est l'architecture la moins chère des trois sur dix ans.**

---

## Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| PCB 4 couches + assemblage | 3 à 5 semaines | **Chemin critique matériel** |
| **[A3]** PCB bouton + boîtiers IP65 | 3 à 5 semaines | En parallèle de la carte AGV |
| `RFM95W-868S2` | 1 à 3 semaines | **Contrefaçons fréquentes** : acheter chez un distributeur référencé, pas sur une place de marché |
| `PTM 210` / `TCM 515` | 1 à 2 semaines | Peu distribués par RS : prévoir un distributeur EnOcean |
| `ER14505` Li-SOCl₂ | 1 à 2 semaines | **Restrictions de transport aérien** sur le lithium |
| `ESP32-DEVKITC`, `Mega2560 Pro`, `IRF520` | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : le relevé de
couverture radio et l'arbitrage du facteur d'étalement peuvent changer
l'antenne, la puissance d'émission et le nombre de nœuds.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 5 à 8 jours-homme après la phase 0 qui rend ce dossier
  autonome, hors développement.
- **La conformité RED** : si les cartes sont produites en série et mises sur le
  marché, dossier technique, marquage CE et déclaration UE de conformité à
  conserver 10 ans. Compter 3 à 5 jours-homme. Sans objet en usage interne.
- **La carte de rechange** : ~73,44 € pour un échange standard. Recommandé.
- **Un éventuel relais LoRa** si le relevé révèle une zone morte : ~48,00 €,
  mais surtout une complexité applicative hors périmètre actuel.
---

## Analyse critique de cette nomenclature

### ✅ Corrigé : le convertisseur du bouton était inutile

La nomenclature d'étude prévoyait un `TPS62740` (buck ultra-basse consommation)
entre la pile et l'électronique. Il n'a pas lieu d'être : une pile Li-SOCl₂
délivre **3,6 V**, et les deux consommateurs l'acceptent directement :
`STM32L071` de 1,65 à 3,6 V, `RFM95W` de 1,8 à 3,7 V. Le convertisseur ajoutait
un composant, un courant de repos et un mode de panne, pour rien.

Le vrai besoin est ailleurs : une cellule Li-SOCl₂ a une **impédance interne
élevée**, et l'émission LoRa tire ~120 mA pendant quelques centaines de
millisecondes. Sans réservoir, la tension s'effondre et le microcontrôleur
redémarre : panne classique, et intermittente, donc pénible à diagnostiquer.

`TPS62740` remplacé par **220 µF tantale + 10 µF X7R** : 1,68 € d'économie, un
composant de moins, et le problème réellement traité.

Une diode Schottky est ajoutée en protection de la pile (0,24 €) contre une
inversion au remplacement.

### ⚠️ L'`IRF520` n'est pas un MOSFET « logic-level »

La V6.0 hérite de l'étage de sortie de la V5.0.1 : **23 `IRF520`**, dont la
tension de seuil est spécifiée de 2 à 4 V et le `Rds(on)` garanti à Vgs = 10 V.
Attaqué par une broche à 5 V, il conduit, mais hors des conditions du
constructeur.

Pour quelques milliampères sur une entrée d'automate, cela fonctionne. Ce n'est
pas un défaut bloquant, c'est un choix hors spécification qu'il faut connaître
avant de l'attribuer à autre chose le jour où une voie se comporte mal en
température. L'`IRL520` est la version logic-level du même composant, **au même
brochage** : une substitution sans reroutage.

### ⚠️ À vérifier : l'autonomie annoncée

5 à 8 ans sur une `ER14505` de 2,6 Ah suppose un sommeil profond sous 2 µA et
quelques appuis par jour. **À mesurer au banc** (phase 7 de `DEPLOY.md`) : un
courant de repos de 20 µA au lieu de 2 divise l'autonomie par cinq, et
transforme une maintenance décennale en corvée annuelle.

### ✅ Confirmé : les 43 lignes sur les broches de l'ATmega

La V6.0 reprend la topologie de la V5.0.1 : le `Mega2560 Pro` porte les 43
lignes sur ses propres broches, sans expandeur ni registre à décalage. Le
relevé de câblage montre que les 22 sorties occupent 5 ports, soit **5
écritures ≈ 0,3 µs** en section critique : 500 fois plus rapide qu'un expandeur
I²C.

Le driver `avr_port_bus.cpp` est déjà écrit et testé, et le relevé complet vit
dans `firmware/mega/src/board_ports.h`. Il n'y a rien à concevoir de ce côté.


---

## Mode d'emploi pour le service achats

1. Rechercher chaque `Réf. fabricant` sur [fr.rs-online.com](https://fr.rs-online.com).
2. Renseigner la référence RS (numéro de stock), le prix unitaire TTC et le total.
3. Si la référence est absente du catalogue RS : chercher sur Amazon, et
   **noter la source dans la colonne `Réf. catalogue`** : la traçabilité de
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
| `IRF520` | `IRL520N` : version **logic-level**, brochage identique |
| `RFM95W-868S2` | Tout module SX1276 868 MHz au même brochage |
| `TSR 1-2450` | `OKI-78SR-5/1.5-W36-C` (Murata), même brochage |
| `AP2112K-3.3TRG1` | `MCP1700T-3302E`, `XC6206P332MR` |
| `ER14505` | `LS14500` (Saft), `SL-360` (Tadiran) : Li-SOCl₂ 3,6 V AA |
| `TCM 515` | `TCM 310` si l'accusé opérateur n'est pas retenu |

