# Brief projet — Remplacement carte AIO AGV Control V5.0.1

> À placer en `CLAUDE.md` à la racine du dépôt.
> Rôle attendu : développement de l'ensemble du logiciel embarqué et poste fixe.
> Le matériel est spécifié par ailleurs ; ce document couvre le logiciel.

---

## 1. Contexte

Un AGV **MEIDEN à guidage magnétique** circule dans une usine. Des opérateurs l'appellent
depuis des boutons déportés placés aux points d'arrêt. La carte de communication d'origine
(**AIO AGV Control V5.0.1**, ATmega2560 + ESP32, liaison Wi-Fi 2,4 GHz) doit être remplacée :
l'environnement 2,4 GHz du site est saturé.

La carte n'est **pas** une simple passerelle. Elle porte la mémoire de mission : l'AGV ne
connaît qu'**une seule destination à la fois** et l'oublie au redémarrage. C'est la carte qui
maintient une file de jusqu'à 5 courses. C'est le point fonctionnel le plus important du projet.

Trois architectures de liaison ont été étudiées. Le logiciel doit être écrit pour les
**supporter toutes les trois derrière une abstraction commune**, car le choix final n'est pas
encore tranché avec le client :

| | Architecture | Statut |
|---|---|---|
| **A1** | Cellulaire : SMS (variante A) ou LTE-M/MQTT (variante B) | Étudiée à la demande du client, **non recommandée en liaison principale** ; retenue en complément bas volume pour alertes hors site |
| **A2** | LoRa P2P 868 MHz (RFM95W), boutons sur pile | Solution de référence homogène |
| **A3** | Hybride EnOcean (boutons sans pile) + LoRa | **Architecture retenue**, sous réserve que l'exigence « sans pile » soit réelle |

---

## 2. Ce que tu dois produire

Monorepo, structure attendue :

```
/firmware
  /agv                  ESP32 embarqué sur l'AGV — le cœur du projet
  /poste-esp32          Poste fixe en variante ESP32 (A2 / A3)
  /bouton-lora          Nœud bouton sur pile (A2 uniquement)
  /common               Bibliothèques partagées (protocole, CRC, transport, config)
/poste-unipi            Poste fixe en variante UniPi E413 (A1)
/sim                    Simulateur d'automate MEIDEN (banc de test hors AGV)
/web                    Interface de supervision (servie par le poste fixe)
/docs                   Chronogrammes, tables de signaux, procédures d'essai
/tools                  Scripts de flash, de relevé, de rejeu de trames
```

Toolchain : **PlatformIO + ESP-IDF** (pas Arduino IDE). Framework Arduino toléré pour
`/bouton-lora` seulement. C++17. Poste UniPi : **Python 3.11**, service systemd.

---

## 3. Contraintes non négociables

### 3.1 Sûreté

Cette carte est un **organe de commande, pas un organe de sécurité**. L'arrêt d'urgence,
les bumpers et le scrutateur laser restent dans une chaîne indépendante (ISO 3691-4).
Ne jamais écrire de code qui prétende assurer une fonction de sécurité.

Comportement obligatoire en perte de liaison : **arrêt sûr au point d'arrêt suivant**, jamais
d'état indéterminé. Chien de garde applicatif : absence de trame valide pendant `N` secondes
→ état sûr + LED `FAULT`. `N` est un paramètre de configuration, pas une constante.

À la mise sous tension et après tout reset, **le bus X doit être physiquement à zéro** avant
que le firmware ne prenne la main.

### 3.2 Rien de figé tant que ce n'est pas mesuré

Plusieurs paramètres matériels ne sont **pas encore relevés sur la carte d'origine**. Ils sont
listés au §12. Règle absolue : **aucun de ces paramètres ne doit apparaître en dur dans le code.**
Ils vivent dans un fichier de configuration unique (`common/config/hardware_profile.h` +
`profiles/*.yaml`), avec des valeurs par défaut explicitement marquées `// PROVISOIRE — voir §12`.

### 3.3 Compatibilité avec l'existant

La procédure de diagnostic `agvdump` est utilisée en production par le client. Le nouveau
firmware doit servir une page `/agvdump` **au format compatible** (mêmes champs, mêmes noms de
compteurs), sinon les procédures d'atelier deviennent caduques. Voir §9.

---

## 4. Le cœur : driver du bus MEIDEN

C'est la partie critique. Elle est **identique dans les trois architectures** et doit être
totalement découplée de la couche transport.

### 4.1 Bus X — 22 sorties (carte → automate)

| Groupe | Signaux | Rôle |
|---|---|---|
| Destination | `X96`, `X97`, `XA0`…`XA7` | 10 bits, 1024 valeurs |
| Vitesse demandée | `X86`, `X87`, `X90`, `X91` | 4 bits, 16 niveaux |
| Aiguillage / sens | `X84`, `X85` | changement de direction |
| Marche / arrêt | `X82` (standby release / start), `X83` (standby stop) | |
| Protocole d'écriture | `X92` (instruction data input switch), `X93` (write strobe), `X94` (type de donnée : station ou comptage de marqueurs), `X95` (frein externe) | |

### 4.2 Bus Y — 21 entrées (automate → carte)

| Groupe | Signaux | Rôle |
|---|---|---|
| État / diagnostic | `Y03` (défaut), `Y21` (pas de destination programmée) | |
| Mouvement | `Y05` (moving flag), `Y10` (in station flag) | |
| Monitors | `Y15` (écho aiguillage), `Y20` (écho sens), `Y22` (instruction reading complete) | |
| Vitesse courante | `Y11`…`Y14` | 4 bits |
| **Position courante** | `Y23`…`Y34` | **10 bits, 1024 valeurs** |

Numérotation **octale** (convention Meiden/Mitsubishi). Attention aux conversions.

### 4.3 Séquenceur trois phases

À reproduire à l'identique du firmware d'origine. Les noms de compteurs doivent correspondre
littéralement à la section `AGV STATE` de `agvdump`.

**Phase ÉCRITURE**
1. Positionner `X94` (type de donnée)
2. Écrire la destination sur les 10 bits d'adresse
3. Écrire la vitesse sur les 4 bits
4. Activer `X92`
5. Monter `X93` (strobe)
6. Attendre `Y22`. Timeout → `write_tries++`, on recommence. Résultat final → `write_op_return`
7. Redescendre `X93` puis `X92`

**Phase DÉMARRAGE**
8. Monter `X82`
9. Attendre `Y05 == 1` → `start_tries`, `start_op_return`
10. Redescendre `X82`

**Phase TRANSIT**
11. Lire `Y23`…`Y34` en continu, décoder les 10 bits → `current_station`
12. Surveiller `Y03`, `Y21`, `Y15`, `Y20`

**Phase ARRIVÉE**
13. `Y10 == 1` → arrivé
14. Éventuellement `X83` → `stop_tries`, `stop_op_return`
15. Dépiler la course suivante → retour à l'étape 1

Implémente ça en **machine à états explicite** (pas de `delay()`, pas de blocage), avec
transitions journalisées et instrumentation de chaque timeout.

### 4.4 Déterminisme temporel — point d'attention majeur

Le MEGA d'origine posait les 10 bits d'adresse en un seul cycle (`PORTA = x`). Le remplacement
n'a pas cette propriété gratuite. Trois variantes matérielles sont en discussion :

| Variante | Pose du bus | Conséquence pour le code |
|---|---|---|
| 4× MCP23017 sur I²C | ~150 µs, GPIOA et GPIOB décalés de ~25 µs | Écrire GPIOA/GPIOB en une seule transaction ; documenter le décalage résiduel |
| 3× 74HC595 + 3× 74HC165 sur SPI | ~3 µs, **simultanée** (latch `RCLK` commun) | Variante préférable ; abstraire derrière la même interface |
| ATmega2560 conservé | < 1 µs | Second firmware + protocole inter-MCU UART à définir |

**Le driver doit être écrit derrière une interface `IBusDriver`** (`writeX(uint32_t)`,
`readY()`, `pulse(signal, duration_us)`) avec trois implémentations interchangeables. Le choix
matériel n'est pas encore arrêté ; ne le présuppose pas.

Contrainte FreeRTOS : la séquence de pose du bus tourne dans une **tâche épinglée sur le cœur 1**,
en section critique pendant la pose adresse+vitesse+strobe. La pile radio/modem vit sur le cœur 0.

### 4.5 File de courses

Jusqu'à **5 destinations**. Sur la carte d'origine elle n'existe qu'en RAM et se perd à chaque
coupure. **Amélioration à intégrer** : persistance en **NVS** (flash ESP32), avec restauration
au boot et politique de validité (une course de plus de X minutes est écartée — X configurable).

Champs à exposer, noms compatibles `agvdump` : `nb_courses_programmed`, `programmed_courses[5]`.

---

## 5. Abstraction transport

Toute la logique métier passe par une interface unique. C'est ce qui permet de basculer entre
architectures sans retoucher le séquenceur.

```cpp
class ITransport {
public:
  virtual bool begin() = 0;
  virtual bool send(const Frame& f) = 0;          // non bloquant
  virtual bool poll(Frame& out) = 0;              // trame reçue ?
  virtual LinkHealth health() const = 0;          // RSSI/RSRP, âge du dernier ACK, état
  virtual const char* name() const = 0;
};
```

Implémentations : `LoRaTransport`, `SmsTransport`, `MqttLteTransport`.
Le sélecteur se fait à la compilation (`-D TRANSPORT_LORA`) **et** à l'exécution si plusieurs
sont compilées — l'architecture retenue prévoit LoRa en principal + SMS en complément alerte.

### 5.1 Trame applicative — commune à tous les transports

Compacte, versionnée, avec numéro de séquence :

| Champ | Taille | Rôle |
|---|---|---|
| `ver` | 4 bits | version de protocole |
| `type` | 4 bits | `CMD_GOTO`, `CMD_STOP`, `ACK`, `TELEMETRY`, `PING`, `PAIR` |
| `node_id` | 16 bits | émetteur |
| `seq` | 8 bits | numéro de séquence, roulant |
| `station` | 10 bits | destination |
| `speed` | 4 bits | vitesse |
| `flags` | 8 bits | priorité, purge de file, etc. |
| `crc` | 16 bits | CRC-16/CCITT sur tout ce qui précède |

**Idempotence obligatoire** : si l'AGV reçoit deux fois la même `seq` du même `node_id`, il
**ré-acquitte sans ré-exécuter**. Sans ça, un ACK perdu déclenche une course en double.
Fenêtre anti-rejeu : garder les 16 derniers `(node_id, seq)` vus.

Chiffrement **AES-128-CTR** sur le payload, clé partagée en NVS. Recommandé même en réseau
privé : sans lui, n'importe qui avec un module à 10 € peut appeler l'AGV.

---

## 6. Architecture 1 — LoRa P2P

- Module **RFM95W / SX1276** sur SPI.
- 868 MHz, **SF9**, BW 125 kHz, CR 4/5, sync word privé (≠ 0x34 réservé LoRaWAN).
- Latence cible ~200 ms, pire cas ~800 ms après 3 retransmissions.
- **Le RFM95W est half-duplex.** Le firmware du poste fixe doit alterner proprement entre
  écoute de télémétrie et émission de commande, avec fenêtre d'écoute d'ACK après chaque
  émission. À écrire explicitement dans l'ordonnanceur, pas à improviser.
- **Compteur de rapport cyclique obligatoire** : EN 300 220 / ERC 70-03 imposent 1 % sur la
  bande. Implémenter un budget glissant sur 1 h qui **refuse l'émission** au-delà, et remonte
  le refus en défaut applicatif visible. C'est une obligation réglementaire, pas une option.

### 6.1 Nœud bouton sur pile (`/bouton-lora`)

- Sommeil profond permanent (< 2 µA), réveil sur front GPIO.
- Émission `CMD_GOTO`, attente d'ACK jusqu'à 400 ms, 3 tentatives.
- Retour visuel : LED verte fixe 2 s = ACK reçu ; LED rouge clignotante = échec après 3 essais.
  **Cette fonction est absente de la solution EnOcean pure** — c'est un argument à conserver.
- Ajout d'un bouton = flasher un `node_id` + une station. Aucune modification côté AGV.

---

## 7. Architecture 3 — EnOcean + LoRa (retenue)

Chaîne : bouton **PTM 210** (sans pile) → récepteur **TCM 515** (UART, protocole ESP3) sur
l'ESP32 du poste fixe → traduction en trame LoRa → **RFM95W** → ESP32 AGV.

- Décodeur **ESP3** à écrire ou à intégrer (`/common/enocean/`) : trames sync 0x55, CRC8 header,
  CRC8 data, types RADIO_ERP1.
- Le PTM 210 émet **3 sous-télégrammes** identiques par appui → déduplication obligatoire
  dans une fenêtre de ~100 ms.
- Le PTM 210 envoie un **identifiant émetteur 32 bits gravé usine**, pas un numéro de station.
  Il faut donc une **table d'appairage** `enocean_id → station`, persistée en NVS, alimentée
  par un **mode appairage** (bouton/commande web : « appuyez sur le bouton à associer »).
- ⚠️ **Le TCM 515 est réception seule.** Aucun retour visuel n'est possible vers le bouton
  EnOcean. Si un accusé côté opérateur est exigé, il faut passer au **TCM 310** (bidirectionnel)
  ou prévoir un voyant déporté câblé au poste. **À trancher avant de figer l'IHM.**

---

## 8. Architecture 2 — Cellulaire

### 8.1 Variante A — SMS

Poste fixe : **UniPi E413 (variante à modem LTE intégré)** ou ESP32 + SIM7600E-H.
AGV : ESP32 + **SIM7600E-H**, piloté en AT sur UART2.

Ce qu'il faut savoir avant d'écrire une ligne : **le SMS n'offre ni latence bornée, ni ordre de
remise, ni garantie de remise, ni protection contre les doublons.** Le firmware doit donc :

- Numéroter et acquitter **au niveau applicatif** de bout en bout (le SR opérateur ne prouve
  que la remise au terminal, pas le traitement).
- **Rejeter toute trame plus ancienne que la dernière traitée** (`seq` avec fenêtre) — c'est la
  seule protection contre un `STOP` qui arriverait avant le `GOTO` qu'il annule.
- Refuser d'exécuter une commande dont l'horodatage dépasse `max_command_age_s` (défaut : 15 s).
  Une commande vieille de 3 minutes sortie du SMSC ne doit **jamais** faire bouger l'AGV.

Pile AT : machine à états explicite, `+CMTI` sur RI → `AT+CMGR` → parse → `AT+CMGD`.
Séquence `PWRKEY` : 1 000 ms pour allumer, 2 500 ms pour éteindre proprement.
Gérer : PIN, APN, roaming, perte d'attachement, SIM éjectée, modem muet.
**Chien de garde matériel obligatoire** (TPL5010) : la pile AT peut se bloquer sans que le
watchdog logiciel ne le voie.

### 8.2 Variante B — LTE-M/NB-IoT + MQTT

Modem **SIM7080G**, broker **Mosquitto** (VPS ou serveur usine).
Topics : `agv/<id>/cmd`, `agv/<id>/ack`, `agv/<id>/telemetry`, `agv/<id>/status`.
QoS 1 minimum. **Last Will and Testament** sur `status` → détection immédiate de perte de l'AGV.
TLS avec certificat client si le broker est hors site.

Si le cellulaire est imposé par le client, **c'est cette variante qu'il faut coder, jamais le SMS.**

### 8.3 Usage résiduel légitime du SMS

Quelle que soit l'architecture retenue, prévoir un module `AlertGateway` sur le poste fixe :
quelques SMS par mois vers un technicien de maintenance **hors site** en cas de défaut bloquant.
Bas volume, forte valeur, ~10 €/an. Ce module est indépendant de la chaîne de commande et ne
doit jamais pouvoir la piloter.

---

## 9. Poste fixe

### 9.1 Variante ESP32 (A2 / A3)

- `TCM 515` sur UART1, `RFM95W` sur SPI, **Ethernet filaire** via W5500 ou WT32-ETH01.
  L'Ethernet filaire est un choix délibéré : **aucune émission 2,4 GHz permanente**.
- `ESPAsyncWebServer` + **WebSocket** pour la mise à jour temps réel, assets en **LittleFS**.
- mDNS : `agv.local`.

### 9.2 Variante UniPi E413 (A1)

Python 3.11, service systemd, entrées TOR lues via l'API de l'automate.
⚠️ **Vérifier d'abord le runtime réellement disponible** sur la référence commandée (Mervis IDE
vs. Linux + API E/S). Ne pas présupposer : poser la question avant d'écrire le code d'accès aux E/S.

### 9.3 Supervision web

Champs à afficher, mappés directement sur les signaux `agvdump` existants :

| Champ | Source |
|---|---|
| Station courante | `Y23`…`Y34` décodés |
| En mouvement | `Y05` |
| En station | `Y10` |
| Défaut | `Y03`, `Y21` |
| Vitesse courante | `Y11`…`Y14` |
| Tension batterie | ADC AGV (attention : traverse la barrière d'isolation, voir schéma) |
| Fraîcheur de liaison | horodatage du dernier ACK / de la dernière télémétrie |
| File de courses | `nb_courses_programmed`, `programmed_courses[5]` |
| Compteurs | `write_tries`, `start_tries`, `stop_tries` et leurs `*_op_return` |

### 9.4 Wi-Fi de maintenance sur l'AGV

Le Wi-Fi de l'ESP32 AGV est **désactivé par défaut**. Activation à la demande par contact ILS
(aimant) ou bouton, **extinction automatique après 10 minutes**. Pendant cette fenêtre, l'ESP32
sert le point d'accès et la page `/agvdump` au format historique. Objectif : préserver la
procédure de diagnostic du client sans polluer le 2,4 GHz en permanence.

---

## 10. Simulateur d'automate MEIDEN (`/sim`)

**Écris-le en premier.** Aucun des relevés matériels bloquants n'est fait (§12), et l'accès à
l'AGV réel sera compté. Le simulateur permet de développer et de tester 90 % du logiciel sans
matériel.

Deux niveaux :
1. **Simulateur logiciel** : implémentation de `IBusDriver` qui rejoue le comportement de
   l'automate (accusés `Y22`/`Y05`/`Y10` avec délais paramétrables, injection de timeouts, de
   défauts `Y03`, de rebonds). Permet les tests unitaires en natif sur PC.
2. **Banc HIL** : un second ESP32 (ou un Arduino MEGA) qui présente les 43 lignes en face de la
   carte réelle et joue le rôle de l'automate. Permet de valider les chronogrammes à l'oscillo
   avant tout branchement sur l'AGV.

Le simulateur doit accepter un **profil de timings** en YAML, pour rejouer aussi bien un
automate rapide qu'un automate lent, et vérifier que le séquenceur tient dans les deux cas.

---

## 11. Tests attendus

- Tests unitaires natifs (Unity/PlatformIO) sur : encodage/décodage de trames, CRC,
  idempotence `seq`, décodage octal Meiden, budget de rapport cyclique LoRa, décodeur ESP3.
- Tests d'intégration contre le simulateur : les 4 phases du séquenceur, tous les chemins de
  timeout, la persistance NVS de la file, la reprise après coupure.
- Tests de dégradation : perte de liaison, trame corrompue, trame rejouée, trame périmée,
  file pleine, modem muet, SIM absente.
- **Un test explicite par ligne du §12** vérifiant que le paramètre est bien lu depuis la
  configuration et non figé.

---

## 12. Points ouverts — à ne jamais figer en dur

Ces éléments ne sont **pas encore relevés**. Le code doit les traiter comme des paramètres.
Si tu as besoin d'une valeur pour avancer, prends la valeur par défaut, marque-la
`// PROVISOIRE §12` et **signale-le dans ton compte rendu**.

1. **Amplitude réelle des lignes Y** (6 V rail LM7806 ou 24 V) — relevé oscillo sur `Y05`. Impacte le matériel, pas le code, mais conditionne les seuils de debounce.
2. **Brochage réel des SUB-D 25** — divergence non tranchée entre deux tables de câblage (CN61/62/63 vs CN62/63/64). Le mapping signal → broche doit vivre dans un fichier de configuration unique et modifiable sans recompiler la logique.
3. **Logique de l'automate : PNP ou NPN** — inverse la polarité des 22 voies X. Prévoir un booléen de configuration `bus_x_active_high`.
4. **`t_setup` du bus X** — temps de stabilisation avant le strobe `X93`. Chronogramme oscillo à relever sur la V5.0.1. Paramètre `t_setup_us`, défaut provisoire 200 µs.
5. **Timeouts des accusés** `Y22`, `Y05`, `Y10` — à relever, pas à deviner. Paramètres distincts.
6. **Correspondance des repères** T9, T10, T12, T13, T20…T24 sérigraphiés sur la V5.0.1 avec les signaux Y.
7. **Protocole ESP32 ↔ application mobile « AIO AGV Remote »** — non documenté. Décider si on le reproduit ou si l'application est abandonnée au profit de l'interface web.
8. **TCM 515 (Rx seul) ou TCM 310 (bidirectionnel)** — conditionne l'existence d'un retour d'accusé côté opérateur EnOcean.
9. **Runtime disponible sur l'UniPi E413** commandé.
10. **Variante matérielle d'interface bus** retenue : MCP23017 / 74HC595+165 / ATmega2560 conservé.

---

## 13. Conventions

- C++17, `-Wall -Wextra -Werror`. Pas de `new`/`delete` après l'initialisation ; allocation
  statique ou pools. Pas de `String` Arduino dans le chemin temps réel.
- Python : `ruff` + `mypy --strict`.
- Journalisation structurée à niveaux, sortie UART + tampon circulaire consultable via `/agvdump`.
- Tout paramètre configurable est dans `profiles/*.yaml`, généré en `.h` au build. Une seule
  source de vérité.
- Commits conventionnels. Une PR = une fonction.
- Commentaires en français (le client et le mainteneur sont francophones), identifiants en anglais.

---

## 14. Ordre de travail suggéré

1. `/sim` — simulateur logiciel de l'automate + tests natifs.
2. `/common` — trame, CRC, AES, idempotence, `IBusDriver`, `ITransport`, couche de configuration.
3. `/firmware/agv` — séquenceur trois phases contre le simulateur, file NVS, machine à états.
4. `LoRaTransport` + `/firmware/poste-esp32` — chaîne A2 complète, ordonnanceur half-duplex.
5. `/web` — supervision WebSocket + `/agvdump` compatible.
6. Décodeur EnOcean ESP3 + mode appairage → chaîne A3.
7. `SmsTransport` et `MqttLteTransport` → chaîne A1.
8. `AlertGateway` (SMS bas volume, toutes architectures).
9. Banc HIL, puis intégration sur AGV réel.

---

## 15. Comment je veux que tu travailles

- **Pose des questions plutôt que de supposer** sur tout ce qui touche au §12. Une hypothèse
  silencieuse sur un chronogramme d'automate coûte plus cher qu'un aller-retour.
- Avant d'écrire du code temps réel, **écris le test qui le contraint** contre le simulateur.
- Signale explicitement chaque endroit où tu poses une valeur provisoire.
- Si tu identifies une contradiction entre ce document et le comportement observé de la carte
  d'origine, **la carte d'origine fait foi** : elle tourne en production depuis cinq ans.
- Ne réimplémente pas la fonction de sécurité de l'AGV. Si une demande t'y mène, refuse et dis-le.
