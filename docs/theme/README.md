# Thème AIO

Fond **blanc**, texte **noir**, **bleu outremer** pour l'action, logo AIO en
haut à gauche. Toutes les interfaces du dépôt s'y conforment : une IHM
d'architecture, un banc et le planning doivent se ressembler, sinon
l'exploitant croit changer d'outil.

## Source unique, copies contrôlées

[`theme.css`](theme.css) est **l'original**. Chaque interface en porte une
copie dans son `web/`, parce que les dossiers sont livrables seuls (voir
[`../ORGANISATION.md`](../ORGANISATION.md)) — et parce que le banc EnOcean part
en `rsync` sur une UniPi qui n'a pas le reste du dépôt.

```bash
outils/theme.sh              # signale les copies qui ont dérivé
outils/theme.sh --appliquer  # repropage l'original
```

**Ne jamais modifier une copie.** Une copie sans contrôle est une divergence en
sursis : c'est pour cela que le script existe et qu'il figure dans les recettes
de déploiement.

## Ce que la feuille apporte

| | |
|---|---|
| Palette | `--outremer`, `--texte`, `--discret`, `--bord`, `--ok`, `--alerte`, `--attention` |
| Structure | `header` + `.logo` + `.titre` + `.onglets`, `main`, `section` |
| Commandes | `button` et ses variantes `.primaire` `.secondaire` `.danger`, champs de saisie |
| Éléments | `.badge`, `.pastille`, `table`, `.bandeau`, `.voile`/`.modale`, `.fenetre`, `#toast` |

Ce qui est **propre à une page** vit dans le `style.css` voisin ou dans un
`<style>` de la page : la frise du planning, les tuiles de supervision, la
liste des boutons EnOcean.

## Deux pièges qui ont déjà coûté

**`[hidden]` est fragile.** L'attribut n'est qu'un `display:none` de la feuille
par défaut du navigateur : la moindre règle d'auteur déclarant `display` le bat.
La feuille le rétablit en `!important`, et `.voile` laisse volontairement son
`display` au script — une fenêtre modale du banc EnOcean est restée ouverte en
permanence pour cette raison.

**`currentColor` se résout contre l'élément lui-même**, pas contre son parent.
Poser `color` sur un élément dont le `background` vaut `currentColor` le rend
invisible. Les drapeaux de la frise ont disparu ainsi.

## Interfaces concernées

| Interface | Chemin |
|---|---|
| Supervision AGV (×4 architectures) | `Comm distance/architectures/*/web/` |
| Banc EnOcean | `Comm distance/bancs/enocean/web/` |
| Planning — agvschedule et agvdump | `Timer/banc_api/web/` |
