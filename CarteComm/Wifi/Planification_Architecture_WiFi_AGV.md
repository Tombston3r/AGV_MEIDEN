# AGV MEIDEN — Architecture Wi-Fi sur réseau d'entreprise
## Document de planification

**Version** : 1.0
**Objet** : remplacement du mode point d'accès autonome de la carte AIO AGV control V5.0.1 par une connexion cliente au réseau Wi-Fi d'entreprise, avec appel par boutons EnOcean via un poste fixe UniPi.

---

## 1. Architecture cible

```
┌────────────────────┐
│  Boutons EnOcean   │   PTM 210 — auto-alimentés, sans pile
│  (× N marqueurs)   │   868 MHz, 3 sous-télégrammes par appui
└─────────┬──────────┘
          │  EnOcean 868 MHz (~30 m indoor)
          ▼
┌──────────────────────────────────────────────────┐
│         POSTE FIXE — UniPi E413                   │
│  • TCM 515 (récepteur EnOcean, UART/ESP3)        │
│  • Broker MQTT local (Mosquitto)                  │
│  • Table de correspondance ID EnOcean → station   │
│  • Interface web de supervision (état AGV)        │
│  • Raccordement Ethernet filaire au réseau usine  │
└─────────┬────────────────────────────────────────┘
          │  Réseau d'entreprise (VLAN dédié OT)
          │  MQTT/TLS
          ▼
     ┌────────────┐
     │  AP Wi-Fi  │   Infrastructure existante du client
     └─────┬──────┘
           │  802.11 (2,4 GHz avec ESP32-WROOM-32E)
           ▼
┌──────────────────────────────────────────────────┐
│              AGV — Carte V5.0.1                   │
│  • ESP32 : client Wi-Fi (STA) + client MQTT       │
│            + liaison série vers ATmega            │
│  • ATmega2560 : séquenceur X/Y, file de courses,  │
│            décodage position 10 bits              │
│  • Bus X/Y → automate Meiden (SUB-D 25 × 2)       │
└──────────────────────────────────────────────────┘
```

### Différences par rapport à l'existant

| | Carte V5.0.1 actuelle | Architecture cible |
|---|---|---|
| Rôle Wi-Fi de l'ESP32 | Point d'accès (`agv_atelier`) | Client du réseau d'entreprise |
| Source des ordres | App mobile AIO AGV Remote | Poste fixe UniPi via MQTT |
| Diagnostic | Page `/agvdump` sur 192.168.4.1 | Interface web sur le poste fixe |
| Déclenchement | App mobile | Boutons EnOcean physiques |
| Matériel AGV | ESP32 + ATmega2560 | **Inchangé** (firmware seul) |

---

## 2. Protocole applicatif — MQTT

Broker Mosquitto hébergé sur l'UniPi. L'ESP32 est client, le poste fixe est client et broker.

| Topic | Sens | QoS | Retained | Charge utile |
|---|---|---|---|---|
| `agv/1/state` | AGV → poste | 1 | oui | `{station, moving, fault, speed, battery, ts}` |
| `agv/1/cmd` | poste → AGV | 1 | non | `{seq, dest, speed, ts}` |
| `agv/1/ack` | AGV → poste | 1 | non | `{seq, status, ts}` |
| `agv/1/status` | LWT | 1 | oui | `online` / `offline` |
| `poste/1/button/<id>` | poste → journal | 0 | non | traçabilité des appuis |

**Mécanismes retenus** :
- Publication de `state` toutes les 1 s → détection de perte de liaison par péremption côté poste
- **Last Will and Testament** : le broker publie `offline` automatiquement si l'ESP32 disparaît sans se déconnecter proprement
- Numéro de séquence monotone sur `cmd` → rejet des doublons
- Horodatage + péremption 30 s → une commande retardée ne s'exécute jamais

**Repli de sécurité (non négociable)** : l'ESP32 entretient une ligne de heartbeat matérielle vers l'ATmega. Si le heartbeat disparaît plus de 2 s, l'ATmega termine le mouvement en cours, s'arrête à la station suivante et refuse toute nouvelle commande jusqu'au retour de la liaison. La chaîne de sécurité (arrêt d'urgence, bumpers, scrutateur) reste entièrement indépendante — cette carte est un organe de commande, pas de sécurité (ISO 3691-4).

---

## 3. Phase 0 — Prérequis bloquants

Aucune de ces tâches ne peut être contournée. Elles conditionnent toutes les phases suivantes.

| # | Tâche | Charge | Livrable |
|---|---|---|---|
| 0.1 | **Négociation service informatique** : VLAN OT dédié, réservation DHCP ou IP statique, règles de pare-feu, méthode d'authentification (WPA2-PSK ou 802.1X EAP), politique de notification de changement | 2-15 j (calendaire, hors contrôle) | Accord écrit + paramètres réseau |
| 0.2 | **Relevé de couverture Wi-Fi** à la hauteur réelle de l'antenne AGV, le long du trajet complet, en heures de production. RSSI + SNR + comptage des handovers | 1 j | Cartographie RSSI, décision 1 AP ou plusieurs |
| 0.3 | **Test de portée EnOcean** bouton → poste fixe, à chaque marqueur envisagé | 0,5 j | Marge RSSI par poste, besoin ou non de répéteur |
| 0.4 | **Rétro-ingénierie bus X/Y** : amplitude d'une ligne Y à l'oscillo (Y05), continuité du brochage SUB-D 25, chronogramme X/Y à l'analyseur logique (mesure de t_setup), confirmation logique PNP/NPN | 3-5 j | Table de vérité + chronogramme documenté |
| 0.5 | **Tentative de lecture des flash existantes** (`esptool read_flash`, `avrdude` via l'ICSP présent sur la carte) | 0,5 j | Firmware récupéré ou confirmation de verrouillage |
| 0.6 | **Décision matérielle** : réutilisation de la carte V5.0.1 en l'état, ou nouvelle carte | 0,5 j | Arbitrage documenté |
| 0.7 | **Décision de bande** : ESP32-WROOM-32E en 2,4 GHz, ESP32-C5 bi-bande, ou client bridge industriel | 0,5 j | Arbitrage documenté |

**Charge Phase 0 : 8-13 jours-homme**, mais le chemin critique est la tâche 0.1, dont le délai calendaire ne dépend pas de vous. À lancer en premier, le jour 1.

---

## 4. Phase 1 — Infrastructure réseau

Prérequis : 0.1, 0.2, 0.7

| # | Tâche | Charge |
|---|---|---|
| 1.1 | Configuration VLAN, adressage, pare-feu côté client | 1 j |
| 1.2 | Installation et durcissement de Mosquitto sur l'UniPi (TLS, authentification par utilisateur, ACL par topic) | 1 j |
| 1.3 | Validation du positionnement des AP si la Phase 0.2 révèle des trous de couverture | 1-3 j |
| 1.4 | Test de handover si plusieurs AP : mesure du temps de reconnexion, activation de l'IP statique pour éliminer le délai DHCP | 1 j |

**Charge : 4-6 jours-homme**

---

## 5. Phase 2 — Firmware AGV

Prérequis : 0.4, 0.5, 0.6

| # | Tâche | Charge |
|---|---|---|
| 2.1 | **Séquenceur X/Y sur ATmega2560** : phases écriture (X92/X93/X94 + attente Y22), démarrage (X82 + attente Y05), arrêt (X83 + Y10), avec compteurs de tentatives | 5-8 j |
| 2.2 | Décodage de position 10 bits (Y23→Y34) et vitesse 4 bits (Y11→Y14) | 1 j |
| 2.3 | File de courses en RAM (jusqu'à 5 destinations, comme l'original) | 1-2 j |
| 2.4 | Protocole série ATmega ↔ ESP32 (trames avec CRC, commandes et remontées d'état) | 2 j |
| 2.5 | **Heartbeat matériel et repli de sécurité** sur l'ATmega | 1-2 j |
| 2.6 | ESP32 : client Wi-Fi STA, gestion de reconnexion, IP statique, WPA2-PSK ou 802.1X | 2 j |
| 2.7 | ESP32 : client MQTT (publication `state`/`ack`, abonnement `cmd`, LWT) | 2 j |
| 2.8 | ESP32 : Wi-Fi de maintenance à la demande (AP temporaire déclenché par ILS ou bouton, extinction auto après 10 min) pour préserver la procédure `agvdump` existante | 1-2 j |

**Charge : 15-21 jours-homme.** La tâche 2.1 est le poste le plus lourd et le plus risqué du projet — c'est la réécriture d'une logique temps réel dont on n'a pas les sources.

---

## 6. Phase 3 — Poste fixe UniPi

Prérequis : 0.1, 0.3

| # | Tâche | Charge |
|---|---|---|
| 3.1 | Pilote TCM 515 : lecture UART, décodage des trames ESP3, filtrage des sous-télégrammes redondants | 2-3 j |
| 3.2 | Table de correspondance ID EnOcean (32 bits usine) → numéro de station, avec mode *pairing* déclenchable depuis l'interface web | 2 j |
| 3.3 | Logique de traduction : télégramme EnOcean → publication MQTT `cmd` avec séquence et horodatage | 1 j |
| 3.4 | Interface web de supervision : état instantané (station, mouvement, défaut, vitesse, batterie) + **indicateur de fraîcheur de liaison** en WebSocket | 3-4 j |
| 3.5 | Journal d'événements (appuis, commandes, accusés, pertes de liaison) | 1-2 j |
| 3.6 | Accusé visuel au poste : pilotage d'un actionneur EnOcean en retour, ou voyant local | 1-2 j |
| 3.7 | Service systemd, démarrage automatique, watchdog applicatif | 1 j |

**Charge : 11-15 jours-homme**

---

## 7. Phase 4 — Intégration et essais

| # | Tâche | Charge |
|---|---|---|
| 4.1 | Essais au banc : ATmega + ESP32 + UniPi, sans AGV, avec simulateur de bus X/Y | 2-3 j |
| 4.2 | Essais sur AGV à l'arrêt : validation de la séquence complète sur le vrai automate | 2 j |
| 4.3 | Essais en mouvement sur le trajet complet | 2 j |
| 4.4 | **Campagne de validation en heures de production** : latence P50/P95/P99, taux de perte, handovers, comportement en coupure réseau volontaire | 3-5 j |
| 4.5 | Essais de dégradation : coupure AP, redémarrage UniPi, perte de heartbeat, bouton hors portée | 2 j |

**Charge : 11-15 jours-homme**

### Critères d'acceptation à faire valider par le client AVANT les essais

Faire fixer ces seuils par le client, puis les consigner au compte rendu — le critère devient le sien, pas le vôtre.

| Indicateur | Seuil à négocier |
|---|---|
| Latence appui bouton → départ AGV | P95 < 2 s (à confirmer avec le client) |
| Taux d'appels perdus | < 0,1 % |
| Perte de liaison Wi-Fi non détectée | 0 (le heartbeat doit toujours déclencher) |
| Temps de reprise après coupure AP | < 10 s |
| Portée EnOcean à chaque poste | marge ≥ 10 dB au-dessus de la sensibilité |

---

## 8. Phase 5 — Documentation et clôture

| # | Tâche | Charge |
|---|---|---|
| 5.1 | Mise à jour du wiki : procédures d'installation, de *pairing*, de diagnostic | 2-3 j |
| 5.2 | Documentation du protocole MQTT et du bus X/Y (pour la maintenabilité future) | 2 j |
| 5.3 | Dossier technique et conformité RED si une nouvelle carte est produite (marquage CE, déclaration UE de conformité, conservation 10 ans) | 3-5 j |
| 5.4 | Formation des opérateurs et de la maintenance | 1 j |

**Charge : 8-11 jours-homme**

---

## 9. Récapitulatif de charge

| Phase | Charge | Peut démarrer |
|---|---|---|
| 0 — Prérequis | 8-13 j | Immédiatement |
| 1 — Infrastructure réseau | 4-6 j | Après 0.1, 0.2, 0.7 |
| 2 — Firmware AGV | 15-21 j | Après 0.4, 0.5, 0.6 |
| 3 — Poste fixe | 11-15 j | Après 0.1, 0.3 — **en parallèle de la Phase 2** |
| 4 — Intégration | 11-15 j | Après 2 et 3 |
| 5 — Documentation | 8-11 j | En continu, clôture après 4 |
| **Total** | **57-81 jours-homme** | |

Les Phases 2 et 3 sont parallélisables si deux personnes travaillent sur le projet. En solo, compter sur la somme.

**Chemin critique** : 0.1 (accord IT) → 0.4 (rétro-ingénierie) → 2.1 (séquenceur ATmega) → 4.4 (validation en production).

---

## 10. Registre des risques

| Risque | Impact | Probabilité | Parade |
|---|---|---|---|
| **Refus ou délai du service informatique** | Bloque tout le projet | Moyenne | Lancer 0.1 le jour 1 ; préparer l'architecture LoRa en repli documenté |
| **Trous de couverture à hauteur d'AGV** | Appels perdus en zone morte | Moyenne | Relevé 0.2 avant tout engagement ; ajout d'AP ou repli LoRa |
| **Séquenceur ATmega plus complexe que prévu** | Dérive de 5-10 j | Élevée | Sniffer le bus avec l'app existante avant de coder ; obtenir le manuel d'interface E/S Meiden auprès du fournisseur |
| **Roaming ESP32 insuffisant entre AP** | Coupures de 2-5 s en mouvement | Faible si 1 seul AP | Valider en 1.4 ; IP statique ; sinon client bridge industriel |
| **Portée EnOcean insuffisante à un poste** | Bouton inopérant, silencieusement | Moyenne | Répéteur niveau 1/2, réception en diversité, antenne hors armoire, voyant d'accusé obligatoire |
| **Maintenance IT non notifiée** | Arrêt de production imprévu | Moyenne | Faire classer l'AGV équipement critique ; dégradation propre au repli |
| **Bande 2,4 GHz toujours encombrée** | Objectif initial partiellement manqué | Moyenne | Arbitrage 0.7 ; mesurer avant/après pour objectiver le gain |

---

## 11. Nomenclature indicative

| Élément | Qté | Prix unitaire indicatif |
|---|---|---|
| Boutons EnOcean PTM 210 (avec enveloppe) | N | 40-60 € |
| Récepteur TCM 515 | 1 | 20-30 € |
| UniPi E413 (1 Go / 8 Go eMMC) | 1 | ~375 € |
| Actionneur EnOcean pour voyant d'accusé | N | 50-80 € |
| Répéteur EnOcean (si nécessaire) | 0-N | 100-150 € |
| Carte AGV | 0 ou 1 | 0 € si réutilisée |
| Antenne Wi-Fi déportée AGV + pigtail | 1 | ~30 € |
| Points d'accès Wi-Fi additionnels (si nécessaire) | 0-N | à charge du client |

---

## 12. Points à trancher avec le client

1. **Le sans-pile est-il une exigence réelle** ou un confort ? Si c'est un confort, des boutons Wi-Fi ou filaires simplifient l'architecture (mais l'autonomie d'un bouton Wi-Fi sur pile se compte en mois, pas en années — l'EnOcean garde donc l'avantage).
2. **Historique ou état instantané ?** Si un historique consultable sur plusieurs semaines est attendu, l'UniPi est justifié. Sinon, un ESP32 avec module Ethernet ferait le même travail pour 10 € au lieu de 375 €.
3. **Le service informatique acceptera-t-il un équipement OT sur son réseau**, avec VLAN dédié et notification de changement ? La réponse conditionne le choix entre cette architecture et l'architecture LoRa.
4. **Quel seuil de latence** au-delà duquel un opérateur considère que l'appel a échoué ? À faire fixer par le client avant les essais.
5. **Combien de marqueurs à terme ?** Le bus 10 bits de l'AGV en adresse 1024, donc aucune limite matérielle — mais le nombre de boutons conditionne la nomenclature et les tests de portée.

---

*Document de planification — à mettre à jour au fil des arbitrages. Voir le document de synthèse des architectures pour la comparaison avec les variantes LoRa et cellulaire.*
