# -*- coding: utf-8 -*-
"""Génère les trois feuilles de sourcing (BOM.md) des architectures.

Les prix vivent ICI, en un seul endroit, et les totaux sont CALCULÉS — jamais
recopiés à la main. C'est ce qui garantit qu'un récapitulatif ne peut pas
diverger de son propre détail, comme c'était arrivé sur l'outillage LoRa.

    python3 CarteComm/tools/generer_bom.py

Régénère les trois BOM.md. Après une mise à jour des prix, penser à reporter les
totaux dans CarteComm/COMPARAISON.md et CarteComm/README.md.
"""
TVA = 1.20

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
                "| Désignation | Réf. fabricant | Qté | Source | Réf. catalogue | PU TTC | Total TTC | *Repère TTC* |",
                "|---|---|---:|---|---|---:|---:|---:|"]
        for desig, ref, qty, source, ht in self.lines:
            rep = eur(qty * ht * TVA)
            out.append(f"| {desig} | `{ref}` | {qty} | {source} | ☐ | ☐ | ☐ | *{rep}* |")
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
bouton_a1.add("Convertisseur buck ultra-basse conso", "TPS62740DSSR (TI)", 1, "RS", 2.00)
bouton_a1.add("LED bicolore verte/rouge + résistances", "Kingbright L-59EGW", 1, "RS", 1.00)
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

s_poste_unipi = Section("Poste fixe — option B : UniPi E413",
                        "À retenir seulement si un **historique sur plusieurs semaines** est demandé.\n⚠️ §12.9 : vérifier le runtime livré avant commande.")
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

SMS_SECTIONS = [s_carte, s_poste_esp, s_poste_unipi, s_boutons, s_outil]

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

LORA_EXTRA = f"""---

{r1}
{r3}
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
      LORA_SECTIONS, LORA_EXTRA)
print(f"A1 = {a1_ht:.2f} HT / {a1_ht*TVA:.2f} TTC ; A3 = {a3_ht:.2f} HT / {a3_ht*TVA:.2f} TTC")

# --- Wi-Fi ------------------------------------------------------------------
wifi_ht = sum(s.ht for s in WIFI_SECTIONS)
rw, _ = recap([("Carte AGV (nomenclature KiCad)", w_carte.ht, True),
               ("Harnais de raccordement", w_harnais.ht, False),
               ("Antenne Wi-Fi déportée", w_antenne.ht, False),
               ("Poste fixe UniPi", w_poste.ht, False),
               ("2 boutons EnOcean", w_boutons.ht, False),
               ("Outillage", w_outil.ht, False)])

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
      WIFI_SECTIONS, WIFI_EXTRA)

# --- SMS + EnOcean ----------------------------------------------------------
s_carte595 = s_carte.ht + bus595.ht
sms_esp_ht = s_carte595 + s_poste_esp.ht + s_boutons.ht + s_outil.ht
sms_uni_ht = s_carte595 + s_poste_unipi.ht + s_boutons.ht + s_outil.ht
rs_, _ = recap([("Carte AGV (variante `shift595`)", s_carte595, False),
                ("Poste fixe ESP32 (option A)", s_poste_esp.ht, False),
                ("2 boutons EnOcean", s_boutons.ht, False),
                ("Outillage", s_outil.ht, False)],
               "Récapitulatif — variante B (LTE-M / MQTT), poste ESP32")

SMS_EXTRA = f"""---

## Interface bus

Les deux variantes d'interface bus de la section précédente s'appliquent aussi
ici : `shift595` est retenue par défaut. Sous-total carte AGV complète :
***{eur(s_carte595 * TVA)}*** TTC ({eur(s_carte595)} HT).

{rs_}
### Avec le poste UniPi (option B)

| Poste | *Repère TTC* | *Repère HT* |
|---|---:|---:|
| Carte AGV | *{eur(s_carte595 * TVA)}* | *{eur(s_carte595)}* |
| Poste UniPi E413 | *{eur(s_poste_unipi.ht * TVA)}* | *{eur(s_poste_unipi.ht)}* |
| 2 boutons EnOcean | *{eur(s_boutons.ht * TVA)}* | *{eur(s_boutons.ht)}* |
| Outillage | *{eur(s_outil.ht * TVA)}* | *{eur(s_outil.ht)}* |
| **TOTAL** | ***{eur(sms_uni_ht * TVA)}*** | ***{eur(sms_uni_ht)}*** |

L'écart entre les deux postes est de **{eur((s_poste_unipi.ht - s_poste_esp.ht) * TVA)}** :
c'est le prix de l'historique long terme et d'un automate référencé.

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
- **La carte de rechange** : ~{eur(s_carte595 * TVA)} pour un échange standard.
"""

write("/home/mathieu/AIO/AGV_MEIDEN/CarteComm/SMS_EnOcean/BOM.md",
      "architecture SMS + EnOcean (carte neuve)", "../COMPARAISON.md",
      [s_carte, bus595, busmcp, s_poste_esp, s_poste_unipi, s_boutons, s_outil], SMS_EXTRA)

print()
print("=== TOTAUX (HT / TTC) ===")
print(f"LoRa A1      {a1_ht:8.2f} / {a1_ht*TVA:8.2f}")
print(f"LoRa A3      {a3_ht:8.2f} / {a3_ht*TVA:8.2f}")
print(f"Wi-Fi        {wifi_ht:8.2f} / {wifi_ht*TVA:8.2f}")
print(f"LTE-M ESP32  {sms_esp_ht:8.2f} / {sms_esp_ht*TVA:8.2f}  (10 ans {(sms_esp_ht+960):8.2f} / {(sms_esp_ht+960)*TVA:8.2f})")
print(f"LTE-M UniPi  {sms_uni_ht:8.2f} / {sms_uni_ht*TVA:8.2f}")
print(f"SMS          {625:8.2f} / {625*TVA:8.2f}  (10 ans {15625:8.2f} / {15625*TVA:8.2f})")
