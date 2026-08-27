# Architectures : les quatre solutions étudiées

Chaque dossier est une **solution complète** : firmware, simulateur, tests,
nomenclature chiffrée et procédure de mise en service. La numérotation suit
l'**ordre chronologique d'étude**, pas un classement.

| | Dossier | Carte | Liaison | Boutons | Coût 10 ans | Tests |
|---|---|---|---|---|---:|---:|
| **A1** | [`A1_Cellulaire/`](A1_Cellulaire/) | neuve | SMS ou LTE-M/MQTT | EnOcean au poste | 1 366 € | 112 |
| **A2** | [`A2_Hybride/`](A2_Hybride/) | V6.0 | LoRa + poste relais | EnOcean sans pile | 428 € | 130 |
| **A3** | [`A3_LoRa/`](A3_LoRa/) | V6.0 | LoRa direct | LoRa sur pile | **341 €** | 119 |
| **A4** | [`A4_Wifi/`](A4_Wifi/) | V5.0.1 | Wi-Fi d'entreprise | EnOcean au poste | 692 € | 109 + 17 |

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
