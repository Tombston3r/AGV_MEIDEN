# Rendre le Timer déployable sur la carte V5.0.1

> Cible retenue : la **`AIO_AGV_Control_V5.0.1`**, celle de l'architecture
> [A4_Wifi](../../Comm%20distance/architectures/A4_Wifi/). Ce choix change la
> réponse : l'ESP32 y est **client du Wi-Fi d'entreprise**, donc l'heure réseau
> est à portée : le DS3231 cesse d'être un préalable et devient une garantie.
>
> État à jour : [`ETAT_PROJET.md`](ETAT_PROJET.md).

## Ce que la carte apporte déjà

Beaucoup, et c'est ce qui rend le chemin court.

| Besoin du Timer | Déjà là, dans A4 |
|---|---|
| Poser une destination sur le bus X/Y | **`Sequencer`** : trois phases, `t_setup`, acquit Y, retries, refus d'empiler sans accusé |
| Transporter la mission vers l'ATmega | **`link_protocol`** : `Cmd::Goto`, CRC, `SoftwareSerial` D52/D53 |
| Repli si l'ESP32 se tait | **heartbeat** + `SafeStop` côté ATmega |
| Serveur web | **`esp_http_server`**, déjà utilisé pour `/agvdump` |
| Stockage persistant | **`NvsStore`** : clé/valeur en flash, avec repli RAM si la NVS est illisible |
| Savoir si l'AGV roule | **`LinkState`** : `seq_state`, `state_flag::kMoving` |
| Format d'atelier | **`agvdump`**, déjà servi |

Le Timer n'a donc **rien à écrire** du séquencement, du transport, du repli de
sécurité ni du stockage bas niveau.

## Ce qui manque, par ordre de dépendance

### 1. L'heure : le seul vrai préalable

`EspClock::set_wall_clock()` **existe et n'est appelé nulle part** : l'horloge
murale vaut donc 0 en permanence aujourd'hui. Sans elle, le moteur reste gelé
et **rien ne part**.

À faire :

- **client SNTP** au raccordement Wi-Fi, appelant `set_wall_clock()` ;
- **fuseau** : `setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3")` + `tzset()` au
  démarrage : le moteur fait tout le reste, transitions d'heure comprises ;
- **état de confiance** (§2.3) alimentant le `heure_fiable` que le moteur
  consomme déjà : `TIME_UNTRUSTED` tant qu'aucune synchronisation n'a eu lieu.

⚠️ **Le DS3231 redevient nécessaire ici, et pour une raison propre à ce site.**
Le Wi-Fi 2,4 GHz y est saturé : c'est la raison d'être du projet. Un AGV mis
hors tension le soir et qui ne retrouve pas le réseau le matin **n'aura pas
d'heure, donc ne partira pas**. Ce n'est pas dangereux, c'est une
indisponibilité silencieuse un matin sur dix.

Le composant coûte 3 €, `IO21`/`IO22` sont **libres et sans aucun périphérique**
sur cette carte, et le moteur n'a pas à changer : seule la source de
`heure_fiable` change.

### 2. La persistance : `NvsStore`, pas LittleFS

La spec §3.3 parle de LittleFS ; **`NvsStore` suffit et existe déjà**. Le
document de planning fait 1 à 2 ko en JSON, largement sous la limite d'un blob
NVS.

Deux clés :

- `planning` : le document, tel que le produit `document_vers_json()` ;
- `consommees` : **les occurrences déjà parties**.

⚠️ **La seconde est la plus importante, et c'est le seul défaut connu du
moteur.** Sans elle, un redémarrage **dans la fenêtre de grâce** rejoue un
départ déjà effectué : l'AGV repart vers une destination qu'il a déjà servie.
C'est le chantier **T1**, à faire *avant* T2.

Le CRC32 et la version de schéma de la spec restent utiles ; l'écriture
atomique, elle, est **déjà assurée par la NVS**.

### 3. L'IHM joignable en permanence

Aujourd'hui le serveur web ne tourne que pendant la **fenêtre de maintenance** :
600 s, ouverte au contact ILS. Le planning doit être joignable **toute la
journée** depuis un poste d'atelier.

À faire : servir les routes du planning sur l'interface **cliente** (STA), en
permanence, sans toucher au point d'accès de maintenance qui garde son rôle.

⚠️ **C'est ce point qui rend l'authentification nécessaire**, pas le
déclenchement autonome en soi : une page joignable en permanence sur le réseau
d'entreprise, qui commande un véhicule, ne peut pas rester ouverte. Une
authentification simple suffit : le réseau est déjà maîtrisé.

Le **contrat REST est figé et testé** par [`../banc_api/`](../banc_api/) :
`ETag`/`409`/`428`, validation, pause, saut, appel, acquittement d'alerte. Le
portage a une référence à faire passer, pas une prose à interpréter.

### 4. Le chaînage mission → bus

Trois branchements, tous courts :

| Sens | À écrire |
|---|---|
| `Mission` → ATmega | `link::encode_goto(seq, station, speed, flags)` : la passerelle MQTT fait déjà exactement cela |
| `Arrived` → moteur | `mission_arrivee()` quand `LinkState::seq_state` passe à `Arrived` |
| Coupure → moteur | `interruption_agv(en_deplacement)` où `en_deplacement` = `seq_state == Transit` **ou** `flags & kMoving` |

Le dernier porte la règle de l'alerte : le déduire d'une simple perte de liaison
ferait crier au loup à chaque maintenance.

### 5. Ce qui n'est pas bloquant

- **Sélecteur physique « autorisation planning »** : recommandé comme verrou
  d'exploitation (maintenance, nettoyage). Impact nomenclature. Voir
  [`../DEPLOY.md`](../DEPLOY.md) §0.1.
- **Reboot quotidien** (§7) : utile en mise au point, à verrouiller contre un
  reboot en pleine mission.
- **Instrumentation du tas** (§7.4), **à faire tôt malgré tout** : le moteur
  utilise `std::string` et `std::vector` (21 occurrences), et le journal est un
  anneau de chaînes. Sur un système qui tourne des mois, la fragmentation est
  le risque réel, et le reboot quotidien la **masquerait**.

## Encombrement

Mesuré à `-Os` sur hôte, en ordre de grandeur pour la cible :

| | Code |
|---|---:|
| Moteur de planning | ~11 ko |
| Codec JSON | ~16 ko |
| **Total** | **~27 ko** |

Sans commune mesure avec les 4 Mo de flash de l'ESP32. La RAM, elle, dépend du
nombre d'entrées et de la profondeur du journal, à surveiller avec
l'instrumentation ci-dessus.

## Ordre proposé

1. **SNTP + fuseau + état de confiance** → le moteur cesse d'être gelé
2. **DS3231** → l'AGV part même quand le réseau est absent
3. **T1 : occurrences consommées en NVS** → plus de rejeu au redémarrage
4. **T2 : planning en NVS** → le planning survit à la coupure
5. **Routes REST sur l'interface cliente + authentification**
6. **Chaînage mission/arrivée/interruption**
7. Sélecteur, reboot quotidien, instrumentation

Les étapes 1 à 4 rendent le Timer **fonctionnel sans IHM** : pilotable par le
planning enregistré. L'étape 5 le rend **exploitable**.
