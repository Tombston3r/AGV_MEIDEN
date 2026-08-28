# AGV MEIDEN

Deux chantiers sur le même chariot filoguidé MEIDEN (carte d'origine
`AIO AGV Control V5.0.1`) :

Toutes les architectures suivent la **même chaîne** depuis le 2026-08-28 :

```
Boutons d'appel (optionnels) -> Poste Central -> Carte AGV Control -> AGV
                                API planning
                                appels
```

Les boutons sont **optionnels** : le planning seul fait rouler l'AGV. Les trois
architectures ne diffèrent plus que par la **liaison du poste vers l'AGV** :
[`docs/ARCHITECTURE_COMMUNE.md`](docs/ARCHITECTURE_COMMUNE.md).

| Dossier | Chantier | État |
|---|---|---|
| [`Comm distance/`](Comm%20distance/) | **Remplacement du système d'appel** : 4 architectures étudiées (cellulaire, hybride, LoRa, Wi-Fi), bancs d'essai, matériel KiCad | 470 tests verts, choix client non tranché |
| [`Timer/`](Timer/) | **Planning journalier** : missions déclenchées sur horloge, validation quotidienne par l'opérateur | moteur livré, 18 tests verts |

Chaque chantier porte son `README.md` **et** son `DEPLOY.md`, à quoi ça sert
d'un côté, comment le mettre en service et ce qu'on doit voir de l'autre. La
règle de rangement commune est dans
[`docs/ORGANISATION.md`](docs/ORGANISATION.md) : *tout ce qui sert à une chose
vit avec elle, documentation comprise.*

Toutes les interfaces web du dépôt suivent le **même thème** : fond blanc,
bleu outremer, logo AIO : [`docs/theme/`](docs/theme/), propagé par
`outils/theme.sh`.

Les deux partagent la même carte et le même cœur métier : le Timer produira
ses missions vers le séquenceur déjà écrit dans
[`Comm distance/architectures/A3_Wifi/`](Comm%20distance/architectures/A3_Wifi/).
