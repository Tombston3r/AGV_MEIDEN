# Procédures d'essai

## 0. Ordre imposé

Aucun branchement sur l'AGV réel avant d'avoir passé les étapes 1 à 3. L'accès à
l'AGV est compté ; le simulateur existe pour ça.

## 1. Tests natifs (poste de développement, aucun matériel)

```bash
python3 tools/genconfig.py profiles/default.yaml \
        firmware/common/config/generated_profile.h
make test                    # 101 tests, 389 assertions
make test FILTER=point_12    # uniquement les tests des points ouverts §12
make test FILTER=heartbeat   # le repli de sécurité de cette architecture
```

Critère de passage : `0 échecs`. Le build est en `-Wall -Wextra -Werror` :
un avertissement est un échec.

Côté poste UniPi :

```bash
cd poste-unipi
python3 -m pytest tests -q      # 17 tests : ESP3, déduplication, appairage, MQTT
ruff check . && mypy --strict agv_poste
```

## 2. Essais contre le simulateur (profils de timings)

Le simulateur rejoue un automate rapide et un automate lent
(`TimingProfile::fast()` / `slow()`), avec injection de timeouts, de défauts
`Y03` et de rebonds. Un profil YAML plat peut être chargé
(`TimingProfile::load`) pour rejouer un automate mesuré sur site.

Vérifications attendues :

- les 4 phases aboutissent sur les deux profils, **après réglage des timeouts** ;
- l'automate lent échoue avec les timeouts par défaut : c'est le résultat
  attendu, et la raison pour laquelle le §12.5 doit être relevé ;
- perte de liaison → arrêt au point d'arrêt suivant, file suivante non lancée ;
- coupure d'alimentation → file restaurée depuis la NVS, courses périmées
  écartées.

## 3. Relevé du brochage SUB-D : AVANT TOUT BRANCHEMENT

**Prérequis absolu de cette architecture.** Le câblage entre l'ATmega et les
SUB-D 25 n'est pas documenté ; un mot d'adresse mal câblé n'échoue pas, il
envoie l'AGV à la mauvaise station.

Procédure complète : [`subd25_atmega.md`](subd25_atmega.md), mode découverte du
firmware MEGA, **automate débranché**.

## 4. Banc HIL (avant tout branchement sur l'AGV)

Matériel : un second ESP32 ou un Arduino MEGA présentant les 43 lignes en face
de la carte, jouant le rôle de l'automate.

1. Relever à l'oscilloscope le front `X93` par rapport au dernier front
   d'adresse → **c'est la mesure de `t_setup_us` (§12.4)**.
2. Vérifier la simultanéité de la pose des 22 lignes :
   - variante 74HC595 : un seul front `RCLK`, écart attendu < 1 µs ;
   - variante MCP23017 : mesurer le décalage GPIOA/GPIOB, comparer à
     `mcp_ab_skew_us` et **vérifier qu'il reste très inférieur à `t_setup_us`** ;
   - variante MEGA : écart attendu < 1 µs.
3. Injecter des timeouts `Y22`, `Y05`, `Y10` et vérifier :
   `write_tries`, `start_tries`, `stop_tries` et les `*_op_return` dans
   `/agvdump`.
4. **Débrancher l'ESP32 de la liaison série** en pleine course et vérifier :
   l'AGV termine sa course, s'arrête à la station atteinte, refuse toute
   nouvelle commande, et `safe_stop` remonte dans l'état. C'est LE test de
   cette architecture.
5. Rebrancher : vérifier que l'AGV **ne repart pas tout seul**, et qu'une
   nouvelle commande est de nouveau acceptée.
6. Couper l'alimentation en cours de course, redémarrer : la file est en RAM
   (planification §2.3), elle est donc perdue : vérifier que l'AGV redémarre en
   `safe_stop` et n'exécute aucune course résiduelle.

## 4. Relevés sur la V5.0.1 (carte d'origine, avant dépose)

À faire **avant** de déposer la carte, elle ne sera plus disponible ensuite.

| Mesure | Signal | Sert à |
|---|---|---|
| Amplitude | `Y05` | §12.1 : dimensionnement d'entrée et debounce |
| Écart adresse → `X93` | `X93` + une ligne d'adresse | §12.4 : `t_setup_us` |
| Délai `X93` → `Y22` | les deux | §12.5 : `y22_write_ack_ms` |
| Délai `X82` → `Y05` | les deux | §12.5 : `y05_start_ack_ms` |
| Durée de course typique | `Y10` | §12.5 : `y10_arrival_ms` |
| Polarité au repos | toute ligne X | §12.3 : PNP/NPN |
| Continuité SUB-D 25 | toutes | §12.2 : table de brochage |
| Sortie `agvdump` complète | - | §3.3 : format à reproduire |

Reporter chaque valeur dans `profiles/default.yaml`, régénérer l'en-tête,
relancer `make test`, puis cocher la ligne correspondante dans
`docs/questions_ouvertes.md`.

## 5. Relevé radio

**Prérequis bloquant de cette architecture** (Archi_2 §7) : relevé RSRP/RSRQ en
tous points du parcours, aux heures de production, machines en marche. Une usine
est une structure métallique et la couverture au sol d'une allée entre racks
n'est **pas garantie** : contrairement à un réseau privé, on ne peut pas la
corriger en ajoutant un répéteur. **Un seul point d'arrêt sous −110 dBm
disqualifie la solution.**

Ensuite seulement : 200 aller-retours réels en conditions de production, avec la
distribution complète des latences. C'est le 99ᵉ percentile qui compte, pas la
moyenne. Le niveau relevé est visible en exploitation dans `/agvdump`
(`rssi_dbm`) et sur la page de supervision.

## 6. Mise en service

1. Provisionner les identifiants MQTT et le certificat de l'autorité sur
   l'ESP32 et sur le poste. Le chiffrement est assuré par **TLS**, pas par une
   clé applicative : sans certificat valide, la connexion échoue franchement,
   elle ne se dégrade pas en clair.
2. Durcir Mosquitto : authentification par utilisateur et **ACL par topic**,
   l'AGV ne doit pouvoir publier que sur `agv/<id>/#`.
3. Appairer chaque bouton EnOcean depuis l'IHM du poste, puis **vérifier chaque
   bouton depuis son emplacement définitif** : la portée se teste là où le
   bouton sera posé, pas sur l'établi.
4. Vérifier `/agvdump` sur l'AGV via la fenêtre de maintenance (contact ILS).
5. Faire valider par l'atelier que le format `/agvdump` reste exploitable avec
   leurs procédures existantes.
