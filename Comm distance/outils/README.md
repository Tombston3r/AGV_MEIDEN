# Outils partagés

| Outil | Rôle |
|---|---|
| [`generer_bom.py`](generer_bom.py) | **génère les quatre `BOM.md`** : les prix vivent ici, en un seul endroit |
| [`prix_a_completer.md`](prix_a_completer.md) | feuille de sourcing à remplir par le service achats |
| [`exporter_architecture.sh`](exporter_architecture.sh) | reconstitue une architecture **autonome** dans un zip |

## Générer les nomenclatures

```bash
python3 outils/generer_bom.py
```

Les quatre `BOM.md` sont **générés** : ne jamais les éditer à la main, la
modification serait perdue à la génération suivante. Un prix se change dans
`generer_bom.py`, où les totaux sont **calculés** : c'est ce qui garantit qu'un
récapitulatif ne peut pas diverger de son propre détail.

Après une mise à jour de prix, penser à reporter les totaux dans
[`../docs/COMPARAISON.md`](../docs/COMPARAISON.md) et dans le `README.md`
racine, qui raisonnent **accessoires compris** là où les `BOM.md` ne comptent
que les composants déterminants.

## Exporter une architecture

```bash
./outils/exporter_architecture.sh A2_LoRa
```

Produit un zip contenant l'architecture, **sa carte**, le brief et le
comparatif : ce qu'un dossier d'architecture ne porte plus tout seul depuis que
le matériel et le brief sont partagés.
