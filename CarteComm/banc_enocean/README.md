# Banc d'essai des boutons EnOcean

Petit service web pour **valider des boutons EnOcean avant de choisir une
architecture**. Il se déploie sur une **UniPi E413** équipée d'un dongle
EnOcean USB, enregistre les boutons, et affiche une fenêtre verte à chaque
appui.

Il ne dépend d'**aucun** dossier d'architecture : c'est un outil de bench, pas
un livrable. Il sert autant à [`A1_Cellulaire/`](../A1_Cellulaire/) qu'à
[`A2_Hybride/`](../A2_Hybride/) ou [`A4_Wifi/`](../A4_Wifi/), qui utilisent tous
des `PTM 210`.

## Ce qu'il fait

- **Liste des boutons enregistrés**, avec nom, identifiant et heure du dernier
  appui vu ;
- **Ajout d'un bouton** de deux façons — saisir l'identifiant, ou **le détecter
  en appuyant dessus** ;
- **Fenêtre verte à chaque appui** : nom, code, heure et niveau reçu.

Un appui d'un bouton **non enregistré** ouvre une fenêtre **ambre** plutôt que
d'être ignoré : au banc, c'est précisément ce qu'on cherche à voir.

## Essayer sans matériel

```bash
cd CarteComm/banc_enocean
python3 -m banc_enocean --simulation
```

Le mode simulation injecte un appui toutes les 5 secondes, en alternant un
bouton connu et un inconnu. Ouvrir <http://localhost:8080>.

## Déploiement sur la UniPi E413

```bash
# 1. Dépendance du dongle — la seule du projet
sudo apt install python3-serial

# 2. Copie
sudo mkdir -p /opt/banc-enocean
sudo cp -r banc_enocean web /opt/banc-enocean/

# 3. Compte de service, sans connexion, membre de dialout pour le port série
sudo useradd --system --no-create-home --shell /usr/sbin/nologin -G dialout banc

# 4. Service
sudo cp systemd/banc-enocean.service /etc/systemd/system/
sudo systemctl enable --now banc-enocean
```

Puis <http://IP-DE-LA-UNIPI:8080>.

### Vérifier le dongle

```bash
ls -l /dev/serial/by-id/          # le dongle doit y apparaître
journalctl -u banc-enocean -f     # le port retenu est journalisé au démarrage
```

Le port est détecté automatiquement, en préférant les liens `by-id` : un
`/dev/ttyUSB0` change de numéro au gré des rebranchements, pas un lien `by-id`.
`--port /dev/ttyUSB0` force un port précis si besoin.

## Deux règles qui font la différence

Un `PTM 210` **émet trois sous-télégrammes par appui**, et **émet aussi au
relâchement**. Sans traitement, une pression afficherait **six** fenêtres.

Le banc déduplique sur `(identifiant, données)` dans une fenêtre de 400 ms, et
ne retient que le front d'appui — le bit *energy bow* du télégramme RPS. C'est
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
  bouton, si son appui a été reçu. C'est une limite du matériel, pas du banc —
  et c'est l'argument principal en faveur des boutons LoRa de
  [`A3_LoRa/`](../A3_LoRa/), qui rendent un accusé visuel.
- **Aucune authentification.** Un télégramme `PTM 210` est en clair et
  rejouable dans un rayon d'une trentaine de mètres. Ce banc affiche ce qu'il
  reçoit ; il ne prétend pas que l'émetteur est légitime.
- **Aucun contrôle d'accès sur l'interface web.** Elle est destinée à un réseau
  d'atelier, pas à être exposée.
