# Déploiement du banc EnOcean sur une UniPi E413

Durée : une vingtaine de minutes, dongle en main.

## 0. Ce qu'il faut avoir

- une **UniPi E413** sous Debian, joignable en SSH ;
- un **dongle EnOcean USB** (USB 300 ou TCM 515U) ;
- au moins un **bouton `PTM 210` en version EU 868 MHz**.

⚠️ Les déclinaisons 902 et 928 MHz ne sont pas utilisables en France et **rien
dans la désignation courante ne les distingue**. Vérifier avant de commander.

## 1. Brancher le dongle et le voir

```bash
ls -l /dev/serial/by-id/
```

**Attendu** : une entrée contenant `EnOcean` ou `FT232`.
Si rien n'apparaît, `dmesg | tail -20` dira si le noyau a vu le périphérique.

## 2. Installer — et mettre à jour

```bash
sudo apt install python3-serial      # une seule fois

# Depuis un poste de développement :
./deployer.sh unipi@<IP-UNIPI>

# Ou directement sur la UniPi, dans le dossier du banc :
./deployer.sh
```

`deployer.sh` fait tout : envoi, copie dans `/opt`, compte de service, unité
systemd, redémarrage, puis **contrôle que la version servie est bien celle du
dépôt**.

⚠️ **Ne pas copier les fichiers à la main.** Un correctif présent dans le dépôt
mais absent de `/opt` donne exactement les symptômes du défaut qu'il corrige, et
l'on cherche longtemps. Le script utilise `rsync --delete` : un fichier retiré
du dépôt disparaît aussi de la cible.

Le groupe `dialout` est ce qui autorise le service à ouvrir le port série.
Sans lui, le démarrage échoue sur un `Permission denied`.

## 3. Vérifier le démarrage

```bash
journalctl -u banc-enocean -n 20
```

**Attendu** :

```
banc EnOcean sur http://0.0.0.0:8080 — dongle /dev/serial/by-id/usb-EnOcean_..., 0 bouton(s) enregistré(s)
```

Si la ligne dit `aucun dongle EnOcean détecté`, revenir à l'étape 1.

## 4. Recette

Ouvrir `http://<IP-UNIPI>:8080`.

| # | Geste | Attendu |
|---|---|---|
| 1 | Charger la page | Pastille **verte**, « à l'écoute », le port affiché |
| 2 | Appuyer sur un bouton non enregistré | Fenêtre **ambre** « Bouton non enregistré » avec un code à 8 chiffres hexadécimaux |
| 3 | Appuyer **une** fois | **Une seule** fenêtre — pas trois |
| 4 | « Ajouter un bouton », puis appuyer | Le code **se remplit seul**, en vert et dans le champ de saisie. Aucune fenêtre ambre |
| 5 | Nommer et enregistrer | Le bouton apparaît dans la liste |
| 6 | Réappuyer dessus | Fenêtre **verte** : nom, code, heure, niveau reçu |
| 7 | Relâcher le bouton | **Aucune** fenêtre supplémentaire |

Le point 3 est le plus important : un `PTM 210` émet **trois sous-télégrammes
par appui**. Trois fenêtres signifieraient que la déduplication ne fonctionne
pas — et en production, trois courses pour un appui.

Le point 7 vaut autant : le module émet aussi au **relâchement**. Une fenêtre
de plus signifierait deux courses par pression.

## 5. Relevé de portée

Promener le bouton le long du parcours, en regardant le niveau reçu.

**Attendu** : au-delà de **−90 dBm**, la marge devient faible. L'EnOcean porte
une trentaine de mètres en intérieur, et c'est **le maillon court** des
architectures A1, A2 et A4 — pas la liaison LoRa ni le Wi-Fi.

Noter les points où les appuis se perdent : ce sont eux qui décideront de
l'emplacement du poste fixe.

## En cas de panne

| Symptôme | Cause probable |
|---|---|
| Page sans aucun style, ou fond noir | `theme.css` non déployé — relancer `outils/theme.sh --appliquer` puis `./deployer.sh` |
| La fenêtre d'ajout reste affichée en permanence | **version déployée périmée** — relancer `./deployer.sh` et comparer la version du bandeau à celle du dépôt |
| Fenêtre ambre alors que la boîte d'ajout est ouverte | l'écoute n'est plus armée : le navigateur ne joint plus le banc, vérifier la pastille |
| `Permission denied` sur le port | l'utilisateur `banc` n'est pas dans `dialout` |
| Trois fenêtres par appui | déduplication inopérante — ouvrir une anomalie |
| Aucune fenêtre, pastille verte | mauvais dongle, ou bouton hors bande 868 MHz |
| Pastille rouge | le service est tombé — `journalctl -u banc-enocean -f` |
