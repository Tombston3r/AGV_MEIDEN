# AGV MEIDEN — remplacement de la carte de communication

Remplacement de la carte **AIO AGV Control V5.0.1** d'un chariot filoguidé
MEIDEN à guidage magnétique. Quatre architectures de liaison ont été étudiées ;
le choix final n'est pas tranché avec le client.

## Où est quoi

| Dossier | Contenu | Quand y aller |
|---|---|---|
| [`architectures/`](architectures/) | les **4 solutions complètes** — firmware, tests, nomenclature, déploiement | pour travailler sur une solution |
| [`bancs/`](bancs/) | les **bancs d'essai** — valider un composant ou une liaison sur table | pour vérifier que le matériel fait ce qu'on croit |
| [`materiel/`](materiel/) | les **projets KiCad** des cartes | pour lire ou modifier un schéma |
| [`docs/`](docs/) | ce qui est **transverse** : brief, comparatif | pour décider, ou comprendre le cadre |
| [`outils/`](outils/) | scripts partagés | pour régénérer les nomenclatures |

**Une règle de rangement, une seule** : *tout ce qui sert à une chose vit avec
elle.* Un banc porte son `README.md` et son `DEPLOY.md` ; une architecture porte
sa nomenclature, ses tests et sa procédure de mise en service. Rien à chercher
ailleurs.

## Les quatre architectures

| | Dossier | Carte AGV | Liaison | Boutons | Tests |
|---|---|---|---|---|---|
| **A1** | [`A1_Cellulaire/`](architectures/A1_Cellulaire/) | neuve | SMS ou LTE-M/MQTT, réseau opérateur | EnOcean au poste | 112 |
| **A2** | [`A2_Hybride/`](architectures/A2_Hybride/) | **V6.0** | LoRa 868 MHz + poste relais | EnOcean **sans pile** | 130 |
| **A3** | [`A3_LoRa/`](architectures/A3_LoRa/) | **V6.0** | LoRa 868 MHz direct | **LoRa sur pile**, accusé visuel | 119 |
| **A4** | [`A4_Wifi/`](architectures/A4_Wifi/) | V5.0.1 | Wi-Fi d'entreprise / MQTT | EnOcean au poste | 109 + 17 |

La numérotation suit l'**ordre chronologique d'étude**, pas un classement.

📊 **[`docs/COMPARAISON.md`](docs/COMPARAISON.md)** compare les quatre sur quinze
critères et contient l'arbre de décision. C'est le document à ouvrir pour
trancher.

## Les bancs d'essai

| Banc | Ce qu'il prouve | Matériel |
|---|---|---|
| [`bancs/enocean/`](bancs/enocean/) | Les boutons `PTM 210` émettent, sont identifiés et dédupliqués | UniPi E413 + dongle EnOcean USB |
| [`bancs/lora/`](bancs/lora/) | Deux extrémités LoRa se parlent **et se comprennent** | ESP32 + RFM95W, ou Linux + SPI |

Les deux tournent **sans matériel** en mode simulation : c'est ce qui permet de
les éprouver avant d'aller en atelier.

## Le matériel

| Projet KiCad | Sert à | Ce qui la distingue |
|---|---|---|
| [`AIO_AGV_Control_V5.0.1`](materiel/AIO_AGV_Control_V5.0.1/) | **A4** | Mega2560 Pro + ESP32, étage à 23 MOSFET — 57 empreintes |
| [`AIO_AGV_Control_V6.0`](materiel/AIO_AGV_Control_V6.0/) | **A2** et **A3** | La même **plus un `RFM95W-868S2`** et son embase d'antenne — 59 empreintes |

Le diff des deux projets ne montre aucun autre écart : la V6.0 est la V5.0.1
augmentée d'une radio LoRa intégrée. Brochage relevé : `NSS`→`IO5`,
`SCK`→`IO18`, `MISO`→`IO19`, `MOSI`→`IO23`, `DIO0`→`IO26`.

⚠️ **`RESET` n'est pas câblée** sur la V6.0 : aucun reset logiciel n'est
possible. `IO27` est libre et conviendrait.

## Démarrer

```bash
# Travailler sur une architecture
cd architectures/A3_LoRa && make test

# Éprouver un banc sans matériel
cd bancs/enocean && python3 -m banc_enocean --simulation

# Régénérer les quatre nomenclatures après une mise à jour de prix
python3 outils/generer_bom.py
```

Chaque dossier d'architecture et chaque banc suit **la même disposition** :

| Fichier | Rôle |
|---|---|
| `README.md` | à quoi ça sert, comment le lancer, **ce qu'on doit obtenir** |
| `DEPLOY.md` | mise en service pas à pas, relevés éliminatoires, recette |
| `docs/ETAT_PROJET.md` | état, kanban — **tenu à jour à chaque modification** |
| `BOM.md` | nomenclature chiffrée *(architectures seulement)* |

## Ce qui est commun

[`docs/BRIEF.md`](docs/BRIEF.md) est la **référence unique** du projet. Toutes
les références `§N` du dépôt y renvoient.

Le **cœur métier est identique dans les quatre architectures** — séquenceur
trois phases du bus MEIDEN, file de 5 courses, protocole applicatif,
idempotence, simulateur d'automate. Seul le transport change, derrière
l'interface `ITransport`.

⚠️ Ce cœur est donc **dupliqué quatre fois**, pour que chaque architecture reste
livrable seule. **Toute correction du cœur doit être reportée dans les quatre
dossiers** ; un correctif appliqué à un seul crée une divergence silencieuse.
[`../docs/ORGANISATION.md`](../docs/ORGANISATION.md) explique ce compromis et où poser
une nouveauté.
