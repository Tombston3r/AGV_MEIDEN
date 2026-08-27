# Relevé de prix à compléter

> Liste **dédoublonnée** des trois architectures : une référence commune à
> plusieurs solutions n'apparaît qu'une fois.
>
> Remplir la colonne **`PU TTC`**. Le reste se recalcule tout seul.
>
> La colonne **`Lien d'achat`** lance une **recherche** sur la référence chez le
> distributeur, RS en premier partout. Ce ne sont pas des fiches produit : le
> site de RS bloque l'accès automatisé, aucun numéro de stock n'a pu être
> vérifié. Un lien secondaire n'apparaît que là où RS ne distribue
> habituellement pas la référence.
>
> `Qté` = quantité **maximale** nécessaire, toutes architectures confondues.
> C'est ce qui compte pour choisir le bon conditionnement.

**Priorité** : les 12 lignes marquées ⭐ portent à elles seules ~70 % du budget
matériel. Si le temps manque, faire celles-là d'abord.

### ⚠️ Les accessoires n'y sont plus

Antennes, pigtails, boîtiers, coffrets, enveloppes murales et poussoirs de
façade ont été **retirés** de cette feuille comme des nomenclatures : ils se
substituent librement d'un fournisseur à l'autre, s'arbitrent en fin de projet
selon le budget, et n'engagent aucun choix de conception. Les chercher au
catalogue serait du temps perdu tant que le reste n'est pas figé.

### ⚠️ Ce total n'est pas un budget

Le total ci-dessous additionne **toutes les lignes à leur quantité maximale,
alternatives comprises** : les trois postes fixes UniPi, les trois étages de
sortie possibles, les deux variantes d'interface bus. Aucun projet ne les
achète tous.

C'est une **enveloppe de sourcing** : elle borne l'effort de recherche, pas la
dépense. Les coûts réels par architecture sont dans les trois `BOM.md`, où les
options s'excluent proprement.

| | HT |
|---|---:|
| **Enveloppe de sourcing** : 42 lignes, quantités maximales | **~1 630 €** |

---

## 1. Modules et microcontrôleurs

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| ⭐ | `Unipi Gate G100` | Passerelle Linux DIN, 2× Eth, USB, RS485 | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Unipi+Gate+G100) · [Unipi](https://www.unipi.technology/search?query=Unipi+Gate+G100) | *200,00 €* | |
| | `Unipi Gate G110` | Idem + 2ᵉ port RS485 isolé | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Unipi+Gate+G110) · [Unipi](https://www.unipi.technology/search?query=Unipi+Gate+G110) | *224,00 €* | |
| | `UniPi E413` **si le modèle existe encore** | Automate à E/S, pour comparaison | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=UniPi+E413) · [Unipi](https://www.unipi.technology/search?query=E413) | *375,00 €* | |
| ⭐ | `UniPi E413` **variante LTE** | Idem, avec modem LTE intégré | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=UniPi+E413) · [Unipi](https://www.unipi.technology/search?query=E413) | *350,00 €* | |
| ⭐ | `Mega2560 Pro` (clone, format compact) | Module MCU du projet KiCad | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Mega2560+Pro) · [Amazon](https://www.amazon.fr/s?k=Mega2560+Pro) | *18,00 €* | |
| ⭐ | `ESP32-DEVKITC-32D-F` | Module Wi-Fi/BT sur support | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-DEVKITC-32D-F) | *12,00 €* | |
| | `ESP32-WROOM-32E-N8` | Module MCU nu, 8 Mo flash | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ESP32-WROOM-32E-N8) | *5,00 €* | |
| | `STM32L071KBU6` | MCU ultra-basse conso (bouton LoRa) | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=STM32L071KBU6) | *3,50 €* | |

## 2. Radio

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| ⭐ | `PTM 210` EnOcean, EU 868 MHz | Émetteur auto-alimenté, sans pile | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PTM+210) · [Mouser](https://www.mouser.fr/c/?q=PTM+210) | *30,00 €* | |
| ⭐ | `TCM 515` EnOcean, EU 868 MHz | Récepteur UART ESP3 | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TCM+515) · [Mouser](https://www.mouser.fr/c/?q=TCM+515) | *28,00 €* | |
| ⭐ | `SIM7080G` (SIMCom) | Modem LTE-M / NB-IoT | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SIM7080G) · [Amazon](https://www.amazon.fr/s?k=SIM7080G) | *18,00 €* | |
| ⭐ | `RFM95W-868S2` (HopeRF) | Module LoRa SX1276 868 MHz | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RFM95W-868S2) · [Amazon](https://www.amazon.fr/s?k=RFM95W-868S2) | *10,00 €* | |

## 3. Étage de sortie et interface bus

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| ⭐ | `IRF520` (Vishay, TO-220) | MOSFET N canal : étage de sortie | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=IRF520) | *0,60 €* | |
| ⭐ | `PC847` (Sharp) | Optocoupleur quadruple | 11 | [RS](https://fr.rs-online.com/web/c/?searchTerm=PC847) | *0,60 €* | |
| | `SN74HC595N` (TI, DIP-16) | Registre à décalage, sortie | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SN74HC595N) | *0,50 €* | |
| | `SN74HC165N` (TI, DIP-16) | Registre à décalage, entrée | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SN74HC165N) | *0,50 €* | |
| | `MCP23017-E/SP` (Microchip) | Expandeur I²C 16 GPIO (variante) | 4 | [RS](https://fr.rs-online.com/web/c/?searchTerm=MCP23017-E%2FSP) | *2,50 €* | |
| | Résistance 1 kΩ THT 1 % (0411) | Grilles des MOSFET | 25 | [RS](https://fr.rs-online.com/web/c/?searchTerm=R%C3%A9sistance+1+k%CE%A9+THT+1+%25) | *0,05 €* | |

## 4. Alimentation

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| ⭐ | `TDN 5-2411WISM` (Traco Power) | DC/DC **isolé** 24 V → 5 V, 5 W | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TDN+5-2411WISM) | *25,00 €* | |
| ⭐ | `HDR-15-24` (MEAN WELL) | Alim. rail DIN 230 V → 24 V 15 W | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=HDR-15-24) | *14,00 €* | |
| | `TSR 1-2450` (Traco Power) | DC/DC 24 V → 5 V 1 A non isolé | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=TSR+1-2450) | *7,00 €* | |
| | `L7806CV` (ST) | Régulateur 6 V TO-220 | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=L7806CV) | *0,90 €* | |
| | `AP2112K-3.3TRG1` (Diodes Inc) | LDO 3,3 V 600 mA | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=AP2112K-3.3TRG1) | *0,60 €* | |
| | `220 µF tantale + 10 µF X7R` | Réservoir d'impulsion émission LoRa (bouton) | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=220+%C2%B5F+tantale+%2B+10+%C2%B5F+X7R) | *0,50 €* | |
| | `BAT54` | Schottky de protection pile (bouton) | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=BAT54) | *0,17 €* | |
| | `ULN2803A` | **Alternative** à 23× IRF520 : étage de sortie Wi-Fi | 3 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ULN2803A) | *1,20 €* | |
| | `IRL520N` | **Alternative** logic-level, même brochage que l'IRF520 | 23 | [RS](https://fr.rs-online.com/web/c/?searchTerm=IRL520N) | *1,10 €* | |
| | `SMBJ33A` (Littelfuse) | Diode TVS 33 V | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=SMBJ33A) | *0,50 €* | |
| | `ER14505` ou `LS14500` (Saft) | Pile Li-SOCl₂ 3,6 V AA + support | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=ER14505) | *6,00 €* | |

## 5. Connectique

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| | `DB25P564CTXLF` (Amphenol) | SUB-D 25 mâle, coudé CI | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=DB25P564CTXLF) | *6,00 €* | |
| | `DB25S564GTLF` (Amphenol) | SUB-D 25 femelle, coudé CI | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=DB25S564GTLF) | *6,00 €* | |
| | SUB-D 25 mâle **IDC** (nappe) | `Amphenol L17D25P` ou équiv. | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L17D25P) | *4,00 €* | |
| | SUB-D 25 femelle **IDC** (nappe) | `Amphenol L17D25S` ou équiv. | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+L17D25S) | *4,00 €* | |
| | Capot SUB-D 25 métallisé | `Amphenol 17E-1726-2` ou équiv. | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Amphenol+17E-1726-2) | *3,50 €* | |
| | Nappe 25 conducteurs, au mètre | `3M 3365/25` ou équiv. | 2 m | [RS](https://fr.rs-online.com/web/c/?searchTerm=3M+3365%2F25) | *6,00 €* | |
| | `WIZ850io` (WIZnet) | Module Ethernet W5500 + RJ45 | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=WIZ850io) | *6,00 €* | |
| | ILS (reed) + aimant | `Standex KSK-1A66` ou équiv. | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Standex+KSK-1A66) | *2,00 €* | |

## 6. Outillage (achat unique)

| ⭐ | Réf. à rechercher | Désignation | Qté | Lien d'achat | *Repère HT* | **PU TTC** |
|:-:|---|---|---:|---|---:|---|
| | Pince à sertir + jeu de cosses | Knipex ou `Engineer PA-09` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Engineer+PA-09) | *45,00 €* | |
| | `RTL-SDR Blog V4` + antenne | Mesure d'occupation 868 MHz | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=RTL-SDR+Blog+V4) · [Amazon](https://www.amazon.fr/s?k=RTL-SDR+Blog+V4) | *30,00 €* | |
| | Analyseur logique 8 voies USB | clone Saleae 24 MHz | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=Analyseur+logique+8+voies+USB) · [Amazon](https://www.amazon.fr/s?k=Analyseur+logique+8+voies+USB) | *15,00 €* | |
| | Programmateur ISP AVR | `USBasp` ou `USBtinyISP` | 1 | [RS](https://fr.rs-online.com/web/c/?searchTerm=USBasp) · [Amazon](https://www.amazon.fr/s?k=USBasp) | *8,00 €* | |
| | Adaptateur USB-série 3,3 V | `FT232RL` ou `CP2102` | 2 | [RS](https://fr.rs-online.com/web/c/?searchTerm=FT232RL) · [Amazon](https://www.amazon.fr/s?k=FT232RL) | *6,00 €* | |

---

## Ce dont je n'ai PAS besoin

Ces lignes ne sont pas des articles de catalogue : inutile de les chercher :

| Ligne | Pourquoi |
|---|---|
| **Antennes** (LoRa, EnOcean, Wi-Fi, LTE) et pigtails | Accessoires arbitrables selon le budget : voir ci-dessus |
| **Boîtiers, coffrets rail DIN, enveloppes murales** | Idem |
| **Poussoir Ø22 de façade** | Idem |
| PCB 2 et 4 couches | Devis chez un fabricant (JLCPCB, Eurocircuits, PCBWay) à partir des Gerber |
| Plaques de repérage gravées | Sur mesure |
| Lots de passifs (résistances, condensateurs, LED) | Forfait, sans incidence sur l'arbitrage |
| Visserie, colliers, gaine, adhésif | Idem |

---

## Comment me renvoyer les prix

N'importe quel format lisible me convient :

- ce tableau avec la colonne `PU TTC` remplie ;
- ou en vrac, une ligne par référence : `IRF520 = 0,89 €` ;
- ou un export CSV / le contenu d'un panier RS.

**Ce qui m'aide le plus, en plus du prix** : le **numéro de stock RS** et le
**conditionnement** (à l'unité, par 5, par 100). Un prix de 0,12 € n'a pas le
même sens selon qu'il s'agit d'une pièce ou d'un sachet de 100.

Je recalcule ensuite les trois nomenclatures, le comparatif et les totaux sur
dix ans.
