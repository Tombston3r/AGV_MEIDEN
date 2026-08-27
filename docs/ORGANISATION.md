# Organisation du dépôt — pourquoi, et où poser une nouveauté

**Ce document vaut pour tout le dépôt**, les deux chantiers compris :
`Comm distance/` et `Timer/`.

## La règle, en une phrase

**Tout ce qui sert à une chose vit avec elle** — et *tout* comprend la
documentation.

## La paire obligatoire

Tout dossier livrable — chantier, architecture, banc — porte **deux**
documents. Pas un.

| Fichier | Ce qu'il répond |
|---|---|
| `README.md` | à quoi ça sert, comment le lancer, **ce qu'on doit obtenir** |
| `DEPLOY.md` | mise en service pas à pas, **relevés éliminatoires**, recette, diagnostic de panne |

La séparation n'est pas cosmétique : le `README` s'adresse à qui découvre, le
`DEPLOY` à qui a les mains dans la machine et veut savoir **ce qu'il doit voir**
à chaque étape. Écrire l'un en croyant couvrir l'autre laisse toujours la
recette de côté — celle qui dit qu'un appui EnOcean produisant trois fenêtres
est un défaut, ou qu'un RSSI sous −115 dBm condamne une implantation.

⚠️ **Un `DEPLOY.md` reste dû même quand rien n'est déployable.** Il décrit alors
le chemin ordonné vers la mise en service et **ce qui l'interdit aujourd'hui** —
voir [`../Timer/DEPLOY.md`](../Timer/DEPLOY.md), dont la phase 0 est faite de
verrous non logiciels.

S'y ajoute `docs/ETAT_PROJET.md` — état, kanban, journal — **tenu à jour à
chaque modification**.

## Où poser une nouveauté

| Ce que vous ajoutez | Où | Ce qu'il faut fournir avec |
|---|---|---|
| Un **chantier** entier | `<Nom>/` à la racine | `README.md`, `DEPLOY.md`, `docs/ETAT_PROJET.md`, une ligne dans le `README.md` racine |
| Une **solution complète** de bout en bout | `Comm distance/architectures/A<n>_<Nom>/` | `README.md`, `DEPLOY.md`, `BOM.md`, `docs/ETAT_PROJET.md`, tests |
| Un **banc** qui valide un composant, une liaison ou un contrat | `<chantier>/bancs/<nom>/` ou `<chantier>/banc_<nom>/` | `README.md`, `DEPLOY.md`, tests exécutables **sans matériel** |
| Un **projet KiCad** | `Comm distance/materiel/<NOM_REV>/` | une entrée dans `materiel/README.md` |
| Un **script partagé** | `Comm distance/outils/` | une entrée dans `outils/README.md` |
| Un **document transverse** | `docs/` (racine) | un lien depuis le `README.md` racine |

Si l'on hésite entre une architecture et un banc : un banc **prouve quelque
chose** — sur du matériel, ou sur un contrat — une architecture **fait rouler
l'AGV**.

## Trois choix expliqués

### Les architectures sont autonomes, et c'est cher

Chaque dossier d'architecture porte sa propre copie du cœur métier, pour rester
livrable seul à un client ou à un collègue. Le prix est réel : **une correction
du séquenceur doit être reportée quatre fois**, et un correctif appliqué à un
seul dossier crée une divergence que rien ne signale.

Le compromis a été retenu en connaissance de cause. Il impose une discipline :
après toute modification de `firmware/common/app/` ou `firmware/common/proto/`,
vérifier les quatre dossiers.

### Le matériel, lui, n'est PAS dupliqué

Les projets KiCad vivent une seule fois dans `materiel/`, même quand deux
architectures partagent une carte.

Ce n'est pas une exception arbitraire : la duplication du matériel s'est déjà
retournée contre le projet. Trois copies divergentes de la V6.0 ont coexisté —
dont une que KiCad avait recréée à un chemin renommé — et il a fallu comparer
les schémas pour déterminer laquelle portait le travail réel. Un firmware
dupliqué se compare par `diff` ; deux schémas KiCad divergents, non.

**Une carte, un projet, un seul endroit.**

### Les bancs sont sortis des architectures

Les bancs LoRa vivaient dans `A3_LoRa/test/esp32/` et `A3_LoRa/test/unipi/`,
tandis que le banc EnOcean était à la racine. Deux choses de même nature à deux
endroits différents : personne ne pouvait deviner où chercher.

`bancs/` les rassemble, tous à la même forme. `architectures/*/test/native/`
ne contient plus que les **tests unitaires** du cœur, qui tournent avec
`make test` et n'ont besoin d'aucun matériel.

## Envoyer une architecture à quelqu'un

Le matériel et le brief étant partagés, un dossier d'architecture n'est plus
complet à lui seul. `outils/exporter_architecture.sh` reconstitue un ensemble
autonome :

```bash
cd "Comm distance"
./outils/exporter_architecture.sh A3_LoRa
# -> A3_LoRa_2026-08-26.zip : l'architecture, sa carte, le brief, le comparatif
```

C'est le même résultat qu'avant, mais payé **au moment de l'export** plutôt que
tous les jours.

## Tenue des documents

`docs/ETAT_PROJET.md` de chaque dossier est **mis à jour à chaque
modification** : état, déploiement, kanban, journal. C'est là qu'on lit ce qui a
bougé et pourquoi, sans relire l'historique Git.
