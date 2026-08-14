# SUB-D 25 ↔ ATmega2560 — table de câblage

> ⚠️ **CE DOCUMENT EST UNE HYPOTHÈSE, PAS UN RELEVÉ.**
> Le câblage est imposé par le PCB de la V5.0.1, dont on n'a ni le schéma ni le
> firmware. Deux tables de câblage circulent et se contredisent :
> **CN61/62/63** contre **CN62/63/64** (§12.2). Rien ne doit être branché sur
> l'automate avant la procédure de relevé ci-dessous.

## Pourquoi c'est dangereux de se tromper

Un mot d'adresse posé sur les mauvaises broches **ne provoque pas de panne
franche**. L'AGV part — vers la mauvaise station. C'est exactement le genre de
défaut qu'on ne veut pas découvrir en production, et c'est pourquoi le relevé
est un prérequis bloquant (planification 0.4).

## Hypothèse de travail

Source exécutable : `firmware/mega/src/board_ports.h`. Toute correction se fait
là, puis se recopie ici.

### Bus X — 22 sorties (carte → automate)

| Bits | Port ATmega | Signaux | SUB-D / broches |
|---|---|---|---|
| 0–7 | `PORTA` | `X82` `X83` `X84` `X85` `X86` `X87` `X90` `X91` | **à relever** |
| 8–15 | `PORTC` | `X92` `X93` `X94` `X95` `X96` `X97` `XA0` `XA1` | **à relever** |
| 16–21 | `PORTL` (6 bits) | `XA2` `XA3` `XA4` `XA5` `XA6` `XA7` | **à relever** |

Les 2 bits de poids fort de `PORTL` ne sont **pas** écrasés par le firmware :
ils peuvent servir à une autre fonction de la carte.

### Bus Y — 21 entrées (automate → carte)

| Bits | Port ATmega | Signaux | SUB-D / broches |
|---|---|---|---|
| 0–7 | `PINK` | `Y03` `Y05` `Y10` `Y11` `Y12` `Y13` `Y14` `Y15` | **à relever** |
| 8–15 | `PINF` | `Y20` `Y21` `Y22` `Y23` `Y24` `Y25` `Y26` `Y27` | **à relever** |
| 16–20 | `PINB` (5 bits) | `Y30` `Y31` `Y32` `Y33` `Y34` | **à relever** |

Rappel de numérotation **octale** : après `Y27` vient `Y30`. La plage
`Y23`…`Y34` compte **10** signaux, pas 12.

## Procédure de relevé — mode découverte

Le firmware ATmega embarque un mode qui active **une seule sortie X à la fois**
et annonce laquelle sur la console série.

**Automate DÉBRANCHÉ. Les deux SUB-D 25 doivent être déconnectés.**

1. Flasher le firmware MEGA (`pio run -e mega -t upload`).
2. Ouvrir la console série USB à 115 200 bauds.
3. Envoyer `d` → le firmware annonce `# DECOUVERTE bit X=0`, puis change
   d'index toutes les 3 secondes.
4. Pour chaque index annoncé, chercher au multimètre (mode continuité ou
   tension) quelle broche de quel SUB-D est active.
5. Consigner dans le tableau ci-dessous.
6. Envoyer `n` pour revenir en mode normal.

| Index bit X | Signal (selon `pinmap`) | Connecteur | Broche | Relevé le |
|---:|---|---|---|---|
| 0 | `X82` | | | |
| 1 | `X83` | | | |
| 2 | `X84` | | | |
| 3 | `X85` | | | |
| 4 | `X86` | | | |
| 5 | `X87` | | | |
| 6 | `X90` | | | |
| 7 | `X91` | | | |
| 8 | `X92` | | | |
| 9 | `X93` | | | |
| 10 | `X94` | | | |
| 11 | `X95` | | | |
| 12 | `X96` | | | |
| 13 | `X97` | | | |
| 14 | `XA0` | | | |
| 15 | `XA1` | | | |
| 16 | `XA2` | | | |
| 17 | `XA3` | | | |
| 18 | `XA4` | | | |
| 19 | `XA5` | | | |
| 20 | `XA6` | | | |
| 21 | `XA7` | | | |

## Relevé des entrées Y

Le mode découverte ne peut pas piloter les entrées. Deux méthodes :

- **côté automate** : forcer un signal Y depuis l'automate (mode manuel) et
  lire l'état renvoyé par la commande `GetState` de la liaison série ;
- **injection** : appliquer le niveau attendu sur une broche du SUB-D côté
  automate débranché, et observer quel bit change dans l'état renvoyé.

⚠️ L'amplitude réelle des lignes Y n'est pas connue (§12.1) — 6 V du rail
LM7806, ou 24 V. Mesurer **avant** d'injecter quoi que ce soit.

| Index bit Y | Signal | Connecteur | Broche | Amplitude mesurée | Relevé le |
|---:|---|---|---|---|---|
| 0 | `Y03` | | | | |
| 1 | `Y05` | | | | |
| 2 | `Y10` | | | | |
| 3–6 | `Y11`…`Y14` | | | | |
| 7 | `Y15` | | | | |
| 8 | `Y20` | | | | |
| 9 | `Y21` | | | | |
| 10 | `Y22` | | | | |
| 11–15 | `Y23`…`Y27` | | | | |
| 16–20 | `Y30`…`Y34` | | | | |

## Après le relevé

1. Corriger `firmware/mega/src/board_ports.h` si la répartition sur les ports
   diffère de l'hypothèse.
2. Corriger la section `pinmap` de `profiles/default.yaml` si l'ordre des bits
   diffère.
3. `python3 tools/genconfig.py profiles/default.yaml firmware/common/config/generated_profile.h`
4. `make test` — les tests doivent rester verts avec la vraie table.
5. Cocher la ligne §12.2 dans [`questions_ouvertes.md`](questions_ouvertes.md)
   et mettre à jour [`ETAT_PROJET.md`](ETAT_PROJET.md).
