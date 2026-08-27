# Latence LoRa et budget de rapport cyclique : chiffres à valider avec le client

> Ce document remonte un écart entre l'objectif de latence du brief (§6) et ce
> que donne le paramétrage radio annoncé. Il ne tranche pas : c'est un
> arbitrage portée / latence qui appartient au client.

## Temps d'antenne mesuré par le code

Formule Semtech AN1200.13, implémentée dans `transport/duty_cycle.cpp` et
vérifiée par test (`temps_d_antenne_lora_sf9_ordre_de_grandeur`).

Trame applicative : 9 octets de base + 6 d'en-tête chiffré = **15 octets**
(13 octets si l'horodatage est absent, ce qui est le cas en LoRa).

| SF | BW | Temps d'antenne (13 o.) | Aller-retour cmd+ACK | 3 tentatives |
|---:|---:|---:|---:|---:|
| 7 | 125 kHz | ~46 ms | ~92 ms | ~280 ms |
| 9 | 125 kHz | **~165 ms** | **~330 ms** | **~990 ms** |
| 12 | 125 kHz | ~1 320 ms | ~2 640 ms | ~7 900 ms |

## Écart avec l'objectif du brief

Le §6 vise « latence cible ~200 ms, pire cas ~800 ms après 3 retransmissions »
**avec SF9**. Les deux chiffres sont incompatibles avec SF9 : l'aller simple y
coûte déjà 165 ms, l'aller-retour 330 ms, et trois tentatives frôlent la
seconde.

Trois issues possibles, à trancher avec le client :

1. **SF7** : tient la cible (92 ms aller-retour, 280 ms pire cas) au prix d'une
   portée et d'une pénétration réduites. À valider par un relevé de couverture
   le long du parcours, machines en marche.
2. **SF9 conservé, objectif de latence révisé** à ~350 ms typique / ~1 s pire
   cas. Pour un AGV appelé à un point d'arrêt, c'est probablement sans
   conséquence opérationnelle, mais ça doit être dit, pas subi.
3. **SF adaptatif** : SF7 en nominal, repli SF9 sur échec. Complexité
   supplémentaire dans l'ordonnanceur half-duplex ; à ne faire que si le relevé
   montre des zones réellement difficiles.

Le paramètre vit dans `profiles/*.yaml` (`lora.spreading_factor`) : changer de
SF ne demande aucune modification de code.

## Budget de rapport cyclique : obligation réglementaire

EN 300 220 / ERC 70-03 : **1 % sur 1 h glissante**, soit 36 s de temps d'antenne
par heure. Implémenté dans `DutyCycleBudget`, qui **refuse** l'émission au-delà
et remonte le refus en défaut applicatif visible (IHM et `agvdump`).

| SF | Émissions max / heure (budget seul) |
|---:|---:|
| 7 | ~780 |
| 9 | ~218 |
| 12 | ~27 |

À SF9, en comptant commande + ACK + télémétrie, le budget devient le facteur
limitant bien avant la latence :

- télémétrie toutes les 2 s = 1 800 émissions/h → **très au-delà du budget** ;
- la période de télémétrie doit donc être choisie en fonction du SF retenu.

**Recommandation** : à SF9, télémétrie à 30 s (120 émissions/h, ~20 s
d'antenne, 55 % du budget) en laissant la marge restante aux commandes. À SF7,
une télémétrie à 10 s reste confortable. `AgvApp::set_telemetry_period_ms()`
prend cette valeur ; elle n'est pas figée dans le code.
