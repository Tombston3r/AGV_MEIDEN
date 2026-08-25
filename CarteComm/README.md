# CarteComm — un dossier par architecture

Remplacement de la carte **AIO AGV Control V5.0.1** d'un AGV MEIDEN à guidage
magnétique. Trois architectures de liaison ont été étudiées ; le choix final
n'est pas tranché avec le client.

Chaque architecture vit dans **son propre dossier, complet et autonome** : il
contient son firmware, son simulateur, ses tests, ses outils, sa documentation
et son matériel. Un dossier peut donc être zippé et transmis seul.

| Dossier | Code | Architecture | Matériel AGV | Statut | Autonome ? |
|---|:-:|---|---|---|---|
| [`Wifi/`](Wifi/) | **A4** | Boutons EnOcean → poste UniPi → **Wi-Fi d'entreprise / MQTT** → carte V5.0.1 | **Carte conservée**, deux firmwares réécrits | Spécifiée en détail | ✅ oui — 101 tests C++ + 17 Python |
| [`SMS_EnOcean/`](SMS_EnOcean/) | **A2** | Boutons EnOcean au poste + liaison cellulaire (SMS ou LTE-M/MQTT) | Nouvelle carte | Étudiée à la demande du client | ✅ oui — 112 tests |
| [`LoRa/`](LoRa/) | **A1**<br>**A3** | LoRa P2P 868 MHz, seul (A1) ou hybride EnOcean (A3) | **Carte V5.0.1 réutilisée** | Sources extraites, **pas encore un projet complet** | ❌ pas encore |

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
| [`LoRa/`](LoRa/BOM.md) **A1** — LoRa homogène | 208 € | 0 €/an | **~220 €** | +60 € |
| [`LoRa/`](LoRa/BOM.md) **A3** — EnOcean + LoRa | 307 € | 0 €/an | **~307 €** | +46 € |
| [`Wifi/`](Wifi/BOM.md) **A4** — Wi-Fi entreprise | 692 € | 0 €/an | **~692 €** | +50 € |
| [`SMS_EnOcean/`](SMS_EnOcean/BOM.md) **A2** — LTE-M/MQTT | 406 € | 96 €/an | **~1 366 €** | +50 € |
| [`SMS_EnOcean/`](SMS_EnOcean/BOM.md) **A2** — SMS *(déconseillé)* | 625 € | 1 500 €/an | **~15 625 €** | +50 € |

Deux lectures de ce tableau :

- **le récurrent domine tout** dès qu'un opérateur entre dans la boucle. Le SMS
  coûte 50 fois le LoRa sur dix ans, pour un service inférieur ;
- **le Wi-Fi reste plus cher que le LoRa**, même après avoir remplacé l'automate
  du poste par une passerelle Unipi Gate G100 (~200 € au lieu de 375 €) : 692 €
  contre 208 €. Sa carte AGV est **fabriquée**, pas réutilisée — 155 € TTC de
  composants d'après le projet KiCad.

Chaque dossier porte son propre [`DEPLOY.md`](Wifi/DEPLOY.md) : les procédures
n'ont presque rien en commun. Le Wi-Fi impose de sauvegarder puis d'écraser les
firmwares existants ; le cellulaire commence par un relevé de couverture
éliminatoire et une décision de coût ; le LoRa par un arbitrage portée/latence
et un budget d'émission réglementaire.

L'architecture `Wifi/` se distingue des deux autres sur un point décisif : elle
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
architectures : [`SMS_EnOcean/CLAUDE_CODE_BRIEF_AGV_MEIDEN.md`](SMS_EnOcean/CLAUDE_CODE_BRIEF_AGV_MEIDEN.md).
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

En pratique, `SMS_EnOcean/` et `Wifi/` portent chacun une copie à jour du cœur.
Un nouveau dossier d'architecture se crée en copiant l'arborescence de l'un
d'eux, puis en remplaçant la couche de liaison.

⚠️ `Wifi/` a fait diverger le cœur sur un point : le séquenceur y tourne sur
l'ATmega, ce qui a imposé d'en retirer toute dépendance à `snprintf` et aux
transports. Les corrections de séquenceur restent portables entre les deux
dossiers ; les ajouts de dépendances, non.
