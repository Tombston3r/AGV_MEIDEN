# -*- coding: utf-8 -*-
"""Génère les trois feuilles de sourcing (BOM.md) des architectures.

Les prix vivent ICI, en un seul endroit, et les totaux sont CALCULÉS — jamais
recopiés à la main. C'est ce qui garantit qu'un récapitulatif ne peut pas
diverger de son propre détail, comme c'était arrivé sur l'outillage LoRa.

    python3 CarteComm/tools/generer_bom.py

Régénère les trois BOM.md. Après une mise à jour des prix, penser à reporter les
totaux dans CarteComm/COMPARAISON.md et CarteComm/README.md.
"""
from urllib.parse import quote_plus

TVA = 1.20

# Références non recherchables : lots de passifs, fichiers Gerber, assemblages.
NON_RECHERCHABLE = ("lot", "Gerber", "projet", "équiv. selon")

def _terme(ref):
    """Nettoie une référence fabricant pour en faire un terme de recherche.

    Le nom du fabricant entre parenthèses et les « ou équiv. » font chuter le
    nombre de résultats à zéro chez RS : on les retire.
    """
    t = ref.split(" ou équiv")[0].split(" (")[0].strip()
    return t.replace("`", "")

def lien(ref, source):
    """Lien d'achat, RS en tête chaque fois que c'est plausible.

    Ce sont des liens de RECHERCHE sur la référence fabricant, pas des fiches
    produit : RS bloque l'accès automatisé, aucun numéro de stock n'a donc pu
    être vérifié. C'est au service achats de retenir la fiche et de reporter la
    référence catalogue dans la colonne prévue.
    """
    pcb = "[JLCPCB](https://jlcpcb.com/quote) · [PCBWay](https://www.pcbway.com/orderonline.aspx)"
    if any(m in ref for m in NON_RECHERCHABLE):
        return pcb if source == "PCB" else "—"
    q = quote_plus(_terme(ref))
    rs = f"[RS](https://fr.rs-online.com/web/c/?searchTerm={q})"
    if source == "RS":
        return rs
    if source == "PCB":
        return pcb
    if source == "Amazon":
        return f"{rs} · [Amazon](https://www.amazon.fr/s?k={q})"
    # Spécialiste : EnOcean et Unipi ne sont pas des lignes RS courantes.
    if "unipi" in ref.lower():
        return f"{rs} · [Unipi](https://www.unipi.technology/search?query={q})"
    return f"{rs} · [Mouser](https://www.mouser.fr/c/?q={q}) · [Digi-Key](https://www.digikey.fr/fr/products/result?keywords={q})"


def eur(x):
    return f"{x:,.2f} €".replace(",", " ").replace(".", ",")

class Section:
    def __init__(self, title, note=None):
        self.title, self.note, self.lines = title, note, []
    def add(self, desig, ref, qty, source, ht):
        self.lines.append((desig, ref, qty, source, ht))
        return self
    @property
    def ht(self):
        return sum(q * p for _, _, q, _, p in self.lines)
    @property
    def ttc(self):
        return self.ht * TVA
    def md(self, level="###"):
        out = [f"{level} {self.title}"]
        if self.note:
            out += ["", self.note]
        out += ["",
                "| Désignation | Réf. fabricant | Qté | Lien d'achat | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |",
                "|---|---|---:|---|---|---:|---:|---:|"]
        for desig, ref, qty, source, ht in self.lines:
            rep = eur(qty * ht * TVA)
            out.append(f"| {desig} | `{ref}` | {qty} | {lien(ref, source)} | ☐ | ☐ | ☐ | *{rep}* |")
        out.append(f"| **Sous-total** | | | | | | **☐** | ***{eur(self.ttc)}*** |")
        return "\n".join(out) + "\n"

HEADER = """# Nomenclature — {titre}

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
[`{cmp}`]({cmp}) raisonne en HT. Diviser un total TTC par 1,20 donne le HT.

---
"""

TAIL_SOURCING = """
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
   [`{cmp}`]({cmp})** si les écarts changent le classement des architectures.

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
"""

# ===========================================================================
#  LoRa — A1 (homogène) et A3 (hybride EnOcean)
# ===========================================================================
carte = Section("Carte AGV — commune à A1 et A3",
                "Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte** :\nc'est le retour arrière de cette architecture.")
carte.add("Module MCU Wi-Fi/BT, 8 Mo flash", "ESP32-WROOM-32E-N8", 1, "RS", 5.00)
carte.add("Module LoRa SX1276 868 MHz", "RFM95W-868S2 (HopeRF)", 1, "Amazon", 10.00)
carte.add("Pigtail U.FL → SMA femelle + passe-cloison", "Amphenol 336312-24-0100", 1, "RS", 3.00)
carte.add("Antenne 868 MHz 1/4 onde 2 dBi, embase SMA", "Siretta ALPHA-1A ou équiv.", 1, "RS", 6.00)
carte.add("Optocoupleur quadruple — 43 voies", "PC847 (Sharp)", 11, "RS", 0.60)
carte.add("Convertisseur DC/DC 24 V → 5 V 1 A", "TSR 1-2450 (Traco Power)", 1, "RS", 7.00)
carte.add("LDO 3,3 V 600 mA", "AP2112K-3.3TRG1 (Diodes)", 1, "RS", 0.60)
carte.add("Diode TVS protection 24 V", "SMBJ33A (Littelfuse)", 2, "RS", 0.50)
carte.add("Résistances 1 %, découplages, LED d'état", "lot", 1, "RS", 8.00)
carte.add("ILS (reed) + aimant — Wi-Fi de maintenance", "Standex KSK-1A66 ou équiv.", 1, "RS", 2.00)
carte.add("SUB-D 25 mâle et femelle, coudés CI", "Amphenol L717SDB25xA4CH4F", 2, "RS", 3.00)
carte.add("PCB 4 couches ~120 × 100 mm (série de 5)", "Gerber projet", 1, "PCB", 12.00)
carte.add("Boîtier, fixation, presse-étoupes, conn. de prog.", "Hammond 1590 ou Fibox", 1, "RS", 28.00)

bus595 = Section("Interface bus — variante `shift595` (recommandée)")
bus595.add("Registre à décalage sortie 8 bits", "SN74HC595N (TI)", 3, "RS", 0.50)
bus595.add("Registre à décalage entrée 8 bits", "SN74HC165N (TI)", 3, "RS", 0.50)

busmcp = Section("Interface bus — variante `mcp23017` (alternative)")
busmcp.add("Expandeur I²C 16 GPIO", "MCP23017-E/SP (Microchip)", 4, "RS", 2.50)

bouton_a1 = Section("**[A1]** Bouton d'appel sur pile — l'unité")
bouton_a1.add("MCU ultra-basse consommation", "STM32L071KBU6 (ST)", 1, "RS", 3.50)
bouton_a1.add("Module LoRa 868 MHz", "RFM95W-868S2 (HopeRF)", 1, "Amazon", 10.00)
bouton_a1.add("Antenne 868 MHz + embase SMA", "Siretta ALPHA-1A ou équiv.", 1, "RS", 6.00)
bouton_a1.add("Bouton poussoir Ø22 IP65", "Schneider XB4BA31 ou équiv.", 1, "RS", 12.00)
bouton_a1.add("Pile Li-SOCl₂ 3,6 V 2,6 Ah + support", "ER14505 / Saft LS14500", 1, "RS", 6.00)
bouton_a1.add("Réservoir d'impulsion pour l'émission LoRa", "220 µF tantale + 10 µF X7R", 1, "RS", 0.60)
bouton_a1.add("LED bicolore verte/rouge + résistances", "Kingbright L-59EGW", 1, "RS", 1.00)
bouton_a1.add("Diode Schottky de protection pile", "BAT54 ou équiv.", 1, "RS", 0.20)
bouton_a1.add("PCB 2 couches ~50 × 50 mm", "Gerber projet", 1, "PCB", 3.00)
bouton_a1.add("Boîtier IP65, presse-étoupe, embase antenne", "Fibox PC 095808 ou équiv.", 1, "RS", 18.00)

poste_a3 = Section("**[A3]** Poste fixe EnOcean → LoRa")
poste_a3.add("Module MCU, 8 Mo flash (LittleFS + pages web)", "ESP32-WROOM-32E-N8", 1, "RS", 5.00)
poste_a3.add("Récepteur EnOcean 868 MHz, UART ESP3", "TCM 515 (EnOcean)", 1, "Spécialiste", 28.00)
poste_a3.add("Antenne EnOcean 868 MHz déportée", "EnOcean ANT300 ou équiv.", 1, "Spécialiste", 8.00)
poste_a3.add("Module LoRa SX1276", "RFM95W-868S2 (HopeRF)", 1, "Amazon", 10.00)
poste_a3.add("Pigtail U.FL → SMA + antenne LoRa 2 dBi", "Amphenol + Siretta", 1, "RS", 9.00)
poste_a3.add("Contrôleur Ethernet SPI + RJ45 magnétique", "WIZnet WIZ850io (W5500)", 1, "RS", 6.00)
poste_a3.add("LED d'accusé bicolore, LED de vie, résistances", "lot", 1, "RS", 1.50)
poste_a3.add("Bouton d'appairage + bouton reset", "Omron B3F-1000", 2, "RS", 1.00)
poste_a3.add("Alimentation rail DIN 230 V → 24 V 15 W", "MEAN WELL HDR-15-24", 1, "RS", 14.00)
poste_a3.add("24 V → 5 V → 3,3 V", "TSR 1-2450 + AP2112K-3.3", 1, "RS", 8.00)
poste_a3.add("PCB 2 couches ~100 × 80 mm", "Gerber projet", 1, "PCB", 6.00)
poste_a3.add("Boîtier mural IP54, presse-étoupes, embases SMA", "Fibox ou Hammond 1554", 1, "RS", 30.00)

bouton_a3 = Section("**[A3]** Bouton EnOcean sans pile — l'unité")
bouton_a3.add("Module émetteur auto-alimenté, **sans pile**", "PTM 210 (EnOcean, EU 868)", 1, "Spécialiste", 30.00)
bouton_a3.add("Enveloppe / poussoir mural compatible PTM 210", "Eltako, NodOn ou Trio2Sys", 1, "Spécialiste", 12.00)
bouton_a3.add("Plaque de repérage station gravée", "sur mesure", 1, "Amazon", 4.00)

outil_lora = Section("Outillage — non récurrent")
outil_lora.add("Dongle RTL-SDR + antenne — occupation de la bande 868 MHz", "RTL-SDR Blog V4", 1, "Amazon", 30.00)
outil_lora.add("Analyseur logique 8 voies — chronogrammes X/Y", "clone Saleae 24 MHz", 1, "Amazon", 15.00)
outil_lora.add("Adaptateur USB-série 3,3 V", "FTDI FT232RL ou CP2102", 1, "Amazon", 6.00)
outil_lora.add("Mesure de courant µA — sommeil profond **[A1]**", "multimètre à faible burden voltage", 1, "Amazon", 9.00)

LORA_SECTIONS = [carte, bus595, busmcp, bouton_a1, poste_a3, bouton_a3, outil_lora]

# ===========================================================================
#  Wi-Fi — carte V5.0.1 conservée
# ===========================================================================
w_carte = Section("Carte AGV — extraite du projet KiCad",
                  "**Nomenclature réelle**, extraite de\n"
                  "[`hardware/AIO_AGV_Control_V5.0.1/`](hardware/AIO_AGV_Control_V5.0.1/) :\n"
                  "57 composants placés au PCB. Ce n'est plus une estimation d'étude.\n\n"
                  "⚠️ Cette carte est **fabriquée**, pas réutilisée : elle reprend le couple\n"
                  "ATmega2560 + ESP32 de l'originale, sur supports, avec son propre étage de\n"
                  "sortie. La ligne « carte AGV à 0 € » des versions précédentes de ce\n"
                  "document était donc fausse.")
w_carte.add("Module MCU — carte Mega2560 Pro sur support", "Clone Mega2560 Pro (A1)", 1, "Amazon", 18.00)
w_carte.add("Module Wi-Fi/BT sur support", "ESP32-DEVKITC-32D-F (U1)", 1, "RS", 12.00)
w_carte.add("**Étage de sortie** — MOSFET N canal TO-220", "IRF520 (Vishay, T1–T24)", 23, "RS", 0.60)
w_carte.add("Résistances de grille des MOSFET", "1 kΩ THT 0411 (R1–R24)", 23, "RS", 0.05)
w_carte.add("Diviseurs de mesure", "4,7 k / 2,2 k / 22 k / 220 k (R30, R31, R40, R41)", 4, "RS", 0.05)
w_carte.add("Régulateur 6 V — alimentation de l'ATmega", "L7806CV (LM1)", 1, "RS", 0.90)
w_carte.add("Convertisseur DC/DC **isolé** 24 V → 5 V, 5 W", "TDN 5-2411WISM (Traco, TDN1)", 1, "RS", 25.00)
w_carte.add("Diode de protection DO-41", "1N4007 ou équiv. (D1)", 1, "RS", 0.10)
w_carte.add("Connecteur SUB-D 25 **mâle** coudé CI (entrées)", "Amphenol DB25P564CTXLF (J1)", 1, "RS", 6.00)
w_carte.add("Connecteur SUB-D 25 **femelle** coudé CI (sorties)", "Amphenol DB25S564GTLF (J2)", 1, "RS", 6.00)
w_carte.add("Supports et barrettes pour les deux modules", "barrettes tulipe 2,54 mm", 1, "RS", 3.00)
w_carte.add("PCB ~150 × 100 mm (série de 5)", "Gerber projet", 1, "PCB", 15.00)
w_carte.add("Boîtier, entretoises, presse-étoupes, visserie", "Hammond 1590 ou Fibox", 1, "RS", 28.00)

w_harnais = Section("Harnais de raccordement",
                    "La carte existe, mais son câblage vers l'automate est à refaire.\nDétail : [`docs/subd25_atmega.md`](docs/subd25_atmega.md).")
w_harnais.add("Nappe 25 conducteurs, gaine souple, ~1 m", "3M 3365/25 ou équiv.", 2, "RS", 6.00)
w_harnais.add("Connecteur IDC SUB-D 25 **mâle** (entrées)", "Amphenol L17D25P", 1, "RS", 4.00)
w_harnais.add("Connecteur IDC SUB-D 25 **femelle** (sorties)", "Amphenol L17D25S", 1, "RS", 4.00)
w_harnais.add("Capot métallisé SUB-D 25 avec serre-câble", "Amphenol 17E-1726-2", 2, "RS", 3.50)
w_harnais.add("Cosses à sertir côté AGV (CN61 à CN64)", "selon bornier automate", 50, "RS", 0.15)
w_harnais.add("Gaine tressée, colliers, repérage des fils", "lot", 1, "RS", 7.50)

w_antenne = Section("Antenne Wi-Fi déportée",
                    "L'antenne d'origine émet depuis l'intérieur d'un châssis métallique.\n⚠️ Vérifier au démontage que le module ESP32 dispose d'un connecteur U.FL.")
w_antenne.add("Antenne 2,4 GHz 2 dBi, embase SMA, déportée", "Siretta DELTA-6A ou équiv.", 1, "RS", 18.00)
w_antenne.add("Pigtail U.FL → SMA femelle + passe-cloison", "Amphenol 336312-24-0100", 1, "RS", 8.00)
w_antenne.add("Support de fixation, visserie", "lot", 1, "RS", 4.00)

w_poste = Section("Poste fixe — Unipi Gate G100",
                  "Le poste porte le récepteur EnOcean, le broker MQTT et l'interface de\n"
                  "supervision. **Le Gate G100 remplace l'E413 initialement prévu** : voir\n"
                  "la justification en fin de document.")
w_poste.add("Passerelle Linux DIN — Debian, 16 Go eMMC, 2× Ethernet, USB 3.0, RS485", "Unipi Gate G100", 1, "Spécialiste", 200.00)
w_poste.add("Récepteur EnOcean 868 MHz, UART ESP3", "TCM 515 (EnOcean)", 1, "Spécialiste", 28.00)
w_poste.add("Antenne EnOcean 868 MHz déportée + pigtail", "EnOcean ANT300 ou équiv.", 1, "Spécialiste", 10.00)
w_poste.add("Adaptateur USB-série vers le TCM 515", "FTDI FT232RL", 1, "RS", 8.00)
w_poste.add("Alimentation rail DIN 230 V → 24 V 15 W", "MEAN WELL HDR-15-24", 1, "RS", 14.00)
w_poste.add("Coffret rail DIN, bornier, presse-étoupes", "Fibox ou Schneider", 1, "RS", 20.00)
w_poste.add("Câble Ethernet blindé vers le réseau usine", "Cat 6 S/FTP, 5 m", 1, "RS", 6.00)

w_boutons = Section("Boutons d'appel EnOcean — 2 stations")
w_boutons.add("Module émetteur auto-alimenté, **sans pile**", "PTM 210 (EnOcean, EU 868)", 2, "Spécialiste", 30.00)
w_boutons.add("Enveloppe / poussoir mural compatible PTM 210", "Eltako, NodOn ou Trio2Sys", 2, "Spécialiste", 12.00)
w_boutons.add("Plaque de repérage station gravée", "sur mesure", 2, "Amazon", 4.00)
w_boutons.add("Fixation, visserie, adhésif industriel", "3M VHB ou équiv.", 2, "RS", 4.00)

w_outil = Section("Outillage — non récurrent")
w_outil.add("Programmateur ISP — **sauvegarde puis flash de l'ATmega**", "USBasp ou USBtinyISP", 1, "Amazon", 8.00)
w_outil.add("Adaptateur USB-série 3,3 V — ESP32 et liaison inter-MCU", "FTDI FT232RL ou CP2102", 1, "Amazon", 6.00)
w_outil.add("Analyseur logique 8 voies — chronogrammes X/Y", "clone Saleae 24 MHz", 1, "Amazon", 15.00)
w_outil.add("Jeu de cosses + pince à sertir — confection du harnais", "Knipex ou Engineer PA-09", 1, "RS", 45.00)
w_outil.add("Kit réseau : testeur RJ45, sertisseuse", "lot", 1, "Amazon", 31.00)

WIFI_SECTIONS = [w_carte, w_harnais, w_antenne, w_poste, w_boutons, w_outil]

# ===========================================================================
#  SMS + EnOcean — variante B (LTE-M / MQTT)
# ===========================================================================
s_carte = Section("Carte AGV — variante LTE-M",
                  "Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte**.")
s_carte.add("Module MCU, 8 Mo flash", "ESP32-WROOM-32E-N8", 1, "RS", 5.00)
s_carte.add("Modem LTE-M / NB-IoT, très basse consommation", "SIM7080G (SIMCom)", 1, "Amazon", 18.00)
s_carte.add("Antenne LTE 4 dBi déportée + pigtail U.FL → SMA", "Siretta ECHO-9 ou équiv.", 1, "RS", 12.00)
s_carte.add("Support SIM nano, protection ESD", "Molex 785900001", 1, "RS", 1.50)
s_carte.add("Optocoupleur quadruple — 43 voies", "PC847 (Sharp)", 11, "RS", 0.60)
s_carte.add("Convertisseur DC/DC 24 V → 5 V 1 A", "TSR 1-2450 (Traco Power)", 1, "RS", 7.00)
s_carte.add("LDO 3,3 V 600 mA", "AP2112K-3.3TRG1 (Diodes)", 1, "RS", 0.60)
s_carte.add("Diode TVS protection 24 V", "SMBJ33A (Littelfuse)", 2, "RS", 0.50)
s_carte.add("**Réservoir capacitif** — pics d'émission modem (2 A)", "470 µF low-ESR 16 V", 1, "RS", 3.00)
s_carte.add("Résistances 1 %, découplages, LED d'état", "lot", 1, "RS", 8.00)
s_carte.add("ILS (reed) + aimant — Wi-Fi de maintenance", "Standex KSK-1A66 ou équiv.", 1, "RS", 2.00)
s_carte.add("SUB-D 25 mâle et femelle, coudés CI", "Amphenol L717SDB25xA4CH4F", 2, "RS", 3.00)
s_carte.add("PCB 4 couches ~120 × 100 mm (série de 5)", "Gerber projet", 1, "PCB", 12.00)
s_carte.add("Boîtier, fixation, presse-étoupes, conn. de prog.", "Hammond 1590 ou Fibox", 1, "RS", 28.00)

s_poste_esp = Section("Poste fixe — option A : ESP32 (recommandée)",
                      "Suffit dès lors que l'historique long terme n'est pas exigé.")
s_poste_esp.add("Module MCU, 8 Mo flash (LittleFS + pages web)", "ESP32-WROOM-32E-N8", 1, "RS", 5.00)
s_poste_esp.add("Modem LTE-M / NB-IoT", "SIM7080G (SIMCom)", 1, "Amazon", 18.00)
s_poste_esp.add("Antenne LTE déportée + pigtail", "Siretta ECHO-9 ou équiv.", 1, "RS", 12.00)
s_poste_esp.add("Récepteur EnOcean 868 MHz, UART ESP3", "TCM 515 (EnOcean)", 1, "Spécialiste", 28.00)
s_poste_esp.add("Antenne EnOcean déportée", "EnOcean ANT300 ou équiv.", 1, "Spécialiste", 8.00)
s_poste_esp.add("Ethernet SPI + RJ45 — **liaison filaire**", "WIZnet WIZ850io (W5500)", 1, "RS", 6.00)
s_poste_esp.add("LED d'accusé, LED de vie, boutons appairage/reset", "lot", 1, "RS", 3.50)
s_poste_esp.add("Alimentation rail DIN 230 V → 24 V 15 W", "MEAN WELL HDR-15-24", 1, "RS", 14.00)
s_poste_esp.add("24 V → 5 V → 3,3 V", "TSR 1-2450 + AP2112K-3.3", 1, "RS", 8.00)
s_poste_esp.add("PCB 2 couches ~100 × 80 mm", "Gerber projet", 1, "PCB", 6.00)
s_poste_esp.add("Boîtier mural IP54, presse-étoupes, embases SMA", "Fibox ou Hammond 1554", 1, "RS", 30.00)
s_poste_esp.add("Support SIM, passifs", "Molex 785900001 + lot", 1, "RS", 3.00)
s_poste_esp.add("Câble Ethernet blindé", "Cat 6 S/FTP, 5 m", 1, "RS", 6.00)

s_poste_gate = Section("Poste fixe — option B : Unipi Gate G100 (**si Ethernet disponible**)",
                  "Le modem du poste ne sert à rien dès qu'une prise réseau est à portée :\n"
                  "le poste parle au broker par le fil. Cela **supprime une SIM sur deux** et\n"
                  "ouvre la gamme Gate, qui n'a pas de cellulaire. Debian d'origine.")
s_poste_gate.add("Passerelle Linux DIN — Debian, 16 Go eMMC, 2× Ethernet, USB 3.0, RS485", "Unipi Gate G100", 1, "Spécialiste", 200.00)
s_poste_gate.add("Récepteur EnOcean 868 MHz, UART ESP3", "TCM 515 (EnOcean)", 1, "Spécialiste", 28.00)
s_poste_gate.add("Antenne EnOcean 868 MHz déportée + pigtail", "EnOcean ANT300 ou équiv.", 1, "Spécialiste", 10.00)
s_poste_gate.add("Adaptateur USB-série vers le TCM 515", "FTDI FT232RL", 1, "RS", 8.00)
s_poste_gate.add("Alimentation rail DIN 230 V → 24 V 15 W", "MEAN WELL HDR-15-24", 1, "RS", 14.00)
s_poste_gate.add("Coffret rail DIN, bornier, presse-étoupes", "Fibox ou Schneider", 1, "RS", 20.00)
s_poste_gate.add("Câble Ethernet blindé vers le réseau usine", "Cat 6 S/FTP, 5 m", 1, "RS", 6.00)

s_poste_unipi = Section("Poste fixe — option C : UniPi E413 LTE (**seulement sans Ethernet**)",
                  "À ne retenir que si le poste est hors de portée d'une prise réseau. Le\n"
                  "modem intégré est alors la raison d'être du modèle — et son surcoût.")
s_poste_unipi.add("Automate compact Linux, E/S TOR, modem LTE intégré", "UniPi E413 (variante LTE)", 1, "Spécialiste", 350.00)
s_poste_unipi.add("Antenne LTE externe déportée", "Siretta ECHO-9 ou équiv.", 1, "RS", 15.00)
s_poste_unipi.add("Récepteur EnOcean + antenne", "TCM 515 + ANT300", 1, "Spécialiste", 36.00)
s_poste_unipi.add("Coffret rail DIN, alimentation, bornier", "Fibox + MEAN WELL HDR-15-24", 1, "RS", 34.00)
s_poste_unipi.add("Câble Ethernet blindé", "Cat 6 S/FTP, 5 m", 1, "RS", 4.00)

s_boutons = Section("Boutons d'appel EnOcean — 2 stations")
s_boutons.add("Module émetteur auto-alimenté, **sans pile**", "PTM 210 (EnOcean, EU 868)", 2, "Spécialiste", 30.00)
s_boutons.add("Enveloppe / poussoir mural compatible PTM 210", "Eltako, NodOn ou Trio2Sys", 2, "Spécialiste", 12.00)
s_boutons.add("Plaque de repérage station gravée", "sur mesure", 2, "Amazon", 4.00)
s_boutons.add("Fixation, visserie", "lot", 2, "RS", 4.00)

s_outil = Section("Outillage — non récurrent")
s_outil.add("Analyseur logique 8 voies — chronogrammes X/Y", "clone Saleae 24 MHz", 1, "Amazon", 15.00)
s_outil.add("Adaptateur USB-série 3,3 V — mise au point pile AT", "FTDI FT232RL ou CP2102", 1, "Amazon", 6.00)
s_outil.add("Jeu de cosses, pince à sertir, consommables", "Knipex ou Engineer PA-09", 1, "RS", 24.00)

SMS_SECTIONS = [s_carte, s_poste_esp, s_poste_gate, s_poste_unipi, s_boutons, s_outil]

# ===========================================================================
#  Émission
# ===========================================================================
def recap(rows, title="Récapitulatif"):
    out = [f"## {title}", "",
           "| Poste | Total TTC relevé | *Repère TTC* | *Repère HT* |",
           "|---|---:|---:|---:|"]
    total_ht = 0.0
    for label, ht, bold in rows:
        total_ht += ht
        lab = f"**{label}**" if bold else label
        out.append(f"| {lab} | ☐ | *{eur(ht * TVA)}* | *{eur(ht)}* |")
    out.append(f"| **TOTAL** | **☐** | ***{eur(total_ht * TVA)}*** | ***{eur(total_ht)}*** |")
    return "\n".join(out) + "\n", total_ht

def write(path, titre, cmp_path, sections, extra):
    body = [HEADER.format(titre=titre, cmp=cmp_path)]
    for s in sections:
        body.append(s.md())
    body.append(extra)
    body.append(TAIL_SOURCING.format(cmp=cmp_path))
    open(path, "w").write("\n".join(body))
    print("écrit :", path)

# --- LoRa -------------------------------------------------------------------
carte595 = carte.ht + bus595.ht
a1_ht = carte595 + 2 * bouton_a1.ht + outil_lora.ht
a3_ht = carte595 + poste_a3.ht + 2 * bouton_a3.ht + outil_lora.ht
r1, _ = recap([("Carte AGV (variante `shift595`)", carte595, False),
               ("2 boutons sur pile", 2 * bouton_a1.ht, False),
               ("Outillage", outil_lora.ht, False)], "Récapitulatif — variante A1 (LoRa homogène)")
r3, _ = recap([("Carte AGV (variante `shift595`)", carte595, False),
               ("Poste fixe EnOcean → LoRa", poste_a3.ht, False),
               ("2 boutons PTM 210", 2 * bouton_a3.ht, False),
               ("Outillage", outil_lora.ht, False)], "Récapitulatif — variante A3 (EnOcean + LoRa)")

cross = []
for n in (2, 4, 6, 8, 12):
    a1 = (carte595 + outil_lora.ht + bouton_a1.ht * n) * TVA
    a3 = (carte595 + poste_a3.ht + outil_lora.ht + bouton_a3.ht * n) * TVA
    win = "**A1**" if a1 < a3 else ("**A3**" if a3 < a1 else "égalité")
    cross.append(f"| {n} | {eur(a1)} | {eur(a3)} | {win} |")

# Repères chiffrés cités dans l'analyse critique.
gain = eur((2.00 - 0.60) * TVA)     # TPS62740 remplacé par un réservoir capacitif
bat  = eur(0.20 * TVA)              # diode Schottky de protection pile

ANALYSE_LORA = f"""---

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

`TPS62740` remplacé par **220 µF tantale + 10 µF X7R** : {gain} d'économie, un
composant de moins, et le problème réellement traité.

Une diode Schottky est ajoutée en protection de la pile — {bat} — contre une
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
"""

LORA_EXTRA = f"""---

{r1}
{r3}
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
| Dongle LoRa USB | ~{eur(25 * TVA)} + hub | L'unique port USB est déjà pris par le TCM 515 |
| Modem LoRa UART/RS485 (`E32-868T20D`, `RAK3172`) | ~{eur(15 * TVA)} | **Le module gère le PHY lui-même** : il faudrait réécrire `LoraTransport`, et surtout **abandonner le contrôle du budget de rapport cyclique** que le firmware applique et teste aujourd'hui. C'est une obligation réglementaire, pas un réglage |
| Gate + carte ESP32 en frontal radio | ~{eur((200 + poste_a3.ht) * TVA)} | On paie les deux |

**Le poste LoRa n'a d'ailleurs pas besoin de Linux.** Son travail est une
traduction de protocole : EnOcean entre, LoRa sort. Il n'héberge pas de broker,
et l'AGV lui parle directement.

L'asymétrie avec l'architecture Wi-Fi est donc logique :

| | Poste Wi-Fi | Poste LoRa |
|---|---|---|
| Doit héberger un broker MQTT | **oui** | non |
| Doit piloter une radio au niveau PHY | non — le réseau est Ethernet | **oui** — SX1276 sur SPI |
| Matériel qui en découle | boîtier Linux industriel | microcontrôleur avec SPI |
| Retenu | Unipi Gate G100 (~{eur(200 * TVA)} TTC) | carte ESP32 (~{eur(poste_a3.ht * TVA)} TTC) |

⚠️ **Deux antennes 868 MHz sur le même boîtier** — EnOcean et LoRa. Les espacer
d'au moins 20 cm, ou en déporter une. Une désensibilisation du récepteur EnOcean
par l'émetteur LoRa se traduirait par des appuis perdus, silencieusement.

### Où se croisent les deux courbes

| Stations | A1 (TTC) | A3 (TTC) | Moins cher |
|---:|---:|---:|---|
{chr(10).join(cross)}

Le point de bascule est à **8 stations**. En dessous, A1 coûte moins **et**
rend un accusé visuel à l'opérateur. Au-delà, A3 prend l'avantage grâce à des
boutons à {eur(bouton_a3.ht * TVA)} au lieu de {eur(bouton_a1.ht * TVA)}, et supprime les piles.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| **[A1]** bouton sur pile | **{eur(bouton_a1.ht * TVA)}** | {eur(bouton_a1.ht)} |
| **[A3]** bouton PTM 210 | **{eur(bouton_a3.ht * TVA)}** | {eur(bouton_a3.ht)} |

### Coûts récurrents

| Poste | Annuel |
|---|---:|
| Abonnement opérateur | **0 €** — bande ISM libre |
| Infrastructure | **0 €** — aucune |
| **[A1]** Remplacement des piles | ~{eur(6 * TVA)} par bouton tous les 5 à 8 ans |
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
- **La carte de rechange** : ~{eur(carte595 * TVA)} pour un échange standard. Recommandé.
- **Un éventuel relais LoRa** si le relevé révèle une zone morte : ~{eur(40 * TVA)},
  mais surtout une complexité applicative hors périmètre actuel.
"""

write("/home/mathieu/AIO/AGV_MEIDEN/CarteComm/LoRa/BOM.md",
      "architecture LoRa 868 MHz (carte neuve)", "../COMPARAISON.md",
      LORA_SECTIONS, LORA_EXTRA + ANALYSE_LORA)
print(f"A1 = {a1_ht:.2f} HT / {a1_ht*TVA:.2f} TTC ; A3 = {a3_ht:.2f} HT / {a3_ht*TVA:.2f} TTC")

# --- Wi-Fi ------------------------------------------------------------------
wifi_ht = sum(s.ht for s in WIFI_SECTIONS)
rw, _ = recap([("Carte AGV (nomenclature KiCad)", w_carte.ht, True),
               ("Harnais de raccordement", w_harnais.ht, False),
               ("Antenne Wi-Fi déportée", w_antenne.ht, False),
               ("Poste fixe UniPi", w_poste.ht, False),
               ("2 boutons EnOcean", w_boutons.ht, False),
               ("Outillage", w_outil.ht, False)])

# Alternatives à l'étage de sortie de la carte routée.
irf = eur(23 * 0.60 * TVA)          # IRF520 ×23, tel que routé
irl = eur(23 * 1.10 * TVA)          # IRL520 ×23, version logic-level, même brochage
uln = eur(3 * 1.20 * TVA)           # ULN2803A ×3, réseau Darlington
tsr = eur(7.00 * TVA)               # TSR 1-2450 non isolé
eco = eur((25.00 - 7.00) * TVA)     # économie si l'isolation n'est pas requise

ANALYSE_WIFI = f"""---

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
| `IRF520` ×23 (actuel) | ~{irf} | **23 boîtiers TO-220** | Hors spec à 5 V, très encombrant |
| `IRL520` ×23 | ~{irl} | idem | Version **logic-level** du même composant, brochage identique — **substitution sans reroutage** |
| `ULN2803A` ×3 | ~{uln} | 3 boîtiers DIP-18 | Réseau Darlington à collecteur ouvert : même fonction, **résistances de grille supprimées**, surface divisée par dix |

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
  inopérante, et un `TSR 1-2450` non isolé à ~{tsr} remplacerait le `TDN` — soit
  **{eco} d'économie** sur la ligne la plus chère de la carte.

### ⚠️ Le 6 V du `L7806CV` alimente un microcontrôleur prévu pour 5,5 V

Rappel de [`docs/subd25_atmega.md`](docs/subd25_atmega.md) : 6,0 V est le
**maximum absolu** de l'ATmega2560. Le `TDN` fournit déjà un 5 V propre sur la
carte. Si le Mega peut être alimenté depuis ce 5 V, le `L7806CV` devient inutile
et le microcontrôleur revient dans sa plage recommandée.

À vérifier au schéma : le 6 V va-t-il sur `Vin` du module Mega — auquel cas il
est *trop bas* pour son régulateur — ou directement sur `V_CC` ?
"""

WIFI_EXTRA = f"""---

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
| Légèrement au-dessus | 21 diviseurs résistifs 1 % | *{eur(8 * TVA)}* |
| 24 V ou incompatible | 6× `PC847` + résistances + carte fille | *{eur(45 * TVA)}* |

Dans le troisième cas, il faut aussi **router une carte fille** : plusieurs
semaines de délai, pas seulement un coût.

{rw}
### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| Bouton PTM 210 complet | **{eur(50 * TVA)}** | {eur(50)} |
| Avec accusé EnOcean par station | {eur(130 * TVA)} | {eur(130)} |

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
remplit la même fonction pour ~{eur(85 * TVA)} au lieu de {eur(w_poste.ht * TVA)} — soit
**{eur((w_poste.ht - 85) * TVA)} d'économie**. Le Gate ne se justifie que par la
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
"""

write("/home/mathieu/AIO/AGV_MEIDEN/CarteComm/Wifi/BOM.md",
      "architecture Wi-Fi (carte V5.0.1 conservée)", "../COMPARAISON.md",
      WIFI_SECTIONS, WIFI_EXTRA + ANALYSE_WIFI)

# --- SMS + EnOcean ----------------------------------------------------------
s_carte595 = s_carte.ht + bus595.ht
sms_esp_ht = s_carte595 + s_poste_esp.ht + s_boutons.ht + s_outil.ht
sms_uni_ht  = s_carte595 + s_poste_unipi.ht + s_boutons.ht + s_outil.ht
sms_gate_ht = s_carte595 + s_poste_gate.ht  + s_boutons.ht + s_outil.ht
rs_, _ = recap([("Carte AGV (variante `shift595`)", s_carte595, False),
                ("Poste fixe ESP32 (option A)", s_poste_esp.ht, False),
                ("2 boutons EnOcean", s_boutons.ht, False),
                ("Outillage", s_outil.ht, False)],
               "Récapitulatif — variante B (LTE-M / MQTT), poste ESP32")

poste_esp = eur(s_poste_esp.ht * TVA)
# Modem + antenne + support SIM du poste, inutiles s'il est raccordé en Ethernet.
eco_sim = eur((18.00 + 12.00 + 3.00) * TVA)

ANALYSE_SMS = f"""---

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
{poste_esp}) reste la moins chère dans les deux cas.

### ⚠️ À vérifier — la limitation de courant des optocoupleurs

Comme pour l'architecture LoRa : 43 canaux de `PC847` demandent 43 résistances
de limitation dimensionnées pour la tension réelle des lignes (§12.1). À sortir
du forfait « passifs » une fois la mesure faite.

### ⚠️ Deux antennes cellulaires, deux abonnements

L'AGV **et** le poste portent chacun un `SIM7080G` et une SIM. C'est ce qui
double le coût récurrent. Si le poste dispose d'un raccordement Ethernet — ce
qui est le cas dès qu'il est dans un local technique — **son modem est inutile**
et il parle au broker par le réseau filaire : {eco_sim} de matériel et la moitié
du récurrent en moins.

C'est probablement l'économie la plus simple de cette architecture, et elle n'a
aucune contrepartie technique.
"""

SMS_EXTRA = f"""---

## Interface bus

Les deux variantes d'interface bus de la section précédente s'appliquent aussi
ici : `shift595` est retenue par défaut. Sous-total carte AGV complète :
***{eur(s_carte595 * TVA)}*** TTC ({eur(s_carte595)} HT).

{rs_}
### Avec un poste UniPi — laquelle des deux options ?

| Poste | *Repère TTC* | *Repère HT* | Récurrent |
|---|---:|---:|---|
| **B — Unipi Gate G100** *(si Ethernet)* | ***{eur(sms_gate_ht * TVA)}*** | ***{eur(sms_gate_ht)}*** | **1 SIM** |
| **C — UniPi E413 LTE** *(sans Ethernet)* | *{eur(sms_uni_ht * TVA)}* | *{eur(sms_uni_ht)}* | 2 SIM |

**{eur((s_poste_unipi.ht - s_poste_gate.ht) * TVA)} d'écart de matériel — et la
moitié du récurrent.** Le Gate n'a pas de modem cellulaire ; c'est précisément ce
qui le rend éligible ici, puisqu'un poste raccordé en Ethernet n'en a aucun
besoin. Il apporte au passage Debian d'origine, deux ports Ethernet et 16 Go
d'eMMC.

**La question à poser au client tient en une phrase : y a-t-il une prise réseau
là où le poste sera fixé ?** Si oui, l'option C n'a plus de justification.

⚠️ L'unique port USB du Gate est pris par l'adaptateur série du `TCM 515`.
Prévoir un concentrateur si un autre périphérique USB devient nécessaire.

Face à l'option A (ESP32, {eur(s_poste_esp.ht * TVA)}), l'écart restant est de
**{eur((s_poste_gate.ht - s_poste_esp.ht) * TVA)}** : c'est le prix de
l'historique long terme et d'un matériel référencé.

### Coût par station supplémentaire

| | TTC | HT |
|---|---:|---:|
| Bouton PTM 210 complet | **{eur(50 * TVA)}** | {eur(50)} |

---

## Abonnements et infrastructure

| Poste | HT | TTC |
|---|---:|---:|
| 2 SIM M2M data LTE-M, ~1,50 €/mois | 36 €/an | {eur(36 * TVA)}/an |
| Broker MQTT — VPS mutualisé | 60 €/an | {eur(60 * TVA)}/an |
| *Alternative* : Mosquitto sur un serveur usine existant | 0 €/an | 0 €/an |
| **Total récurrent** | **{eur(96)}/an** | **{eur(96 * TVA)}/an** |

Héberger le broker sur un serveur du client supprime le poste VPS **et** la
dépendance à un tiers. À proposer systématiquement.

### Coût sur 10 ans — variante B

| | Poste ESP32 | Poste UniPi |
|---|---:|---:|
| Matériel et outillage (TTC) | {eur(sms_esp_ht * TVA)} | {eur(sms_uni_ht * TVA)} |
| Récurrent sur 10 ans (TTC) | {eur(960 * TVA)} | {eur(960 * TVA)} |
| **Total 10 ans (TTC)** | **{eur((sms_esp_ht + 960) * TVA)}** | **{eur((sms_uni_ht + 960) * TVA)}** |
| *pour mémoire, en HT* | *{eur(sms_esp_ht + 960)}* | *{eur(sms_uni_ht + 960)}* |

---

## Variante A — SMS, pour mémoire

Chiffrée pour la comparaison. **Ne pas déployer** en liaison principale : ni
latence bornée, ni ordre de remise, ni garantie de remise.

| Poste | *Repère TTC* | *Repère HT* |
|---|---:|---:|
| Poste UniPi E413 (LTE) + antenne + boutons filaires + câblage | *{eur(439 * TVA)}* | *{eur(439)}* |
| Carte AGV avec modem `SIM7600E-H` (Cat-1, plus gourmand) | *{eur(141 * TVA)}* | *{eur(141)}* |
| Outillage | *{eur(45 * TVA)}* | *{eur(45)}* |
| **Total matériel** | ***{eur(625 * TVA)}*** | ***{eur(625)}*** |

| Récurrent | HT | TTC |
|---|---:|---:|
| 2 abonnements SIM M2M | 120 à 240 €/an | {eur(144)} à {eur(288)}/an |
| Volume SMS (appels + accusés + télémétrie dégradée) | 1 000 à 1 300 €/an | {eur(1200)} à {eur(1560)}/an |
| **Total** | **~1 500 €/an** | **~{eur(1800)}/an** |
| **Sur 10 ans, tout compris** | **~{eur(15625)}** | **~{eur(18750)}** |

Le surcoût sur dix ans face à la variante B est de **~{eur(15625 - (sms_esp_ht + 960))} HT**,
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
| `ESP32`, `SN74HC595N`, `PC847` | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : un seul
point d'arrêt sous −110 dBm disqualifie l'architecture, et le choix de variante
d'interface bus conditionne le routage du PCB.

## Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 6 à 9 jours-homme de mise en œuvre, hors développement.
- **Le contrôle de l'obsolescence 2G/3G** : le `SIM7080G` est LTE-M/NB-IoT, donc
  hors calendrier d'extinction. Un module 2G ne le serait pas.
- **La carte de rechange** : ~{eur(s_carte595 * TVA)} pour un échange standard.
"""

write("/home/mathieu/AIO/AGV_MEIDEN/CarteComm/SMS_EnOcean/BOM.md",
      "architecture SMS + EnOcean (carte neuve)", "../COMPARAISON.md",
      [s_carte, bus595, busmcp, s_poste_esp, s_poste_gate, s_poste_unipi, s_boutons, s_outil], SMS_EXTRA + ANALYSE_SMS)

print()
print("=== TOTAUX (HT / TTC) ===")
print(f"LoRa A1      {a1_ht:8.2f} / {a1_ht*TVA:8.2f}")
print(f"LoRa A3      {a3_ht:8.2f} / {a3_ht*TVA:8.2f}")
print(f"Wi-Fi        {wifi_ht:8.2f} / {wifi_ht*TVA:8.2f}")
print(f"LTE-M ESP32  {sms_esp_ht:8.2f} / {sms_esp_ht*TVA:8.2f}  (10 ans {(sms_esp_ht+960):8.2f} / {(sms_esp_ht+960)*TVA:8.2f})")
print(f"LTE-M Gate   {sms_gate_ht:8.2f} / {sms_gate_ht*TVA:8.2f}")
print(f"LTE-M UniPi  {sms_uni_ht:8.2f} / {sms_uni_ht*TVA:8.2f}")
print(f"SMS          {625:8.2f} / {625*TVA:8.2f}  (10 ans {15625:8.2f} / {15625*TVA:8.2f})")
