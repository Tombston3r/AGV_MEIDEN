# SUB-D 25 ↔ ATmega2560 : table de câblage RELEVÉE

> **Source** : relevé de câblage fourni par le client. Ce n'est plus une
> hypothèse : c'est la table de référence.
> Source exécutable : [`../firmware/mega/src/board_ports.h`](../firmware/mega/src/board_ports.h).
> Toute correction se fait là, puis se recopie ici.

## Confection des nappes

Deux nappes de 25 fils, une par sens.

| | Connecteur côté MEGA | Côté AGV |
|---|---|---|
| **Entrées** (AGV → carte) | IDC SUB-D 25 **mâle** | cosses serties, CN62 / CN63 / CN64 |
| **Sorties** (carte → AGV) | IDC SUB-D 25 **femelle** | cosses serties, CN61 / CN62 / CN63 |

Les broches 1 et 14 de chaque SUB-D sont les masses (0 V).

---

## Alimentation : le rôle réel du L7806CV

Analyse du PCB : les fils 24 et 25 de la nappe d'entrée amènent **RP24B (24 V)**
depuis **CN64 A6 et B6**. Un **L7806CV** abaisse ce 24 V à **6 V**, qui alimente
l'**ATmega2560**.

Le 6 V est donc une **tension d'alimentation**, pas un niveau de signal. Il ne
dit rien, ni sur l'amplitude des lignes Y, ni sur ce qu'attendent les entrées de
l'automate.

### ⚠️ Point à vérifier sur le PCB : où arrive exactement ce 6 V ?

Trois cas, aux conséquences très différentes :

| Le 6 V arrive sur… | Conséquence |
|---|---|
| **V_CC de l'ATmega, directement** | 6,0 V est la **valeur maximale absolue** du datasheet, et la plage recommandée s'arrête à 5,5 V (4,5–5,5 V à 16 MHz). Un L7806 sort typiquement 5,76 à 6,24 V : le microcontrôleur peut donc fonctionner **au-delà de son maximum absolu** |
| Un second régulateur 5 V | Cas sain, rien à signaler |
| `Vin` d'une carte Arduino MEGA | 6 V est **sous** le minimum recommandé (7 V) pour le régulateur embarqué : avec ~1,1 V de chute, le rail 5 V peut s'affaisser |

**À mesurer** : la tension réelle sur la broche V_CC de l'ATmega, carte sous
tension. C'est une mesure de trente secondes qui écarte une question de
fiabilité à long terme.

### Ce que ça pourrait vouloir dire : hypothèse à confirmer

Si le 6 V alimente bien directement l'ATmega, c'est peut-être **délibéré** :
les sorties du microcontrôleur suivent V_CC, donc les lignes X sortiraient
naturellement du 0–6 V, et les entrées Y toléreraient jusqu'à ~6,5 V. Le
concepteur d'origine aurait ainsi adapté les niveaux du bus MEIDEN sans étage
de conversion.

C'est cohérent, mais ce n'est **pas** établi. Deux mesures le confirment ou
l'infirment : la tension V_CC, et l'amplitude d'une ligne Y.

---

## ⚠️ Niveaux du bus : ce qui reste inconnu

Le rôle du L7806 étant élucidé, **plus rien ne renseigne sur les niveaux du
bus** :

- **amplitude des lignes Y** (§12.1) : toujours inconnue. Le relevé les amène
  directement sur des broches d'ATmega. Mesurer sur `Y05` avant de brancher la
  nappe d'entrées, au-delà de V_CC + 0,5 V, l'entrée est détruite ;
- **topologie des entrées de l'automate** : tirées à une tension quelconque
  (la carte devrait alors seulement tirer à la masse), ou attendant un courant
  fourni par la carte ? Non déterminé.

### ✅ La topologie de sortie, elle, est connue

Le projet KiCad de la carte
([`../../../materiel/AIO_AGV_Control_V5.0.1/`](../../../materiel/AIO_AGV_Control_V5.0.1/))
montre **23 MOSFET N canal IRF520** en TO-220, attaqués par des résistances de
grille de 1 kΩ. Le collecteur ouvert est donc réalisé **par le matériel** : le
MOSFET tire la ligne de l'automate à la masse et ne sort jamais de niveau haut.

Conséquence sur le firmware : le microcontrôleur pilote une **grille**, pas la
ligne de l'automate. Il doit être en **sortie poussée**
(`bus.x_open_drain: false`, valeur du profil). Le mode collecteur ouvert côté
microcontrôleur laisserait la grille **flottante** à l'état actif : un MOSFET à
grille flottante peut conduire partiellement, ce qui est le pire état possible
sur un étage de puissance.

L'inversion est faite par le MOSFET : microcontrôleur à l'état haut → MOSFET
passant → ligne automate tirée à 0 V.

⚠️ **23 MOSFET pour 22 voies X** : `T13` est absent de la numérotation. À
vérifier au schéma : voie de réserve, ou signal supplémentaire non identifié.

Le mode collecteur ouvert reste implémenté et testé : il redevient le mode sûr
pour toute carte dépourvue d'étage de sortie.

---

## Ce que le relevé confirme

Deux points ouverts du brief sont **tranchés** par cette table :

- **§12.2, brochage SUB-D** : les connecteurs AGV sont CN61 à CN64, et non le
  jeu CN61/62/63 seul. La divergence des deux tables qui circulaient est levée.
- **§12.6, ordre des bits** : les libellés constructeur donnent les poids sans
  ambiguïté (`Station/Marker x1` … `x512`, `Speed x1` … `x8`). L'ordre codé dans
  `kDefaultLayout` est confirmé, il n'est plus provisoire.

---

## Entrées : SUB-D 25 mâle (AGV → carte)

| Fil | SUB-D | MEGA | Port AVR | Signal | Connecteur | Broche | Description |
|---:|---:|---|---|---|---|---|---|
| 1 | 1 | GND | - | 0 V | CN62 | B2 | masse |
| 2 | 14 | GND | - | 0 V | CN62 | A2 | masse |
| 3 | 2 | D11 | **PB5** | `Y03` | CN62 | B3 | défaut (error lamp flag) |
| 4 | 15 | D15 | **PJ0** | `Y05` | CN62 | A3 | moving flag |
| 5 | 3 | D9 | **PH6** | `Y10` | CN63 | B1 | in station flag |
| 6 | 16 | D17 | **PH0** | `Y11` | CN63 | A1 | vitesse courante bit 1 |
| 7 | 4 | D7 | **PH4** | `Y12` | CN63 | B2 | vitesse courante bit 2 |
| 8 | 17 | D19 | **PD2** | `Y13` | CN63 | A2 | vitesse courante bit 3 |
| 9 | 5 | D5 | **PE3** | `Y14` | CN63 | B3 | vitesse courante bit 4 |
| 10 | 18 | D21 | **PD0** | `Y15` | CN63 | A3 | écho aiguillage |
| 11 | 6 | D3 | **PE5** | `Y20` | CN63 | B14 | écho sens (avant/arrière) |
| 12 | 19 | D23 | **PA1** | `Y21` | CN63 | A14 | pas de destination programmée |
| 13 | 7 | D2 | **PE4** | `Y22` | CN63 | B15 | instruction reading complete |
| 14 | 20 | A1 | **PF1** | `Y23` | CN63 | A15 | position ×1 |
| 15 | 8 | D4 | **PG5** | `Y24` | CN63 | B16 | position ×2 |
| 16 | 21 | A3 | **PF3** | `Y25` | CN63 | A16 | position ×4 |
| 17 | 9 | D6 | **PH3** | `Y26` | CN63 | B17 | position ×8 |
| 18 | 22 | A5 | **PF5** | `Y27` | CN63 | A17 | position ×16 |
| 19 | 10 | D8 | **PH5** | `Y30` | CN64 | B1 | position ×32 |
| 20 | 23 | A7 | **PF7** | `Y31` | CN64 | A1 | position ×64 |
| 21 | 11 | D10 | **PB4** | `Y32` | CN64 | B2 | position ×128 |
| 22 | 24 | A9 | **PK1** | `Y33` | CN64 | A2 | position ×256 |
| 23 | 12 | D12 | **PB6** | `Y34` | CN64 | B3 | position ×512 |
| 24 | 25 | 24 V → L7806CV → 6 V | - | RP24B | CN64 | A6 | **alimentation de l'ATmega** |
| 25 | 13 | 24 V → L7806CV → 6 V | - | RP24B | CN64 | B6 | **alimentation de l'ATmega** |

Rappel de numérotation **octale** : après `Y27` vient `Y30`. La plage
`Y23`…`Y34` compte **10** signaux, pas 12, et les libellés `×1` à `×512` le
confirment.

## Sorties : SUB-D 25 femelle (carte → AGV)

| Fil | SUB-D | MEGA | Port AVR | Signal | Connecteur | Broche | Description |
|---:|---:|---|---|---|---|---|---|
| 1 | 1 | GND | - | 0 V | CN61 | B7 | masse |
| 2 | 14 | GND | - | 0 V | CN61 | A7 | masse |
| 3 | 2 | D39 | **PG2** | `X96` | CN61 | B6 | destination ×1 |
| 4 | 15 | D49 | **PL0** | `X97` | CN61 | A6 | destination ×2 |
| 5 | 3 | D33 | **PC4** | `X94` | CN61 | B5 | type de donnée (station ou marqueur) |
| 6 | 16 | D50 | **PB3** | `X95` | CN61 | A5 | frein externe |
| 7 | 4 | D26 | **PA4** | `X92` | CN61 | B4 | instruction data input switch |
| 8 | 17 | D47 | **PL2** | `X93` | CN61 | A4 | write input data switch (strobe) |
| 9 | 5 | D31 | **PC6** | `X90` | CN61 | B3 | vitesse ×4 |
| 10 | 18 | D46 | **PL3** | `X91` | CN61 | A3 | vitesse ×8 |
| 11 | 6 | D41 | **PG0** | `XA0` | CN62 | B6 | destination ×4 |
| 12 | 19 | D44 | **PL5** | `XA1` | CN62 | A6 | destination ×8 |
| 13 | 7 | D35 | **PC2** | `XA2` | CN62 | B7 | destination ×16 |
| 14 | 20 | D45 | **PL4** | `XA3` | CN62 | A7 | destination ×32 |
| 15 | 8 | D28 | **PA6** | `XA4` | CN62 | B8 | destination ×64 |
| 16 | 21 | D37 | **PC0** | `XA5` | CN62 | A8 | destination ×128 |
| 17 | 9 | D29 | **PA7** | `XA6` | CN62 | B9 | destination ×256 |
| 18 | 22 | D24 | **PA2** | `XA7` | CN62 | A9 | destination ×512 |
| 19 | 10 | D43 | **PL6** | `X82` | CN63 | B5 | standby release (départ) |
| 20 | 23 | D25 | **PA3** | `X83` | CN63 | A5 | standby stop |
| 21 | 11 | D22 | **PA0** | `X84` | CN63 | B6 | changement d'aiguillage |
| 22 | 24 | D51 | **PB2** | `X85` | CN63 | A6 | changement de sens |
| 23 | 12 | D30 | **PC7** | `X86` | CN63 | B7 | vitesse bit 1 |
| 24 | 25 | D48 | **PL1** | `X87` | CN63 | A7 | vitesse bit 2 |
| 25 | 13 | D27 | PA5 | - | : | - | **non connecté** |

---

## Répartition sur les ports : ce qui contraint le firmware

Les 43 signaux occupent **onze ports**, par bits épars.

| Port | Sorties X | Entrées Y | |
|---|---|---|---|
| **PORTA** | bits 0, 2, 3, 4, 6, 7 | **bit 1** (`Y21`) | ⚠️ **mixte** |
| **PORTB** | bits 2, 3 | **bits 4, 5, 6** | ⚠️ **mixte** |
| PORTC | bits 0, 2, 4, 6, 7 | - | |
| PORTD | - | bits 0, 2 | |
| PORTE | - | bits 3, 4, 5 | |
| PORTF | - | bits 1, 3, 5, 7 | |
| **PORTG** | bits 0, 2 | **bit 5** (`Y24`) | ⚠️ **mixte** |
| PORTH | - | bits 0, 3, 4, 5, 6 | |
| PORTJ | - | bit 0 | |
| PORTK | - | bit 1 | |
| PORTL | bits 0 à 6 | - | |

### Conséquence 1 : trois ports mixtes

`PORTA`, `PORTB` et `PORTG` portent des sorties X **et** des entrées Y. Toute
écriture de `DDRx` ou de `PORTx` doit être **masquée**. Un `DDRA = 0xFF`
mettrait `D23` (`Y21`) en sortie face à une sortie de l'automate : conflit
électrique franc.

Le driver applique donc partout un `|= masque_X` / `&= ~masque_Y`, et une
lecture-modification-écriture sur les données. C'est vérifié par le test
`avr_les_ports_mixtes_ne_voient_pas_leur_direction_ecrasee`.

### Conséquence 2 : la pose n'est plus strictement simultanée

Les sorties occupent 5 ports → **5 écritures** par pose complète, ~0,3 µs à
16 MHz. Les 10 bits d'adresse s'étalent sur 4 ports (`PORTA`, `PORTC`, `PORTG`,
`PORTL`) → ~0,25 µs entre le premier et le dernier bit posé.

Ce n'est pas la simultanéité stricte d'un `PORTA = x`, mais :

| Interface | Pose des 22 lignes | Écart entre premier et dernier bit |
|---|---|---|
| **Ce câblage** | ~0,3 µs | ~0,25 µs |
| 4× MCP23017 (I²C) | ~150 µs | ~25 µs |
| `t_setup` attendu | - | 200 µs (PROVISOIRE §12.4) |

L'écart résiduel est **800 fois plus petit** que le `t_setup` provisoire. Il
reste à confirmer au banc que l'automate ne voit rien passer d'incohérent : la
mesure se lit dans `port_writes_per_pose()` et à l'analyseur logique.

---

## Contrôle du câblage avant mise en service

Même avec une table relevée, un contrôle s'impose : une nappe peut être sertie à
l'envers, et un mot d'adresse mal câblé **n'échoue pas** : il envoie l'AGV à la
mauvaise station.

**Automate DÉBRANCHÉ. Les deux SUB-D 25 déconnectés.**

1. Flasher le firmware MEGA (`pio run -e mega -t upload`).
2. Console série USB à 115 200 bauds.
3. Envoyer `d` → le firmware annonce `# DECOUVERTE bit X=0` puis change d'index
   toutes les 3 secondes, une seule sortie active à la fois.
4. Pour chaque index, vérifier au multimètre que la broche SUB-D attendue par la
   table ci-dessus est bien celle qui bouge.
5. Envoyer `n` pour revenir en mode normal.

Pour les entrées, forcer un signal Y depuis l'automate en mode manuel et lire
l'état renvoyé par la commande `GetState` de la liaison série
(`tools/decode_link.py` décode la trame).

Toute divergence se corrige dans
[`board_ports.h`](../firmware/mega/src/board_ports.h), puis `make test`.
