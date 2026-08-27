# AGV MEIDEN — architecture Wi-Fi (carte V5.0.1 conservée)

**Le matériel ne change pas. Les deux firmwares sont réécrits.**

La carte AIO AGV Control V5.0.1 (ESP32 + ATmega2560) est conservée telle quelle.
Son firmware d'origine n'étant disponible ni en sources ni en binaire garanti,
les deux programmes sont réécrits à partir du comportement documenté du bus
MEIDEN.

> ⚠️ **Organe de commande, pas organe de sécurité.** L'arrêt d'urgence, les
> bumpers et le scrutateur laser restent dans une chaîne indépendante conforme
> à l'ISO 3691-4.

> **Matériel** : le projet KiCad vit dans [`../../materiel/AIO_AGV_Control_V5.0.1/`](../../materiel/AIO_AGV_Control_V5.0.1/) — une seule copie pour tout le dépôt.
> **Bancs** : [`../../bancs/enocean/`](../../bancs/enocean/) valide la liaison de cette architecture sur table.

## Démarrage rapide

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test        # 101 tests, 389 assertions — aucun matériel requis
```

## Répartition des rôles

| | ATmega2560 | ESP32 |
|---|---|---|
| Porte | séquenceur X/Y, file de 5 courses, décodage position | Wi-Fi STA, MQTT, heartbeat, `/agvdump` |
| Ne porte pas | rien de réseau | **jamais le bus MEIDEN** |

Le séquenceur est sur l'ATmega **délibérément** : chaque maillon réseau (Wi-Fi,
DHCP, handover, TLS, broker, poste fixe) peut tomber pour des raisons hors du
projet. La commande de l'AGV ne doit dépendre d'aucun d'eux.

**Repli de sécurité** : sans heartbeat de l'ESP32 pendant 2 s, l'ATmega termine
la course engagée jusqu'au point d'arrêt suivant et refuse toute nouvelle
course. Ce mécanisme ne dépend ni du Wi-Fi, ni du réseau d'entreprise, ni du
poste fixe.

## Chaîne complète

```
Bouton EnOcean PTM 210  ─868 MHz─▶  Poste UniPi (TCM 515 + Mosquitto)
                                            │ MQTT/TLS
                                            ▼
                                    Wi-Fi d'entreprise
                                            │
                                            ▼
                        ESP32 ──UART+CRC──▶ ATmega2560 ──▶ bus X/Y ──▶ automate
```

## Documentation

- **[`DEPLOY.md`](DEPLOY.md) — procédure de déploiement complète**, de la
  première mesure au procès-verbal de recette. À dérouler dans l'ordre.
- **[`BOM.md`](BOM.md) — nomenclature complète**, matériel, outillage,
  récurrent et coût sur 10 ans.
- **[`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md) — état, déploiement, kanban.
  Point d'entrée, tenu à jour à chaque modification.**
- [`docs/Planification_Architecture_WiFi_AGV.md`](docs/Planification_Architecture_WiFi_AGV.md) — la spécification suivie
- [`docs/carte_v5_architecture.md`](docs/carte_v5_architecture.md) — la carte, et pourquoi la mission est sur l'ATmega
- [`docs/subd25_atmega.md`](docs/subd25_atmega.md) — **brochage SUB-D : relevé obligatoire avant tout branchement**
- [`docs/chronogrammes.md`](docs/chronogrammes.md) — les quatre phases du séquenceur
- [`docs/signal_map.md`](docs/signal_map.md) — bus X/Y et numérotation octale
- [`docs/procedures_essai.md`](docs/procedures_essai.md) — du poste de dev à l'AGV
- [`docs/questions_ouvertes.md`](docs/questions_ouvertes.md) — **ce qui doit être tranché**

## Avertissement sur la bande 2,4 GHz

Le projet est né de la **saturation du 2,4 GHz** du site. Cette architecture y
reste : l'ESP32-WROOM-32E ne fait pas de 5 GHz. Elle supprime l'émission
permanente en point d'accès et permet au client d'améliorer sa couverture, mais
ce n'est pas une réponse complète. L'arbitrage (bi-bande, bridge industriel, ou
bande ISM) est la tâche 0.7 de la planification — à trancher avant l'engagement.
