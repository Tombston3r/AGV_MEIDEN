# Architecture LoRa P2P 868 MHz — sources extraites

> **Ce dossier n'est pas encore un projet autonome.** Il contient les sources
> spécifiques à LoRa, retirées du dossier `SMS_EnOcean/` lors du passage à une
> organisation par architecture. Il leur manque le cœur métier (séquenceur,
> file de courses, protocole, simulateur) pour se construire seules.
>
> Elles ne sont pas pour autant du code mort : **les 10 tests ont été rejoués
> après extraction et passent** (68 assertions), compilés contre le cœur du
> dossier voisin :
>
> ```bash
> cd CarteComm
> g++ -std=c++17 -Wall -Wextra -Werror -O1 \
>     -I LoRa/firmware/common -I SMS_EnOcean/firmware/common -I SMS_EnOcean/test/native \
>     SMS_EnOcean/test/native/test_main.cpp LoRa/test/native/test_lora.cpp \
>     LoRa/firmware/common/transport/*.cpp \
>     SMS_EnOcean/firmware/common/proto/*.cpp \
>     SMS_EnOcean/firmware/common/config/hardware_profile.cpp -o /tmp/lora_tests
> /tmp/lora_tests
> ```

## Déploiement

[`DEPLOY.md`](DEPLOY.md) donne la procédure complète, à commencer par la phase 0
qui fait de ce dossier un projet autonome.

## Contenu

| Chemin | Rôle |
|---|---|
| `firmware/common/transport/lora_transport.{h,cpp}` | Ordonnanceur **half-duplex** : fenêtre d'écoute d'ACK après chaque émission, retransmissions bornées |
| `firmware/common/transport/lora_radio.h` | Port matériel `ILoraRadio`, pour rester testable en natif |
| `firmware/common/config/lora_config.h` | `LoraConfig` — les paramètres radio vivent ici, pas dans le profil du cœur |
| `profiles/lora_fragment.yaml` | Section `lora:` à réintégrer dans `profiles/*.yaml` |
| `firmware/common/transport/duty_cycle.{h,cpp}` | **Budget de rapport cyclique 1 %/1 h** (EN 300 220 / ERC 70-03) + calcul du temps d'antenne |
| `firmware/common/platform/esp32/sx1276_radio.{h,cpp}` | Pilote RFM95W / SX1276 au registre |
| `firmware/bouton-lora/src/main.cpp` | Nœud bouton sur pile : sommeil profond, 3 tentatives, LED verte/rouge |
| `test/native/test_lora.cpp` | 10 tests : temps d'antenne, budget légal, half-duplex, retransmissions |
| `DEPLOY.md` | Procédure de déploiement, A1 et A3 |
| `docs/latence_lora.md` | **Écart latence/SF à arbitrer avec le client** |
| `docs/Archi_1_LoRa_P2P_homogene.md` | Document de référence de l'architecture 1 : LoRa P2P homogène, boutons sur pile |
| `docs/Archi_3_Hybride_EnOcean_LoRa.md` | Document de référence de l'architecture 3 : hybride EnOcean + LoRa, **architecture retenue** |
| `hardware/schema_detail_voies.svg` | Schémas détaillés de la carte LoRa : chaîne d'alimentation, étages d'E/S |

## Pour en faire un projet complet

```bash
cp -rn ../SMS_EnOcean/{firmware,sim,test,tools,web,profiles,docs,Makefile,platformio.ini} .
cat profiles/lora_fragment.yaml >> profiles/default.yaml
# puis :
#  - retirer firmware/common/transport/{sms_transport,mqtt_lte_transport,at_engine}.*
#    et firmware/common/app/alert_gateway.* si l'alerte hors site n'est pas retenue
#  - rebrancher les deux main.cpp sur LoraTransport + Sx1276Radio
#    (LoraTransport prend désormais sa LoraConfig en second argument)
#  - ajouter l'environnement [env:bouton] dans platformio.ini (framework arduino)
make test
```

`LoraConfig` est volontairement séparé de `HardwareProfile` : le cœur métier est
partagé entre architectures et n'a aucune raison de transporter un facteur
d'étalement. C'est ce qui rend cette greffe indolore.

## Points à ne pas perdre de vue

- Le **budget de rapport cyclique est une obligation réglementaire**, pas une
  option : il doit refuser l'émission au-delà de 1 % sur 1 h glissante et
  remonter le refus en défaut applicatif visible.
- Le **sync word doit différer de 0x34**, réservé LoRaWAN.
- Le retour visuel du bouton sur pile (LED verte = ACK reçu) est **absent de la
  solution EnOcean pure** : c'est un argument à conserver dans la comparaison.
- L'arbitrage SF7 / SF9 conditionne à la fois la latence et le nombre
  d'émissions possibles par heure — voir [`docs/latence_lora.md`](docs/latence_lora.md).
