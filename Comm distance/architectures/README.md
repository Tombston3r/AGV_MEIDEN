# Architectures : les quatre solutions étudiées

Chaque dossier est une **solution complète** : firmware, simulateur, tests,
nomenclature chiffrée et procédure de mise en service. La numérotation suit
l'**ordre chronologique d'étude**, pas un classement.

Toutes suivent la **même chaîne** : boutons d'appel optionnels, **poste central**
qui héberge l'API de planning et reçoit les appels, carte AGV Control, AGV. Voir
[`../../docs/ARCHITECTURE_COMMUNE.md`](../../docs/ARCHITECTURE_COMMUNE.md).

| | Dossier | Bouton vers poste | Poste vers AGV | Carte | Coût 10 ans | Tests |
|---|---|---|---|---|---:|---:|
| **A1** | [`A1_Cellulaire/`](A1_Cellulaire/) | EnOcean | LTE-M / MQTT | neuve | 1 486 € | 112 |
| **A2** | [`A2_Hybride/`](A2_Hybride/) | EnOcean sans pile | LoRa | V6.0 | 557 € | 130 |
| **A3** | [`A3_LoRa/`](A3_LoRa/) | **LoRa sur pile** | LoRa | V6.0 | **538 €** | 119 |
| **A4** | [`A4_Wifi/`](A4_Wifi/) | EnOcean | Wi-Fi d'entreprise | V5.0.1 | 692 € | 109 + 17 |

⚠️ **A2 et A3 ne sont plus séparées que par 19 €** et par la technologie du
bouton, pile contre sans-pile. Depuis que le poste central est commun, leur
liaison vers l'AGV est identique. La question de les garder toutes les deux est
ouverte dans [`../../docs/COMPARAISON.md`](../docs/COMPARAISON.md).

Coûts **HT sur dix ans, deux points d'appel, accessoires compris** : voir
[`../docs/COMPARAISON.md`](../docs/COMPARAISON.md) pour l'arbitrage complet.

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
cd A3_LoRa && make test        # tests unitaires, aucun matériel requis
```

## Le cœur est dupliqué

Les quatre dossiers portent chacun une copie du cœur métier, pour rester
livrables seuls. **Toute correction du cœur doit être reportée quatre fois.**
A2 et A3 sont les plus proches, même carte, même firmware AGV, même transport
- et ne diffèrent que par la couche d'appel.

Voir [`../../docs/ORGANISATION.md`](../../docs/ORGANISATION.md).
