# Banc API sur carte (ESP32 de l'AGV Control V5.0.1)

Ce dossier fait tourner l'API du planning journalier **sur l'ESP32 de la carte**,
qui ouvre un point d'accès Wi-Fi `agv-atelier`. On s'y connecte, on ouvre
`http://192.168.4.1/`, et c'est l'interface agvschedule habituelle.

## Ce que ce banc prouve, et ce qu'il ne prouve pas

Le banc local (`make banc`, sur `127.0.0.1`) valide le moteur et les routes. Il
ne dit rien de ce qui casse une fois embarqué : la RAM, la pile IP, plusieurs
clients, la fragmentation du tas sur des heures. C'est exactement ce que ce
banc-ci ajoute.

En revanche, il ne faut pas lui prêter plus :

| Il vérifie | Il ne vérifie pas |
| --- | --- |
| L'API répond sur la carte, à travers le Wi-Fi | La commande de l'AGV : **aucune trame n'est émise vers la MEGA** |
| La tenue mémoire du serveur et du moteur | La liaison ESP32/MEGA (D52/D53, SoftwareSerial 38400) |
| L'interface est servie depuis la flash | Le comportement horaire au long cours : la carte n'a ni RTC ni NTP |

Une mission due est journalisée et servie par `/api/missions`. Elle ne fait rien
bouger. Ce banc est un banc d'API, pas un pilote.

## L'heure

Une carte nue démarre en 1970 : pas de RTC sur la V5.0.1, et pas de NTP sur un
point d'accès sans internet. Le moteur démarre donc **gelé** (`heure_fiable`
à `false`, spec §2.5) et n'exécute aucun départ tant qu'un opérateur n'a pas
posé l'heure :

```
POST /api/sim/heure   {"heure":"2026-08-28T14:30:00"}
```

C'est volontaire. L'alternative serait de tirer les départs du 1er janvier 1970.
Le panneau de simulation de l'interface fait la même chose en un clic.

## Le serveur n'est pas recopié ici

`platformio.ini` va chercher `../serveur.cpp` et `../../moteur/`. Le binaire
embarqué contient le **même** code que le banc local : deux points seulement
divergent, isolés sous `ESP_PLATFORM` dans `serveur.cpp`, les pages web
(embarquées faute de système de fichiers) et l'adresse d'écoute.

Les pages sont générées en tableaux C++ par `outils/embarquer_web.py`. Le
fichier produit, `src/web_embarque.h`, est **généré** : ne pas l'éditer.
Après toute modification de `banc_api/web/`, relancer :

```
python3 ../../../outils/embarquer_web.py
```

## Construire et flasher

Voir `DEPLOY.md`. **Ne pas flasher sans avoir lu le point 1** : la carte porte
aujourd'hui le firmware de production de l'AGV, dont il n'existe aucune
sauvegarde.

## Réseau

| Paramètre | Valeur |
| --- | --- |
| SSID | `agv-atelier` |
| Sécurité | WPA2-PSK |
| Mot de passe | `agv-atelier-2026` (à changer, `src/main.cpp`) |
| Adresse du banc | `http://192.168.4.1/` |
| Clients simultanés | 4 |

Le WPA2 n'est pas du zèle : les routes du banc posent l'heure et déclenchent
des appels. Un point d'accès ouvert dans un atelier laisserait n'importe quel
terminal écrire dedans.
