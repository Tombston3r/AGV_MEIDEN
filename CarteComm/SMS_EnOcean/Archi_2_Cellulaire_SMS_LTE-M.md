> Document de référence — projet « Remplacement de la carte AIO AGV Control V5.0.1 »
> AGV MEIDEN à guidage magnétique, appel depuis boutons déportés vers points d'arrêt.

# Architecture 2 — Liaison cellulaire (SMS, puis variante LTE-M/MQTT)

**Statut : architecture étudiée à la demande du client, non recommandée comme liaison principale.**
Ce document en fait l'analyse complète — c'est précisément ce qui permet de la démonter sur des arguments chiffrés plutôt que sur une opinion.

---

## 1. Fonction

Assurer la liaison bidirectionnelle poste d'appel ↔ AGV en s'appuyant sur **l'infrastructure radio d'un opérateur mobile** plutôt que sur une bande ISM libre. La motivation est unique et non technique : répondre à la crainte exprimée par le client de collisions sur le canal 868 MHz.

---

## 2. Deux variantes très différentes

Il faut absolument distinguer les deux, car elles n'ont ni les mêmes performances ni la même pérennité.

### Variante A — SMS (celle initialement évoquée)

```
 BOUTON             POSTE FIXE                  RÉSEAU OPÉRATEUR              AGV
┌────────┐      ┌───────────────────┐                                 ┌──────────────────┐
│ Bouton │─fil─▶│ UniPi E413 (LTE)  │──SMS──▶ SMSC (store & forward) ─┼─▶ Modem LTE       │
│ IP65   │      │  ou ESP32+SIM7600 │                    │            │      │ UART       │
└────────┘      │  + SIM M2M        │◀──SMS── (retour) ◀─┘            │  ESP32           │
                └───────────────────┘                                 │      │ I²C        │
                                                                       │  4× MCP23017     │
                                                                       │  Optocoupleurs   │
                                                                       │  Bus X/Y MEIDEN  │
                                                                       └──────────────────┘
```

### Variante B — LTE-M / NB-IoT + MQTT (techniquement très supérieure)

```
 BOUTON          POSTE FIXE              RÉSEAU LTE-M          BROKER MQTT        AGV
┌────────┐   ┌──────────────────┐                          ┌────────────┐   ┌──────────────┐
│ Bouton │──▶│ ESP32 + SIM7080G │──── PUBLISH agv/cmd ────▶│  Mosquitto │──▶│ ESP32+SIM7080│
└────────┘   │  (LTE-M/NB-IoT)  │◀─── SUBSCRIBE agv/ack ───│  (VPS ou   │◀──│  bus MEIDEN  │
             └──────────────────┘                          │  on-prem)  │   └──────────────┘
                                                            └────────────┘
```

**Si le cellulaire devait vraiment être imposé, c'est la variante B qu'il faudrait retenir, jamais le SMS.** MQTT sur LTE-M apporte : latence de 0,5 à 2 s, QoS 1/2 avec accusé applicatif, ordre garanti par connexion TCP, Last Will and Testament (détection immédiate de perte de l'AGV), coût data ~1–2 €/mois/SIM au lieu de facturation à l'unité.

---

## 3. Fonctionnement détaillé (variante A — SMS)

### 3.1 Chaîne de transmission

1. L'opérateur appuie sur le bouton, câblé en filaire sur une entrée TOR du poste fixe.
2. Le poste fixe compose un SMS applicatif court, par ex. `GOTO;02;SPD=08;SEQ=41`.
3. Le modem le remet au **SMSC** (Short Message Service Centre) de l'opérateur.
4. Le SMSC stocke le message et tente de le remettre à la SIM de l'AGV **quand il le peut** — c'est le principe *store-and-forward*.
5. Le modem AGV reçoit, notifie l'ESP32 en UART (`+CMTI:`), l'ESP32 lit le message (`AT+CMGR`), parse, valide le CRC/la séquence.
6. L'ESP32 exécute la séquence sur le bus MEIDEN (identique à l'architecture 1 : pose adresse 10 bits + vitesse 4 bits, front X82, attente Y05).
7. L'ESP32 renvoie un SMS d'accusé `ACK;SEQ=41;OK` vers le poste fixe.
8. Le poste allume le voyant vert.

### 3.2 Ce que le SMS ne garantit pas — le cœur du problème

| Propriété | SMS | Conséquence sur un AGV |
|---|---|---|
| **Latence** | **Non bornée par conception.** Le standard ne fixe aucun délai maximal de remise | 4–8 s dans le meilleur cas, 30 s couramment, plusieurs minutes possibles en cas de congestion du SMSC |
| **Ordre de remise** | Aucune garantie. Deux SMS émis dans l'ordre peuvent arriver inversés | Un `STOP` peut arriver **avant** le `GOTO` qu'il annule. Sur un engin mobile, c'est un problème de sécurité, pas de confort |
| **Remise** | Best effort. L'accusé de remise (SR) confirme la remise au terminal, pas le traitement | Nécessite un ACK applicatif de bout en bout de toute façon |
| **Doublons** | Possibles (retransmission SMSC) | Impose la même logique d'idempotence par `SEQ` que l'architecture 1 — donc aucune économie de développement |

**Le point à retenir pour l'argumentaire client** : le SMS a été conçu comme un canal de signalisation opportuniste utilisant les créneaux libres du réseau. Il n'a jamais été conçu pour du contrôle-commande. La crainte du client porte sur les collisions LoRa (probabilité faible, effet : une retransmission de 100 ms) et la « solution » qu'il propose introduit une latence non bornée et un désordre de remise possible. **Le remède est plus dangereux que le mal supposé.**

### 3.3 Comparaison de latence

| Solution | Latence typique | Latence pire cas | Bornée ? |
|---|---|---|---|
| LoRa P2P | ~200 ms | ~800 ms (3 retransmissions) | **Oui** |
| LTE-M + MQTT | 0,5–2 s | ~10 s (reconnexion) | Partiellement |
| SMS | 4–8 s | Minutes | **Non** |

---

## 4. Points forts

Ils existent, et il faut les énoncer honnêtement — c'est ce qui rend l'analyse crédible.

| | |
|---|---|
| **Aucune infrastructure radio à déployer** | On s'appuie sur un réseau existant, entretenu et supervisé par un opérateur. Zéro étude de propagation, zéro antenne à positionner |
| **Portée illimitée** | Le poste fixe et l'AGV peuvent être à n'importe quelle distance. Pertinent si le site s'étend ou si plusieurs bâtiments sont concernés |
| **Notification hors site native** | C'est le **seul avantage réellement différenciant** : alerter un technicien absent de l'usine sur son téléphone personnel, sans aucun développement, sans VPN, sans application |
| **Robustesse de la couche physique** | Le LTE gère nativement le handover, le contrôle de puissance, la correction d'erreurs et le contrôle d'accès au médium — là où LoRa nu ne fait rien de tout ça |
| **Pas de question de conformité radio** | Le module est certifié, l'usage de la bande est celui de l'opérateur. Aucun calcul de rapport cyclique, aucun dossier ERP à monter |
| **Traçabilité opérateur** | Les échanges sont journalisés côté réseau, ce qui peut servir en cas de litige |

---

## 5. Points faibles

| | |
|---|---|
| **Latence non bornée (SMS)** | Rédhibitoire pour du contrôle-commande d'un engin mobile. Voir §3.2 |
| **Pas de garantie d'ordre (SMS)** | Risque fonctionnel et sécuritaire réel |
| **Coût récurrent significatif** | ~1 500 €/an estimé pour le SMS (2 SIM M2M + volume de messages avec accusés). À comparer à **0 €/an** pour les architectures 1 et 3. Sur 10 ans : ~15 000 € contre le coût matériel initial de ~272 € en LoRa |
| **Obsolescence 2G/3G planifiée** | Les opérateurs français ont un calendrier d'extinction. Un module GSM 2G acheté aujourd'hui a une durée de vie limitée. **Vérifier le calendrier à jour au moment de l'étude** — il évolue. Impose de partir directement sur LTE-M/Cat-M1 ou Cat-1 bis, plus cher |
| **Couverture intérieure incertaine** | Une usine, c'est une structure métallique. La couverture opérateur au sol d'un atelier, dans une allée entre des racks, n'est **pas garantie** — et contrairement à un réseau privé, on ne peut pas la corriger en ajoutant un répéteur. **Mesure de niveau RSRP/RSRQ obligatoire en tous points du parcours avant tout engagement** |
| **Dépendance à un tiers** | Panne opérateur, saturation locale (événement, incident), résiliation, changement tarifaire : autant de risques hors de tout contrôle. L'usine ne peut plus appeler son AGV parce que le réseau mobile est tombé — situation difficile à expliquer |
| **Antenne mobile sur l'AGV** | Un AGV qui circule dans une structure métallique verra son niveau de signal varier fortement. Le handover cellule à cellule en intérieur est une source de latence supplémentaire |
| **Fiabilisation applicative toujours nécessaire** | ACK, séquence, idempotence, timeout : exactement le même travail de développement qu'en LoRa. **Le cellulaire ne fait économiser aucune ligne de code de fiabilisation** |
| **Consommation** | Un modem LTE consomme des pics de 2 A en émission. Incompatible avec un bouton sur pile → les boutons doivent être **câblés** au poste fixe, ce qui perd toute la souplesse d'implantation |
| **Complexité de mise au point** | Débogage AT, gestion des états du modem, reconnexion, roaming, APN, PIN : une couche de complexité non triviale, et difficile à diagnostiquer sur site |

---

## 6. Nomenclature (BOM)

Prix indicatifs HT, 2026.

### 6.1 Variante A — SMS avec UniPi E413

**Précision importante levée en conversation** : l'emplacement SIM de l'UniPi E413 correspond à une **variante à modem LTE intégré**, pas à un emplacement générique nécessitant un modem externe. Il faut donc commander explicitement la référence LTE.

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| UniPi E413 (variante LTE) | Automate compact Linux, E/S TOR, modem LTE intégré | 1 | ~350 € | 350 € |
| — | Antenne LTE externe déportée (poste fixe) | 1 | 15 € | 15 € |
| — | SIM M2M professionnelle (poste) | 1 | — | abonnement |
| — | Bouton poussoir Ø22 IP65 + câblage TOR | 2 | 12 € | 24 € |
| — | Câblage filaire bouton → poste (par bouton, ~20 m) | 2 | 25 € | 50 € |
| **Sous-total poste fixe** | | | | **≈ 439 €** |
| SIM7600E-H | Modem LTE Cat-1 + SIM, module | 1 | 40 € | 40 € |
| — | Antenne LTE embarquée AGV, montage déporté | 1 | 12 € | 12 € |
| ESP32-WROOM-32E | MCU | 1 | 5 € | 5 € |
| MCP23017 | Expandeurs I²C 16 GPIO | 4 | 2,50 € | 10 € |
| PC847 | Optocoupleurs quadruples (43 voies) | 11 | 0,60 € | 6,60 € |
| TSR 1-2450 + AP2112K | Alimentation 24 V → 5 V → 3,3 V | 1 | 8 € | 8 € |
| — | Réservoir capacitif pour pics d'émission modem (2 A) | 1 | 3 € | 3 € |
| — | SUB-D 25 M + F, coudés CI | 2 | 3 € | 6 € |
| — | PCB 4 couches, boîtier, connectique, passifs | 1 | 50 € | 50 € |
| **Sous-total carte AGV** | | | | **≈ 141 €** |
| **Total matériel** | | | | **≈ 580 €** |

### 6.2 Coûts récurrents (variante A — SMS)

| Poste | Estimation annuelle |
|---|---:|
| 2 abonnements SIM M2M | 120–240 € |
| Volume SMS (appels + accusés + télémétrie dégradée) | 1 000–1 300 € |
| **Total récurrent** | **≈ 1 500 €/an** |
| **Sur 10 ans** | **≈ 15 000 €** |

### 6.3 Variante B — LTE-M/NB-IoT + MQTT (si le cellulaire est imposé)

| Réf. | Désignation | Qté | PU | Total |
|---|---|---:|---:|---:|
| SIM7080G | Modem LTE-M / NB-IoT, très basse conso | 2 | 18 € | 36 € |
| ESP32-WROOM-32E | MCU (poste + AGV) | 2 | 5 € | 10 € |
| — | Antennes LTE + montage | 2 | 12 € | 24 € |
| — | Reste identique à 6.1 côté interface MEIDEN | 1 | 90 € | 90 € |
| — | Broker MQTT : Mosquitto sur VPS ou serveur usine | 1 | ~60 €/an | — |
| **Total matériel** | | | | **≈ 160 €** |
| **Récurrent** | 2 SIM LTE-M data (~1,5 €/mois) + VPS | | | **≈ 100 €/an** |

La variante B est **15× moins chère en récurrent que le SMS et 4× plus rapide.** Si le client tient au cellulaire, c'est le seul chemin défendable.

### 6.4 Outillage de validation

| Désignation | Prix | Usage |
|---|---:|---|
| Testeur de couverture / smartphone en mode ingénieur | 0 € | Relevé RSRP/RSRQ en tous points du parcours AGV. **Prérequis bloquant** |
| SIM de test opérateur (prêt commercial) | 0 € | Essai réel de latence SMS en conditions de production |

---

## 7. Conditions à réunir avant même d'envisager cette architecture

1. **Relevé de couverture cellulaire complet** le long du parcours magnétique, aux heures de production, machines en marche. Si le RSRP descend sous −110 dBm à un seul point d'arrêt, la solution est disqualifiée.
2. **Essai de latence réel** sur 200 SMS aller-retour en conditions de production, avec mesure de la distribution (pas seulement la moyenne — c'est le 99ᵉ percentile qui compte).
3. **Vérification du calendrier d'extinction 2G/3G** auprès des opérateurs, à jour au moment de l'étude.
4. **Chiffrage contradictoire** du coût sur 10 ans, mis en regard des architectures 1 et 3.
5. Les prérequis d'interface MEIDEN sont **identiques aux autres architectures** (mesure oscillo Y05, relevé de continuité SUB-D 25, chronogrammes X/Y) — voir document Architecture 1, §7.

---

## 8. Conclusion et positionnement

Le SMS comme liaison principale bouton → AGV cumule : la plus mauvaise latence des trois architectures, la seule latence non bornée, le seul coût récurrent significatif, la seule dépendance à un tiers, le seul risque d'obsolescence planifié — et ne fait économiser **aucun** développement de fiabilisation.

**Recommandation** : ne pas retenir comme liaison principale.

**Usage légitime résiduel** : en **complément bas volume** des architectures 1 ou 3, pour les seules alertes nécessitant une portée hors site — par exemple prévenir un technicien de maintenance absent de l'usine en cas de défaut bloquant de l'AGV. Dans ce rôle, quelques SMS par mois via une passerelle sur le poste fixe coûtent une dizaine d'euros par an et apportent une vraie valeur.

C'est cet usage-là qu'il faut proposer au client : il obtient la fonction cellulaire qu'il demande, là où elle est réellement supérieure, sans compromettre la chaîne de commande.
