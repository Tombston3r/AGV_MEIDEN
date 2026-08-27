# Banc API — planning journalier en simulation locale

Ce banc fait tourner **le vrai moteur** derrière l'API de la spec (§6), sur
`127.0.0.1`, avec une **horloge simulée pilotable**. Un planning journalier
testé en temps réel est inutilisable : ici on se place à 05:59, on accélère
×600, on franchit minuit, on simule une heure douteuse — et on regarde ce que
fait le moteur qui partira en production.

## L'interface (itération 2)

Thème **blanc / noir / bleu outremer**, logo AIO en haut à gauche. La journée
s'affiche en **frise horizontale** 00h–24h, curseur de l'heure courante
compris :

- **poser un départ** : choisir un poste, *Prendre le drapeau* 🚩, cliquer sur
  la frise à l'heure voulue (pas de 5 min, un fantôme suit la souris). Le
  drapeau posé est un départ **du jour uniquement** (`debut = fin =
  aujourd'hui`) — la frise représente la journée, pas une récurrence ;
- **postes** : `Poste 1` et `Poste 2` par défaut — les postes réels de
  l'installation. *⚙ Gestion des postes* ouvre la fenêtre d'ajout/retrait ;
  retirer un poste ne retire que la pastille, jamais les départs déjà posés ;
- **retirer un départ** : cliquer sur son drapeau. Si l'entrée est récurrente,
  la confirmation le dit en toutes lettres avant de supprimer ;
- **appeler l'AGV** : choisir son poste, *L'AGV vient à ce poste* 📢 — mission
  immédiate, hors planning ;
- **logs des départs** : chaque départ confirmé ✓ (avec l'heure prévue), chaque
  saut ✗ avec son motif ;
- couleurs des drapeaux : **outremer** à venir, **vert** effectué ou appel,
  **gris** sauté ; un liseré **ambre** signale un départ trop rapproché du
  précédent.

### L'échelle est honnête

Un trajet dure **au plus 5 minutes** à vitesse lente, soit **0,35 %** d'une
journée. La frise dessine donc deux choses distinctes :

| | Ce que c'est |
|---|---|
| **le bloc sur l'axe** | l'occupation **réelle** de l'AGV — 5 min, quelques pixels |
| **l'étiquette au-dessus** | un simple libellé, détaché, qui n'engage aucune durée |

Les confondre — étiquette posée sur l'axe — laissait lire **deux heures**
d'immobilisation pour cinq minutes de trajet. La durée vit dans
`Config::duree_mission_s` (moteur), exposée par `/api/time` : une seule source
de vérité, pas une constante de feuille de style.

Poser un départ à moins de 5 min d'un autre est **refusé à la saisie** : le
séquenceur refuse d'empiler une destination tant que la précédente n'est pas
acquittée (§5), le départ serait perdu. Les plannings déjà enregistrés qui
présentent ce cas sont signalés (liseré ambre + compteur sous la frise).

⚠️ Poser ou retirer un drapeau **révoque la validation de la journée** : le
bandeau repasse rouge, et rien ne part tant qu'elle n'est pas revalidée. Ce
n'est pas un bug, c'est le §3.2 — ce qui a été validé n'est plus ce qui est en
mémoire.

## Lancer

```bash
cd Timer
make banc          # http://127.0.0.1:8081
```

## Ce qu'on doit obtenir

| # | Geste (dans la page) | Attendu |
|---|---|---|
| 1 | Charger la page | Bandeau **rouge** « planning non validé », horloge qui avance |
| 2 | Publier le planning d'exemple (textarea → Publier) | ETag passe à `"v2"` ; les **prochaines occurrences** se remplissent |
| 3 | « Aller à » demain 05:59, puis **Valider la journée** | Bandeau **vert** avec votre nom |
| 4 | Avancer **+1 min** | La mission apparaît dans **Missions parties**, journal `executee` |
| 5 | Avancer **+1 jour** | Bandeau repasse **rouge** : la validation d'hier ne vaut plus |
| 6 | Valider, « simuler heure douteuse », avancer +1 min | **Rien ne part**, badge `HEURE NON FIABLE — GELÉ` |
| 7 | Rétablir l'heure (dans les 5 min simulées) | La mission part — le gel n'a rien perdu |
| 8 | Modifier le planning, Publier | Bandeau repasse **rouge** : toute édition révoque l'autorisation |
| 9 | Ouvrir la page dans **deux onglets**, publier dans l'un puis l'autre | Le second reçoit **409 version périmée** — le verrou optimiste travaille |

Chaque saut, chaque gel, chaque refus est motivé dans le **journal du moteur**
en bas de page.

## L'API (contrat de la spec §6)

| Méthode | Route | Rôle |
|---|---|---|
| `GET` | `/api/planning` | document + `ETag` |
| `PUT` | `/api/planning` | remplacement complet — `If-Match` requis (428 sinon), **409 si périmé**, **révoque la validation du jour** |
| `GET` | `/api/planning/next` | 10 prochaines occurrences calculées |
| `GET` | `/api/planning/jour` | occurrences d'**aujourd'hui, passées comprises** — la frise (calculées par le moteur, DST compris) |
| `POST` | `/api/planning/validate` | `{"par":"nom"}` — autorisation du jour |
| `POST` | `/api/planning/pause` | `{"actif":bool}` |
| `POST` | `/api/planning/skip` | saute la prochaine occurrence |
| `POST` | `/api/appel` | `{"station":n}` — **mission immédiate**, hors planning : le geste opérateur, pas un déclenchement autonome (la validation §3.2 ne s'y applique pas) |
| `GET` | `/api/time` | heure, source, fiabilité, état de validation |
| `GET` | `/api/missions`, `/api/journal` | observation du banc |

### Simulation seulement — absent de la cible ESP32

| Route | Rôle |
|---|---|
| `POST /api/sim/heure` | `{"locale":"AAAA-MM-JJ HH:MM"}` ou `{"epoch":n}` |
| `POST /api/sim/avancer` | `{"secondes":n}` |
| `POST /api/sim/vitesse` | `{"facteur":1..86400}` |
| `POST /api/sim/fiable` | `{"fiable":bool}` — simule `TIME_UNTRUSTED` (§2.3) |

## Tests automatiques

```bash
make test      # moteur + codec + contrat API (démarre le serveur tout seul)
```

Le scénario du contrat (`tests/test_api.py`) déroule : verrou optimiste
(428/409), publication, validation, mission à l'heure **une seule fois**,
expiration à minuit, saut sur demande, gel heure non fiable, révocation à la
modification.

## Limites assumées

- **Horloge vers le futur uniquement** : revenir en arrière ne « dé-consomme »
  pas les occurrences déjà traitées. Redémarrer le banc pour repartir à neuf.
- **Aucune authentification** : c'est un banc local (`127.0.0.1` en dur).
  Sur la cible, l'API commande un véhicule — l'authentification est un
  prérequis de mise en service (spec §9), pas une option.
- **Interpolation temps réel par sondage** (700 ms) : le WebSocket de la spec
  §6 sera tranché au portage ESP32.
- Rien ne persiste : l'état vit en mémoire du processus (la persistance §3.3
  est le chantier T2).
