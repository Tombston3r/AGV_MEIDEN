# Mise en œuvre du banc API

Ce banc tourne **sur un poste de développement**, pas sur la carte. Il n'y a ni
matériel ni réseau : c'est précisément ce qui permet d'éprouver le contrat de
l'API avant qu'un ESP32 n'existe.

## 0. Ce qu'il faut avoir

- un compilateur C++17 (`g++` ou `clang++`) ;
- `python3` pour les tests de contrat ;
- rien d'autre — aucune bibliothèque, aucun accès réseau.

## 1. Construire

```bash
cd Timer
make test
```

**Attendu :**

```
19 tests, 0 echec(s)      moteur
13 tests, 0 echec(s)      codec JSON
Ran 10 tests ... OK       contrat API
```

Si la compilation échoue, c'est le compilateur qui est trop ancien : `-std=c++17`
et `std::optional` sont requis.

## 2. Lancer

```bash
make banc
```

**Attendu :**

```
BANC_API PORT=8081
banc API planning sur http://127.0.0.1:8081 — horloge simulee x1
```

Le banc écoute **uniquement sur `127.0.0.1`**, en dur dans le code. Il n'est
joignable ni depuis le réseau, ni depuis une autre machine — voir §5.

`--port 0` fait choisir un port libre par le système, et la ligne `BANC_API
PORT=` l'annonce : c'est ce qu'utilisent les tests pour tourner en parallèle
sans se marcher dessus.

## 3. Recette — neuf gestes

Ouvrir <http://127.0.0.1:8081>.

| # | Geste | Attendu |
|---|---|---|
| 1 | Charger la page | Bandeau **rouge** « planning non validé », horloge qui avance |
| 2 | Publier le planning (textarea → **Publier**) | ETag passe à `"v2"`, les prochaines occurrences se remplissent |
| 3 | « Aller à » demain 05:59, puis **Valider la journée** | Bandeau **vert** avec votre nom |
| 4 | **+1 min** | Mission dans « Missions parties », journal `executee` |
| 5 | **+1 jour** | Bandeau repasse **rouge** : la validation d'hier ne vaut plus |
| 6 | Valider, **simuler heure douteuse**, +1 min | **Rien ne part**, badge `HEURE NON FIABLE — GELÉ` |
| 7 | Rétablir l'heure (dans les 5 min simulées) | La mission part : le gel n'a rien perdu |
| 8 | Modifier le planning, **Publier** | Bandeau repasse **rouge** : toute édition révoque l'autorisation |
| 9 | Deux onglets, publier dans l'un puis dans l'autre | Le second reçoit **409 version périmée** |

Les points **4 à 8** sont ceux qui comptent. Une mission qui partirait sans
validation, ou qui repartirait deux fois, est le défaut que ce banc existe pour
attraper — c'est la règle centrale du §3.2 de la spec.

Le point **9** est celui qu'on oublie de tester : un poste atelier et un poste
bureau ouverts en même temps, c'est le cas courant, et sans verrou optimiste le
second écrase le premier en silence.

## 4. Vérifier en ligne de commande

```bash
# ETag courant
curl -s -D - -o /dev/null localhost:8081/api/planning | grep -i etag

# Publication (le PUT SANS If-Match doit être refusé en 428)
curl -s -X PUT localhost:8081/api/planning \
     -d '{"schema":1,"entrees":[{"id":"demo","heure":"06:00","station":7}]}'

# Se placer avant l'heure, valider, franchir l'échéance
curl -s -X POST localhost:8081/api/sim/heure -d '{"locale":"2026-09-01 05:59"}'
curl -s -X POST localhost:8081/api/planning/validate -d '{"par":"essai"}'
curl -s -X POST localhost:8081/api/sim/avancer -d '{"secondes":90}'
curl -s localhost:8081/api/missions
```

**Attendu** : une mission `demo` vers la station 7, prévue `06:00:00`.

## 5. Ce que ce banc ne fait pas — et pourquoi

- **Aucune authentification.** C'est délibéré : sur `127.0.0.1`, elle
  n'apporterait rien et masquerait le fait qu'elle **reste à faire sur la
  cible**. L'API y commandera un véhicule : l'authentification est un prérequis
  de mise en service (spec §9), pas une option.
- **Rien ne persiste.** L'état vit en mémoire du processus ; arrêter le banc
  remet tout à zéro. La persistance est le chantier T2.
- **Horloge vers le futur uniquement.** Le serveur démarre à l'heure réelle, et
  reculer ne « dé-consomme » pas les occurrences déjà traitées : le moteur les
  a marquées. Pour un scénario vierge, **redémarrer le banc**.
- **Pas de WebSocket.** La page sonde toutes les 700 ms. Le temps réel du §6
  sera tranché au portage ESP32 ; le contrat REST, lui, est déjà figé.

## En cas de panne

| Symptôme | Cause probable |
|---|---|
| `impossible d'ecouter sur 127.0.0.1:8081` | port déjà pris — `make banc` deux fois, ou `--port 0` |
| Page blanche, API qui répond | mauvais dossier web — lancer depuis `Timer/`, ou passer `--web` |
| `428` sur chaque publication | `If-Match` absent : recharger la page pour reprendre l'ETag |
| `409` répété | un autre onglet a publié entretemps — **Recharger (GET)** puis republier |
| Aucune mission alors que l'heure est passée | bandeau rouge (non validée), badge pause, ou occurrence déjà consommée |
| Rien ne bouge après « Aller à » une date passée | l'horloge ne va que vers le futur — redémarrer le banc |
