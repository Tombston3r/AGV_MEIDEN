# Spécification : Planning journalier des déplacements (carte AGV V5.0.1)

> Document de cadrage destiné à l'implémentation. Il fixe les décisions prises,
> signale les points encore ouverts et ne présume pas du découpage en modules.

---

## 1. Objectif

Permettre le déclenchement automatique de missions de déplacement AGV à des
horaires programmés, configurables depuis une interface web (modèle `agvdump`),
sur la base de la date et de l'heure.

**Changement de nature à retenir :** on passe d'un système d'appel à la demande
par un opérateur à un déclenchement autonome sur horloge. Ce n'est pas une simple
fonctionnalité logicielle (voir §9).

---

## 2. Source de temps

### 2.1 Décision

| Élément | Choix | Justification |
|---|---|---|
| Horloge de travail | Heure système ESP32 (`esp_timer`, quartz 40 MHz) | Dérive 10–50 ppm, soit quelques secondes/jour. Suffisant entre deux resynchronisations. |
| Référence persistante | **DS3231** sur I²C, adresse `0x68` | TCXO ±2 ppm, pile CR2032. Aucun conflit avec les MCP23017 (`0x20`–`0x27`). |
| Resynchronisation | NTP pendant les fenêtres de maintenance → écriture DS3231 | Corrige la dérive cumulée. |
| Fuseau horaire | `setenv("TZ", ...)` + `tzset()` | Gère les transitions été/hiver automatiquement. |

### 2.2 Bibliothèque `ESP32Time` : écartée

Ce n'est pas une base de temps : c'est une surcouche Arduino autour de
`settimeofday()` / `localtime_r()`. Elle n'apporte rien que la libc de l'ESP-IDF
ne fournisse déjà, et ne traite aucun des deux points bloquants :

- **Perte à la coupure d'alimentation.** L'heure vit en `RTC_SLOW_MEM` : elle
  survit au deep sleep, pas à une mise hors tension. Redémarrage à l'epoch 1970.
- **Aucun drapeau de non-fiabilité.** `getEpoch()` renvoie toujours une valeur,
  sans dire si elle vient d'une synchro réelle ou d'un compteur reparti de zéro.

S'ajoute que sa gestion du fuseau se limite à un offset fixe en secondes (pas de
DST), et que le projet est en **ESP-IDF/PlatformIO** : tirer le core Arduino en
composant pour ce wrapper n'a pas d'intérêt.

### 2.3 État de confiance de l'heure

Machine à trois états, exposée dans la télémétrie et l'IHM :

- `TIME_UNTRUSTED` : **planning gelé**, alarme IHM
- `TIME_RTC` : heure issue du DS3231, exécution autorisée
- `TIME_NTP` : resynchronisée récemment

Une heure fausse, sur un système qui déclenche des déplacements, est pire qu'une
absence d'heure. Le passage en `TIME_UNTRUSTED` doit être franc et visible.

### 2.4 Séquence de boot

```c
// Au démarrage
if (ds3231_read(&tm, &osf) != OK || osf || tm.tm_year < 124) {
    time_state = TIME_UNTRUSTED;   // planning gelé + alarme IHM
} else {
    struct timeval tv = { .tv_sec = mktime_utc(&tm) };
    settimeofday(&tv, NULL);
    time_state = TIME_RTC;
}

setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
tzset();

// Toutes les heures       : relecture DS3231, correction de la dérive esp_timer
// Fenêtre de maintenance  : NTP -> écriture DS3231 -> time_state = TIME_NTP
```

Le bit `OSF` (oscillator stop flag) du DS3231 est le signal de non-fiabilité :
il indique que l'oscillateur s'est arrêté depuis la dernière lecture.

**Règle transverse :** tout est stocké et manipulé en **UTC** en interne. La
conversion en heure locale n'intervient qu'à l'affichage et à l'évaluation des
règles de planning.

---

## 3. Modèle de données

### 3.1 Entrée de planning

Pas de cron brut : illisible pour l'exploitant.

| Champ | Contenu |
|---|---|
| `id` | UUID court, stable |
| `enabled` | actif / suspendu |
| `heure` | HH:MM, heure locale |
| `jours` | masque binaire lun–dim |
| `validite` | date de début / fin (optionnelle) |
| `exceptions` | liste de dates exclues (fériés, congés) |
| `mission` | station de destination (1–1024) + bits de commande |
| `priorite` | arbitrage si deux entrées coïncident |

### 3.2 Validation quotidienne

Champs de niveau document, **distincts des entrées** :

| Champ | Rôle |
|---|---|
| `valide_pour` | date AAAA-MM-JJ |
| `valide_par` | identifiant opérateur |
| `valide_le` | horodatage |

**Règle centrale :** le moteur refuse toute exécution si
`valide_pour != date_du_jour`. Le planning persiste ; c'est son autorisation
d'exécution qui expire chaque jour. Rien n'est effacé, mais rien ne part sans un
geste explicite.

`valide_par` + `valide_le` fournissent au passage la traçabilité de qui a
autorisé quels déplacements : utile le jour où il y a un incident.

### 3.3 Persistance

- JSON sur **LittleFS**
- En-tête avec **CRC32** et **numéro de version de schéma**
- Écriture **atomique** : fichier temporaire puis `rename`
- Conservation de la version N-1 pour rollback si le CRC échoue au boot

---

## 4. Moteur d'ordonnancement

### 4.1 Contrainte d'architecture

Le cœur est du **C++ pur**, sans dépendance Arduino ni ESP-IDF, avec une horloge
injectable :

```cpp
time_t next_occurrence(const Entry& e, time_t now);
```

Testable sur PC avec une horloge simulée. Une semaine de planning se valide en
quelques millisecondes. Même approche que le codec de trames LoRa.

### 4.2 Sémantiques à trancher explicitement

Faute de décision explicite, ces trois cas se tranchent tout seuls, et mal.

**Rattrapage.** L'AGV était occupé ou hors tension à 14:00 ; que fait-on à 14:07 ?
→ Fenêtre de grâce paramétrable (défaut suggéré : 5 min). Au-delà : saut +
journalisation. **Ne jamais rejouer la journée entière au redémarrage.**

**Idempotence.** Clé d'occurrence = `id_entrée + date_locale`, marquée comme
consommée. Règle au passage le changement d'heure d'automne, où 02:30 existe deux
fois.

**Changement d'heure de printemps.** Les entrées entre 02:00 et 03:00 n'existent
pas ce jour-là. Choix à faire : exécution à 03:00 ou saut, mais l'IHM doit
l'indiquer.

---

## 5. Chaînon vers le bus X/Y

**Le planning n'écrit jamais directement sur les MCP23017.** Il produit une
*mission*, consommée par un séquenceur :

1. Pose de l'adresse station sur les 22 signaux X
2. Respect du `t_setup` (**valeur non encore mesurée : voir §10**)
3. Attente d'acquittement sur les signaux Y
4. Timeout + retry borné
5. Refus d'empiler une nouvelle destination tant que la précédente n'est pas
   acquittée

### Répartition ESP32 / ATmega2560

| Rôle | Cible |
|---|---|
| Temps, stockage, web, décision | ESP32 |
| Séquencement déterministe sur le bus | ATmega2560 *(si la mesure de `t_setup` le confirme)* |

**Heartbeat entre les deux :** si l'ATmega ne voit plus l'ESP32 pendant N
secondes, il refuse les missions issues du planning.

---

## 6. Interface web

Modèle `agvdump` : REST pour la configuration, WebSocket pour le temps réel.

```
GET    /api/planning          → liste + version (ETag)
PUT    /api/planning          → remplacement complet, 409 si version périmée
GET    /api/planning/next     → 10 prochaines occurrences calculées
POST   /api/planning/pause    → suspension globale
POST   /api/planning/skip     → saute la prochaine occurrence
GET    /api/time              → heure RTC, source, dérive estimée
POST   /api/planning/validate → validation de la journée
```

**Contrôle de version optimiste (ETag / 409)** : évite qu'un second navigateur
écrase silencieusement les modifications du premier. Cas fréquent dès qu'il y a
un poste atelier et un poste bureau.

### Affichage

La vue la plus utile est la **liste des prochaines occurrences calculées**, pas
la liste des règles : c'est ce qui permet à l'exploitant de vérifier qu'il a bien
compris ce qu'il a saisi.

### Bandeau de validation (non contournable)

```
Planning du 27/08 non validé : aucun déplacement automatique
Planning en mémoire : 6 missions (dernière validation : 26/08 par M. Dupont)
[ Reprendre le planning d'hier ]  [ Modifier ]  [ Valider la journée ]
```

Le bouton « reprendre hier » n'est pas cosmétique : si les journées se
ressemblent, imposer une ressaisie complète pousse les gens à contourner le
système. Un clic de confirmation obtient le même engagement conscient sans la
friction.

**Rappel actif :** à 06:30, si la journée n'est pas validée alors que des
missions étaient prévues → voyant pupitre ou notification. C'est ce qui manque à
un rappel purement passif : sans cela, l'opérateur découvre l'absence de planning
par l'absence de mouvement, indiscernable d'une panne.

---

## 7. Reboot quotidien

### 7.1 Retenu, mais pour une autre raison que celle envisagée

**Écarté comme mécanisme d'effacement du planning.** Utiliser un mode de
défaillance comme fonctionnalité pose trois problèmes : perte de données réelle
(planning du lendemain saisi à 17h, effacé à 03:00), signal muet (personne n'est
prévenu), et fragilité (le jour où le reboot échoue, le rappel disparaît sans
que personne ne s'en aperçoive). Remplacé par la validation quotidienne (§3.2).

**Retenu comme exercice du chemin de démarrage.** Un reboot programmé quotidien
fait apparaître les bugs de boot (RTC illisible, LittleFS corrompu, séquenceur
mal réinitialisé) en phase de mise au point plutôt que six mois plus tard lors
du premier redémarrage réel. Argument valable surtout en prototype.

### 7.2 Note technique

Sur un redémarrage **logiciel** (`esp_restart()`), le domaine RTC n'est pas coupé
et l'ESP-IDF restitue l'heure système au boot. Le reboot programmé ne fait donc
pas perdre l'heure : contrairement à une coupure d'alimentation ou un brownout.
Cela ne dispense pas du DS3231 : disjoncteur, maintenance et déplacement de
l'AGV restent des scénarios réels.

### 7.3 Verrous obligatoires

- Fenêtre à heure creuse (suggéré : **03:00**)
- **Interdiction de reboot si une mission est en cours ou si l'AGV n'est pas à
  quai.** Un redémarrage en cours de déplacement relâche les sorties X sans que
  le contrôleur MEIDEN en soit informé.

### 7.4 Instrumentation mémoire, à faire indépendamment

Si le heap se dégrade au fil des jours, c'est une fuite ou une fragmentation. Le
reboot la **masque** au lieu de la corriger. Risque : livrer un système qui fuit
à 200 o/h, invisible six mois, puis qui plante en trois jours.

À exposer en télémétrie et dans l'IHM :

```c
esp_get_free_heap_size();          // heap courant
esp_get_minimum_free_heap_size();  // plancher depuis le boot
esp_reset_reason();                // journalisé à chaque démarrage
```

Un plancher décroissant de façon monotone sur une semaine = fuite. Suspects
habituels sur ce type de carte : buffers WebSocket par client, parsing JSON à
chaque requête, sockets de clients déconnectés brutalement non libérés.

---

## 8. Journalisation

Journal circulaire horodaté, exportable en **CSV** :

- Missions exécutées / sautées (avec motif du saut)
- Validations quotidiennes (`valide_par`, `valide_le`)
- Transitions d'état de l'heure
- `esp_reset_reason()` à chaque démarrage

Utile pour la démonstration client autant que pour le débogage.

---

## 9. Sécurité machine, à traiter avant le codage

Le passage au déclenchement autonome sur horloge est un **changement de mode de
fonctionnement au sens sécurité machine**. L'**EN ISO 3691-4** s'applique aux
AGV : l'analyse de risques est à reprendre (scénario type : personne présente sur
le trajet à 06:00 alors que personne n'a rien demandé).

Minimum à prévoir :

- **Sélecteur physique « autorisation planning »** en série avec l'exécution
- **Authentification** sur l'interface web : elle commande désormais un véhicule,
  plus seulement une consultation
- Journal horodaté (§8)

---

## 10. Points ouverts

| Point | Nature | Impact |
|---|---|---|
| `t_setup` du bus X/Y | **Mesure physique requise** | Détermine si l'ATmega2560 doit être conservé pour le séquencement |
| DST de printemps (02:00–03:00) | Décision produit | Exécution à 03:00 ou saut |
| Fenêtre de grâce du rattrapage | Décision produit | Valeur par défaut suggérée : 5 min |
| Capacité du bus I²C / pull-ups | Vérification | Ajout du DS3231 sur les lignes existantes |
| Diffusion NTP depuis le poste fixe | Option | Possible si le poste fixe est conservé (Ethernet filaire), mais réintroduit une dépendance radio pour une fonction qui doit rester autonome. Le DS3231 reste la référence locale dans tous les cas. |
