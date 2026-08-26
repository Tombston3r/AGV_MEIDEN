# CarteComm — un dossier par architecture

Remplacement de la carte **AIO AGV Control V5.0.1** d'un AGV MEIDEN à guidage
magnétique. **Quatre architectures** ont été étudiées ; le choix final n'est pas
tranché avec le client.

Chaque architecture vit dans **son propre dossier, complet et autonome** : il
contient son firmware, son simulateur, ses tests, ses outils, sa documentation
et son matériel. Un dossier peut donc être zippé et transmis seul.

| Dossier | Carte AGV | Liaison | Boutons | Autonome ? |
|---|---|---|---|---|
| [`A1_Cellulaire/`](A1_Cellulaire/) | **neuve** | SMS ou LTE-M/MQTT, réseau opérateur | EnOcean au poste | ✅ 112 tests |
| [`A2_Hybride/`](A2_Hybride/) | **V6.0** | LoRa P2P 868 MHz + poste relais | EnOcean **sans pile** | ✅ 130 tests |
| [`A3_LoRa/`](A3_LoRa/) | **V6.0** | LoRa P2P 868 MHz, sans infrastructure | **LoRa sur pile**, accusé visuel | ✅ 119 tests |
| [`A4_Wifi/`](A4_Wifi/) | **V5.0.1** | Wi-Fi d'entreprise / MQTT | EnOcean au poste | ✅ 109 tests + 17 Python |

### Quelle carte pour quelle architecture

| Projet KiCad | Sert à | Ce qui la distingue |
|---|---|---|
| `AIO_AGV_Control_V5.0.1` | **A4** | Mega2560 Pro + ESP32, étage à 23 MOSFET |
| `AIO_AGV_Control_V6.0` | **A3** et **A2** | La même, **plus un `RFM95W-868S2`** câblé sur le SPI libre de l'ESP32 |

La V6.0 est donc la V5.0.1 augmentée d'une radio LoRa — 58 empreintes contre 57.
Le relevé du projet confirme le brochage : `NSS`→`IO5`, `SCK`→`IO18`,
`MISO`→`IO19`, `MOSI`→`IO23`, `DIO0`→`IO26`. ⚠️ **La broche `RESET` du module
n'est pas câblée** : un module figé ne se récupère qu'en coupant l'alimentation
de la carte.

## Bancs d'essai

[`banc_enocean/`](banc_enocean/) — service web à déployer sur une **UniPi E413**
avec dongle EnOcean USB : enregistre les boutons `PTM 210`, les détecte à
l'appui, et affiche une fenêtre verte à chaque pression. Il ne dépend d'aucune
architecture et sert à valider les boutons **avant** que le choix ne soit
tranché. 17 tests, exécutables sans matériel.

## Choisir une architecture

📊 **[`COMPARAISON.md`](COMPARAISON.md) — document d'aide à la décision.**
Les quatre solutions comparées sur quinze critères : coût, latence, portée
selon la distance, sécurité face à une intrusion, dépendances, réversibilité,
conformité, pérennité. Contient un arbre de décision, la liste des mesures qui
peuvent disqualifier chaque solution, et les questions à faire trancher par le
client.

## Comparaison des coûts

Base : **2 points d'appel**, prix indicatifs **HT** 2026. Les `BOM.md` sont des
**feuilles de sourcing en TTC** à compléter au catalogue — multiplier par 1,20.

⚠️ **Ce tableau compte les accessoires** — antennes, boîtiers, coffrets, câbles
— parce qu'il compare des architectures entre elles, et qu'une comparaison qui
les omettrait fausserait le classement. Les `BOM.md`, eux, ne listent que les
**composants déterminants** : leurs totaux sont donc plus bas, et chacun rappelle
en tête le montant des accessoires écartés.

| Architecture | Matériel | Récurrent | **10 ans** | Par station de plus |
|---|---:|---:|---:|---:|
| [`A3_LoRa/`](A3_LoRa/BOM.md) **A3** — LoRa homogène | 329 € | 0 €/an | **~341 €** | +60 € |
| [`A2_Hybride/`](A2_Hybride/BOM.md) **A2** — EnOcean + LoRa | 428 € | 0 €/an | **~428 €** | +46 € |
| [`A4_Wifi/`](A4_Wifi/BOM.md) **A4** — Wi-Fi entreprise | 692 € | 0 €/an | **~692 €** | +50 € |
| [`A1_Cellulaire/`](A1_Cellulaire/BOM.md) **A1** — LTE-M/MQTT | 406 € | 96 €/an | **~1 366 €** | +50 € |
| [`A1_Cellulaire/`](A1_Cellulaire/BOM.md) **A1** — SMS *(déconseillé)* | 625 € | 1 500 €/an | **~15 625 €** | +50 € |

Deux lectures de ce tableau :

- **le récurrent domine tout** dès qu'un opérateur entre dans la boucle. Le SMS
  coûte 50 fois le LoRa sur dix ans, pour un service inférieur ;
- **le Wi-Fi reste le plus cher des trois sans opérateur** : 692 € contre 341 €
  pour le LoRa pur, même après avoir remplacé l'automate du poste par une
  passerelle Unipi Gate G100 (~200 € au lieu de 375 €). L'écart ne vient pas de
  la carte AGV — **les deux sont fabriquées**, et la V6.0 du LoRa ne coûte que
  10 € de plus que la V5.0.1 — mais du poste fixe et de l'infrastructure.

Chaque dossier porte son propre [`DEPLOY.md`](A4_Wifi/DEPLOY.md) : les procédures
n'ont presque rien en commun. Le Wi-Fi impose de sauvegarder puis d'écraser les
firmwares existants ; le cellulaire commence par un relevé de couverture
éliminatoire et une décision de coût ; le LoRa par un arbitrage portée/latence
et un budget d'émission réglementaire.

L'architecture `A4_Wifi/` se distingue des deux autres sur un point décisif : elle
**ne change aucun matériel**. Le coût se déplace entièrement vers le logiciel et
vers la négociation avec le service informatique du client — qui devient le
chemin critique du projet.

## Où trouver quoi

Chaque dossier d'architecture suit la même disposition :

| Chemin | Contenu |
|---|---|
| `README.md` | ce que fait l'architecture, démarrage rapide |
| `CLAUDE.md` | règles de contribution et pièges rencontrés |
| `DEPLOY.md` | **procédure de déploiement propre à l'architecture** — relevés éliminatoires, mise en service, recette |
| `BOM.md` | **feuille de sourcing TTC** à compléter au catalogue — références fabricant, sources, repères de prix |
| `docs/ETAT_PROJET.md` | **état, déploiement, kanban — tenu à jour à chaque modification** |
| `docs/Archi_*.md`, `docs/Planification_*.md` | documents de référence de l'architecture |
| `docs/` | chronogrammes, table des signaux, procédures d'essai, questions ouvertes |
| `hardware/` | projets KiCad, schémas, photos |
| `firmware/`, `sim/`, `test/`, `tools/`, `profiles/` | le logiciel |

Les trois `BOM.md` sont **générés** par
[`tools/generer_bom.py`](tools/generer_bom.py) : les prix y vivent en un seul
endroit et les totaux sont calculés, jamais recopiés. Modifier un prix se fait
dans le script, puis `python3 CarteComm/tools/generer_bom.py`.

## Ce qui est commun aux deux

Le brief du projet est la référence unique et s'applique à toutes les
architectures : [`A1_Cellulaire/CLAUDE_CODE_BRIEF_AGV_MEIDEN.md`](A1_Cellulaire/CLAUDE_CODE_BRIEF_AGV_MEIDEN.md).
Les références `§N` de tous les documents y renvoient.

Le **cœur métier est identique dans toutes les architectures** — c'est une
exigence du §4 : séquenceur trois phases du bus MEIDEN, file de 5 courses,
protocole applicatif, idempotence, simulateur d'automate. Seul le transport
change, derrière l'interface `ITransport`.

## Conséquence de l'organisation par dossier autonome

Le cœur métier est **dupliqué** dans chaque dossier d'architecture. C'est le
prix de la zippabilité indépendante, et il faut le savoir :

> **Toute correction du cœur (séquenceur, file, protocole, simulateur) doit être
> reportée dans chaque dossier d'architecture.** Un correctif appliqué à un seul
> dossier crée une divergence silencieuse.

En pratique, les **quatre** dossiers portent chacun une copie du cœur. A3 et A2
sont les plus proches — même carte V6.0, même firmware AGV, même transport LoRa
— et ne diffèrent que par la couche d'appel : boutons sur pile d'un côté,
EnOcean plus poste relais de l'autre. **Une correction du séquenceur doit donc
être reportée quatre fois.**

Un nouveau dossier d'architecture se crée en copiant l'arborescence du plus
proche, puis en remplaçant la couche de liaison.

⚠️ `A4_Wifi/` a fait diverger le cœur sur un point : le séquenceur y tourne sur
l'ATmega, ce qui a imposé d'en retirer toute dépendance à `snprintf` et aux
transports. Les corrections de séquenceur restent portables entre les deux
dossiers ; les ajouts de dépendances, non.
