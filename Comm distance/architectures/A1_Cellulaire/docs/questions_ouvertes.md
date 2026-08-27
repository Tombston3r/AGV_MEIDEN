# Questions ouvertes, à poser avant de figer quoi que ce soit

> Reprend le §12 du brief. Chaque ligne est un paramètre du logiciel, jamais une
> constante. Tant qu'une case « Relevé » est vide, la valeur du profil est
> PROVISOIRE et doit être traitée comme telle en revue.

| # | Point | Valeur provisoire retenue | Où elle vit | Relevé |
|---|---|---|---|---|
| 12.1 | Amplitude réelle des lignes Y (6 V rail LM7806 ou 24 V) | `y_debounce_us: 2000` | `profiles/*.yaml` | ☐ |
| 12.2 | Brochage SUB-D 25 | **RELEVÉ** : CN61 à CN64, voir `Wifi/docs/subd25_atmega.md` | `profiles/*.yaml` → `CFG_PIN_*` | ✅ |
| 12.3 | Logique automate PNP ou NPN | `x_active_high: true`, `y_active_high: true` | `profiles/*.yaml` | ☐ |
| 12.4 | `t_setup` avant strobe X93 | `t_setup_us: 200` | `profiles/*.yaml` | ☐ |
| 12.5 | Timeouts Y22 / Y05 / Y10 | 300 ms / 1 500 ms / 120 s | `profiles/*.yaml` | ☐ |
| 12.6 | Ordre des bits d'adresse et de vitesse | **CONFIRMÉ** par les libellés ×1…×512 du relevé | `bus/bus_signals.h` | ✅ |
| 12.6b | Repères sérigraphiés T9…T24 ↔ signaux Y | non utilisés par le code | `docs/signal_map.md` | ☐ |
| 12.7 | Protocole application mobile « AIO AGV Remote » | non reproduit ; champ `ver` réservé | `proto/frame.h` | ☐ |
| 12.8 | TCM 515 (Rx seul) ou TCM 310 (bidirectionnel) | `enocean.rx_only: true` | `profiles/*.yaml` | ☐ |
| - | Numéros MSISDN (pair et technicien d'astreinte) | `+33600000000` | `profiles/*.yaml` | ☐ |
| 12.9 | Runtime de l'UniPi E413 commandé | aucun backend par défaut | `poste-unipi/agv_poste/io_backend.py` | ☐ |
| 12.10 | Variante d'interface bus | trois implémentations livrées | `profiles/*.yaml` → `driver_variant` | ☐ |

## Questions à poser au client / à l'atelier

### Chronogrammes (bloquant pour la mise en service)
1. Quelle est l'amplitude mesurée sur `Y05` à l'oscilloscope, machine en marche ?
2. Combien de temps la V5.0.1 laisse-t-elle stabiliser le bus X avant de monter
   `X93` ? (mesure entre le dernier front d'adresse et le front de `X93`)
3. Quel est le délai typique et le délai maximal observés entre `X93` et `Y22` ?
   entre `X82` et `Y05` ? Un percentile 99 vaut mieux qu'une moyenne.
4. `X83` est-il réellement utilisé en fin de course sur l'installation, ou
   l'arrêt est-il uniquement géré par l'automate ?

### Câblage
5. ~~Laquelle des deux tables de câblage fait foi ?~~ **Tranché** : CN61 à CN64.
6. Les repères sérigraphiés T9, T10, T12, T13, T20…T24 correspondent à quels
   signaux Y ? (utile pour les procédures d'atelier, pas pour le code)
7. Les entrées de l'automate sont-elles PNP ou NPN ?

### Exploitation
8. Une sortie réelle de `agvdump` de la V5.0.1 peut-elle être fournie ? Le
   format actuel reprend les noms de compteurs mais pas la mise en page exacte.
9. L'application mobile « AIO AGV Remote » est-elle encore utilisée ? Peut-elle
   être abandonnée au profit de l'interface web ?
10. Un accusé de réception côté opérateur EnOcean est-il exigé ? Si oui, le
    surcoût d'un TCM 310 (bidirectionnel) est-il accepté ?
11. Quelle est la référence exacte de l'UniPi E413 commandée, et quel OS y
    tourne ? (conditionne l'existence même du service Python)
12. Combien de temps une course en attente reste-t-elle pertinente après une
    coupure d'alimentation ? (`course_validity_min`, 30 min par défaut)

### Radio : PRÉREQUIS BLOQUANTS de cette architecture
13. Le relevé **RSRP/RSRQ en tous points du parcours**, aux heures de
    production et machines en marche, a-t-il été fait ? Un seul point d'arrêt
    sous −110 dBm **disqualifie l'architecture** (Archi_2 §7.1).
14. L'essai de latence réel sur 200 aller-retours a-t-il été fait, avec la
    distribution complète ? C'est le 99ᵉ percentile qui compte, pas la moyenne
    (Archi_2 §7.2).
15. Le calendrier d'extinction 2G/3G des opérateurs a-t-il été vérifié à jour ?
    Il impose de partir sur LTE-M/Cat-M1 ou Cat-1 bis (Archi_2 §5).
16. Le chiffrage sur 10 ans a-t-il été mis en regard des architectures sans
    coût récurrent ? (~15 000 € en SMS contre ~0 € en ISM : Archi_2 §6.2)
