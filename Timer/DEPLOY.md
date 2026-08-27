# Déploiement du planning journalier

> ⚠️ **Ce chantier n'est pas encore déployable** — pour des raisons purement
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
> 📍 **Cible retenue : la carte `V5.0.1`** (celle de l'architecture A4). Le
> chemin détaillé, ce qui manque et dans quel ordre :
> [`docs/CHEMIN_V5.md`](docs/CHEMIN_V5.md).

## Phase 0 — Ce qui interdit la mise en service aujourd'hui

Trois verrous, à lever **avant** toute exécution automatique sur un chariot.
Aucun n'est logiciel.

### 0.1 Sécurité machine — à documenter, pas éliminatoire

**Correction du 2026-08-27.** Une version antérieure de ce document présentait
ce point comme bloquant, sur le scénario « une personne sur le trajet à 06:00
alors que personne n'a rien demandé ». **C'était surestimé** : l'AGV porte sa
propre chaîne de sécurité, indépendante de ce logiciel, qui coupe la puissance
sur obstacle et n'est réarmée que par un **appui physique**. Le risque de
heurter quelqu'un est traité par cette chaîne, que le déplacement soit demandé
par un opérateur ou par une horloge.

Ce qui change réellement avec le déclenchement autonome :

- **personne n'attend le mouvement.** Ce n'est plus un risque de blessure, mais
  un risque de **surprise** — et surtout d'**indisponibilité** : un arrêt à
  06:00 sans personne sur place laisse l'AGV bloqué jusqu'à ce que quelqu'un
  vienne le réarmer ;
- l'analyse de risques **gagne à être relue** (EN ISO 3691-4, changement de
  mode de fonctionnement), mais elle documente un mode d'exploitation, elle ne
  découvre pas un danger nouveau.

Restent recommandés, pour des raisons d'**exploitation** plus que de sécurité :

- **sélecteur physique « autorisation planning »**, en série avec l'exécution —
  un verrou franc pour la maintenance, le nettoyage, les périodes où l'on ne
  veut aucun mouvement autonome. Impact **nomenclature** ;
- **authentification** de l'interface web : elle commande un véhicule ;
- journal horodaté (spec §8), déjà produit par le moteur.

Le moteur reste un **organe de commande, pas de sécurité**. Il ne remplace pas
la chaîne : il en **rapporte l'effet** — voir §0.4.

### 0.4 Arrêt de sécurité : ce que le planning en dit

Quand la chaîne coupe la puissance **pendant un déplacement**, l'IHM affiche un
bandeau rouge qui ne peut pas être manqué :

```
⛔ Arrêt de sécurité pendant un déplacement
   Départ « livraison-matin » vers le poste 2 — interrompu à 06:00:30.
   Réarmement physique requis sur l'AGV.
   Les départs programmés sont suspendus jusqu'à l'acquittement.
```

Trois décisions derrière ce bandeau :

- **uniquement pendant un déplacement.** Une coupure alors que l'AGV est à
  quai — maintenance, fin de poste, quelqu'un qui passe devant — est journalisée
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
existant — `SeqState::Transit` et `state_flag::kMoving` — sur la carte A4. Le
banc le simule (`POST /api/sim/interruption`).

### 0.2 Mesure de `t_setup` — éliminatoire

Non mesurée à ce jour. Elle conditionne la répartition ESP32/ATmega du §5, et
figure déjà au kanban de l'architecture A4 : **c'est la même mesure**, à ne
pas refaire deux fois.

### 0.3 Capacité du bus I²C

Le DS3231 (`0x68`) s'ajoute sur `IO21`/`IO22` de l'ESP32. Sur la V5.0.1
relevée, **ces lignes ne portent aucun périphérique** : le DS3231 y serait
seul, et la question des pull-ups se règle avec ceux de son module.

⚠️ La spec évoque un conflit avec des MCP23017 (`0x20`–`0x27`) : **il n'y en a
pas sur cette carte.** Voir
[`docs/ALIGNEMENT_COMM_DISTANCE.md`](docs/ALIGNEMENT_COMM_DISTANCE.md).

## Phase 1 — Éprouver sans matériel *(faisable aujourd'hui)*

```bash
cd Timer
make test        # 19 moteur + 13 codec + 10 contrat API
make banc        # http://127.0.0.1:8081
```

Recette complète du banc :
[`banc_api/DEPLOY.md`](banc_api/DEPLOY.md) — neuf gestes, dont le franchissement
de minuit et le gel sur heure douteuse.

C'est la phase où l'on fait valider **le comportement** par l'exploitant :
bandeau de validation, rattrapage, saut motivé. Bien plus simple à corriger
maintenant qu'après le portage.

## Phase 2 — Horloge matérielle *(chantier T3)*

Prototypage recommandé **sur la LILYGO T-Beam** du banc LoRa plutôt que sur la
carte AGV : son I²C est déjà actif et son écran affiche l'heure lue.

| # | Étape | Attendu |
|---|---|---|
| 1 | DS3231 sur l'I²C, lecture de l'identifiant | réponse à l'adresse `0x68` |
| 2 | Lecture du bit **`OSF`** (oscillator stop flag) | `1` au premier démarrage : l'oscillateur n'a jamais tourné |
| 3 | Écriture de l'heure, coupure secteur 1 min, relecture | heure conservée, `OSF` à `0` — **c'est le test de la pile CR2032** |
| 4 | Dérive sur 48 h | quelques secondes au plus (TCXO ±2 ppm) |

La machine à trois états du §2.3 se branche là-dessus : `TIME_UNTRUSTED` tant
que `OSF` vaut `1` ou que l'année lue est aberrante, `TIME_RTC` ensuite,
`TIME_NTP` après resynchronisation.

⚠️ **`TIME_UNTRUSTED` doit geler le planning et être VISIBLE.** Le moteur le
gère déjà (`tick(now, heure_fiable=false)`) : rien ne part, rien n'est
consommé. Une heure fausse, sur un système qui déclenche des déplacements, est
pire qu'une absence d'heure.

## Phase 3 — Persistance *(chantiers T1 et T2)*

Le codec JSON existe et est testé. Restent la couche fichier LittleFS,
l'écriture atomique et le CRC32 (spec §3.3).

⚠️ **T1 avant T2** : les clés d'occurrences consommées doivent être persistées
**avec** le planning. Sans elles, un redémarrage **dans la fenêtre de grâce**
rejoue une mission déjà partie. C'est le seul défaut connu du moteur, et il est
documenté au kanban plutôt que masqué.

## Phase 4 — Portage sur la cible *(chantier T4)*

Le contrat REST est **figé et testé** par `banc_api/` : `ETag`/`409`/`428`,
validation, pause, saut. Le portage consiste à câbler ces routes sur le serveur
web **existant** de `Comm distance/architectures/A4_Wifi/`, pas à en créer un.

Le WebSocket du §6 se tranche à ce moment-là ; le banc sonde, ce qui suffit à
valider le comportement.

## Phase 5 — Chaînage vers le bus *(chantier T6)*

**Rien à écrire côté séquenceur.** La pose sur les signaux X, le respect du
`t_setup`, l'attente d'acquittement Y, les retries bornés, le refus d'empiler
sans accusé et le heartbeat ESP32↔ATmega **existent, écrits et testés** dans
`A4_Wifi`. Le moteur produit une `Mission` ; elle part en `Cmd::Goto` sur la
liaison série existante.

## Phase 6 — Recette sur chariot

À ne lancer qu'après la phase 0.

| # | Essai | Attendu |
|---|---|---|
| 1 | Sélecteur « autorisation planning » sur **arrêt** | aucune mission, quel que soit le planning |
| 2 | Journée **non validée**, échéance franchie | aucun déplacement, journal `sautee : journee non validee` |
| 3 | Journée validée, échéance franchie | l'AGV part vers la station programmée |
| 4 | Coupure secteur, redémarrage à midi | **aucune mission du matin rejouée**, sauts journalisés |
| 5 | Pile CR2032 retirée, redémarrage | `TIME_UNTRUSTED`, planning gelé, alarme visible en IHM |
| 6 | Reboot programmé 03:00 pendant une mission | **reboot refusé** — verrou du §7.3 |
| 7 | Obstacle **pendant** un déplacement programmé | Bandeau ⛔ ; départs suspendus ; réarmement physique puis acquittement nommé les rétablit |
| 8 | Coupure alors que l'AGV est **à quai** | **Aucun** bandeau — ligne au journal seulement |

Les essais **4** et **5** sont ceux qui distinguent un système qu'on peut
laisser tourner d'un système qu'il faut surveiller.

## En cas de panne

| Symptôme | Cause probable |
|---|---|
| Aucune mission alors que l'heure est passée | journée non validée (§3.2), pause active, ou occurrence déjà consommée — **lire le journal, il porte le motif** |
| Missions du matin rejouées après un redémarrage | clés d'occurrences non persistées — **chantier T1**, défaut connu |
| Planning gelé, IHM en alarme | `TIME_UNTRUSTED` : bit `OSF` du DS3231 à `1`, ou pile CR2032 vide |
| Heure juste au démarrage puis dérive de minutes | l'ESP32 tourne sur son quartz seul : le DS3231 n'est pas relu périodiquement |
| Mission décalée d'une heure fin mars | comportement **attendu** : horaire inexistant reporté au premier instant valide (décision du 2026-08-27) |
| Mission jouée deux fois fin octobre | idempotence en défaut : la clé doit porter la **date locale**, pas l'instant |
| L'AGV part alors que le sélecteur est sur arrêt | le sélecteur n'est pas **en série avec l'exécution** — verrou d'exploitation inopérant |
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
