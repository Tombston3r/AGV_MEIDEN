# Confrontation de la spec au dépôt `Comm distance/`

La spec a été rédigée en amont ; le dépôt contient des relevés et du code qui
tranchent ou déplacent plusieurs de ses points. **À lire avant d'implémenter
la suite** — plusieurs paragraphes de la spec sont déjà faits, d'autres
reposent sur un matériel qui n'est pas celui relevé.

## Le séquenceur du §5 existe déjà — ne pas le réécrire

Le chaînon « mission → bus X/Y » décrit au §5 est **écrit et testé** dans
`Comm distance/architectures/A4_Wifi/` (copies A2/A3) :

| Exigence §5 | Réalisation existante |
|---|---|
| Pose de l'adresse sur les signaux X | `Sequencer` + `avr_port_bus` (ATmega) |
| Respect du `t_setup` | profil `profiles/*.yaml`, tests dédiés |
| Attente d'acquittement Y, timeout, retries | `Sequencer`, séquence trois phases |
| Refus d'empiler sans accusé | file de 5 courses + `CmdResult` |
| Heartbeat ESP32 ↔ ATmega | `link_protocol` + repli de sécurité côté ATmega |

Le moteur du planning est un **producteur de missions de plus** : il émet vers
la même liaison série que la passerelle radio (`Cmd::Goto`), rien d'autre.

## Le matériel de la spec n'est pas celui relevé

- **Pas de MCP23017 sur la V5.0.1.** Le relevé KiCad montre 43 lignes
  directement sur les ports de l'ATmega. L'analyse de conflit I²C
  (`0x20–0x27`) est sans objet : sur la V5.0.1, le bus I²C de l'ESP32
  (`IO21`/`IO22`, libres) ne porte **aucun** périphérique aujourd'hui — le
  DS3231 (`0x68`) y serait seul.
- **`t_setup`** : des valeurs de profil existent et sont testées en natif ; la
  **mesure physique** reste ouverte (kanban A4). La répartition ESP32/ATmega
  du §5 est déjà tranchée dans les architectures : séquenceur sur ATmega.
- **Stations** : 10 bits, `0–1023` (`kStationMax`, brief §5.1) — la spec dit
  « 1–1024 », s'aligner sur le code.

## Ce qui existe déjà côté ESP32

- **IHM modèle `agvdump`** : serveur web + AP de maintenance dans A4 ; le
  REST/WebSocket et l'ETag du §6 sont à ajouter dessus, pas à créer de zéro.
- **Horloge murale** : `EspClock::set_wall_clock()` existe (SNTP côté poste,
  contrôles de fraîcheur §8.1). L'état de confiance à trois niveaux (§2.3) est
  à construire par-dessus — le moteur le consomme déjà (`heure_fiable`).
- **Banc T-Beam** (`Comm distance/bancs/lora/`) : I²C actif, écran — support
  commode pour prototyper le DS3231 et la séquence de boot §2.4 sans toucher à
  la carte AGV.

## Cohérences à préserver

- La règle « organe de commande, pas de sécurité » (brief §3.1) reste
  inchangée ; le §9 de la spec l'étend : le déclenchement sur horloge impose
  le **sélecteur physique** (impact nomenclature) et l'authentification web.
- Décisions produit actées le 2026-08-27 : DST printemps → **exécution au
  premier instant existant** ; fenêtre de grâce par défaut → **5 min**.
