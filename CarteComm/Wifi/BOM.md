# Nomenclature — architecture Wi-Fi (carte V5.0.1 conservée)

> Prix indicatifs **HT, petites quantités, 2026**. À reconsulter au moment de
> l'achat : les modules radio et les automates compacts bougent vite.
>
> Hypothèse de dimensionnement : **2 points d'appel**. Le coût par station
> supplémentaire est donné en §5.
>
> Procédure de mise en œuvre : [`DEPLOY.md`](DEPLOY.md).
> Source des choix matériels : [`docs/Planification_Architecture_WiFi_AGV.md`](docs/Planification_Architecture_WiFi_AGV.md) §11.

---

## Ce qui fait la particularité de cette nomenclature

**La carte AGV coûte 0 €.** C'est le seul intérêt économique décisif de cette
architecture : la V5.0.1 est conservée, seuls ses deux firmwares sont réécrits.
Tout le budget matériel part dans le poste fixe et les boutons.

En contrepartie, deux postes que les autres architectures n'ont pas :

- le **harnais de raccordement** (nappes, connecteurs IDC, cosses) — la carte
  existe, mais son câblage vers l'automate est à refaire ;
- une **adaptation de niveaux conditionnelle**, tant que l'amplitude des lignes
  `Y` n'est pas mesurée (point bloquant W1b, voir §4).

---

## 1. Carte AGV — 0 €

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| AIO AGV Control V5.0.1 | Carte existante, **conservée** — ESP32-WROOM-32E + ATmega2560 | 1 | 0 € | **0 €** |

**Sous-total : 0 €**

Aucun composant n'est ajouté sur la carte. Le L7806CV, l'alimentation 24 V →
6 V et les deux microcontrôleurs sont ceux d'origine.

### 1.1 Harnais de raccordement — 42 €

Deux nappes, une par sens. Détail du câblage :
[`docs/subd25_atmega.md`](docs/subd25_atmega.md).

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| — | Nappe 25 conducteurs, gaine souple, ~1 m | 2 | 6,00 € | 12,00 € |
| — | Connecteur IDC SUB-D 25 **mâle** (entrées) | 1 | 4,00 € | 4,00 € |
| — | Connecteur IDC SUB-D 25 **femelle** (sorties) | 1 | 4,00 € | 4,00 € |
| — | Capots métallisés SUB-D 25 avec serre-câble | 2 | 3,50 € | 7,00 € |
| — | Cosses à sertir côté AGV (CN61 à CN64) | 50 | 0,15 € | 7,50 € |
| — | Gaine tressée, colliers, repérage des fils | lot | — | 7,50 € |
| **Sous-total harnais** | | | | **≈ 42 €** |

### 1.2 Antenne Wi-Fi déportée — 30 €

L'antenne d'origine du module ESP32 émet depuis l'intérieur d'un châssis
métallique. Une antenne déportée n'est pas un confort.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| — | Antenne 2,4 GHz 2 dBi, embase SMA, montage déporté | 1 | 18,00 € | 18,00 € |
| — | Pigtail U.FL → SMA femelle + passe-cloison | 1 | 8,00 € | 8,00 € |
| — | Support de fixation, visserie | 1 | 4,00 € | 4,00 € |
| **Sous-total antenne** | | | | **≈ 30 €** |

⚠️ Vérifier au démontage que le module ESP32 de la V5.0.1 dispose bien d'un
connecteur U.FL. Un module à antenne PCB seule impose de le remplacer (~5 €) ou
de renoncer à l'antenne déportée.

---

## 2. Poste fixe UniPi — 461 €

C'est le poste le plus lourd de cette architecture. Il porte le récepteur
EnOcean, le broker MQTT et l'interface de supervision.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| UniPi E413 | Automate compact Linux, 1 Go RAM / 8 Go eMMC, E/S TOR | 1 | 375,00 € | 375,00 € |
| TCM 515 (EU 868 MHz) | Récepteur EnOcean, interface UART ESP3 | 1 | 28,00 € | 28,00 € |
| — | Antenne EnOcean 868 MHz déportée + pigtail | 1 | 10,00 € | 10,00 € |
| — | Adaptateur USB-série ou raccordement UART vers le TCM 515 | 1 | 8,00 € | 8,00 € |
| MEAN WELL HDR-15-24 | Alimentation rail DIN 230 V → 24 V 15 W | 1 | 14,00 € | 14,00 € |
| — | Coffret rail DIN, bornier, presse-étoupes | 1 | 20,00 € | 20,00 € |
| — | Câble Ethernet blindé vers le réseau usine | 1 | 6,00 € | 6,00 € |
| **Sous-total poste fixe** | | | | **≈ 461 €** |

### 2.1 Alternative à considérer

La planification pose la question (§12.2) : **historique ou état instantané ?**

Si un historique consultable sur plusieurs semaines n'est **pas** attendu, un
ESP32 avec module Ethernet remplit la même fonction pour ~40 € au lieu de 461 €
— soit **420 € d'économie**. L'UniPi ne se justifie que par la persistance,
l'interface web riche et le fait d'héberger le broker.

| Alternative | Coût | Perd |
|---|---:|---|
| ESP32 + W5500 + TCM 515 + alim + boîtier | ~85 € | historique long, broker local (à déporter sur un VPS ~60 €/an) |

À trancher avec le client **avant la commande**.

---

## 3. Boutons d'appel EnOcean — 100 € (2 stations)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| PTM 210 (EU 868 MHz) | Module émetteur auto-alimenté, **sans pile** | 2 | 30,00 € | 60,00 € |
| — | Enveloppe / poussoir mural compatible PTM 210 | 2 | 12,00 € | 24,00 € |
| — | Plaque de repérage station gravée | 2 | 4,00 € | 8,00 € |
| — | Fixation, visserie, adhésif industriel | 2 | 4,00 € | 8,00 € |
| **Sous-total 2 boutons** | | | | **≈ 100 €** |

> **Alternative prête à l'emploi** : un interrupteur EnOcean du commerce (NodOn,
> Trio2Sys, Eltako) évite l'assemblage pour ~45 à 60 € tout compris. À
> privilégier si le volume est faible — le gain de temps dépasse l'écart de prix.

### 3.1 Accusé opérateur — conditionnel, 0 à 160 €

⚠️ **Le TCM 515 est en réception seule** : aucun accusé ne revient vers le
bouton. Si un retour visuel à l'opérateur est exigé, il faut budgéter :

| Option | Coût (2 postes) | Remarque |
|---|---:|---|
| Aucun retour | 0 € | l'opérateur ne sait pas si son appel est parti |
| Voyant câblé au poste fixe | ~30 € | simple, mais l'opérateur doit voir le poste |
| Actionneur EnOcean par point d'appel | 100 à 160 € | retour local, sans câblage |

**À trancher avant de figer l'IHM** (§12.8). Ce n'est pas un détail : c'est la
principale faiblesse ergonomique des boutons sans pile.

---

## 4. Adaptation de niveaux — CONDITIONNEL, 0 à 45 €

⚠️ **Poste chiffré à titre conservatoire, en attente de mesure.**

Le relevé de câblage amène les 21 lignes `Y` **directement** sur des broches de
l'ATmega, sans protection. Leur amplitude n'est pas connue (point bloquant W1b,
phase 1 de [`DEPLOY.md`](DEPLOY.md)).

| Résultat de la mesure sur `Y05` | Matériel à ajouter | Coût |
|---|---|---:|
| ≤ V_CC de l'ATmega | rien | **0 €** |
| Légèrement au-dessus | 21 diviseurs résistifs 1 % | ~8 € |
| 24 V ou niveau incompatible | 6× optocoupleurs quadruples PC847 + résistances + carte fille | ~45 € |

Si le troisième cas se présente, il faut aussi prévoir **une carte fille et son
routage** — soit un délai de plusieurs semaines, pas seulement un coût.

---

## 5. Récapitulatif et coût par station supplémentaire

| Poste | Montant |
|---|---:|
| Carte AGV (conservée) | 0 € |
| Harnais de raccordement | 42 € |
| Antenne Wi-Fi déportée | 30 € |
| Poste fixe UniPi | 461 € |
| 2 boutons EnOcean | 100 € |
| Adaptation de niveaux (conservatoire) | 0 à 45 € |
| Outillage (§6) | 105 € |
| **Total** | **≈ 738 à 783 €** |

L'outillage est compté dans le total, comme dans les nomenclatures des autres
architectures — sinon la comparaison de [`../README.md`](../README.md) serait
faussée de 105 €.

| | Montant |
|---|---:|
| **Chaque station supplémentaire** | **+ 50 €** (PTM 210 + enveloppe + repérage + fixation) |
| Avec accusé EnOcean par station | + 130 € |

### 5.1 Coûts récurrents

| Poste | Annuel |
|---|---:|
| Aucun abonnement opérateur | **0 €** |
| Infrastructure Wi-Fi | à la charge du client (existante) |
| Piles des boutons | **0 €** — les PTM 210 sont auto-alimentés |
| **Total récurrent** | **0 €/an** |

### 5.2 Coût sur 10 ans

| | Montant |
|---|---:|
| Matériel et outillage | ~738 € |
| Récurrent sur 10 ans | 0 € |
| **Total 10 ans** | **≈ 738 €** |

À comparer aux ~15 625 € de l'architecture SMS, ou aux ~1 407 € de la variante
LTE-M — voir [`../SMS_EnOcean/BOM.md`](../SMS_EnOcean/BOM.md).

---

## 6. Outillage — 105 €, non récurrent

| Désignation | Prix | Usage |
|---|---:|---|
| Programmateur ISP (USBasp ou équivalent) | 8 € | Sauvegarde puis flash de l'ATmega — **phase 2 de `DEPLOY.md`, irréversible sans lui** |
| Adaptateur USB-série 3,3 V | 6 € | ESP32, et espionnage de la liaison inter-MCU |
| Analyseur logique 8 voies | 15 € | Chronogrammes X/Y, mesure de `t_setup` |
| Multimètre | — | Supposé disponible. Mesures de la phase 1 |
| Oscilloscope | — | Supposé disponible. **Prérequis bloquant** : amplitude `Y05` |
| Jeu de cosses + pince à sertir | 45 € | Confection du harnais |
| Kit outillage réseau (testeur RJ45, sertisseuse) | 31 € | Raccordement du poste |
| **Total outillage** | **≈ 105 €** | |

---

## 7. Risques d'approvisionnement et délais

| Élément | Délai typique | Risque |
|---|---|---|
| UniPi E413 | 2 à 6 semaines | **Chemin critique matériel.** Vérifier la référence exacte et le runtime livré (§12.9) avant commande |
| PTM 210 / TCM 515 | 1 à 2 semaines | Faible, largement distribués |
| Carte AGV | — | Aucun : elle existe |
| Adaptation de niveaux | 3 à 5 semaines **si nécessaire** | Conditionnel à la mesure W1b — d'où l'urgence de la faire |
| Points d'accès Wi-Fi additionnels | variable | À la charge du client, dépend du relevé de couverture 0.2 |

**Ne rien commander avant la phase 1 de [`DEPLOY.md`](DEPLOY.md)** : le relevé de
couverture Wi-Fi peut disqualifier l'architecture, et la mesure d'amplitude
change la nomenclature.

---

## 8. Ce que cette nomenclature ne couvre pas

- **La main-d'œuvre** : 3 à 5 jours-homme de mise en œuvre, hors développement.
- **Les points d'accès Wi-Fi additionnels**, si le relevé de couverture en
  révèle le besoin. À la charge du client, et potentiellement le poste le plus
  lourd de tout le projet.
- **La carte de rechange** : si la sauvegarde des firmwares d'origine échoue
  (phase 2), il n'y a plus de retour arrière. Prévoir une V5.0.1 de rechange
  devient alors une assurance à chiffrer avec le client.
