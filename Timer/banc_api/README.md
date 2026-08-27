# Banc API : planning journalier en simulation locale

Ce banc fait tourner **le vrai moteur** derrière l'API de la spec (§6), sur
`127.0.0.1`, avec une **horloge simulée pilotable**. Un planning journalier
testé en temps réel est inutilisable : ici on se place à 05:59, on accélère
×600, on franchit minuit, on simule une heure douteuse, et on regarde ce que
fait le moteur qui partira en production.

## Deux pages, une barre d'onglets

| Onglet | Page | Rôle |
|---|---|---|
| **agvschedule** | `/` | la frise du jour, la pose des départs, l'appel |
| **agvdump** | `/agvdump.html` | le **diagnostic d'atelier**, mis en forme |

Sur la cible, les deux seront servies par le **même** serveur web de l'ESP32 :
la barre reflète cette réalité, elle n'est pas un artifice du banc.

### ⚠️ Le format de `/agvdump` ne change pas

`GET /agvdump` renvoie du **texte brut**, dans le format exact du firmware
d'origine (brief §3.3) : les procédures d'atelier du client lisent ces noms de
champs. `agvdump.html` ne fait que le **présenter** : elle va chercher ce
même texte et le met en page. Le brut reste accessible, et téléchargeable.

Le rendu est une **copie octet pour octet** de celui de
`Comm distance/architectures/A4_Wifi/`, et `test/test_agvdump.cpp` **compare
la copie à son original** à chaque `make test`. Sans ce contrôle, une copie est
une divergence en sursis.

Sur le banc il n'y a **pas d'ATmega** : la section `[AGV STATE]` reste à sa
valeur de repos et la page le dit. Les compteurs renseignés sont ceux que le
planning connaît vraiment : `heartbeats_sent` compte les départs partis,
`cmd_expired` les départs sautés, `link_timeouts` les passages en heure non
fiable.

## L'interface (itération 2)

Thème **blanc / noir / bleu outremer**, logo AIO en haut à gauche. La journée
s'affiche en **frise horizontale** 00h–24h, curseur de l'heure courante
compris :

- **poser un départ** : choisir un poste, *Prendre le drapeau* 🚩, cliquer sur
  la frise à l'heure voulue (pas de 5 min, un fantôme suit la souris). Le
  drapeau posé est un départ **du jour uniquement** (`debut = fin =
  aujourd'hui`) : la frise représente la journée, pas une récurrence ;
- **postes** : `Poste 1` et `Poste 2` par défaut, les postes réels de
  l'installation. *⚙ Gestion des postes* ouvre la fenêtre d'ajout/retrait ;
  retirer un poste ne retire que la pastille, jamais les départs déjà posés ;
- **retirer un départ** : cliquer sur son drapeau. Si l'entrée est récurrente,
  la confirmation le dit en toutes lettres avant de supprimer ;
- **appeler l'AGV** : choisir son poste, *L'AGV vient à ce poste* 📢, mission
  immédiate, hors planning ;
- **logs des départs** : chaque départ confirmé ✓ (avec l'heure prévue), chaque
  saut ✗ avec son motif ;
- couleurs des drapeaux : **outremer** à venir, **vert** effectué ou appel,
  **gris** sauté ; un liseré **ambre** signale un départ trop rapproché du
  précédent.

### L'arrêt de sécurité, rapporté et non subi

L'AGV porte sa propre chaîne de sécurité : elle coupe la puissance sur obstacle
et n'est réarmée que par un **appui physique**. Le planning ne la remplace pas :
il en **rapporte l'effet**, parce que l'exploitant doit savoir qu'un départ
programmé ne s'est pas terminé.

Quand la coupure survient **pendant un déplacement**, un bandeau rouge nomme le
départ interrompu, sa destination et l'heure, rappelle le réarmement physique,
et **suspend les départs autonomes** jusqu'à un acquittement nommé.

⚠️ **Uniquement pendant un déplacement.** Une coupure à quai : maintenance, fin
de poste, quelqu'un devant l'AGV, est journalisée sans alerte : crier au loup à
chaque coupure viderait l'alerte de son sens. C'est le drapeau `en_deplacement`
qui porte toute la règle, et le firmware le déduira de `SeqState::Transit` et de
`state_flag::kMoving`.

L'**appel opérateur** reste possible pendant l'alerte : c'est une décision
humaine, comme le réarmement.

### La frise grandit avec la journée

Sa hauteur n'est pas figée : elle suit le nombre de **rangées d'étiquettes
réellement occupées**, jusqu'à dix. Onze départs tiennent ainsi sur quatre
rangées sans se recouvrir, là où une hauteur fixe saturait dès trois. Une
journée vide reste à sa hauteur plancher.

Le placement se fait par **détection de chevauchement** : chaque étiquette
descend d'une rangée seulement si la précédente lui prend la place, et non
par alternance aveugle. Toute la géométrie verticale vit dans un unique objet
`GEO` du script : changer le pas ou le nombre de rangées est une ligne.

### L'échelle est honnête

Un trajet dure **au plus 5 minutes** à vitesse lente, soit **0,35 %** d'une
journée. La frise dessine donc deux choses distinctes :

| | Ce que c'est |
|---|---|
| **le bloc sur l'axe** | l'occupation **réelle** de l'AGV : 5 min, quelques pixels |
| **l'étiquette au-dessus** | un simple libellé, détaché, qui n'engage aucune durée |

Les confondre (étiquette posée sur l'axe) laissait lire **deux heures**
d'immobilisation pour cinq minutes de trajet. La durée vit dans
`Config::duree_mission_s` (moteur), exposée par `/api/time` : une seule source
de vérité, pas une constante de feuille de style.

Poser un départ à moins de 5 min d'un autre est **refusé à la saisie** : le
séquenceur refuse d'empiler une destination tant que la précédente n'est pas
acquittée (§5), le départ serait perdu. Les plannings déjà enregistrés qui
présentent ce cas sont signalés (liseré ambre + compteur sous la frise).

⚠️ Poser ou retirer un drapeau **révoque la validation de la journée** : le
bandeau repasse rouge, et rien ne part tant qu'elle n'est pas revalidée. Ce
n'est pas un bug, c'est le §3.2 : ce qui a été validé n'est plus ce qui est en
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
| 6 | Valider, « simuler heure douteuse », avancer +1 min | **Rien ne part**, badge `HEURE NON FIABLE : GELÉ` |
| 7 | Rétablir l'heure (dans les 5 min simulées) | La mission part : le gel n'a rien perdu |
| 8 | Modifier le planning, Publier | Bandeau repasse **rouge** : toute édition révoque l'autorisation |
| 9 | Ouvrir la page dans **deux onglets**, publier dans l'un puis l'autre | Le second reçoit **409 version périmée** : le verrou optimiste travaille |

Chaque saut, chaque gel, chaque refus est motivé dans le **journal du moteur**
en bas de page.

## L'API (contrat de la spec §6)

| Méthode | Route | Rôle |
|---|---|---|
| `GET` | `/api/planning` | document + `ETag` |
| `PUT` | `/api/planning` | remplacement complet : `If-Match` requis (428 sinon), **409 si périmé**, **révoque la validation du jour** |
| `GET` | `/api/planning/next` | 10 prochaines occurrences calculées |
| `GET` | `/api/planning/jour` | occurrences d'**aujourd'hui, passées comprises** : la frise (calculées par le moteur, DST compris) |
| `POST` | `/api/planning/validate` | `{"par":"nom"}` : autorisation du jour |
| `POST` | `/api/planning/pause` | `{"actif":bool}` |
| `POST` | `/api/planning/skip` | saute la prochaine occurrence |
| `POST` | `/api/alerte/acquitter` | `{"par":"nom"}` : lève l'alerte d'arrêt de sécurité |
| `POST` | `/api/appel` | `{"station":n}`, **mission immédiate**, hors planning : le geste opérateur, pas un déclenchement autonome (la validation §3.2 ne s'y applique pas) |
| `GET` | `/api/time` | heure, source, fiabilité, état de validation |
| `GET` | `/api/missions`, `/api/journal` | observation du banc |

### Simulation seulement : absent de la cible ESP32

| Route | Rôle |
|---|---|
| `POST /api/sim/heure` | `{"locale":"AAAA-MM-JJ HH:MM"}` ou `{"epoch":n}` |
| `POST /api/sim/avancer` | `{"secondes":n}` |
| `POST /api/sim/vitesse` | `{"facteur":1..86400}` |
| `POST /api/sim/fiable` | `{"fiable":bool}` : simule `TIME_UNTRUSTED` (§2.3) |
| `POST /api/sim/interruption` | `{"en_deplacement":bool}` : la chaîne de sécurité coupe |
| `POST /api/sim/arrivee` | l'AGV confirme son arrivée |

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
  Sur la cible, l'API commande un véhicule : l'authentification est un
  prérequis de mise en service (spec §9), pas une option.
- **Interpolation temps réel par sondage** (700 ms) : le WebSocket de la spec
  §6 sera tranché au portage ESP32.
- Rien ne persiste : l'état vit en mémoire du processus (la persistance §3.3
  est le chantier T2).
