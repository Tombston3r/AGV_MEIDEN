# Nomenclature — architecture LoRa 868 MHz (carte neuve)

> Prix indicatifs **HT, petites quantités, 2026**. À reconsulter au moment de
> l'achat.
>
> Hypothèse de dimensionnement : **2 points d'appel**. Le coût par station
> supplémentaire est donné en §5 — c'est là que les deux variantes divergent.
>
> Procédure de mise en œuvre : [`DEPLOY.md`](DEPLOY.md).
> Sources : [`docs/Archi_1_LoRa_P2P_homogene.md`](docs/Archi_1_LoRa_P2P_homogene.md) §6
> et [`docs/Archi_3_Hybride_EnOcean_LoRa.md`](docs/Archi_3_Hybride_EnOcean_LoRa.md) §6.

---

## Deux variantes, deux profils de coût

| | A1 — LoRa homogène | A3 — EnOcean + LoRa |
|---|---:|---:|
| Carte AGV | 103 € | 105 € |
| Poste fixe | **0 €** (aucun) | 128 € |
| Bouton (l'unité) | **62 €** | **46 €** |
| **Total 2 stations** | **≈ 272 €** | **≈ 325 €** |
| Chaque station de plus | **+ 62 €** | **+ 46 €** |
| Récurrent | 0 €/an | 0 €/an |
| Pile à remplacer | tous les 5 à 8 ans | **jamais** |
| Retour visuel au bouton | **oui** — LED verte = ACK | **non** |

**Le point de bascule est à 4 stations** : au-delà, A3 devient moins chère
malgré son poste fixe. En dessous, A1 coûte moins et rend le retour visuel.

**A3 est l'architecture retenue**, sous réserve que le « sans-pile » soit une
exigence réelle. Si ce n'en est pas une, le tableau ci-dessus mérite d'être
reposé au client.

---

## 1. Carte AGV — 103 € (commune aux deux variantes)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU Wi-Fi/BT, 8 Mo flash | 1 | 5,00 € | 5,00 € |
| RFM95W-868S2 | Module LoRa SX1276 868 MHz | 1 | 10,00 € | 10,00 € |
| — | Pigtail U.FL → SMA femelle + passe-cloison | 1 | 3,00 € | 3,00 € |
| — | Antenne 868 MHz 1/4 onde 2 dBi, **déportée sur mât** | 1 | 6,00 € | 6,00 € |
| PC847 | Optocoupleur quadruple — 43 voies → 11 boîtiers | 11 | 0,60 € | 6,60 € |
| TSR 1-2450 | Convertisseur DC/DC 24 V → 5 V, 1 A | 1 | 7,00 € | 7,00 € |
| AP2112K-3.3 | LDO 3,3 V 600 mA pour ESP32 et radio | 1 | 0,60 € | 0,60 € |
| SMBJ33A | Diode TVS, protection alimentation 24 V | 2 | 0,50 € | 1,00 € |
| — | Résistances 1 %, découplages, LED d'état | lot | — | 8,00 € |
| — | ILS (reed) + aimant, ouverture du Wi-Fi de maintenance | 1 | 2,00 € | 2,00 € |
| — | SUB-D 25 mâle et femelle, coudés CI | 2 | 3,00 € | 6,00 € |
| — | PCB 4 couches ~120 × 100 mm (série de 5) | 1 | 12,00 € | 12,00 € |
| — | Boîtier, fixation, presse-étoupes, connecteur de programmation | 1 | 28,00 € | 28,00 € |
| **Sous-total hors interface bus** | | | | **≈ 95 €** |

### 1.1 Interface bus — le choix conditionne le routage

`profiles/default.yaml` → `bus.driver_variant`. Le logiciel supporte les trois,
**le PCB n'en supportera qu'une**.

| Variante | Composants | Coût | Pose des 22 lignes |
|---|---|---:|---|
| **`shift595`** — recommandé | 3× 74HC595 + 3× 74HC165 | **~3,00 €** | ~3 µs, **strictement simultanée** |
| `mcp23017` | 4× MCP23017-E/SP | ~10,00 € | ~150 µs, décalage GPIOA/GPIOB ~25 µs |
| `mega_uart` | ATmega2560 + quartz + passifs | ~12,00 € | < 1 µs, précédée d'un aller-retour UART |

**Sous-total carte AGV (`shift595`) : ≈ 98 €**
**Sous-total carte AGV (`mcp23017`, chiffrage historique) : ≈ 105 €**

⚠️ Ne jamais alimenter un RFM95W sans antenne : l'étage de sortie ne supporte
pas la désadaptation. Prévoir une antenne montée dès le premier essai.

---

## 2. **[A1]** Bouton d'appel sur pile — 62 € l'unité

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| STM32L071KBU6 | MCU ultra-basse consommation | 1 | 3,50 € | 3,50 € |
| — | *Alternative* : ESP32-C3, si l'homogénéité de toolchain prime | (1) | (2,00 €) | — |
| RFM95W-868S2 | Module LoRa 868 MHz | 1 | 10,00 € | 10,00 € |
| — | Antenne 868 MHz + embase SMA | 1 | 6,00 € | 6,00 € |
| — | Bouton poussoir industriel Ø22 IP65, coup de poing ou affleurant | 1 | 12,00 € | 12,00 € |
| ER14505 | Pile Li-SOCl₂ 3,6 V 2,6 Ah + support | 1 | 6,00 € | 6,00 € |
| TPS62740 | Convertisseur buck ultra-basse consommation | 1 | 2,00 € | 2,00 € |
| — | **LED bicolore verte/rouge** + résistances | 1 | 1,00 € | 1,00 € |
| — | PCB 2 couches ~50 × 50 mm | 1 | 3,00 € | 3,00 € |
| — | Boîtier IP65, presse-étoupe, embase antenne | 1 | 18,00 € | 18,00 € |
| **Total par bouton** | | | | **≈ 62 €** |

La LED bicolore est ce qui distingue cette variante : **verte fixe 2 s = ACK
reçu, rouge clignotante = échec après 3 essais**. Aucune solution EnOcean ne
sait le faire.

Autonomie visée : sommeil profond < 2 µA, soit **5 à 8 ans** sur une ER14505 en
usage normal. À vérifier au banc (phase 7 de `DEPLOY.md`).

---

## 3. **[A3]** Poste fixe EnOcean → LoRa — 128 €

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU, 8 Mo flash (LittleFS + pages web) | 1 | 5,00 € | 5,00 € |
| TCM 515 (EU 868 MHz) | Récepteur EnOcean, interface UART ESP3 | 1 | 28,00 € | 28,00 € |
| — | Antenne EnOcean 868 MHz déportée | 1 | 8,00 € | 8,00 € |
| RFM95W-868S2 | Module LoRa SX1276 | 1 | 10,00 € | 10,00 € |
| — | Pigtail U.FL → SMA + antenne LoRa 2 dBi déportée | 1 | 9,00 € | 9,00 € |
| W5500 (module) | Ethernet SPI + RJ45 magnétique — **liaison filaire** | 1 | 6,00 € | 6,00 € |
| — | *Alternative* : WT32-ETH01 (ESP32 + PHY intégrés) | (1) | (12,00 €) | — |
| — | LED d'accusé bicolore, LED de vie, résistances | 1 | 1,50 € | 1,50 € |
| — | Bouton d'appairage + bouton reset | 2 | 1,00 € | 2,00 € |
| MEAN WELL HDR-15-24 | Alimentation rail DIN 230 V → 24 V 15 W | 1 | 14,00 € | 14,00 € |
| TSR 1-2450 + AP2112K | 24 V → 5 V → 3,3 V | 1 | 8,00 € | 8,00 € |
| — | PCB 2 couches ~100 × 80 mm | 1 | 6,00 € | 6,00 € |
| — | Boîtier mural IP54, presse-étoupes, embases SMA | 1 | 30,00 € | 30,00 € |
| **Total poste fixe** | | | | **≈ 128 €** |

⚠️ **Deux antennes 868 MHz sur le même boîtier** — EnOcean et LoRa. Les
espacer d'au moins 20 cm, ou déporter l'une des deux. Une désensibilisation du
récepteur EnOcean par l'émetteur LoRa se traduirait par des appuis perdus,
silencieusement.

---

## 4. **[A3]** Boutons EnOcean — 46 € l'unité

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| PTM 210 (EU 868 MHz) | Module émetteur auto-alimenté, **sans pile** | 1 | 30,00 € | 30,00 € |
| — | Enveloppe / poussoir mural compatible PTM 210 | 1 | 12,00 € | 12,00 € |
| — | Plaque de repérage station gravée | 1 | 4,00 € | 4,00 € |
| **Total par bouton** | | | | **≈ 46 €** |

> **Alternative prête à l'emploi** : interrupteur EnOcean du commerce (NodOn,
> Trio2Sys, Eltako) à ~45–60 € tout compris. À privilégier en petit volume.

### 4.1 Accusé opérateur — conditionnel, 0 à 160 €

Le TCM 515 est en **réception seule**. Options, par ordre de coût :

| Option | Coût (2 postes) | Remarque |
|---|---:|---|
| LED d'accusé au poste fixe | 0 € | déjà au §3, mais l'opérateur doit voir le poste |
| Actionneur EnOcean par point d'appel | 100 à 160 € | retour local |
| Passage au TCM 310 (bidirectionnel) | +15 € sur le poste | à valider, §12.8 |

**Aucune de ces options n'égale la LED intégrée du bouton A1**, qui indique
l'ACK de bout en bout.

---

## 5. Récapitulatif

### 5.1 Variante A1 — LoRa homogène

| Poste | Montant |
|---|---:|
| Carte AGV (`shift595`) | 98 € |
| 2 boutons sur pile | 124 € |
| Outillage | 45 € |
| **Total matériel** | **≈ 267 €** |
| **Chaque station supplémentaire** | **+ 62 €** |

### 5.2 Variante A3 — EnOcean + LoRa

| Poste | Montant |
|---|---:|
| Carte AGV (`shift595`) | 98 € |
| Poste fixe EnOcean → LoRa | 128 € |
| 2 boutons PTM 210 | 92 € |
| Outillage | 45 € |
| **Total matériel** | **≈ 363 €** |
| **Chaque station supplémentaire** | **+ 46 €** |

### 5.3 Où se croisent les deux courbes

| Nombre de stations | A1 | A3 | Moins cher |
|---:|---:|---:|---|
| 2 | 267 € | 363 € | **A1** |
| 4 | 391 € | 455 € | **A1** |
| **6** | **515 €** | **547 €** | A1, de peu |
| 8 | 639 € | 639 € | **égalité** |
| 12 | 887 € | 823 € | **A3** |

À 8 stations les deux se rejoignent. En dessous, A1 est moins chère **et** rend
le retour visuel ; au-delà, A3 prend l'avantage et supprime les piles.

### 5.4 Coûts récurrents

| Poste | Annuel |
|---|---:|
| Abonnement opérateur | **0 €** — bande ISM libre |
| Infrastructure | **0 €** — aucune |
| **[A1]** Remplacement des piles | ~6 € par bouton tous les 5 à 8 ans |
| **[A3]** Piles | **0 €** — PTM 210 auto-alimentés |
| **Total récurrent** | **≈ 0 €/an** |

### 5.5 Coût sur 10 ans (2 stations)

| | A1 | A3 |
|---|---:|---:|
| Matériel initial | 267 € | 363 € |
| Piles (1 remplacement) | 12 € | 0 € |
| **Total 10 ans** | **≈ 279 €** | **≈ 363 €** |

**C'est l'architecture la moins chère des trois sur dix ans**, et de loin :
~15 625 € pour le SMS, ~1 407 € pour le LTE-M, ~738 € pour le Wi-Fi.

---

## 6. Outillage — 60 €, non récurrent

| Désignation | Prix | Usage |
|---|---:|---|
| **Dongle RTL-SDR + antenne** | 30 € | Mesure d'occupation réelle de la bande 868 MHz (`rtl_power`). **Argument objectif à opposer à la crainte de collision** |
| Analyseur logique 8 voies | 15 € | Chronogrammes X/Y, mesure de `t_setup` |
| Adaptateur USB-série 3,3 V | 6 € | Mise au point ESP32 |
| Wattmètre µA ou multimètre à faible burden voltage | 9 € | **[A1]** Vérification du sommeil profond < 2 µA |
| Oscilloscope | — | Supposé disponible. Amplitude `Y05`, **prérequis bloquant** |
| **Total outillage** | **≈ 60 €** | |

Le RTL-SDR mérite d'être budgété même s'il paraît accessoire : c'est lui qui
transforme « on craint les collisions à 868 MHz » en une mesure.

---

## 7. Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| PCB 4 couches + assemblage | 3 à 5 semaines | **Chemin critique matériel** |
| **[A1]** PCB bouton + boîtiers IP65 | 3 à 5 semaines | En parallèle de la carte AGV |
| RFM95W-868S2 | 1 à 3 semaines | Modules très diffusés, mais contrefaçons fréquentes — acheter chez un distributeur référencé |
| PTM 210 / TCM 515 | 1 à 2 semaines | Faible |
| ER14505 Li-SOCl₂ | 1 à 2 semaines | **Restrictions de transport aérien** sur le lithium : prévoir la marge |
| ESP32, 74HC595/165, PC847 | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : le relevé de
couverture radio et l'arbitrage du facteur d'étalement peuvent changer
l'antenne, la puissance d'émission, et le nombre de nœuds.

---

## 8. Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 5 à 8 jours-homme après la phase 0 qui rend ce dossier
  autonome, hors développement.
- **La conformité RED** : si les cartes sont produites en série et mises sur le
  marché, il faut un dossier technique, un marquage CE et une déclaration UE de
  conformité, conservés 10 ans. Compter 3 à 5 jours-homme, hors essais en
  laboratoire. Sans objet pour un équipement fabriqué et utilisé en interne.
- **La carte de rechange** : ~98 € pour un échange standard. Recommandé.
- **Un éventuel relais LoRa**, si le relevé de couverture révèle une zone
  morte : ~40 € par relais, mais surtout une complexité de routage applicatif
  qui n'est pas dans le périmètre actuel du logiciel.
