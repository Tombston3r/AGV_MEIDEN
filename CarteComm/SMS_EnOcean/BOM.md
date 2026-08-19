# Nomenclature — architecture SMS + EnOcean (carte neuve)

> Prix indicatifs **HT, petites quantités, 2026**. À reconsulter au moment de
> l'achat.
>
> Hypothèse de dimensionnement : **2 points d'appel**. Le coût par station
> supplémentaire est donné en §6.
>
> Procédure de mise en œuvre : [`DEPLOY.md`](DEPLOY.md).
> Analyse de l'architecture : [`docs/Archi_2_Cellulaire_SMS_LTE-M.md`](docs/Archi_2_Cellulaire_SMS_LTE-M.md) §6.

---

## ⚠️ Deux variantes, un écart de 14 000 € sur dix ans

| | Variante A — SMS | Variante B — LTE-M / MQTT |
|---|---:|---:|
| Matériel et outillage | ~625 € | ~407 € |
| Récurrent annuel | **~1 500 €** | **~100 €** |
| **Total 10 ans** | **≈ 15 625 €** | **≈ 1 407 €** |
| Latence | 4 à 8 s, **non bornée** | 0,5 à 2 s |
| Ordre de remise | **non garanti** | garanti par TCP |

**La variante B est la seule défendable.** La variante A est chiffrée ici pour
que la comparaison soit opposable au client, pas pour être déployée — c'est
aussi la position du [`DEPLOY.md`](DEPLOY.md), qui ne déploie que la B.

Le reste de ce document chiffre **la variante B**. La variante A fait l'objet du
§7.

---

## 1. Carte AGV — 141 €

Carte neuve à fabriquer. La V5.0.1 d'origine est **conservée intacte** : c'est
le retour arrière de cette architecture.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU, 8 Mo flash | 1 | 5,00 € | 5,00 € |
| SIM7080G | Modem LTE-M / NB-IoT, très basse consommation | 1 | 18,00 € | 18,00 € |
| — | Antenne LTE 4 dBi déportée + pigtail U.FL → SMA | 1 | 12,00 € | 12,00 € |
| — | Support SIM nano, ESD | 1 | 1,50 € | 1,50 € |
| PC847 | Optocoupleur quadruple — 43 voies → 11 boîtiers | 11 | 0,60 € | 6,60 € |
| TSR 1-2450 | Convertisseur DC/DC 24 V → 5 V, 1 A | 1 | 7,00 € | 7,00 € |
| AP2112K-3.3 | LDO 3,3 V 600 mA | 1 | 0,60 € | 0,60 € |
| SMBJ33A | Diode TVS, protection alimentation 24 V | 2 | 0,50 € | 1,00 € |
| — | **Réservoir capacitif** pour les pics d'émission du modem (2 A) | 1 | 3,00 € | 3,00 € |
| — | Résistances 1 %, découplages, LED d'état | lot | — | 8,00 € |
| — | ILS (reed) + aimant, ouverture du Wi-Fi de maintenance | 1 | 2,00 € | 2,00 € |
| — | SUB-D 25 mâle et femelle, coudés CI | 2 | 3,00 € | 6,00 € |
| — | PCB 4 couches ~120 × 100 mm (série de 5) | 1 | 12,00 € | 12,00 € |
| — | Boîtier, fixation, presse-étoupes, connecteur de programmation | 1 | 28,00 € | 28,00 € |
| **Sous-total hors interface bus** | | | | **≈ 111 €** |

### 1.1 Interface bus — le choix conditionne le routage

`profiles/default.yaml` → `bus.driver_variant`. Le logiciel supporte les trois,
**le PCB n'en supportera qu'une**.

| Variante | Composants | Coût | Pose des 22 lignes |
|---|---|---:|---|
| **`shift595`** — recommandé | 3× 74HC595 + 3× 74HC165 | **~3,00 €** | ~3 µs, **strictement simultanée** (latch `RCLK` commun) |
| `mcp23017` | 4× MCP23017-E/SP | ~10,00 € | ~150 µs, décalage GPIOA/GPIOB ~25 µs |
| `mega_uart` | ATmega2560 + quartz + passifs | ~12,00 € | < 1 µs, mais un aller-retour UART de ~0,3 ms avant |

Le moins cher est aussi le plus rapide et le plus simple à router : **6 boîtiers
sur un seul bus SPI, contre 4 sur I²C avec adressage**.

**Sous-total carte AGV (variante `shift595`) : ≈ 114 €**
**Sous-total carte AGV (variante `mcp23017`, chiffrage historique) : ≈ 121 €**

---

## 2. Poste fixe — deux options

### 2.1 Option A — Poste ESP32 (recommandée, 156 €)

Suffit dès lors que l'historique long terme n'est pas exigé.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU, 8 Mo flash (LittleFS + pages web) | 1 | 5,00 € | 5,00 € |
| SIM7080G | Modem LTE-M / NB-IoT | 1 | 18,00 € | 18,00 € |
| — | Antenne LTE déportée + pigtail | 1 | 12,00 € | 12,00 € |
| TCM 515 (EU 868 MHz) | Récepteur EnOcean, UART ESP3 | 1 | 28,00 € | 28,00 € |
| — | Antenne EnOcean déportée | 1 | 8,00 € | 8,00 € |
| W5500 (module) | Ethernet SPI + RJ45 magnétique — **liaison filaire** | 1 | 6,00 € | 6,00 € |
| — | LED d'accusé bicolore, LED de vie, boutons appairage et reset | 1 | 3,50 € | 3,50 € |
| MEAN WELL HDR-15-24 | Alimentation rail DIN 230 V → 24 V 15 W | 1 | 14,00 € | 14,00 € |
| TSR 1-2450 + AP2112K | 24 V → 5 V → 3,3 V | 1 | 8,00 € | 8,00 € |
| — | PCB 2 couches ~100 × 80 mm | 1 | 6,00 € | 6,00 € |
| — | Boîtier mural IP54, presse-étoupes, embases SMA | 1 | 30,00 € | 30,00 € |
| — | Support SIM, passifs | 1 | 3,00 € | 3,00 € |
| — | Câble Ethernet blindé | 1 | 6,00 € | 6,00 € |
| **Sous-total poste ESP32** | | | | **≈ 148 €** |

### 2.2 Option B — Poste UniPi E413 (439 €)

À retenir seulement si un **historique consultable sur plusieurs semaines** est
demandé, ou si le client impose un automate référencé.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| UniPi E413 (variante LTE) | Automate compact Linux, E/S TOR, modem LTE intégré | 1 | 350,00 € | 350,00 € |
| — | Antenne LTE externe déportée | 1 | 15,00 € | 15,00 € |
| TCM 515 + antenne | Récepteur EnOcean | 1 | 36,00 € | 36,00 € |
| — | Coffret rail DIN, alimentation, bornier | 1 | 34,00 € | 34,00 € |
| — | Câble Ethernet blindé | 1 | 4,00 € | 4,00 € |
| **Sous-total poste UniPi** | | | | **≈ 439 €** |

⚠️ §12.9 : **vérifier le runtime livré** avant commande. Sous Mervis, le service
Python de ce dépôt est sans objet et l'intégration passe par Modbus TCP depuis
un autre hôte — soit une architecture différente à rechiffrer.

---

## 3. Boutons d'appel EnOcean — 100 € (2 stations)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| PTM 210 (EU 868 MHz) | Module émetteur auto-alimenté, **sans pile** | 2 | 30,00 € | 60,00 € |
| — | Enveloppe / poussoir mural compatible PTM 210 | 2 | 12,00 € | 24,00 € |
| — | Plaque de repérage station gravée | 2 | 4,00 € | 8,00 € |
| — | Fixation, visserie | 2 | 4,00 € | 8,00 € |
| **Sous-total 2 boutons** | | | | **≈ 100 €** |

### 3.1 Accusé opérateur — conditionnel, 0 à 160 €

Le TCM 515 est en **réception seule** : aucun retour vers le bouton. Voir
[`docs/questions_ouvertes.md`](docs/questions_ouvertes.md) §12.8.

| Option | Coût (2 postes) |
|---|---:|
| Aucun retour | 0 € |
| Voyant câblé au poste (LED d'accusé déjà prévue au §2.1) | 0 € |
| Actionneur EnOcean par point d'appel | 100 à 160 € |

---

## 4. Abonnements et infrastructure

| Poste | Coût |
|---|---:|
| 2 SIM M2M data LTE-M, ~1,50 €/mois | **36 €/an** |
| Broker MQTT — VPS mutualisé | **60 €/an** |
| *Alternative* : Mosquitto sur un serveur usine existant | 0 €/an |
| **Total récurrent** | **≈ 100 €/an** |

Héberger le broker sur un serveur du client supprime le poste VPS **et** la
dépendance à un tiers. À proposer systématiquement.

---

## 5. Outillage — 45 €, non récurrent

| Désignation | Prix | Usage |
|---|---:|---|
| Smartphone en mode ingénieur ou testeur de couverture | 0 € | Relevé RSRP/RSRQ en tous points — **prérequis éliminatoire** |
| SIM de test opérateur (prêt commercial) | 0 € | Essai de latence réel sur 200 échanges |
| Analyseur logique 8 voies | 15 € | Chronogrammes X/Y, mesure de `t_setup` |
| Adaptateur USB-série 3,3 V | 6 € | Mise au point ESP32 et pile AT |
| Oscilloscope | — | Supposé disponible. Amplitude `Y05`, **prérequis bloquant** |
| Jeu de cosses, pince à sertir, consommables | 24 € | Câblage |
| **Total outillage** | **≈ 45 €** | |

---

## 6. Récapitulatif — variante B (LTE-M / MQTT)

| Poste | Poste ESP32 | Poste UniPi |
|---|---:|---:|
| Carte AGV (variante `shift595`) | 114 € | 114 € |
| Poste fixe | 148 € | 439 € |
| 2 boutons EnOcean | 100 € | 100 € |
| Outillage | 45 € | 45 € |
| **Total matériel** | **≈ 407 €** | **≈ 698 €** |

| | Montant |
|---|---:|
| **Chaque station supplémentaire** | **+ 50 €** |
| Avec accusé EnOcean par station | + 130 € |

### 6.1 Coût sur 10 ans

| | Poste ESP32 | Poste UniPi |
|---|---:|---:|
| Matériel initial | 407 € | 698 € |
| Récurrent (100 €/an) | 1 000 € | 1 000 € |
| **Total 10 ans** | **≈ 1 407 €** | **≈ 1 698 €** |

---

## 7. Variante A — SMS, pour mémoire

Chiffrée pour la comparaison. **Ne pas déployer** en liaison principale : ni
latence bornée, ni ordre de remise, ni garantie de remise.

| Poste | Montant |
|---|---:|
| Poste UniPi E413 (variante LTE) + antenne + boutons filaires + câblage | 439 € |
| Carte AGV avec modem SIM7600E-H (Cat-1, plus gourmand) | 141 € |
| Outillage | 45 € |
| **Total matériel** | **≈ 625 €** |

| Récurrent | Annuel |
|---|---:|
| 2 abonnements SIM M2M | 120 à 240 € |
| Volume SMS (appels + accusés + télémétrie dégradée) | 1 000 à 1 300 € |
| **Total** | **≈ 1 500 €/an** |
| **Sur 10 ans** | **≈ 15 625 €** tout compris |

Le surcoût sur dix ans face à la variante B est de **~14 000 €**, pour un
service strictement inférieur. C'est l'argument chiffré à opposer si le SMS est
demandé.

---

## 8. Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| PCB 4 couches + assemblage | 3 à 5 semaines | **Chemin critique matériel** |
| SIM7080G | 2 à 4 semaines | Tensions récurrentes sur les modules cellulaires |
| SIM M2M data LTE-M | 1 à 3 semaines | Contractuel, pas technique |
| UniPi E413 (si option B) | 2 à 6 semaines | Vérifier la référence **et le runtime** |
| PTM 210 / TCM 515 | 1 à 2 semaines | Faible |
| ESP32, 74HC595/165, PC847 | stock | Faible |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : un seul
point d'arrêt sous −110 dBm disqualifie l'architecture, et le choix de variante
d'interface bus conditionne le routage du PCB.

---

## 9. Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 6 à 9 jours-homme de mise en œuvre, hors développement.
- **Le contrôle de l'obsolescence 2G/3G** : le SIM7080G est LTE-M/NB-IoT, donc
  hors calendrier d'extinction. Un module Cat-1 le serait aussi ; un module 2G
  ne le serait pas.
- **La carte de rechange** : ~114 € pour disposer d'un échange standard.
  Recommandé sur un équipement de production.
