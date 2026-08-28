# Rendre le Timer déployable sur le poste central

> Cible depuis le 2026-08-28 : le **poste central**, sous Linux, commun aux
> quatre architectures. Pour A3 c'est une **UniPi Lite 1.1** (Raspberry Pi +
> carte d'extension). Voir
> [`../../docs/ARCHITECTURE_COMMUNE.md`](../../docs/ARCHITECTURE_COMMUNE.md).
>
> État à jour : [`ETAT_PROJET.md`](ETAT_PROJET.md).

## Ce que le déplacement fait disparaître

Le Timer devait tourner sur l'ESP32 de la carte AGV. Une analyse antérieure de
ce document listait quatre obstacles ; **trois s'évanouissent** en passant sous
Linux.

| Obstacle sur ESP32 | Sous Linux |
|---|---|
| `set_wall_clock()` jamais appelé, heure à zéro | **NTP standard**, `systemd-timesyncd` |
| DS3231 à ajouter, I²C à câbler | **Horloge matérielle** déjà présente sur Raspberry Pi via la carte d'extension, ou pile RTC. Aucun câblage projet |
| Stockage NVS, limite de taille de blob | **Système de fichiers.** Le codec JSON existant écrit un fichier |
| Serveur web limité à la fenêtre de maintenance de 600 s | **Le banc EST le serveur.** Il tourne déjà en permanence sur 127.0.0.1 |

Le quatrième obstacle demeure et se déplace : **l'authentification**. Le poste
est sur le réseau d'entreprise et commande un véhicule.

## Le banc devient la cible

C'est le point qui change tout. [`../banc_api/`](../banc_api/) est un serveur
HTTP en C++ pur, sockets POSIX, sans dépendance : il tourne **tel quel** sur un
Raspberry Pi. Le passage en production tient en quatre points :

| # | À faire | Pourquoi |
|---|---|---|
| 1 | Écouter sur l'interface réseau, plus seulement sur `127.0.0.1` | l'adresse de bouclage est codée en dur, volontairement, pour un banc |
| 2 | Remplacer l'horloge simulée par l'horloge système | `/api/sim/*` disparaît de la cible |
| 3 | Persister planning et occurrences consommées dans un fichier | chantiers T1 et T2 |
| 4 | Authentification | le poste n'est plus sur une boucle locale |

Rien à porter, rien à réécrire : le moteur, le codec, l'API et l'IHM sont déjà
ceux qui partiront.

## Ce qui reste, par ordre de dépendance

### 1. Persistance : le fichier, pas la NVS

⚠️ **T1 avant T2.** Sans les occurrences consommées, un redémarrage **dans la
fenêtre de grâce** rejoue un départ déjà effectué. C'est le seul défaut connu du
moteur.

Deux fichiers, écriture atomique par `rename()` :

| Fichier | Contenu |
|---|---|
| `planning.json` | le document, tel que le produit `document_vers_json()` |
| `consommees.json` | les occurrences déjà parties |

Le CRC32 et la version de schéma de la spec §3.3 restent utiles ; l'écriture
atomique est native sur un système de fichiers.

### 2. Horloge : NTP suffit, la RTC rassure

`systemd-timesyncd` et `TZ=Europe/Paris` couvrent le besoin. Le moteur consomme
déjà un drapeau `heure_fiable` : il se déduit de `timedatectl` ou d'un simple
contrôle d'année plausible.

⚠️ Le Wi-Fi du site est saturé, mais **le poste est raccordé en Ethernet** : il
n'a pas le problème que rencontrerait un ESP32 embarqué. Une horloge matérielle
reste souhaitable pour les coupures secteur prolongées.

### 3. Le lien vers la carte AGV

Le poste doit désormais **émettre** les missions. Selon l'architecture :

| | Comment le poste parle à l'AGV |
|---|---|
| **A2, A3** | `RFM95W` sur le SPI du Raspberry Pi. [`../../Comm distance/bancs/lora/linux/`](../../Comm%20distance/bancs/lora/linux/) pilote déjà ce montage, avec la trame du projet |
| **A1** | modem LTE-M en USB, MQTT |
| **A4** | MQTT sur le réseau d'entreprise |

Le banc LoRa côté Linux a été écrit pour un poste fixe : il devient la base du
transport de A2 et A3, sans réécriture.

### 4. Réception des appels

Le poste reçoit les appels des boutons et les traduit en missions immédiates,
comme le fait déjà `POST /api/appel`.

| | Comment le poste entend les boutons |
|---|---|
| **A1, A2, A4** | dongle EnOcean USB. [`../../Comm distance/bancs/enocean/`](../../Comm%20distance/bancs/enocean/) fait exactement cela, déployé et éprouvé |
| **A3** | la même radio LoRa que pour l'émission, le SX1276 étant half-duplex |

### 5. Authentification et service

- Authentification simple sur l'API, le réseau étant déjà maîtrisé ;
- unité `systemd`, redémarrage automatique, journal : le modèle de
  [`../../Comm distance/bancs/enocean/systemd/`](../../Comm%20distance/bancs/enocean/systemd/)
  s'applique tel quel ;
- déploiement par script plutôt qu'à la main, comme
  [`deployer.sh`](../../Comm%20distance/bancs/enocean/deployer.sh) : une copie
  périmée donne les symptômes du défaut qu'elle ne contient pas encore.

## Ordre proposé

1. **T1** : occurrences consommées persistées, plus de rejeu au redémarrage
2. **T2** : planning persisté
3. **Horloge système** à la place de l'horloge simulée
4. **Écoute réseau + authentification**
5. **Transport vers l'AGV** selon l'architecture retenue
6. **Réception des appels**
7. Service `systemd`, script de déploiement, instrumentation

Les étapes 1 à 3 rendent le Timer **fonctionnel sur le poste**. L'étape 4 le
rend **joignable**. Les étapes 5 et 6 le raccordent à l'AGV et aux boutons.

## Ce qui ne s'applique plus

L'analyse de l'encombrement mémoire et de la fragmentation du tas visait un
ESP32. Sur un Raspberry Pi, 27 ko de code et quelques dizaines de kilo-octets
de tas ne sont pas un sujet. L'instrumentation reste utile pour détecter une
fuite, elle n'est plus dimensionnante.
