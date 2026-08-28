> Document de référence : projet « Remplacement de la carte AIO AGV Control V5.0.1 »
> AGV MEIDEN à guidage magnétique, appel depuis boutons déportés vers points d'arrêt.

# Architecture 3 : Hybride EnOcean + LoRa (architecture retenue)

**Statut : architecture validée en fin de conversation initiale, sous réserve que le « sans-pile » soit une exigence réelle du client.**

---

## 1. Fonction

Combiner l'avantage propre à chaque technologie plutôt que d'en imposer une seule :

- **EnOcean** sur le tronçon bouton → poste fixe : boutons **auto-alimentés par l'énergie de l'appui**, sans pile, sans maintenance, sans câblage.
- **LoRa 868 MHz** sur le tronçon poste fixe → AGV : liaison longue portée à latence bornée vers une cible mobile.
- Le **poste fixe** joue le rôle de traducteur de protocole entre les deux, et embarque au passage l'interface de supervision.

---

## 2. Architecture générale

```
   BOUTONS EnOcean            POSTE FIXE (mural, secteur)                    AGV MEIDEN
 ┌──────────────────┐   ┌──────────────────────────────────┐    ┌────────────────────────────┐
 │ PTM 210 station 1│   │  TCM 515 (RX EnOcean 868,3 MHz)  │    │  ESP32-WROOM-32E           │
 │ sans pile        │──))│      │ UART 57600 (ESP3)         │    │      │ SPI                 │
 └──────────────────┘   │      ▼                            │    │  RFM95W (SX1276) 868 MHz  │
                         │  ESP32-WROOM-32E                  │    │      │ ANT déportée mât    │
 ┌──────────────────┐   │   ├── table ID EnOcean → station  │    │      │                     │
 │ PTM 210 station 2│──))│   ├── traduction → trame LoRa    │    │      │ I²C                 │
 │ sans pile        │   │   ├── serveur web (WebSocket)     │    │  4× MCP23017 (64 GPIO)     │
 └──────────────────┘   │   │                                │    │      │                     │
                         │   ├─ SPI ── RFM95W ── ANT ────))))│))))│      ▼                     │
 ┌──────────────────┐   │   │                                │    │  Étage optocouplé          │
 │ … extensible     │──))│   └─ SPI ── W5500 ── RJ45 ────────┼──▶ │   ├─ 22 sorties X          │
 └──────────────────┘   │              (réseau usine filaire)│    │   └─ 21 entrées Y          │
                         │                                    │    │      │                     │
                         │  Voyant d'accusé local (LED)       │    │  SUB-D 25 M / F            │
                         └──────────────────────────────────┘    │  Automate MEIDEN           │
                                                                   └────────────────────────────┘

                                          Wi-Fi de maintenance AGV : à la demande
                                          (ILS/aimant ou bouton, extinction auto 10 min)
                                          → préserve la procédure /agvdump existante
```

**Aucune UniPi dans l'architecture finale.** Les deux rôles (poste fixe et AGV) tiennent sur ESP32, ce qui divise le coût du poste fixe par un facteur ~10 par rapport à la variante UniPi initialement envisagée.

---

## 3. Fonctionnement détaillé

### 3.1 Tronçon 1 : EnOcean (bouton → poste fixe)

**Principe de récupération d'énergie.** Le PTM 210 contient un générateur électrodynamique : l'appui sur la bascule déplace un aimant devant une bobine et produit un bref pulse d'énergie, suffisant pour alimenter l'émetteur radio le temps d'un télégramme. **Aucune pile, aucun condensateur de stockage à remplacer.** Durée de vie annoncée : de l'ordre de 100 000 actionnements, soit plusieurs dizaines d'années à raison de 10 appels/jour.

**Caractéristiques radio** :

| Paramètre | Valeur |
|---|---|
| Fréquence | 868,3 MHz (variante EU) |
| Modulation | ASK, débit 125 kbps |
| Durée d'un télégramme | ~1 ms |
| Répétition | Le télégramme est émis 3 fois (à ~20 et ~40 ms) pour compenser les collisions |
| Portée intérieure typique | 20–30 m en environnement industriel métallique (jusqu'à 100 m en champ libre) |
| Identifiant émetteur | 32 bits, gravé en usine, non modifiable |
| Sens | **Unidirectionnel** : le PTM 210 émet, il ne reçoit rien |

**Chaîne de réception.** Le TCM 515 reçoit le télégramme et le remonte à l'ESP32 en UART à 57600 bauds au format **ESP3** (EnOcean Serial Protocol 3) : trame `0x55` + en-tête + CRC8 en-tête + données + CRC8 données. L'ESP32 parse, extrait l'ID 32 bits et le code de touche (le PTM 210 a 4 contacts : A0, A3, B0, B1).

**Table de correspondance et appairage.** Le PTM 210 envoie un **identifiant d'émetteur**, pas un numéro de station. Il faut donc une table `ID EnOcean (32 bits) → numéro de station` sur l'ESP32 poste, persistée en NVS. Deux modes d'apprentissage complémentaires :

1. **Mode pairing physique** : appui long sur un bouton dédié du poste → le poste passe en écoute, le premier ID reçu est associé à la station saisie.
2. **Via l'interface web**, plus pratique, et l'interface existe déjà : une page « Boutons » listant les ID connus, avec un champ station éditable et un bouton « apprendre le prochain appui ».

⚠️ **Point à vérifier au moment de la commande** : le **TCM 515 est un module de réception**. Si l'on veut renvoyer un accusé EnOcean vers un actionneur/voyant sans fil au poste, il faut un module **bidirectionnel** de type TCM 310. Dans l'architecture retenue, le voyant d'accusé est simplement une **LED pilotée directement par l'ESP32 du poste**, ce qui rend la question sans objet et économise un composant. À trancher explicitement.

### 3.2 Tronçon 2 : LoRa (poste fixe → AGV)

Strictement identique à l'architecture 1 : mêmes modules RFM95W, mêmes paramètres radio, même format de trame applicatif.

| Paramètre | Valeur |
|---|---|
| Fréquence | 869,525 MHz (g3, 10 % duty cycle) ou 868,1 MHz (g1, 1 %) |
| SF / BW / CR | SF9 / 125 kHz / 4/5 |
| Sync word | `0x12` (privé : isole du trafic LoRaWAN) |
| Temps d'antenne (16 o) | ≈ 100 ms |
| Latence poste → AGV → accusé | ≈ 200–250 ms |

**Format de trame** (rappel) :

```
0xA5 | version | node_src | node_dst | type | seq | payload… | CRC16-CCITT
```

| Type | Code | Payload |
|---|---|---|
| `CMD_GOTO` | 0x10 | station (u16) + vitesse (u8, 0-15) |
| `CMD_STOP` | 0x11 | - |
| `CMD_CLEAR` | 0x12 | - |
| `TELEMETRY` | 0x20 | station, vitesse, état, nb courses en file, Vbat |
| `ACK` / `NACK` | 0x30 / 0x31 | seq + code résultat |

**Idempotence par numéro de séquence** : si l'AGV reçoit deux fois la même `seq` du même émetteur, il ré-acquitte sans ré-exécuter. Indispensable, sinon un ACK perdu génère une course en double.

⚠️ **Contrainte half-duplex.** Le RFM95W ne peut pas émettre et recevoir simultanément. Le firmware du poste doit donc alterner proprement entre « j'écoute la télémétrie de l'AGV » et « j'émets une commande », avec une fenêtre d'écoute d'accusé dédiée après chaque émission. C'est un point de conception de firmware, pas de schéma, mais il doit être écrit dans le protocole applicatif **avant** de figer le matériel.

### 3.3 Tronçon 3 : Interface avec l'automate MEIDEN (côté AGV)

Identique aux trois architectures. C'est la partie la plus critique du redesign.

| Bus | Sens | Largeur | Rôle |
|---|---|---|---|
| X | ESP32 → automate | 22 signaux | Adresse destination (10 bits → 1024 valeurs), vitesse (4 bits), bits de contrôle |
| Y | Automate → ESP32 | 21 signaux | Position courante (Y23–Y34), accusés (Y05, Y10), défauts |

**Séquence d'écriture d'une destination**, reproduite du firmware d'origine :

1. Poser l'adresse destination sur les 10 lignes du bus X
2. Poser la vitesse sur les 4 lignes dédiées
3. Attendre t_setup (à mesurer à l'analyseur logique sur la carte d'origine)
4. Lever X82 (validation « start »)
5. Attendre Y05 (accusé automate), avec timeout et compteur `Start tries`
6. Retomber X82

Séquence d'arrêt symétrique : X83 / Y10, compteur `Stop tries`.

**File de courses** : jusqu'à 5 destinations. Sur la carte d'origine elle n'existe qu'en RAM du MEGA et est perdue à chaque coupure. **Amélioration du redesign** : persistance en NVS flash ESP32. Gain fonctionnel gratuit.

**Isolation** : PC847 sur les 43 lignes, dans les deux sens. Le dimensionnement des résistances d'entrée dépend de l'amplitude réelle des lignes Y, **mesure oscilloscope sur Y05 en fonctionnement, prérequis bloquant.**

### 3.4 Interface de supervision (poste fixe)

C'est un apport propre à cette architecture, absent des deux autres.

| Élément | Choix |
|---|---|
| Serveur | ESPAsyncWebServer sur l'ESP32 poste |
| Stockage des pages | LittleFS (HTML/CSS/JS embarqués en flash) |
| Temps réel | WebSocket : poussée de la télémétrie AGV vers le navigateur, sans polling |
| Accès réseau | **Ethernet filaire** (W5500 sur SPI ou module WT32-ETH01) sur le réseau usine |

**Le choix du filaire est structurant** : il élimine toute émission Wi-Fi continue, ce qui était l'objectif initial du projet. Le poste est accessible depuis n'importe quel PC de l'usine par son IP, sans ajouter un seul réseau sans fil.

Fonctions de la page :
- Position courante de l'AGV, vitesse, état, contenu de la file de courses
- Journal des appels (horodatage, bouton, station, résultat)
- Table d'appairage des boutons EnOcean, avec mode apprentissage
- Compteurs de diagnostic (échecs d'ACK LoRa, timeouts Y05, `Start tries`)
- Commande manuelle de secours (envoi d'un `GOTO` depuis le navigateur)

**Wi-Fi de maintenance AGV** : conservé en mode **à la demande**, activé par ILS/aimant ou bouton physique sur l'AGV, extinction automatique après 10 minutes. Cela préserve la procédure `agvdump` existante sans réintroduire une émission 2,4 GHz permanente.

---

## 4. Points forts

| | |
|---|---|
| **Boutons totalement sans pile** | ~100 000 actionnements, aucune maintenance, aucun journal de remplacement à tenir. **C'est l'unique raison d'être de cette architecture** |
| **Boutons sans câblage** | Pose en 5 minutes, déplaçables librement. Aucun percement, aucun chemin de câble |
| **Latence bornée et faible sur le tronçon critique** | ~200 ms sur poste → AGV, là où le mouvement se décide |
| **Accusé de réception restauré** | Le poste attend l'ACK LoRa et allume un voyant. C'est ce qui distingue cette solution d'un EnOcean pur, où l'opérateur n'a **aucun** retour |
| **Interface de supervision incluse** | Sans SBC, sans PC industriel, sans Wi-Fi additionnel : juste un module Ethernet à 6 € |
| **Aucune émission Wi-Fi permanente** | Objectif initial du projet atteint, y compris pour la supervision |
| **Aucun coût récurrent, aucune dépendance externe** | Ni opérateur, ni cloud, ni abonnement |
| **Extensibilité** | +1 station = +1 bouton PTM 210 (~30 €) + une ligne dans la table d'appairage via l'interface web. Aucune intervention sur l'AGV |
| **Technologies matures** | EnOcean est un standard industriel (ISO/IEC 14543-3-1x) déployé massivement en GTB depuis 20 ans |

---

## 5. Points faibles

| | |
|---|---|
| **Portée EnOcean à valider** | 20–30 m typiques en intérieur métallique. C'est le **risque technique n°1**. Parades disponibles : répéteur EnOcean niveau 1 ou 2, diversité de réception (deux TCM 515 déportés), positionnement de l'antenne hors structure métallique. Mais toutes ajoutent du coût et de la complexité. **Essai de portée obligatoire avant engagement** |
| **Point de défaillance unique** | Le poste fixe conditionne **tous** les appels. S'il tombe (alimentation, ESP32, antenne), plus aucun bouton ne fonctionne. Absent des deux autres architectures. Parades : alimentation secourue, chien de garde matériel, voyant de vie visible depuis l'atelier |
| **Complexité doublée** | Deux liaisons radio à dimensionner, deux protocoles à maîtriser (ESP3 et LoRa applicatif), deux jeux de pièces de rechange, deux sources de panne à diagnostiquer. Charge de développement et de recette nettement supérieure à l'architecture 1 |
| **Hétérogénéité du parc** | Deux familles de composants radio à stocker et à connaître. Un technicien de maintenance doit être formé aux deux |
| **EnOcean unidirectionnel** | Le bouton lui-même n'a aucun retour. L'accusé est au **poste**, pas dans la main de l'opérateur. Si l'opérateur ne voit pas le poste depuis l'endroit où il appuie, il n'a aucune confirmation, à vérifier sur le terrain, c'est un point d'ergonomie souvent sous-estimé |
| **ID EnOcean non modifiable** | En cas de remplacement d'un bouton défectueux, il faut refaire l'appairage. Procédure à documenter et à laisser accessible à la maintenance |
| **Coût plus élevé** | ~30 € par bouton PTM 210 + ~30 € de TCM 515 + le poste fixe complet, contre ~62 € par nœud et zéro poste fixe en architecture 1 |
| **Contrainte half-duplex du RFM95W** | Le firmware du poste doit arbitrer écoute/émission. Point de conception à traiter tôt |
| **Rapport cyclique LoRa** | Bride la fréquence de télémétrie. Compteur de duty cycle à implémenter dans le firmware pour la conformité EN 300 220 |
| **Pas de notification hors site** | Complément SMS bas volume à ajouter si le besoin existe (voir document Architecture 2, §8) |

---

## 6. Nomenclature (BOM)

Prix indicatifs HT, petites quantités, 2026.

### 6.1 Poste fixe (×1)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU, 8 Mo flash (LittleFS + pages web) | 1 | 5,00 € | 5,00 € |
| TCM 515 (868 MHz) | Récepteur EnOcean, interface UART ESP3 | 1 | 28,00 € | 28,00 € |
| - | Antenne EnOcean 868 MHz (whip ou déportée) | 1 | 8,00 € | 8,00 € |
| RFM95W-868S2 | Module LoRa SX1276 | 1 | 10,00 € | 10,00 € |
| - | Pigtail U.FL → SMA + antenne LoRa 2 dBi déportée | 1 | 9,00 € | 9,00 € |
| W5500 (module) | Contrôleur Ethernet SPI + RJ45 magnétique | 1 | 6,00 € | 6,00 € |
| - | *Alternative* : WT32-ETH01 (ESP32 + PHY LAN8720 intégrés) | (1) | (12,00 €) | - |
| - | LED d'accusé bicolore + LED de vie, résistances | 1 | 1,50 € | 1,50 € |
| - | Bouton d'appairage + bouton reset | 2 | 1,00 € | 2,00 € |
| MEAN WELL HDR-15-24 | Alimentation rail DIN 230 V → 24 V 15 W | 1 | 14,00 € | 14,00 € |
| TSR 1-2450 + AP2112K | 24 V → 5 V → 3,3 V | 1 | 8,00 € | 8,00 € |
| - | PCB 2 couches ~100 × 80 mm | 1 | 6,00 € | 6,00 € |
| - | Boîtier mural IP54 avec presse-étoupes et embases SMA | 1 | 30,00 € | 30,00 € |
| **Total poste fixe** | | | | **≈ 128 €** |

### 6.2 Carte AGV (×1)

Identique à l'architecture 1.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| ESP32-WROOM-32E-N8 | Module MCU | 1 | 5,00 € | 5,00 € |
| RFM95W-868S2 | Module LoRa SX1276 | 1 | 10,00 € | 10,00 € |
| - | Pigtail U.FL → SMA + antenne 868 MHz déportée sur mât | 1 | 9,00 € | 9,00 € |
| MCP23017-E/SP | Expandeur I²C 16 GPIO (43 lignes → 4 boîtiers) | 4 | 2,50 € | 10,00 € |
| PC847 | Optocoupleur quadruple (43 voies → 11 boîtiers) | 11 | 0,60 € | 6,60 € |
| - | Résistances 1 %, découplages, LED d'état | lot | - | 8,00 € |
| TSR 1-2450 | DC/DC 24 V → 5 V 1 A | 1 | 7,00 € | 7,00 € |
| AP2112K-3.3 | LDO 3,3 V 600 mA | 1 | 0,60 € | 0,60 € |
| SMBJ33A | TVS protection alimentation | 2 | 0,50 € | 1,00 € |
| - | SUB-D 25 mâle, coudé CI | 1 | 3,00 € | 3,00 € |
| - | SUB-D 25 femelle, coudé CI | 1 | 3,00 € | 3,00 € |
| - | ILS (reed) + aimant pour Wi-Fi de maintenance à la demande | 1 | 2,00 € | 2,00 € |
| - | PCB 4 couches ~120 × 100 mm | 1 | 12,00 € | 12,00 € |
| - | Boîtier, fixation, presse-étoupes, connecteur de prog. | 1 | 28,00 € | 28,00 € |
| **Total carte AGV** | | | | **≈ 105 €** |

### 6.3 Boutons d'appel (× nombre de stations)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| PTM 210 (EU 868 MHz) | Module émetteur EnOcean auto-alimenté | 1 | 30,00 € | 30,00 € |
| - | Enveloppe / poussoir mural compatible PTM 210 | 1 | 12,00 € | 12,00 € |
| - | Plaque de repérage station gravée | 1 | 4,00 € | 4,00 € |
| **Total par bouton** | | | | **≈ 46 €** |

> Alternative intégrée : un interrupteur sans fil EnOcean prêt à l'emploi (type NodOn, Trio2Sys, Eltako) évite l'assemblage et coûte ~45–60 € tout compris. À privilégier si le volume est faible.

### 6.4 Option de fiabilisation (si l'essai de portée EnOcean est limite)

| Désignation | PU | Remarque |
|---|---:|---|
| Répéteur EnOcean niveau 1/2 (alimenté secteur) | 60–90 € | Doubler la portée, au prix d'un point d'alimentation supplémentaire |
| 2ᵉ TCM 515 déporté (diversité de réception) | 28 € + câblage | Nécessite un développement firmware de fusion des deux flux |

### 6.5 Outillage et mise au point (non récurrent)

| Désignation | Prix | Usage |
|---|---:|---|
| Dongle RTL-SDR + antenne | 30 € | `rtl_power` sous Fedora : mesure d'occupation réelle de la bande 868 MHz. **Argument objectif à opposer au client sur la crainte de collision LoRa** |
| Analyseur logique 8 voies | 15 € | Chronogrammes du bus X/Y sur la carte V5.0.1 |
| Oscilloscope | - | Mesure d'amplitude sur Y05 : **prérequis bloquant** |

### 6.6 Coût total du système (2 stations)

| Poste | Montant |
|---|---:|
| Poste fixe | 128 € |
| Carte AGV | 105 € |
| 2 boutons EnOcean | 92 € |
| Outillage | 45 € |
| **Total matériel** | **≈ 370 €** |
| Coût récurrent annuel | **0 €** |
| Chaque station supplémentaire | +46 € |

> À comparer : architecture 1 (LoRa homogène) ≈ 272 € et 0 €/an ; architecture 2 (SMS) ≈ 580 € et ~1 500 €/an.

---

## 7. Prérequis techniques à lever avant de figer le schéma

**Spécifiques à cette architecture :**

1. **Essai de portée EnOcean** entre chaque emplacement de bouton envisagé et l'emplacement du poste fixe, machines en marche, avec un kit de démonstration PTM 210 + TCM 515. **Risque n°1 : bloquant.**
2. **Trancher le retour d'accusé** : LED locale pilotée par l'ESP32 (retenu, gratuit) ou actionneur EnOcean sans fil (nécessite un TCM 310 bidirectionnel à la place du TCM 515). Vérifier au passage la visibilité du poste depuis les emplacements de boutons.
3. **Choisir W5500 vs WT32-ETH01** pour le poste : le second intègre le PHY mais impose son brochage ESP32 et laisse moins de GPIO libres.
4. **Concevoir l'arbitrage half-duplex** du RFM95W poste (fenêtres écoute télémétrie / émission commande) avant de figer le firmware.
5. **Définir la procédure d'appairage** et la documenter pour la maintenance.

**Communs aux trois architectures :**

6. **Mesure oscilloscope de l'amplitude des lignes Y** (sur Y05 en fonctionnement) → dimensionnement des résistances d'entrée des optocoupleurs. **Bloquant.**
7. **Relevé de continuité du brochage SUB-D 25** sur la carte V5.0.1 → lever la divergence entre les deux tables de câblage fournies (CN61/62/63 vs CN62/63/64 : deux révisions du produit, ou erreur de transcription ?). **Bloquant.**
8. **Chronogrammes du bus X/Y** à l'analyseur logique → t_setup, t_hold, durée de l'impulsion X82, délai maximal d'apparition de Y05.
9. **Confirmation du rôle des fils jaune/orange/rouge repris au Kapton** (liaison série ESP32 ↔ MEGA ?).
10. **Campagne RTL-SDR** dans l'usine aux heures de production → choix définitif de la sous-bande LoRa.

---

## 8. Positionnement et arbitrage

Cette architecture est la plus riche fonctionnellement : boutons sans pile **et** sans câblage, latence bornée sur le tronçon critique, accusé de réception, supervision web filaire incluse, zéro Wi-Fi permanent, zéro coût récurrent.

Elle est aussi la plus complexe : deux technologies radio, un point de défaillance unique, une portée EnOcean à valider sur site.

**La question à trancher avec le client avant de figer le schéma reste la même :**

> Le « sans-pile » des boutons est-il une **exigence contractuelle** ou un **confort** ?

- **Exigence** → architecture 3, sous réserve que l'essai de portée EnOcean soit concluant.
- **Confort** → architecture 1 (LoRa homogène) atteint le même résultat fonctionnel avec une seule technologie radio, un point de défaillance en moins, ~100 € de moins, et une charge de développement sensiblement réduite. Le prix à payer est un remplacement de pile tous les ~5 ans.

Dans les deux cas, l'interface avec l'automate MEIDEN, les prérequis de mesure et le protocole LoRa applicatif sont **identiques** : ce travail n'est donc pas perdu quel que soit l'arbitrage final.
