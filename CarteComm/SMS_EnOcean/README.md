# AGV MEIDEN — architecture SMS + EnOcean

Remplacement de la carte **AIO AGV Control V5.0.1** (ATmega2560 + ESP32, Wi-Fi
2,4 GHz) d'un AGV MEIDEN à guidage magnétique. Le 2,4 GHz du site étant saturé,
la liaison change ; la fonction, elle, reste : **la carte porte la mémoire de
mission** — jusqu'à 5 courses en file, l'AGV n'en connaissant qu'une à la fois.

**Ce dossier est une architecture complète et autonome** : boutons **EnOcean**
sans pile au poste fixe, liaison **cellulaire** vers l'AGV. Les autres
architectures vivent dans des dossiers frères — voir [`../README.md`](../README.md).

> ⚠️ **Organe de commande, pas organe de sécurité.** L'arrêt d'urgence, les
> bumpers et le scrutateur laser restent dans une chaîne indépendante conforme à
> l'ISO 3691-4.

## Démarrage rapide

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test
```

Aucun matériel n'est nécessaire : le simulateur d'automate MEIDEN (`sim/`)
rejoue les accusés `Y22`/`Y05`/`Y10`, la position sur `Y23`…`Y34`, les timeouts,
les défauts `Y03` et les rebonds, en temps simulé.

## Une seule logique métier, plusieurs transports

Le choix de liaison n'est pas tranché avec le client. Les transports sont
interchangeables **derrière une abstraction commune** (`ITransport`), et le
séquenceur du bus MEIDEN est rigoureusement identique quel que soit le
transport — c'est ce qui rend les architectures comparables à coût nul.

| Élément | Choix de ce dossier | Implémentation |
|---|---|---|
| Appel opérateur | **EnOcean PTM 210** (bouton sans pile) | `enocean/`, `poste_app` |
| Liaison poste → AGV | **LTE-M / MQTT** (recommandé) | `MqttLteTransport` |
| Variante de comparaison | **SMS** — ni latence bornée, ni ordre | `SmsTransport`, `poste-unipi/` |
| Alerte hors site | SMS bas volume | `AlertGateway` |

De même pour l'interface matérielle du bus (§12.10, non tranchée) : trois
implémentations de `IBusDriver` — 4× MCP23017 (I²C), 3× 74HC595 + 3× 74HC165
(SPI, pose simultanée, **variante préférable**), ou ATmega2560 conservé.

## État du projet

| Livrable | État |
|---|---|
| Simulateur d'automate + tests natifs | ✅ 112 tests, 373 assertions |
| Séquenceur trois phases (4 phases, tous les timeouts) | ✅ |
| File de 5 courses + persistance NVS + politique de validité | ✅ |
| Trame, CRC-16, AES-128-CTR, idempotence, anti-rejeu | ✅ |
| Trois variantes d'interface bus | ✅ |
| EnOcean ESP3 + déduplication + mode appairage | ✅ |
| SMS et MQTT/LTE-M | ✅ (SMS déconseillé en liaison principale, cf. `Archi_2`) |
| Passerelle d'alerte SMS bas volume | ✅ |
| Supervision web + `/agvdump` compatible | ✅ (format à recaler, §12.6) |
| Poste UniPi (Python + systemd) | ✅ (runtime à confirmer, §12.9) |
| Banc HIL, intégration AGV réel | ⏳ matériel requis |

**Aucun relevé matériel n'a encore été fait.** Les valeurs par défaut sont
marquées `PROVISOIRE §12.x` dans le code et listées dans
[`docs/questions_ouvertes.md`](docs/questions_ouvertes.md) — à lire avant toute
mise en service.

## Documentation

- **[`DEPLOY.md`](DEPLOY.md) — procédure de déploiement complète**, du relevé de
  couverture éliminatoire au procès-verbal de recette.
- **[`BOM.md`](BOM.md) — nomenclature complète**, matériel, outillage,
  récurrent et coût sur 10 ans.
- **[`docs/ETAT_PROJET.md`](docs/ETAT_PROJET.md) — état, déploiement, kanban.
  Point d'entrée, tenu à jour à chaque modification.**
- [`CLAUDE.md`](CLAUDE.md) — règles de contribution et pièges rencontrés
- [`docs/chronogrammes.md`](docs/chronogrammes.md) — les quatre phases, front par front
- [`docs/signal_map.md`](docs/signal_map.md) — bus X/Y et numérotation octale
- [`docs/procedures_essai.md`](docs/procedures_essai.md) — du poste de dev à l'AGV
- [`docs/protocole_mega.md`](docs/protocole_mega.md) — liaison inter-MCU
- [`docs/questions_ouvertes.md`](docs/questions_ouvertes.md) — **le §12, à faire trancher**

## Licence et propriété

Développement pour le client. Ne pas diffuser la clé AES ni les relevés de site.
