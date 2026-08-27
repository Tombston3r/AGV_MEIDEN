# Outils partagés

| Outil | Rôle |
|---|---|
| [`theme.sh`](theme.sh) | **propage le thème AIO** dans toutes les interfaces, ou signale les copies qui ont dérivé |

Les outils propres au chantier « Comm distance » (génération des nomenclatures,
export d'une architecture) vivent dans
[`../Comm distance/outils/`](../Comm%20distance/outils/).

## Thème

```bash
outils/theme.sh              # contrôle
outils/theme.sh --appliquer  # propagation
```

L'original est [`../docs/theme/theme.css`](../docs/theme/theme.css) ; chaque
interface en porte une copie, parce que les dossiers sont livrables seuls. Voir
[`../docs/theme/README.md`](../docs/theme/README.md).
