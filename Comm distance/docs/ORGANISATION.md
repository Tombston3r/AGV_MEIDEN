# Organisation du dépôt — pourquoi, et où poser une nouveauté

## La règle

**Tout ce qui sert à une chose vit avec elle.**

Un banc porte sa documentation, son code, ses tests et sa procédure de
déploiement dans le même dossier. Une architecture aussi. On ne cherche jamais
la doc d'un composant ailleurs que dans son dossier.

## Où poser une nouveauté

| Ce que vous ajoutez | Où | Ce qu'il faut fournir avec |
|---|---|---|
| Une **solution complète** de bout en bout | `architectures/A<n>_<Nom>/` | `README.md`, `DEPLOY.md`, `BOM.md`, `docs/ETAT_PROJET.md`, tests |
| Un **banc** qui valide un composant ou une liaison | `bancs/<techno>/` | `README.md`, `DEPLOY.md`, tests exécutables **sans matériel** |
| Un **projet KiCad** | `materiel/<NOM_REV>/` | une entrée dans `materiel/README.md` |
| Un **script partagé** | `outils/` | une entrée dans `outils/README.md` |
| Un **document transverse** | `docs/` | un lien depuis le `README.md` racine |

Si l'on hésite entre `architectures/` et `bancs/` : un banc **prouve quelque
chose sur du matériel**, une architecture **fait rouler l'AGV**.

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
./outils/exporter_architecture.sh A3_LoRa
# -> A3_LoRa_2026-08-26.zip : l'architecture, sa carte, le brief, le comparatif
```

C'est le même résultat qu'avant, mais payé **au moment de l'export** plutôt que
tous les jours.

## Tenue des documents

`docs/ETAT_PROJET.md` de chaque dossier est **mis à jour à chaque
modification** : état, déploiement, kanban, journal. C'est là qu'on lit ce qui a
bougé et pourquoi, sans relire l'historique Git.
