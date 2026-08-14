# CarteComm — un dossier par architecture

Remplacement de la carte **AIO AGV Control V5.0.1** d'un AGV MEIDEN à guidage
magnétique. Trois architectures de liaison ont été étudiées ; le choix final
n'est pas tranché avec le client.

Chaque architecture vit dans **son propre dossier, complet et autonome** : il
contient son firmware, son simulateur, ses tests, ses outils, sa documentation
et son matériel. Un dossier peut donc être zippé et transmis seul.

| Dossier | Architecture | Matériel AGV | Statut | Autonome ? |
|---|---|---|---|---|
| [`Wifi/`](Wifi/) | Boutons EnOcean → poste UniPi → **Wi-Fi d'entreprise / MQTT** → carte V5.0.1 | **Carte conservée**, deux firmwares réécrits | Spécifiée en détail | ✅ oui — 101 tests C++ + 17 Python |
| [`SMS_EnOcean/`](SMS_EnOcean/) | Boutons EnOcean au poste + liaison cellulaire (SMS ou LTE-M/MQTT) | Nouvelle carte | Étudiée à la demande du client | ✅ oui — 112 tests |
| [`LoRa/`](LoRa/) | LoRa P2P 868 MHz | Nouvelle carte | Sources extraites, **pas encore un projet complet** | ❌ pas encore |

L'architecture `Wifi/` se distingue des deux autres sur un point décisif : elle
**ne change aucun matériel**. Le coût se déplace entièrement vers le logiciel et
vers la négociation avec le service informatique du client — qui devient le
chemin critique du projet.

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
