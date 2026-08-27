# Bancs d'essai

Un banc **prouve quelque chose sur du matériel**, avant qu'on l'engage dans une
architecture. Il ne fait pas rouler l'AGV.

| Banc | Ce qu'il prouve | Matériel | Sans matériel ? |
|---|---|---|---|
| [`enocean/`](enocean/) | Les `PTM 210` émettent, sont identifiés, et un appui n'en fait qu'un | UniPi E413 + dongle EnOcean USB | ✅ `--simulation` |
| [`lora/`](lora/) | Deux extrémités LoRa se parlent **et se comprennent** | ESP32 + RFM95W, ou Linux + SPI | ✅ contrôle de trame |

## Ce que chaque banc contient

| Fichier | Rôle |
|---|---|
| `README.md` | à quoi il sert, comment le lancer, **ce qu'on doit obtenir** |
| `DEPLOY.md` | mise en service sur la machine cible |
| `tests/` | tests exécutables **sans matériel** |

C'est la contrainte de forme : **un banc qu'on ne peut pas éprouver sans
matériel n'est pas un banc, c'est un pari.** Chacun doit pouvoir tourner sur un
poste de développement avant de partir en atelier.

## Pourquoi ils ne sont pas dans les architectures

Les bancs LoRa vivaient dans `A3_LoRa/test/`, le banc EnOcean à la racine :
deux choses de même nature à deux endroits. Ils servent d'ailleurs à plusieurs
architectures — l'EnOcean vaut pour A1, A2 et A4.

`architectures/*/test/native/` ne contient plus que les **tests unitaires** du
cœur métier, qui tournent avec `make test`.
