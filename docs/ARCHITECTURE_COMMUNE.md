# Le modèle commun aux quatre architectures

Décidé le 2026-08-28. Toutes les architectures adoptent la même chaîne, et ne
se distinguent plus que par la **technologie de chaque maillon**.

```
   Boutons d'appel            Poste Central              Carte AGV Control        AGV
    (optionnels)         ┌──────────────────────┐         ┌─────────────┐
        ○  ────────────► │  API planning        │ ──────► │  Séquenceur │ ──►  bus
        ○  ────────────► │  Réception des appels│         │  X/Y        │      MEIDEN
                         │  Journal, validation │         └─────────────┘
                         └──────────────────────┘
         liaison A                  liaison B
```

## Ce que fait chaque maillon

| Maillon | Rôle | Peut-il manquer ? |
|---|---|---|
| **Boutons d'appel** | Demande immédiate d'un opérateur à son poste | **Oui.** Le planning seul suffit à faire rouler l'AGV |
| **Poste Central** | Héberge l'**API de planning**, reçoit les appels, décide des départs, tient le journal et la validation quotidienne | Non. C'est lui qui décide |
| **Carte AGV Control** | Traduit une mission en séquence sur le bus X/Y, porte le repli de sécurité | Non |
| **AGV** | Exécute | Non |

Deux liaisons distinctes, et c'est là que les architectures diffèrent :

| | Liaison A, bouton vers poste | Liaison B, poste vers AGV |
|---|---|---|
| **A1** Cellulaire | EnOcean 868 MHz | LTE-M / MQTT (réseau opérateur) |
| **A2** Hybride | EnOcean 868 MHz | LoRa 868 MHz |
| **A3** LoRa | **LoRa 868 MHz** (boutons sur pile) | LoRa 868 MHz |
| **A4** Wi-Fi | EnOcean 868 MHz | Wi-Fi d'entreprise / MQTT |

## Ce que ce modèle change

### Le planning quitte la carte AGV

Il vivait sur l'ESP32 embarqué ; il vit désormais sur le **poste central**, sous
Linux. La carte AGV redevient ce qu'elle doit être : un **exécutant**, qui reçoit
des missions et pilote le bus.

Le gain est considérable et il est détaillé dans
[`../Timer/docs/CHEMIN_POSTE.md`](../Timer/docs/CHEMIN_POSTE.md) : plus de
DS3231 à ajouter, plus de contrainte de stockage NVS, plus de serveur web
embarqué à rendre permanent. Le banc de développement **devient** la cible.

### Le poste central doit tourner sous Linux

Héberger une API web, un planning persistant, une horloge fiable et un journal
sur un microcontrôleur est possible, mais coûteux en logiciel et fragile. Sur
Linux, tout cela existe déjà.

⚠️ **Conséquence de coût** : les postes à base d'ESP32 des architectures A1 et
A2 laissent la place à une machine Linux. Voir les nomenclatures.

### A3 gagne un poste, et perd son argument principal

A3 se définissait par l'absence d'infrastructure : les boutons parlaient
**directement** à l'AGV. Ce n'est plus le cas.

Ce qu'elle conserve : des boutons **authentifiés** (AES de bout en bout jusqu'au
poste) et un **accusé visuel** à l'opérateur, que les boutons EnOcean ne savent
pas rendre. Ce qu'elle perd : le coût le plus bas et l'indépendance totale.

⚠️ **A2 et A3 deviennent très proches** : même poste, même liaison B. Leur seule
différence est désormais la technologie du bouton, pile contre sans-pile. La
question de les garder toutes les deux mérite d'être posée au client, elle est
ouverte dans [`COMPARAISON.md`](../Comm%20distance/docs/COMPARAISON.md).

## Ce qui ne change pas

- La carte AGV reste un **organe de commande, pas de sécurité**. La chaîne
  d'arrêt d'urgence, les bumpers et le scrutateur laser restent indépendants ;
- **perte de liaison, arrêt sûr au point d'arrêt suivant** : le repli vit sur
  l'ATmega, il ne dépend d'aucun poste ;
- le **séquenceur X/Y**, le protocole de liaison, l'idempotence et le format
  `/agvdump` sont inchangés.

Le poste central décide **quand** partir. Il ne décide jamais **comment** rouler.
