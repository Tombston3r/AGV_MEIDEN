# Table des signaux — bus MEIDEN

> Source de vérité exécutable : `profiles/default.yaml`, section `pinmap`.
> Ce document explique ; il ne configure rien. En cas d'écart, **le profil YAML
> fait foi** — et si le profil diverge du comportement observé sur la
> V5.0.1, **c'est la carte d'origine qui fait foi** (brief §15).

## Numérotation octale

Convention Meiden/Mitsubishi : les repères sont en **octal**. Après `Y27` vient
`Y30`, pas `Y28`. Conséquences directes :

- `Y23`…`Y34` = **10 signaux**, pas 12 (`octal_span(23, 34) == 10`).
- `X96`, `X97`, `XA0`…`XA7` = 10 bits d'adresse.
- Toute itération de plage doit passer par `octal_step()` / `octal_span()` de
  `bus/bus_signals.h`. Une boucle décimale inventerait `Y28` et `Y29`.

## Bus X — 22 sorties (carte → automate)

| Signal | Bit | Groupe | Rôle |
|---|---:|---|---|
| `X82` | 0 | Marche/arrêt | standby release / start |
| `X83` | 1 | Marche/arrêt | standby stop |
| `X84` | 2 | Aiguillage | changement de direction |
| `X85` | 3 | Sens | |
| `X86` | 4 | Vitesse | b0 |
| `X87` | 5 | Vitesse | b1 |
| `X90` | 6 | Vitesse | b2 |
| `X91` | 7 | Vitesse | b3 |
| `X92` | 8 | Protocole | instruction data input switch |
| `X93` | 9 | Protocole | write strobe |
| `X94` | 10 | Protocole | type de donnée (station / comptage de marqueurs) |
| `X95` | 11 | Protocole | frein externe |
| `X96` | 12 | Destination | b0 |
| `X97` | 13 | Destination | b1 |
| `XA0`…`XA7` | 14…21 | Destination | b2…b9 |

## Bus Y — 21 entrées (automate → carte)

| Signal | Bit | Groupe | Rôle |
|---|---:|---|---|
| `Y03` | 0 | Diagnostic | défaut |
| `Y05` | 1 | Mouvement | moving flag |
| `Y10` | 2 | Mouvement | in station flag |
| `Y11`…`Y14` | 3…6 | Vitesse courante | b0…b3 |
| `Y15` | 7 | Monitor | écho aiguillage |
| `Y20` | 8 | Monitor | écho sens |
| `Y21` | 9 | Diagnostic | pas de destination programmée |
| `Y22` | 10 | Monitor | instruction reading complete |
| `Y23`…`Y27` | 11…15 | Position courante | b0…b4 |
| `Y30`…`Y34` | 16…20 | Position courante | b5…b9 |

## Brochage physique — RELEVÉ

Le câblage signal ↔ broche SUB-D 25 ↔ connecteur AGV est connu : **CN61 à CN64**
(la divergence CN61/62/63 vs CN62/63/64 est levée). La table complète, avec les
libellés constructeur, est dans
[`../../A4_Wifi/docs/subd25_atmega.md`](../../A4_Wifi/docs/subd25_atmega.md) — elle
décrit le côté AGV, donc elle vaut pour toutes les architectures.

Ce relevé confirme aussi **l'ordre des bits** : `Station/Marker ×1` … `×512` sur
`X96`, `X97`, `XA0`…`XA7`, et `Speed ×1` … `×8` sur `X86`, `X87`, `X90`, `X91`.
`kDefaultLayout` est correct.

⚠️ **Les niveaux électriques du bus restent inconnus.** Le L7806CV de la carte
d'origine s'est révélé être l'alimentation du microcontrôleur (24 V venant de
CN64 A6/B6, abaissés à 6 V) : il ne renseigne donc pas sur les signaux.
L'amplitude des lignes Y (§12.1) et la topologie des entrées de l'automate sont
à mesurer avant de dimensionner toute nouvelle carte — voir l'avertissement du
document ci-dessus.

## Points non relevés

- **Repères sérigraphiés** T9, T10, T12, T13, T20…T24 de la V5.0.1 : la
  correspondance avec les signaux Y n'est pas établie. Le code ne s'en sert pas ;
  seules les procédures d'atelier en ont besoin.
- **Polarité** : `bus.x_active_high` / `bus.y_active_high`. La logique
  applicative est TOUJOURS en actif haut ; la conversion PNP/NPN n'a lieu qu'au
  contact du matériel (`Sequencer::write_bus` et `sample_inputs`).
