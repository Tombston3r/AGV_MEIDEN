# Protocole inter-MCU ESP32 ↔ ATmega2560

Variante C du §4.4 (« ATmega2560 conservé »). Le MEGA garde ce qu'il fait mieux
que les alternatives : poser 22 lignes en un cycle (`PORTx = valeur`, < 1 µs,
strictement simultané).

## Principe

Le MEGA est un **organe de pose**, rien de plus. Il n'applique ni `t_setup`, ni
polarité, ni timeout, et ne connaît ni la file de courses ni le séquenceur.
Toute la logique (donc tous les paramètres du §12) reste dans l'ESP32.

Motif : si les deux firmwares portaient la même vérité (un `t_setup` ici, un
autre là), ils divergeraient au premier relevé et personne ne saurait lequel
fait foi.

## Liaison

UART, **500 000 bauds**, 8N1. `Serial1` côté MEGA, `UART_NUM_2` côté ESP32.

## Format

```
ESP32 -> MEGA : A5 | cmd | len | payload[len] | crc16_hi | crc16_lo
MEGA  -> ESP32: 5A | cmd | len | payload[len] | crc16_hi | crc16_lo
```

- CRC-16/CCITT-FALSE calculé sur `cmd | len | payload` (le SOF est exclu).
- Le SOF diffère dans chaque sens : une trame réfléchie ne peut pas être
  confondue avec une requête.
- Trame à CRC faux côté MEGA : **silence**. L'ESP32 gère le timeout et compte la
  désynchronisation (`MegaUartBus::desync_count()`), ce qui remonte dans
  `agvdump`.

## Commandes

| cmd | Nom | Requête | Réponse |
|---:|---|---|---|
| 0x01 | `SET_X` | 3 octets (22 bits utiles, poids forts en tête) | vide |
| 0x02 | `GET_Y` | vide | 3 octets (21 bits utiles) |
| 0x03 | `PULSE` | bit (1) + durée µs (2, big endian) | vide |
| 0x04 | `PING` | vide | version du firmware (1) |

## Exemple

`SET_X` avec le mot `0x0012_34` :

```
A5 01 03 00 12 34 <crc_hi> <crc_lo>
```

Réponse :

```
5A 01 00 <crc_hi> <crc_lo>
```

## Contraintes de temps

- L'ESP32 borne son attente de réponse (20 ms par défaut). Un MEGA muet fait
  échouer la pose, ce qui met le séquenceur en défaut `BUS_WRITE_ERROR`, jamais
  un blocage de la tâche bus.
- L'aller-retour complet à 500 kbauds coûte ~0,3 ms pour 8 octets dans chaque
  sens. C'est **le** coût de cette variante : la pose est instantanée, mais elle
  est précédée d'un aller-retour UART. À comparer aux ~150 µs des MCP23017 et
  aux ~3 µs des 74HC595, qui n'ont, eux, pas de liaison intermédiaire.

## Mise à zéro au démarrage

Le `setup()` du MEGA force `PORTA`, `PORTC` et les bits utiles de `PORTL` à zéro
**avant toute autre instruction**, conformément au §3.1. L'ESP32 le refait via
`writeX(0)` à l'issue du `PING` : la double sécurité est volontaire, les deux
cartes ne démarrent pas forcément en même temps.
