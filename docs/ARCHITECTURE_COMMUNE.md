# Le modèle commun aux trois architectures

Décidé le 2026-08-28. Toutes les architectures adoptent la même chaîne, et ne
se distinguent plus que par **la liaison entre le poste et l'AGV**.

```
   Boutons d'appel            Poste Central              Carte AGV Control        AGV
    (optionnels)         ┌──────────────────────┐         ┌─────────────┐
        ○  ────────────► │  API planning        │ ──────► │  Séquenceur │ ──►  bus
        ○  ────────────► │  Réception des appels│         │  X/Y        │      MEIDEN
                         │  Journal, validation │         └─────────────┘
                         └──────────────────────┘
        (optionnelle)         liaison poste vers AGV
```

## Ce que fait chaque maillon

| Maillon | Rôle | Peut-il manquer ? |
|---|---|---|
| **Boutons d'appel** | Demande immédiate d'un opérateur à son poste | **Oui.** Le planning seul suffit à faire rouler l'AGV |
| **Poste Central** | Héberge l'**API de planning**, reçoit les appels, décide des départs, tient le journal et la validation quotidienne | Non. C'est lui qui décide |
| **Carte AGV Control** | Traduit une mission en séquence sur le bus X/Y, porte le repli de sécurité | Non |
| **AGV** | Exécute | Non |

**Une seule liaison distingue les architectures** : celle du poste vers l'AGV.

| | Poste vers AGV | Dossier |
|---|---|---|
| **A1** Cellulaire | LTE-M / MQTT, réseau opérateur | [`A1_Cellulaire/`](../Comm%20distance/architectures/A1_Cellulaire/) |
| **A2** LoRa | LoRa 868 MHz, bande libre | [`A2_LoRa/`](../Comm%20distance/architectures/A2_LoRa/) |
| **A3** Wi-Fi | Wi-Fi d'entreprise / MQTT | [`A3_Wifi/`](../Comm%20distance/architectures/A3_Wifi/) |

## Les boutons d'appel sont optionnels

Le planning seul suffit à faire rouler l'AGV. Les boutons ne figurent donc plus
dans les nomenclatures : ils s'ajoutent si l'exploitation les demande, et le
choix de leur technologie est **indépendant** de celui de l'architecture.

| Technologie | Ce qu'elle apporte | Ce qu'elle coûte |
|---|---|---|
| **EnOcean** `PTM 210` | aucune pile, jamais | télégramme en clair, aucun accusé à l'opérateur, dongle USB au poste |
| **LoRa sur pile** | bouton **authentifié** AES, **accusé visuel** vert/rouge | pile `ER14505` tous les 5 à 8 ans, carte à fabriquer |

Le banc [`bancs/enocean/`](../Comm%20distance/bancs/enocean/) reste en place : il
valide les `PTM 210` le jour où l'option est retenue. Le firmware du bouton LoRa
vit dans `A2_LoRa/firmware/bouton-lora/`.

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
