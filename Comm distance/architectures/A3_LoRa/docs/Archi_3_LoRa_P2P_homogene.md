# Architecture 1 : LoRa 868 MHz point-à-point privé (solution homogène)

> Document de référence : projet « Remplacement de la carte AIO AGV Control V5.0.1 »
> AGV MEIDEN à guidage magnétique, appel depuis boutons déportés vers points d'arrêt.

---

## 1. Fonction

Assurer la liaison radio bidirectionnelle **directe** entre les boutons d'appel et l'AGV, sans poste intermédiaire, sans infrastructure réseau et sans opérateur externe. Une seule technologie radio pour tout le système : LoRa 868 MHz en mode point-à-point privé.

Trois flux fonctionnels :

1. **Appel** : bouton *n* → AGV, « va à la station *n* »
2. **Accusé** : AGV → bouton, « ordre reçu, mis en file / refusé »
3. **Télémétrie** : AGV → tous les boutons (broadcast périodique), position, vitesse, défaut, batterie

---

## 2. Architecture générale

```
   BOUTON 1 (station 1)                                    AGV MEIDEN
 ┌────────────────────────┐                     ┌──────────────────────────────────┐
 │ Bouton IP65            │                     │  ESP32-WROOM-32E                 │
 │      │                 │                     │       │ SPI                      │
 │  STM32L071 (ou ESP32)  │                     │   RFM95W (SX1276) 868 MHz        │
 │      │ SPI             │                     │       │                          │
 │  RFM95W ── ANT 868 ────┼───────))) (((───────┼─── ANT 868 (déportée sur mât)    │
 │      │                 │                     │       │ I²C                      │
 │  LED verte / rouge     │                     │  4× MCP23017 (64 GPIO)           │
 │  Pile Li-SOCl₂ 3,6 V   │                     │       │                          │
 └────────────────────────┘                     │  Étage d'isolation optocouplée   │
                                                 │   ├─ 22 sorties X (carte → API)  │
   BOUTON 2 (station 2)                          │   └─ 21 entrées Y (API → carte)  │
 ┌────────────────────────┐                     │       │                          │
 │        idem            │────))) (((──────────┤  SUB-D 25 M / SUB-D 25 F         │
 └────────────────────────┘                     │       │                          │
                                                 │  Automate MEIDEN (CN61→CN64)     │
   BOUTON n … (extensible)                       └──────────────────────────────────┘
```

**Point clé** : l'ESP32 embarqué reprend **l'intégralité** du rôle de l'ATmega2560 d'origine (séquenceur temps réel des bus X/Y, file de courses, décodeur de position) **et** le rôle radio de l'ESP32 d'origine. Un seul microcontrôleur remplace les deux.

---

## 3. Fonctionnement détaillé

### 3.1 Couche radio

| Paramètre | Valeur retenue | Justification |
|---|---|---|
| Fréquence | 869,525 MHz (sous-bande g3) ou 868,1 MHz (g1) | g3 autorise 10 % de rapport cyclique et 500 mW ERP → marge confortable ; g1 limitée à 1 % / 25 mW |
| Modulation | LoRa (CSS) | Robustesse au bruit impulsionnel des variateurs / soudeuses |
| Spreading Factor | SF9 | Compromis portée / temps d'antenne. SF7 si portée < 150 m confirmée |
| Bande passante | 125 kHz | Standard |
| Coding Rate | 4/5 | Correction d'erreurs légère suffisante en portée courte |
| Puissance | +14 dBm (g1) / +20 dBm possible en g3 | À ajuster selon mesure de terrain |
| Sync word | `0x12` (privé) | Isole totalement du trafic LoRaWAN environnant (`0x34`) |
| Temps d'antenne (16 o, SF9/125k) | ≈ 100 ms | Base du calcul de rapport cyclique |
| Latence bouton → exécution | ≈ 200–250 ms (appel + accusé) | Aller ~100 ms + retournement + ACK ~100 ms |

**Contrainte de rapport cyclique (ERC 70-03 / EN 300 220)** : c'est le point de dimensionnement le plus souvent oublié :

- En g1 (1 %) avec 100 ms de ToA → **1 émission toutes les 10 s maximum par équipement**. Suffisant pour un bouton d'appel, tendu pour la télémétrie.
- Télémétrie AGV à 30 s de période → 100 ms / 30 s = **0,33 %** → conforme en g1.
- Si on veut de la télémétrie à 5 s, passer en g3 (10 %) est obligatoire.

### 3.2 Format de trame applicatif

Trame courte, à champs fixes, CRC applicatif en plus du CRC LoRa matériel :

```
Octet  0     : 0xA5              préambule applicatif (filtre les faux réveils)
Octet  1     : 0x01              version du protocole
Octet  2     : node_id_source    (0x01..0x7F = boutons, 0x80 = AGV)
Octet  3     : node_id_dest      (0xFF = broadcast)
Octet  4     : type_message      voir table ci-dessous
Octet  5     : seq               numéro de séquence 8 bits roulant
Octet  6..N  : payload
Octet N+1..2 : CRC16-CCITT sur les octets 0..N
```

| Type | Code | Payload | Sens |
|---|---|---|---|
| `CMD_GOTO` | 0x10 | station (u16) + vitesse (u8, 0-15) | Bouton → AGV |
| `CMD_STOP` | 0x11 | - | Bouton → AGV |
| `CMD_CLEAR` | 0x12 | : (vide la file de courses) | Bouton → AGV |
| `TELEMETRY` | 0x20 | station courante (u16), vitesse (u8), état (u8), nb courses en file (u8), Vbat (u8) | AGV → broadcast |
| `ACK` | 0x30 | seq acquitté (u8), code résultat (u8) | AGV → bouton |
| `NACK` | 0x31 | seq (u8), code erreur (u8) | AGV → bouton |

**Fiabilisation** :
- Chaque `CMD_*` est retransmise jusqu'à 3 fois si aucun `ACK` n'arrive dans 400 ms, avec back-off aléatoire de 50–150 ms (évite la collision de deux boutons pressés simultanément).
- Le numéro de séquence rend l'ordre **idempotent** : si l'AGV reçoit deux fois la même `seq` du même `node_id`, il ré-acquitte sans ré-exécuter. Indispensable, sinon un ACK perdu déclenche une course en double.
- Chiffrement optionnel AES-128-CTR (clé partagée en flash) sur le payload. Recommandé même en réseau privé : sans lui, n'importe qui avec un module à 10 € peut appeler l'AGV.

### 3.3 Interface avec l'automate MEIDEN (côté AGV)

C'est la partie la plus critique du redesign, et elle est **identique dans les trois architectures**.

| Bus | Sens | Largeur | Rôle |
|---|---|---|---|
| X | ESP32 → automate | 22 signaux | Commandes : adresse destination (10 bits, 1024 valeurs), vitesse (4 bits), bits de contrôle |
| Y | Automate → ESP32 | 21 signaux | États : position courante (Y23–Y34), accusés (Y05, Y10), défauts |

**Séquence d'écriture d'une destination** (reproduite à l'identique du firmware d'origine) :

1. Poser l'adresse destination sur les 10 lignes du bus X
2. Poser la vitesse sur les 4 lignes dédiées
3. Attendre le temps de stabilisation (t_setup, à mesurer à l'oscilloscope sur la carte d'origine)
4. Lever X82 (front de validation « start »)
5. Attendre Y05 (accusé automate) : timeout et compteur de tentatives (`Start tries`)
6. Retomber X82

La séquence d'arrêt est symétrique : X83 / Y10, avec son propre compteur.

**File de courses** : jusqu'à 5 destinations en attente. Sur la carte d'origine elle n'existe qu'en RAM du MEGA, elle est perdue à chaque coupure. **Amélioration à intégrer** : la stocker en NVS (flash ESP32), ce qui rend le système robuste au redémarrage. C'est un gain fonctionnel gratuit du redesign.

**Isolation électrique** : optocoupleurs sur les 43 lignes, dans les deux sens.
- Sorties X : MCP23017 → PC847 (LED) → phototransistor côté automate, avec résistance de tirage adaptée au rail de l'automate.
- Entrées Y : ligne automate → résistance de limitation → PC847 → MCP23017.
- ⚠️ **Point ouvert** : la valeur des résistances d'entrée dépend de l'amplitude réelle des lignes Y (rail LM7806 = 6 V ? ou 24 V ?). **À mesurer à l'oscilloscope sur Y05 en fonctionnement avant de figer le schéma.**

### 3.4 Boutons d'appel

Chaque bouton est un nœud autonome sur pile :

- Sommeil profond permanent (< 2 µA), réveil sur front du bouton (interruption GPIO)
- Émission de la trame `CMD_GOTO`, attente d'ACK jusqu'à 400 ms, jusqu'à 3 tentatives
- Retour visuel : LED verte fixe 2 s = ACK reçu ; LED rouge clignotante = pas d'accusé après 3 essais (l'opérateur sait immédiatement que l'appel n'est pas passé, fonction absente de la solution EnOcean pure)
- Consommation : ~120 mA pendant 100 ms d'émission. Avec une pile Li-SOCl₂ ER14505 (2,6 Ah), et 100 appels/jour, l'autonomie est dominée par l'autodécharge → **> 5 ans réalistes**, à valider par calcul de budget énergétique complet
- Ajout d'un bouton = flasher un `node_id` + une station. Aucune modification côté AGV.

---

## 4. Points forts

| | |
|---|---|
| **Homogénéité** | Une seule technologie radio, un seul jeu de compétences, une seule pièce de rechange (RFM95W) à stocker |
| **Latence bornée et faible** | ~200 ms, déterministe, sans dépendance à un tiers |
| **Aucun point de défaillance unique** | Chaque bouton parle directement à l'AGV. La panne d'un bouton n'affecte pas les autres |
| **Aucun coût récurrent** | Bande ISM libre, pas d'abonnement, pas de passerelle, pas de serveur |
| **Aucune dépendance externe** | Ni opérateur, ni cloud, ni réseau informatique de l'usine. Fonctionne même si l'IT tombe |
| **Immunité à la saturation 2,4 GHz** | Objectif initial du projet, pleinement atteint |
| **Portée très large** | Plusieurs centaines de mètres en intérieur industriel à SF9, bien au-delà du besoin. Marge de liaison confortable |
| **Accusé de réception natif** | L'opérateur sait si son appel est passé |
| **Extensibilité triviale** | +1 bouton = +1 nœud, sans reconfiguration du système |
| **Coût matériel faible** | ~35–45 € par bouton, ~90 € pour la carte AGV (hors PCB et main d'œuvre) |

---

## 5. Points faibles

| | |
|---|---|
| **Boutons sur pile** | Maintenance à prévoir (remplacement tous les ~5 ans), et un journal de suivi. C'est le seul vrai reproche du client par rapport à l'EnOcean |
| **Rapport cyclique réglementaire** | Bride la fréquence de télémétrie. Impose un choix de sous-bande documenté et un compteur de duty cycle dans le firmware : sinon non-conformité EN 300 220 |
| **Half-duplex** | Le RFM95W ne peut pas émettre et écouter simultanément. Le firmware AGV doit alterner proprement écoute / émission, avec fenêtre d'ACK. Détail de firmware, mais à concevoir dès le départ |
| **Collisions possibles si le parc grossit** | Pas de mécanisme d'accès au médium type CSMA en LoRa nu. Au-delà de ~10 boutons très sollicités, il faudra ajouter un LBT (Listen Before Talk) ou une fenêtre temporelle par nœud |
| **Charge de développement côté intégrateur** | La fiabilisation (ACK, retransmission, séquence, idempotence) est à concevoir et tester en interne. Rien n'est fourni par un standard |
| **Pas de notification hors site** | Aucune alerte possible vers un technicien absent de l'usine. Nécessiterait un complément (SMS bas volume, ou une passerelle réseau) |
| **Pas d'interface de supervision native** | Contrairement à l'architecture hybride qui embarque une page web sur le poste fixe. À ajouter séparément si besoin |
| **Conformité radio à documenter** | Puissance ERP et rapport cyclique à valider et consigner, même en bande libre |
| **Perte de l'app mobile d'origine** | Le protocole ESP32 ↔ AIO AGV Remote n'est pas reproduit. Le Wi-Fi de maintenance reste conservé en mode « à la demande » (ILS/aimant, extinction auto 10 min) pour préserver la procédure `agvdump` |

---

## 6. Nomenclature (BOM)

Prix indicatifs HT, petites quantités, 2026. À reconsulter au moment de l'achat.

### 6.1 Carte AGV (×1)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU Wi-Fi/BT, 8 Mo flash | 1 | 5,00 € | 5,00 € |
| RFM95W-868S2 | Module LoRa SX1276 868 MHz | 1 | 10,00 € | 10,00 € |
| - | Connecteur U.FL + pigtail SMA femelle | 1 | 3,00 € | 3,00 € |
| - | Antenne 868 MHz 1/4 onde 2 dBi, embase SMA | 1 | 6,00 € | 6,00 € |
| MCP23017-E/SP | Expandeur I²C 16 GPIO | 4 | 2,50 € | 10,00 € |
| PC847 | Optocoupleur quadruple (43 voies → 11 boîtiers) | 11 | 0,60 € | 6,60 € |
| - | Résistances 1 %, condensateurs découplage, LED d'état | lot | - | 8,00 € |
| TSR 1-2450 | Convertisseur DC/DC 24 V → 5 V, 1 A, non isolé | 1 | 7,00 € | 7,00 € |
| AP2112K-3.3 | LDO 3,3 V 600 mA pour ESP32 + radio | 1 | 0,60 € | 0,60 € |
| SMBJ33A | Diode TVS protection alimentation 24 V | 2 | 0,50 € | 1,00 € |
| - | Connecteur SUB-D 25 mâle, coudé CI | 1 | 3,00 € | 3,00 € |
| - | Connecteur SUB-D 25 femelle, coudé CI | 1 | 3,00 € | 3,00 € |
| - | PCB 4 couches ~120 × 100 mm (série de 5) | 1 | 12,00 € | 12,00 € |
| - | Boîtier ABS/alu, fixation, presse-étoupes | 1 | 25,00 € | 25,00 € |
| - | Connecteur de programmation (UART/USB-C ou pogo) | 1 | 3,00 € | 3,00 € |
| | | | **Total carte AGV** | **≈ 103 €** |

### 6.2 Bouton d'appel (× nombre de stations)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| STM32L071KBU6 | MCU ultra-basse consommation (ou ESP32-C3 si on privilégie l'homogénéité de toolchain) | 1 | 3,50 € | 3,50 € |
| RFM95W-868S2 | Module LoRa 868 MHz | 1 | 10,00 € | 10,00 € |
| - | Antenne 868 MHz + embase SMA | 1 | 6,00 € | 6,00 € |
| - | Bouton poussoir industriel Ø22 IP65, coup de poing ou affleurant | 1 | 12,00 € | 12,00 € |
| ER14505 | Pile Li-SOCl₂ 3,6 V 2,6 Ah + support | 1 | 6,00 € | 6,00 € |
| TPS62740 | Convertisseur buck ultra-basse conso (ou LDO si budget serré) | 1 | 2,00 € | 2,00 € |

> ⚠️ **Révisé depuis.** Le `TPS62740` a été retiré de la nomenclature : la
> pile Li-SOCl₂ délivre 3,6 V, que le `STM32L071` (1,65–3,6 V) et le `RFM95W`
> (1,8–3,7 V) acceptent directement. Le besoin réel est un **réservoir
> capacitif** pour l'impulsion d'émission. Voir [`../BOM.md`](../BOM.md),
> section « Analyse critique de cette nomenclature ».
| - | LED bicolore verte/rouge + résistances | 1 | 1,00 € | 1,00 € |
| - | PCB 2 couches ~50 × 50 mm | 1 | 3,00 € | 3,00 € |
| - | Boîtier IP65 avec presse-étoupe et embase antenne | 1 | 18,00 € | 18,00 € |
| | | | **Total par bouton** | **≈ 62 €** |

### 6.3 Outillage et mise au point (non récurrent)

| Désignation | Prix | Usage |
|---|---:|---|
| Dongle RTL-SDR + antenne | 30 € | Mesure d'occupation réelle de la bande 868 MHz avant installation (`rtl_power`). **Argument objectif à opposer au client sur la crainte de collision** |
| Analyseur logique 8 voies (type Saleae clone) | 15 € | Relevé des chronogrammes du bus X/Y sur la carte d'origine |
| Oscilloscope (existant ?) | - | Mesure d'amplitude sur Y05 : **prérequis bloquant au schéma** |

### 6.4 Coût total du système (2 stations)

| Poste | Montant |
|---|---:|
| Carte AGV | 103 € |
| 2 boutons | 124 € |
| Outillage | 45 € |
| **Total matériel** | **≈ 272 €** |
| Coût récurrent annuel | **0 €** |
| Chaque station supplémentaire | +62 € |

---

## 7. Prérequis techniques à lever avant de figer le schéma

1. **Mesure oscilloscope de l'amplitude des lignes Y** (sur Y05 en fonctionnement) → détermine le dimensionnement des résistances d'entrée des optocoupleurs. **Bloquant.**
2. **Relevé de continuité du brochage SUB-D 25** sur la carte V5.0.1 → lever la divergence entre les deux tables de câblage fournies (CN61/62/63 vs CN62/63/64 : deux révisions, ou erreur de transcription ?). **Bloquant.**
3. **Chronogrammes du bus X/Y** à l'analyseur logique → mesurer t_setup, t_hold, la durée de l'impulsion X82 et le délai maximal d'apparition de Y05.
4. **Campagne RTL-SDR** dans l'usine, aux heures de production → objectiver l'occupation de la bande 868 MHz et choisir la sous-bande définitive.
5. **Essai de portée en charge** entre le point le plus éloigné du parcours magnétique et un bouton, avec l'AGV en mouvement et les machines en marche.
6. **Confirmation du rôle des fils jaune/orange/rouge repris au Kapton** (liaison série ESP32 ↔ MEGA ?), pour savoir si on peut réutiliser le connecteur d'origine.

---

## 8. Positionnement par rapport aux deux autres architectures

Cette solution est **la référence de simplicité** : elle atteint l'objectif fonctionnel avec une seule technologie radio, un point de défaillance en moins que l'hybride, et zéro coût récurrent.

**Elle est à retenir si** le « sans-pile » des boutons est un confort et non une exigence contractuelle du client.

Si le sans-pile est une exigence réelle → aller vers l'architecture 3 (hybride EnOcean + LoRa).
Le SMS (architecture 2) ne se justifie jamais comme liaison principale bouton → AGV.
