# Questions ouvertes — à poser avant de figer quoi que ce soit

> Reprend le §12 du brief. Chaque ligne est un paramètre du logiciel, jamais une
> constante. Tant qu'une case « Relevé » est vide, la valeur du profil est
> PROVISOIRE et doit être traitée comme telle en revue.

| # | Point | Valeur provisoire retenue | Où elle vit | Relevé |
|---|---|---|---|---|
| 12.1 | Amplitude réelle des lignes Y (6 V rail LM7806 ou 24 V) | `y_debounce_us: 2000` | `profiles/*.yaml` | ☐ |
| 12.2 | Brochage SUB-D 25 | **RELEVÉ** : CN61 à CN64, table complète | `firmware/mega/src/board_ports.h` | ✅ |
| 12.3 | Logique automate PNP ou NPN | `x_active_high: true`, `y_active_high: true` | `profiles/*.yaml` | ☐ |
| 12.4 | `t_setup` avant strobe X93 | `t_setup_us: 200` | `profiles/*.yaml` | ☐ |
| 12.5 | Timeouts Y22 / Y05 / Y10 | 300 ms / 1 500 ms / 120 s | `profiles/*.yaml` | ☐ |
| 12.6 | Ordre des bits d'adresse et de vitesse | **CONFIRMÉ** par les libellés ×1…×512 du relevé | `bus/bus_signals.h` | ✅ |
| 12.6b | Repères sérigraphiés T9…T24 ↔ signaux Y | non utilisés par le code | `docs/signal_map.md` | ☐ |
| 12.7 | Protocole application mobile « AIO AGV Remote » | **abandonné** au profit des boutons EnOcean + IHM web (planif. §1) | — | ✅ |
| 12.8 | TCM 515 (Rx seul) ou TCM 310 (bidirectionnel) | `enocean.rx_only: true` | `profiles/*.yaml` | ☐ |
| 12.9 | Runtime de l'UniPi E413 commandé | aucun backend par défaut ; l'UniPi porte AUSSI le broker | `poste-unipi/` | ☐ |
| 12.10 | Variante d'interface bus | **tranché** : carte conservée, ports de l'ATmega | `bus/avr_port_bus.h` | ✅ |
| W1 | Câblage ATmega ↔ SUB-D 25 | **RELEVÉ** : 11 ports, 3 mixtes (PA, PB, PG) | `firmware/mega/src/board_ports.h` | ✅ |
| **W1b** | **Amplitude des lignes Y** — arrivent directement sur des broches d'ATmega | inconnue | matériel | ☐ **BLOQUANT** |
| W1d | Topologie des entrées de l'automate (tirées ou attendant un courant ?) | `x_open_drain: true` (mode qui ne détruit rien) | `profiles/*.yaml` | ☐ |
| W1e | Tension V_CC réelle de l'ATmega (le L7806CV sort 6 V) | non mesurée | matériel | ☐ |
| W1c | Pull-ups internes sur les Y (collecteur ouvert ?) | `y_pullups: false` | `profiles/*.yaml` | ☐ |
| W2 | UART reliant l'ESP32 à l'ATmega | `Serial1` / `UART1` supposés | `board_ports.h`, `board_pins.h` | ☐ |
| W3 | Ligne de heartbeat matérielle dédiée | aucune identifiée ; heartbeat par trame série | `board_ports.h` | ☐ |
| W4 | Connecteur ICSP pour flasher l'ATmega | supposé présent | procédure de déploiement | ☐ |
| W5 | Paramètres réseau (SSID, IP, VLAN, 802.1X) | placeholders | `profiles/*.yaml` | ☐ |
| W6 | Identifiants et CA du broker MQTT | placeholders | `profiles/*.yaml` | ☐ |
| W7 | Mesure de tension batterie accessible à l'ESP32 | supposée absente | télémétrie | ☐ |

## Questions à poser au client / à l'atelier

### Électrique — à lever avant le premier branchement

Le L7806CV est l'**alimentation** de l'ATmega : 24 V venant de CN64 A6/B6,
abaissés à 6 V. Il ne renseigne donc en rien sur les niveaux du bus, qui
restent entièrement inconnus.

0. **Quelle est l'amplitude réelle des lignes Y ?** Elles arrivent directement
   sur des broches d'ATmega. Au-delà de V_CC + 0,5 V, l'entrée est détruite.
   Mesurer sur `Y05` **avant de brancher la nappe d'entrées**. C'est la mesure
   la plus urgente du projet.
0b. **Quelle tension arrive réellement sur V_CC de l'ATmega ?** Si le 6 V du
   L7806CV l'alimente directement, on est au **maximum absolu** du datasheet
   (6,0 V) et hors plage recommandée (4,5–5,5 V à 16 MHz) — un L7806 sort
   jusqu'à 6,24 V. Vérifier si le 6 V va sur V_CC, sur un second régulateur, ou
   sur `Vin` d'une carte Arduino (où 6 V est au contraire trop bas).
0c. **Les entrées de l'automate sont-elles tirées vers une tension, ou
   attendent-elles un courant fourni par la carte ?** Le firmware part en
   collecteur ouvert : il ne peut rien détruire, mais il est inopérant dans le
   second cas. La mesure tranche, et c'est un simple paramètre.
0d. **Les sorties de l'automate (lignes Y) sont-elles à collecteur ouvert ou
   poussées ?** Collecteur ouvert → `bus.y_pullups: true` indispensable ;
   poussées → pull-up nuisible. Non devinable.

### Chronogrammes (bloquant pour la mise en service)
1. Quelle est l'amplitude mesurée sur `Y05` à l'oscilloscope, machine en marche ?
2. Combien de temps la V5.0.1 laisse-t-elle stabiliser le bus X avant de monter
   `X93` ? (mesure entre le dernier front d'adresse et le front de `X93`)
3. Quel est le délai typique et le délai maximal observés entre `X93` et `Y22` ?
   entre `X82` et `Y05` ? Un percentile 99 vaut mieux qu'une moyenne.
4. `X83` est-il réellement utilisé en fin de course sur l'installation, ou
   l'arrêt est-il uniquement géré par l'automate ?

### Câblage
5. ~~Laquelle des deux tables de câblage fait foi ?~~ **Tranché** : CN61 à CN64,
   voir `docs/subd25_atmega.md`. Reste à confirmer par contrôle au multimètre
   qu'aucune nappe n'est sertie à l'envers (mode découverte du firmware MEGA).
6. Les repères sérigraphiés T9, T10, T12, T13, T20…T24 correspondent à quels
   signaux Y ? (utile pour les procédures d'atelier, pas pour le code)
7. Les entrées de l'automate sont-elles PNP ou NPN ?

### Exploitation
8. Une sortie réelle de `agvdump` de la V5.0.1 peut-elle être fournie ? Le
   format actuel reprend les noms de compteurs mais pas la mise en page exacte.
9. L'application mobile « AIO AGV Remote » peut-elle être définitivement
   abandonnée ? La planification la remplace par les boutons EnOcean et l'IHM
   web du poste — à confirmer avec les opérateurs, pas seulement avec le
   donneur d'ordre.
10. Un accusé de réception côté opérateur est-il exigé ? Le TCM 515 est en
    réception seule : le retour passe par un voyant câblé au poste ou un
    actionneur EnOcean séparé (planification §3.6 et nomenclature).
11. Quelle est la référence exacte de l'UniPi E413 commandée, et quel OS y
    tourne ? Ici l'UniPi porte le service Python **et** le broker Mosquitto :
    sous Mervis, toute l'architecture du poste est à revoir.
12b. Un historique consultable sur plusieurs semaines est-il attendu, ou un
    état instantané suffit-il ? Si l'instantané suffit, un ESP32 + Ethernet
    ferait le travail du poste pour ~10 € au lieu de ~375 €
    (planification §12.2).
12. Combien de temps une course en attente reste-t-elle pertinente après une
    coupure d'alimentation ? (`course_validity_min`, 30 min par défaut)

### Réseau — PRÉREQUIS BLOQUANTS de cette architecture
13. Le relevé **RSRP/RSRQ en tous points du parcours**, aux heures de
    production et machines en marche, a-t-il été fait ? Un seul point d'arrêt
    sous −110 dBm **disqualifie l'architecture** (Archi_2 §7.1).
14. L'essai de latence réel sur 200 aller-retours a-t-il été fait, avec la
    distribution complète ? C'est le 99ᵉ percentile qui compte, pas la moyenne
    (Archi_2 §7.2).
15. Le calendrier d'extinction 2G/3G des opérateurs a-t-il été vérifié à jour ?
    Il impose de partir sur LTE-M/Cat-M1 ou Cat-1 bis (Archi_2 §5).
16. Le chiffrage sur 10 ans a-t-il été mis en regard des architectures sans
    coût récurrent ? (~15 000 € en SMS contre ~0 € en ISM — Archi_2 §6.2)
