# Banc d'essai des boutons EnOcean

Petit service web pour **valider des boutons EnOcean avant de choisir une
architecture**. Il se déploie sur une **UniPi E413** équipée d'un dongle
EnOcean USB, enregistre les boutons, et affiche une fenêtre verte à chaque
appui.

Il ne dépend d'**aucun** dossier d'architecture : c'est un outil de bench, pas
un livrable. Il sert autant à [`A1_Cellulaire/`](../../architectures/A1_Cellulaire/) qu'à
[`A2_LoRa/`](../../architectures/A2_LoRa/) ou [`A3_Wifi/`](../../architectures/A3_Wifi/), qui utilisent tous
des `PTM 210`.

## Ce qu'il fait

- **Liste des boutons enregistrés**, avec nom, identifiant et heure du dernier
  appui vu ;
- **Ajout d'un bouton** de deux façons : saisir l'identifiant, ou **le détecter
  en appuyant dessus**. Tant que la fenêtre d'ajout est ouverte, le banc écoute :
  l'appui d'un bouton **remplit l'identifiant tout seul**, dans le panneau de
  détection comme dans le champ de saisie ;
- **Fenêtre verte à chaque appui** : nom, code, heure et niveau reçu.

Un appui d'un bouton **non enregistré** ouvre une fenêtre **ambre** plutôt que
d'être ignoré : au banc, c'est précisément ce qu'on cherche à voir.

## Essayer sans matériel

```bash
cd bancs/enocean
python3 -m banc_enocean --simulation
```

Le mode simulation injecte un appui toutes les 5 secondes, en alternant un
bouton connu et un inconnu. Ouvrir <http://localhost:8080>.

## Déploiement

```bash
./deployer.sh unipi@<IP-UNIPI>     # depuis un poste de développement
./deployer.sh                      # sur la UniPi elle-même
```

Le script contrôle en fin de course que **la version servie est bien celle du
dépôt**, et le bandeau de l'interface l'affiche. Voir [`DEPLOY.md`](DEPLOY.md)
pour l'installation complète, la recette en sept points et le relevé de portée.

⚠️ **Ne pas copier les fichiers à la main** : un correctif présent dans le dépôt
mais absent de `/opt` donne exactement les symptômes du défaut qu'il corrige.

## Ce qu'on doit obtenir

| Geste | Attendu |
|---|---|
| Page chargée | Pastille **verte**, « à l'écoute », port du dongle affiché |
| Appui sur un bouton **enregistré** | Fenêtre **verte** : nom, code, heure, niveau reçu |
| Appui sur un bouton **inconnu** | Fenêtre **ambre** « Bouton non enregistré » |
| **Un** appui | **Une seule** fenêtre, jamais trois |
| Relâchement | Aucune fenêtre supplémentaire |
| Fenêtre d'ajout ouverte, puis appui | L'identifiant **se remplit seul** : aucune fenêtre ambre |

Un appui qui produit trois fenêtres, ou deux fenêtres par pression, est un
défaut : voir la section suivante.

## Deux règles qui font la différence

Un `PTM 210` **émet trois sous-télégrammes par appui**, et **émet aussi au
relâchement**. Sans traitement, une pression afficherait **six** fenêtres.

Le banc déduplique sur `(identifiant, données)` dans une fenêtre de 400 ms, et
ne retient que le front d'appui : le bit *energy bow* du télégramme RPS. C'est
la même logique que celle du firmware ; les tests la vérifient explicitement.

## Identifiants : toujours en hexadécimal

`00:29:B1:C4`, `0029B1C4`, `0x0029B1C4` et `00-29-b1-c4` sont acceptés, et
désignent le même bouton.

⚠️ La lecture est **hexadécimale sans exception**, y compris pour une chaîne de
chiffres seuls : `2732996` est un hexadécimal valide, et tenter de deviner
entre les deux bases donnerait un identifiant faux une fois sur deux. Les
identifiants EnOcean sont toujours notés en hexadécimal.

## API

| Méthode | Route | Rôle |
|---|---|---|
| `GET` | `/api/boutons` | liste des boutons enregistrés |
| `POST` | `/api/boutons` | `{"id": "0029B1C4", "nom": "Station 4"}` |
| `DELETE` | `/api/boutons/<code>` | retire un bouton |
| `POST` | `/api/apprentissage` | arme la détection ; renvoie aussi le dernier appui capté |
| `POST` | `/api/apprentissage/annuler` | désarme |
| `GET` | `/api/etat` | port, compteurs, mode courant |
| `GET` | `/api/evenements` | flux **SSE** : `appui`, `inconnu`, `apprentissage` |

Le registre est un JSON lisible et modifiable à la main, dans
`/var/lib/banc-enocean/boutons.json` sous systemd.

## Tests

```bash
python3 tests/test_banc.py
```

17 tests, sans matériel : déduplication, rejet du relâchement, resynchronisation
après octet parasite, analyse des identifiants, cycle d'apprentissage,
persistance et tolérance à un registre corrompu.

## Ce que ce banc ne fait pas

- **Aucun accusé vers le bouton.** Un `PTM 210` n'a pas de récepteur et le
  dongle est en réception seule : l'opérateur ne saura jamais, depuis le
  bouton, si son appui a été reçu. C'est une limite du matériel, pas du banc,
  et c'est l'argument principal en faveur des boutons LoRa de
  [`A2_LoRa/`](../../architectures/A2_LoRa/), qui rendent un accusé visuel.
- **Aucune authentification.** Un télégramme `PTM 210` est en clair et
  rejouable dans un rayon d'une trentaine de mètres. Ce banc affiche ce qu'il
  reçoit ; il ne prétend pas que l'émetteur est légitime.
- **Aucun contrôle d'accès sur l'interface web.** Elle est destinée à un réseau
  d'atelier, pas à être exposée.
