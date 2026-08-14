# CarteComm — un dossier par architecture

Remplacement de la carte **AIO AGV Control V5.0.1** d'un AGV MEIDEN à guidage
magnétique. Trois architectures de liaison ont été étudiées ; le choix final
n'est pas tranché avec le client.

Chaque architecture vit dans **son propre dossier, complet et autonome** : il
contient son firmware, son simulateur, ses tests, ses outils, sa documentation
et son matériel. Un dossier peut donc être zippé et transmis seul.

| Dossier | Architecture | Statut | Autonome ? |
|---|---|---|---|
| [`SMS_EnOcean/`](SMS_EnOcean/) | Boutons EnOcean sans pile au poste + liaison cellulaire (SMS ou LTE-M/MQTT) vers l'AGV | Étudiée à la demande du client | ✅ oui — 112 tests verts |
| [`LoRa/`](LoRa/) | LoRa P2P 868 MHz (A1 / A3) | Sources extraites, **pas encore un projet complet** | ❌ pas encore |

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

En pratique, le dossier `SMS_EnOcean/` fait aujourd'hui référence : c'est lui
qui contient le cœur à jour et les 112 tests. Un nouveau dossier d'architecture
se crée en copiant son arborescence puis en remplaçant la couche transport.
