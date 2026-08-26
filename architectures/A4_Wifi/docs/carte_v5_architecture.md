# La carte AIO AGV Control V5.0.1 — architecture et répartition des rôles

> Cette architecture **conserve la carte** et **réécrit les deux firmwares**.
> Le firmware d'origine n'est disponible ni en sources ni en binaire garanti
> (planification 0.5 : tentative de lecture par `esptool read_flash` et
> `avrdude` via l'ICSP, résultat inconnu). Tout ce qui suit est donc écrit à
> partir du comportement documenté du bus MEIDEN, **pas** d'une
> rétro-ingénierie du binaire existant.

## Vue d'ensemble

```
                    Réseau Wi-Fi d'entreprise (VLAN OT)
                                  │
                                  │ MQTT/TLS
┌─────────────────────────────────┼─────────────────────────────────┐
│  CARTE AIO AGV CONTROL V5.0.1   │            (inchangée)          │
│                                 ▼                                 │
│  ┌───────────────────────────────────────┐                        │
│  │ ESP32-WROOM-32E                       │                        │
│  │  • client Wi-Fi STA (plus un AP !)    │                        │
│  │  • client MQTT : state / cmd / ack    │                        │
│  │  • heartbeat vers l'ATmega            │                        │
│  │  • AP de maintenance à la demande     │                        │
│  │  • NE TOUCHE JAMAIS AU BUS MEIDEN     │                        │
│  └───────────────┬───────────────────────┘                        │
│                  │ SoftwareSerial D52/D53, 38 400 bd, CRC-16      │
│                  │ + heartbeat applicatif toutes les 500 ms       │
│  ┌───────────────▼───────────────────────┐                        │
│  │ ATmega2560                            │                        │
│  │  • séquenceur trois phases X/Y        │                        │
│  │  • file de 5 courses                  │                        │
│  │  • décodage position 10 bits          │                        │
│  │  • REPLI DE SÉCURITÉ sur perte du     │                        │
│  │    heartbeat (2 s)                    │                        │
│  └───────────────┬───────────────────────┘                        │
│                  │ 43 lignes, 2 nappes SUB-D 25 (niveaux ⚠ §12.1) │
│  ┌───────────────▼───────────────────────┐                        │
│  │ SUB-D 25 × 2  →  automate MEIDEN      │                        │
│  └───────────────────────────────────────┘                        │
└───────────────────────────────────────────────────────────────────┘
```

## Pourquoi la mission est sur l'ATmega et pas sur l'ESP32

C'est la décision structurante de cette architecture, et elle mérite d'être
explicite.

L'ESP32 est le composant qui dépend du réseau d'entreprise : association Wi-Fi,
DHCP, handover entre points d'accès, TLS, broker MQTT, poste fixe. Chacun de ces
maillons peut tomber pour des raisons totalement hors du projet — une
maintenance IT non notifiée suffit.

Si le séquenceur vivait sur l'ESP32, chacune de ces pannes deviendrait une panne
de commande de l'AGV. En plaçant le séquenceur et la file sur l'ATmega :

- l'AGV termine toujours la course engagée, quoi qu'il arrive au réseau ;
- le comportement en perte de liaison est décidé par un microcontrôleur qui
  n'a ni pile TCP/IP ni horloge réseau — donc très peu de modes de panne ;
- l'ATmega garde une pose de bus très rapide : le relevé de câblage montre que
  les 22 sorties occupent 5 ports, donc **5 écritures ≈ 0,3 µs** en section
  critique. Ce n'est pas la simultanéité stricte d'un `PORTx = valeur` — le
  câblage ne le permet pas, les champs ne sont pas alignés sur les ports — mais
  c'est 500 fois plus rapide qu'un expandeur I²C et 800 fois sous le `t_setup`
  attendu. Détail dans [`subd25_atmega.md`](subd25_atmega.md).

## Le heartbeat — le seul mécanisme qui ne dépend de rien

L'ESP32 émet un message `Heartbeat` toutes les `heartbeat.period_ms` (500 ms).
Si l'ATmega n'en reçoit plus pendant `heartbeat.timeout_ms` (2 s), il :

1. laisse la course en cours aller **jusqu'au point d'arrêt suivant** ;
2. refuse toute nouvelle course, en répondant explicitement `SafeStopActive` ;
3. le signale dans son état (`safe_stop`), donc dans MQTT et dans l'IHM.

Deux choix de conception méritent d'être notés :

- **Le heartbeat continue d'être émis quand le Wi-Fi tombe.** La carte va très
  bien ; c'est le réseau qui est absent. Couper le heartbeat immobiliserait
  l'AGV pour rien.
- **Le retour du heartbeat ne relance rien tout seul.** Il rouvre seulement la
  possibilité d'accepter de nouvelles courses. Un AGV qui redémarrerait
  spontanément après une coupure réseau, alors qu'un opérateur s'en est
  approché, serait le pire comportement possible.

Le seuil de 2 s est un **paramètre** (`profiles/*.yaml`), avec une marge d'au
moins trois battements : rater un heartbeat isolé ne doit pas immobiliser l'AGV.

## Ce qui change par rapport à la carte d'origine

| | V5.0.1 d'origine | Ce firmware |
|---|---|---|
| Rôle Wi-Fi de l'ESP32 | Point d'accès `agv_atelier` permanent | **Client** du réseau d'entreprise |
| Source des ordres | Application mobile « AIO AGV Remote » | Boutons EnOcean via le poste fixe, en MQTT |
| Diagnostic | `/agvdump` sur 192.168.4.1 en permanence | `/agvdump` pendant une fenêtre de 10 min sur contact ILS |
| Émission 2,4 GHz | Permanente | Client seulement ; AP à la demande |
| Répartition ESP32/ATmega | Inconnue | Documentée ici, et testée |
| Comportement en perte de liaison | Inconnu | Arrêt au point d'arrêt suivant, observable |
| Matériel | — | **Inchangé** |

## Ce qui reste inconnu sur cette carte

Ces points conditionnent le premier flash et sont listés dans
[`questions_ouvertes.md`](questions_ouvertes.md) :

- ~~le câblage ATmega ↔ SUB-D 25~~ — **RELEVÉ**, voir
  [`subd25_atmega.md`](subd25_atmega.md). Reste à contrôler au multimètre
  qu'aucune nappe n'est sertie à l'envers (mode découverte) ;
- ~~les niveaux du bus~~ — **vérifiés par le client le 2026-08-21** : la
  connexion directe des lignes Y est confirmée compatible (W1b clos).
  Historique : le L7806CV est l'alimentation de l'ATmega
  (24 V de CN64 A6/B6 abaissés à 6 V), il ne renseigne donc pas sur les
  signaux. L'amplitude des lignes Y (§12.1) et la topologie des entrées de
  l'automate restent inconnues — voir
  [`subd25_atmega.md`](subd25_atmega.md) ;
- ⚠️ **la tension V_CC réelle de l'ATmega** : si le 6 V l'alimente directement,
  c'est au niveau du **maximum absolu** du datasheet (6,0 V) et hors plage
  recommandée. Mesure de trente secondes, à faire ;
- ~~l'UART qui relie l'ESP32 à l'ATmega~~ — **RELEVÉ au KiCad, et ce n'en est
  pas un** : `ESP32 IO17` → `MEGA D52` en direct, `MEGA D53` → pont 2,2 k/4,7 k
  → `ESP32 IO16`. Les trois UART matériels du MEGA sont inutilisables, leurs
  broches de réception portant `Y13`, `Y11` et `Y05`. D'où `SoftwareSerial` sur
  D52/D53 à 38 400 bauds ;
- **l'existence d'une ligne de heartbeat matérielle dédiée** entre les deux
  microcontrôleurs : si le relevé en révèle une, la surveiller **en plus** de la
  trame série détecterait un ESP32 bloqué qui continuerait d'émettre par DMA ;
- **la présence et le brochage d'un connecteur ICSP** pour flasher l'ATmega ;
- **l'existence d'une mesure de tension batterie** accessible à l'ESP32.

## Bande 2,4 GHz — la question qui a lancé le projet

Le remplacement de la carte a été motivé par la **saturation du 2,4 GHz** du
site. Cette architecture reste en 2,4 GHz : l'ESP32-WROOM-32E ne fait pas de
5 GHz. Ce qui change tout de même :

- l'AGV n'émet plus en permanence en point d'accès ;
- il partage l'infrastructure gérée du client au lieu de créer son propre
  réseau ;
- la couverture peut être améliorée par le client (ajout d'AP), ce qui était
  impossible avec un AP embarqué.

Ce n'est pas une réponse complète au problème d'origine. L'arbitrage
(ESP32-C5 bi-bande, client bridge industriel, ou repli sur une bande ISM) est la
tâche 0.7 de la planification, et il doit être tranché **avant** l'engagement.
Mesurer l'occupation de la bande avant et après est la seule façon d'objectiver
le gain.
