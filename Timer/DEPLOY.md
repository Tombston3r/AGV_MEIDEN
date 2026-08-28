# Déploiement du planning journalier

> ⚠️ **Ce chantier n'est pas encore déployable**, pour des raisons purement
> techniques : il n'y a **ni horloge matérielle, ni persistance, ni firmware
> ESP32**. Le moteur, le codec et le contrat d'API sont écrits et testés ; le
> reste est à faire.
>
> Ce n'est **pas** une réserve de sécurité : l'AGV porte sa propre chaîne,
> indépendante, qui coupe la puissance sur obstacle et exige un réarmement
> physique. Voir §0.1, corrigé le 2026-08-27.
>
> État à jour : [`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md).
>
> 📍 **Cible depuis le 2026-08-28 : le poste central**, sous Linux, commun aux
> quatre architectures. Le Timer ne tourne plus sur l'ESP32 embarqué : la carte
> AGV redevient un exécutant. Chemin détaillé :
> [`docs/CHEMIN_POSTE.md`](docs/CHEMIN_POSTE.md), modèle d'ensemble :
> [`../docs/ARCHITECTURE_COMMUNE.md`](../docs/ARCHITECTURE_COMMUNE.md).
>
> ⚠️ Les phases 2 à 4 ci-dessous décrivent encore la cible ESP32. Elles sont
> **caduques** : `CHEMIN_POSTE.md` les remplace.

## Phase 0, À trancher avant la mise en service

Ce qui suit se règle **avant** la première exécution automatique sur un
chariot. Un seul point est réellement éliminatoire : la mesure de `t_setup`.
Les autres se documentent ou s'arbitrent.

### 0.1 Sécurité machine, à documenter, pas éliminatoire

**Correction du 2026-08-27.** Une version antérieure de ce document présentait
ce point comme bloquant, sur le scénario « une personne sur le trajet à 06:00
alors que personne n'a rien demandé ». **C'était surestimé** : l'AGV porte sa
propre chaîne de sécurité, indépendante de ce logiciel, qui coupe la puissance
sur obstacle et n'est réarmée que par un **appui physique**. Le risque de
heurter quelqu'un est traité par cette chaîne, que le déplacement soit demandé
par un opérateur ou par une horloge.

Ce qui change réellement avec le déclenchement autonome :

- **personne n'attend le mouvement.** Ce n'est plus un risque de blessure, mais
  un risque de **surprise**, et surtout d'**indisponibilité** : un arrêt à
  06:00 sans personne sur place laisse l'AGV bloqué jusqu'à ce que quelqu'un
  vienne le réarmer ;
- l'analyse de risques **gagne à être relue** (EN ISO 3691-4, changement de
  mode de fonctionnement), mais elle documente un mode d'exploitation, elle ne
  découvre pas un danger nouveau.

Restent recommandés, pour des raisons d'**exploitation** plus que de sécurité :

- **sélecteur physique « autorisation planning »**, en série avec l'exécution :
  un verrou franc pour la maintenance, le nettoyage, les périodes où l'on ne
  veut aucun mouvement autonome. Impact **nomenclature** ;
- **authentification** de l'interface web : elle commande un véhicule ;
- journal horodaté (spec §8), déjà produit par le moteur.

Le moteur reste un **organe de commande, pas de sécurité**. Il ne remplace pas
la chaîne : il en **rapporte l'effet**, voir §0.4.

### 0.2 Mesure de `t_setup` : éliminatoire

Non mesurée à ce jour. Elle conditionne la répartition ESP32/ATmega du §5, et
figure déjà au kanban de l'architecture A3 : **c'est la même mesure**, à ne
pas refaire deux fois.

### 0.3 Capacité du bus I²C

Le DS3231 (`0x68`) s'ajoute sur `IO21`/`IO22` de l'ESP32. Sur la V5.0.1
relevée, **ces lignes ne portent aucun périphérique** : le DS3231 y serait
seul, et la question des pull-ups se règle avec ceux de son module.

⚠️ La spec évoque un conflit avec des MCP23017 (`0x20`–`0x27`) : **il n'y en a
pas sur cette carte.** Voir
[`docs/ALIGNEMENT_COMM_DISTANCE.md`](docs/ALIGNEMENT_COMM_DISTANCE.md).

### 0.4 Arrêt de sécurité : ce que le planning en dit

Quand la chaîne coupe la puissance **pendant un déplacement**, l'IHM affiche un
bandeau rouge qui ne peut pas être manqué :

```
⛔ Arrêt de sécurité pendant un déplacement
   Départ « livraison-matin » vers le poste 2 : interrompu à 06:00:30.
   Réarmement physique requis sur l'AGV.
   Les départs programmés sont suspendus jusqu'à l'acquittement.
```

Trois décisions derrière ce bandeau :

- **uniquement pendant un déplacement.** Une coupure alors que l'AGV est à
  quai (maintenance, fin de poste, quelqu'un qui passe devant) est journalisée
  sans alerte. Crier au loup à chaque coupure viderait l'alerte de son sens ;
- **l'alerte suspend les départs autonomes** jusqu'à acquittement nommé.
  Repartir seul vers l'obstacle qui vient d'arrêter l'AGV est une boucle que
  personne ne surveille. L'**appel opérateur**, lui, reste possible : c'est une
  décision humaine ;
- **l'acquittement logiciel est distinct du réarmement physique.** Deux gestes,
  parce qu'ils répondent à deux questions : « la puissance est-elle rétablie ? »
  et « quelqu'un a-t-il regardé pourquoi ? »

⚠️ **Ce que le firmware devra fournir** (chantier T6) : le drapeau
`en_deplacement` au moment de la coupure. Il se déduit de l'état du séquenceur
existant (`SeqState::Transit` et `state_flag::kMoving`) sur la carte A3. Le
banc le simule (`POST /api/sim/interruption`).

## Phase 1 : Éprouver sans matériel *(faisable aujourd'hui)*

```bash
cd Timer
make test        # 25 moteur + 13 codec + 3 dump + 12 contrat API
make banc        # http://127.0.0.1:8081
```

Recette complète du banc :
[`banc_api/DEPLOY.md`](banc_api/DEPLOY.md) : vingt gestes, dont le
franchissement de minuit, le gel sur heure douteuse et l'arrêt de sécurité.

C'est la phase où l'on fait valider **le comportement** par l'exploitant :
bandeau de validation, rattrapage, saut motivé. Bien plus simple à corriger
maintenant qu'après le portage.

## Phase 2 : L'heure *(chantiers T3a puis T3b)*

**C'est le seul vrai préalable logiciel.** Sans heure fiable, le moteur reste
gelé par construction et rien ne part jamais.

### 2.1 SNTP, à faire en premier

`EspClock::set_wall_clock()` **existe et n'est appelé nulle part** : l'horloge
murale vaut 0 en permanence aujourd'hui. Sur la V5.0.1, l'ESP32 est **client du
Wi-Fi d'entreprise**, donc l'heure réseau est à portée.

| # | Étape | Attendu |
|---|---|---|
| 1 | Client SNTP au raccordement Wi-Fi, appelant `set_wall_clock()` | `/api/time` renvoie une date plausible, `source=ntp` |
| 2 | `setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3")` + `tzset()` au démarrage | l'heure locale affichée est juste, **transitions d'été comprises** |
| 3 | Redémarrage réseau coupé | `TIME_UNTRUSTED`, planning **gelé**, alarme visible |

Le moteur consomme déjà cet état (`tick(now, heure_fiable)`) : seule la
**source** de ce drapeau est à écrire.

### 2.2 DS3231 : la garantie, pas le luxe

⚠️ **Nécessaire pour une raison propre à ce site.** Le Wi-Fi 2,4 GHz y est
saturé : c'est la raison d'être du projet « Comm distance ». Un AGV mis hors
tension le soir et qui ne retrouve pas le réseau le matin **n'aura pas d'heure,
donc ne partira pas**. Indisponibilité silencieuse, un matin sur dix.

`IO21`/`IO22` sont **libres et sans aucun périphérique** sur cette carte : le
DS3231 (`0x68`) y serait seul. Prototypage recommandé **sur la LILYGO T-Beam**
du banc LoRa plutôt que sur la carte AGV : son I²C est déjà actif et son écran
affiche l'heure lue.

| # | Étape | Attendu |
|---|---|---|
| 1 | DS3231 sur l'I²C, lecture de l'identifiant | réponse à l'adresse `0x68` |
| 2 | Lecture du bit **`OSF`** (oscillator stop flag) | `1` au premier démarrage : l'oscillateur n'a jamais tourné |
| 3 | Écriture de l'heure, coupure secteur 1 min, relecture | heure conservée, `OSF` à `0` : **c'est le test de la pile CR2032** |
| 4 | Dérive sur 48 h | quelques secondes au plus (TCXO ±2 ppm) |

La machine à trois états du §2.3 s'achève ici : `TIME_UNTRUSTED` tant que `OSF`
vaut `1` ou que l'année lue est aberrante, `TIME_RTC` ensuite, `TIME_NTP` après
resynchronisation.

⚠️ **`TIME_UNTRUSTED` doit geler le planning et être VISIBLE.** Une heure
fausse, sur un système qui déclenche des déplacements, est pire qu'une absence
d'heure.

## Phase 3 : Persistance *(chantiers T1 puis T2)*

**`NvsStore` suffit : pas besoin de LittleFS.** La spec §3.3 parle de fichiers ;
le stockage clé/valeur en flash existe déjà dans A3, le document de planning
fait 1 à 2 ko en JSON, et **l'écriture atomique est assurée par la NVS**. Restent
utiles : le CRC32 et la version de schéma.

Deux clés :

| Clé | Contenu |
|---|---|
| `planning` | le document, tel que le produit `document_vers_json()` |
| `consommees` | **les occurrences déjà parties** |

⚠️ **T1 avant T2.** Sans les occurrences consommées, un redémarrage **dans la
fenêtre de grâce** rejoue un départ déjà effectué : l'AGV repart vers une
destination qu'il a déjà servie. C'est le seul défaut connu du moteur, et il est
documenté au kanban plutôt que masqué.

## Phase 4 : L'IHM joignable en permanence *(chantier T4)*

Le contrat REST est **figé et testé** par [`banc_api/`](banc_api/) :
`ETag`/`409`/`428`, validation, pause, saut, appel, acquittement d'alerte. Le
portage consiste à câbler ces routes sur le serveur `esp_http_server`
**existant** de `Comm distance/architectures/A3_Wifi/`, pas à en créer un.

⚠️ **Mais ce serveur ne tourne aujourd'hui que pendant la fenêtre de
maintenance** : 600 s, ouverte au contact ILS, sur le point d'accès. Le planning
doit être joignable **toute la journée** depuis un poste d'atelier : les routes
passent donc sur l'interface **cliente** (STA), en permanence, sans toucher au
point d'accès de maintenance qui garde son rôle pour `/agvdump`.

⚠️ **C'est cette permanence qui impose l'authentification**, pas le
déclenchement autonome en soi : une page joignable en continu sur le réseau
d'entreprise, et qui commande un véhicule, ne peut pas rester ouverte. Une
authentification simple suffit : le réseau est déjà maîtrisé.

Le WebSocket du §6 se tranche à ce moment-là ; le banc sonde, ce qui suffit à
valider le comportement.

## Phase 5 : Chaînage vers le bus *(chantier T6)*

**Rien à écrire côté séquenceur.** La pose sur les signaux X, le respect du
`t_setup`, l'attente d'acquittement Y, les retries bornés, le refus d'empiler
sans accusé et le heartbeat ESP32↔ATmega **existent, écrits et testés** dans
`A3_Wifi`. Le moteur produit une `Mission` ; elle part en `Cmd::Goto` sur la
liaison série existante.

Trois branchements, tous courts :

| Sens | À écrire |
|---|---|
| `Mission` → ATmega | `link::encode_goto(seq, station, speed, flags)` : la passerelle MQTT fait déjà exactement cela |
| `Arrived` → moteur | `mission_arrivee()` quand `LinkState::seq_state` passe à `Arrived` |
| Coupure → moteur | `interruption_agv(en_deplacement)` où `en_deplacement` = `seq_state == Transit` **ou** `flags & kMoving` |

⚠️ Le dernier porte la règle de l'alerte (§0.4). Le déduire d'une simple perte
de liaison ferait crier au loup à chaque maintenance.

## Phase 6 : Recette sur chariot

À ne lancer qu'après la phase 0.

| # | Essai | Attendu |
|---|---|---|
| 1 | Sélecteur « autorisation planning » sur **arrêt** | aucune mission, quel que soit le planning |
| 2 | Journée **non validée**, échéance franchie | aucun déplacement, journal `sautee : journee non validee` |
| 3 | Journée validée, échéance franchie | l'AGV part vers la station programmée |
| 4 | Coupure secteur, redémarrage à midi | **aucune mission du matin rejouée**, sauts journalisés |
| 5 | Pile CR2032 retirée, redémarrage | `TIME_UNTRUSTED`, planning gelé, alarme visible en IHM |
| 5b | **Réseau Wi-Fi coupé**, redémarrage, DS3231 en place | l'heure vient du DS3231, **les départs ont lieu** : c'est ce que le composant achète |
| 6 | Reboot programmé 03:00 pendant une mission | **reboot refusé** : verrou du §7.3 |
| 7 | Obstacle **pendant** un déplacement programmé | Bandeau ⛔ ; départs suspendus ; réarmement physique puis acquittement nommé les rétablit |
| 8 | Coupure alors que l'AGV est **à quai** | **Aucun** bandeau : ligne au journal seulement |

Les essais **4** et **5** sont ceux qui distinguent un système qu'on peut
laisser tourner d'un système qu'il faut surveiller.

## En cas de panne

| Symptôme | Cause probable |
|---|---|
| Aucune mission alors que l'heure est passée | journée non validée (§3.2), pause active, ou occurrence déjà consommée : **lire le journal, il porte le motif** |
| Missions du matin rejouées après un redémarrage | clés d'occurrences non persistées : **chantier T1**, défaut connu |
| Planning gelé, IHM en alarme | `TIME_UNTRUSTED` : bit `OSF` du DS3231 à `1`, ou pile CR2032 vide |
| Heure juste au démarrage puis dérive de minutes | l'ESP32 tourne sur son quartz seul : le DS3231 n'est pas relu périodiquement |
| `/api/time` renvoie l'epoch 1970, planning gelé | **SNTP non appelé** : `set_wall_clock()` n'est branché nulle part par défaut (phase 2.1) |
| IHM injoignable hors fenêtre de maintenance | les routes sont restées sur le point d'accès : les passer sur l'interface cliente (phase 4) |
| Mission décalée d'une heure fin mars | comportement **attendu** : horaire inexistant reporté au premier instant valide (décision du 2026-08-27) |
| Mission jouée deux fois fin octobre | idempotence en défaut : la clé doit porter la **date locale**, pas l'instant |
| L'AGV part alors que le sélecteur est sur arrêt | le sélecteur n'est pas **en série avec l'exécution** : verrou d'exploitation inopérant |
| Bandeau ⛔ à chaque coupure, même à l'arrêt | le drapeau `en_deplacement` est mal déduit : il doit venir de `SeqState::Transit` + `kMoving`, pas d'une simple perte de liaison |
| Aucun bandeau après un arrêt en plein trajet | la mission n'était pas suivie : `mission_arrivee()` appelée trop tôt, ou interruption non remontée |
| Reboot 03:00 pendant une mission | verrou du §7.3 absent : à corriger avant toute exploitation |

Le premier réflexe est toujours le **journal** : chaque saut y est motivé, et
c'est ce qui distingue une panne d'un refus délibéré.

## Instrumentation à laisser en place *(spec §7.4)*

`esp_get_free_heap_size()`, `esp_get_minimum_free_heap_size()` et
`esp_reset_reason()` exposés en télémétrie et journalisés à chaque démarrage.

Un plancher de heap décroissant sur une semaine est une fuite. Le reboot
quotidien la **masquerait** au lieu de la corriger : c'est pour cela qu'il est
retenu comme exercice du chemin de démarrage, **pas** comme remède.
