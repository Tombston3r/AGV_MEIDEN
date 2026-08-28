# Architectures : les quatre solutions étudiées

Chaque dossier est une **solution complète** : firmware, simulateur, tests,
nomenclature chiffrée et procédure de mise en service. La numérotation suit
l'**ordre chronologique d'étude**, pas un classement.

Toutes suivent la **même chaîne** : boutons d'appel optionnels, **poste central**
qui héberge l'API de planning et reçoit les appels, carte AGV Control, AGV. Voir
[`../../docs/ARCHITECTURE_COMMUNE.md`](../../docs/ARCHITECTURE_COMMUNE.md).

| | Dossier | Poste vers AGV | Carte AGV | Coût 10 ans | Tests |
|---|---|---|---|---:|---:|
| **A1** | [`A1_Cellulaire/`](A1_Cellulaire/) | LTE-M / MQTT | neuve | 1 386 € | 112 |
| **A2** | [`A2_LoRa/`](A2_LoRa/) | LoRa 868 MHz | V6.0 | **417 €** | 119 |
| **A3** | [`A3_Wifi/`](A3_Wifi/) | Wi-Fi d'entreprise | V5.0.1 | 592 € | 109 + 17 |

Coûts **HT sur dix ans, accessoires compris, boutons exclus** : ceux-ci sont
optionnels et leur choix ne dépend pas de l'architecture.

## Ce que chaque dossier contient

| Chemin | Rôle |
|---|---|
| `README.md` | ce que fait l'architecture, démarrage rapide |
| `DEPLOY.md` | mise en service, relevés éliminatoires, recette |
| `BOM.md` | nomenclature chiffrée : **générée**, ne pas éditer à la main |
| `CLAUDE.md` | règles de contribution et pièges rencontrés |
| `docs/ETAT_PROJET.md` | état, kanban, journal |
| `docs/Archi_*.md` | document de référence de l'architecture |
| `firmware/`, `sim/`, `test/native/`, `profiles/`, `tools/` | le logiciel |

```bash
cd A2_LoRa && make test        # tests unitaires, aucun matériel requis
```

## Le cœur est dupliqué

Les quatre dossiers portent chacun une copie du cœur métier, pour rester
livrables seuls. **Toute correction du cœur doit être reportée quatre fois.**
A2 et A3 sont les plus proches, même carte, même firmware AGV, même transport
- et ne diffèrent que par la couche d'appel.

Voir [`../../docs/ORGANISATION.md`](../../docs/ORGANISATION.md).
