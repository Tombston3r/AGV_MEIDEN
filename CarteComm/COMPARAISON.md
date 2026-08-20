# Comparaison des quatre solutions — document d'aide à la décision

> Compare les quatre architectures étudiées pour le remplacement de la carte
> AIO AGV Control V5.0.1, critère par critère, pour permettre un arbitrage
> argumenté avec le client.
>
> Chiffres issus des nomenclatures du dépôt : [`LoRa/BOM.md`](LoRa/BOM.md),
> [`SMS_EnOcean/BOM.md`](SMS_EnOcean/BOM.md), [`Wifi/BOM.md`](Wifi/BOM.md).

## Note de lecture — ce qui est mesuré et ce qui ne l'est pas

| Statut | Ce que ça recouvre |
|---|---|
| ✅ **Établi** | Calculé par le code et vérifié par test (temps d'antenne LoRa, budget réglementaire, taille des trames), ou relevé sur site (câblage SUB-D 25) |
| 📐 **Estimé** | Ordres de grandeur documentés, prix catalogue, spécifications constructeur |
| ❓ **À mesurer** | Aucun relevé n'existe : couverture radio, couverture cellulaire, amplitude du bus, chronogrammes |

**Aucune couverture radio n'a été relevée sur le site.** Les trois relevés
éliminatoires — LoRa, Wi-Fi, cellulaire — sont les prérequis de chaque
`DEPLOY.md`. Ce document classe les solutions sur ce qui est connaissable
aujourd'hui ; **un seul relevé défavorable peut en disqualifier une**.

---

## 1. Résumé exécutif

| | Recommandation |
|---|---|
| **Si le sans-pile est une exigence réelle** | **Hybride LoRa + EnOcean (A3)** |
| **Si le sans-pile est un confort** | **LoRa pur (A1)** — moins cher jusqu'à 8 stations, et seul à rendre un accusé visuel à l'opérateur |
| **Si le client refuse toute nouvelle carte** | **Wi-Fi + EnOcean** — mais son coût dépasse celui du LoRa |
| **Si le cellulaire est imposé** | **LTE-M/MQTT**, jamais le SMS |

**Le SMS est à écarter** : il cumule la plus mauvaise latence, la seule latence
non bornée, l'absence de garantie d'ordre, et un coût récurrent de ~1 500 €/an
— soit 15 625 € sur dix ans contre 291 € pour le LoRa pur.

---

## 2. Les quatre solutions en un coup d'œil

### LoRa pur (A1)

```
Bouton sur pile ──868 MHz LoRa P2P──▶ Carte AGV neuve ──▶ automate
   + LED verte/rouge                    (RFM95W)
```

Aucune infrastructure. Le bouton parle directement à l'AGV et **affiche
lui-même** si l'ordre est passé.

### Hybride LoRa + EnOcean (A3) — *architecture retenue*

```
Bouton PTM 210 ──868 MHz EnOcean──▶ Poste fixe ──868 MHz LoRa──▶ Carte AGV neuve
  (sans pile)                     (TCM 515 + RFM95W)
```

Le bouton ne consomme rien et ne se change jamais. En contrepartie, un poste
fixe s'intercale, et **aucun retour n'arrive au bouton**.

### Wi-Fi + EnOcean

```
Bouton PTM 210 ──EnOcean──▶ Poste UniPi ──MQTT/TLS──▶ Wi-Fi entreprise ──▶ Carte V5.0.1
  (sans pile)              (+ Mosquitto)                                   CONSERVÉE
```

Aucun matériel embarqué à fabriquer : les deux firmwares de la carte existante
sont réécrits. En contrepartie, l'AGV dépend du réseau du client.

### SMS + EnOcean (cellulaire)

```
Bouton PTM 210 ──EnOcean──▶ Poste UniPi ──SMS ou LTE-M/MQTT──▶ Réseau opérateur ──▶ Carte AGV neuve
```

Deux variantes très différentes : **SMS** (déconseillée) et **LTE-M/MQTT**
(seule défendable).

---

## 3. Tableau de synthèse

Notation : `++` très favorable · `+` favorable · `~` acceptable · `−` défavorable · `−−` rédhibitoire

| Critère | LoRa pur | LoRa + EnOcean | Wi-Fi + EnOcean | SMS | LTE-M/MQTT |
|---|:---:|:---:|:---:|:---:|:---:|
| **Coût sur 10 ans** | `++` 291 € | `++` 378 € | `+` 692 € | `−−` 15 625 € | `~` 1 366 € |
| **Latence** | `+` ~330 ms | `+` ~380 ms | `++` ~50 ms | `−−` non bornée | `~` 0,5–2 s |
| **Déterminisme** | `++` borné | `++` borné | `~` handover | `−−` aucun | `−` reconnexion |
| **Portée / pénétration** | `++` sub-GHz | `+` limité par EnOcean | `−` 2,4 GHz | `~` opérateur | `~` opérateur |
| **Indépendance** | `++` totale | `++` totale | `−` service IT | `−` opérateur | `−` opérateur |
| **Sécurité transport** | `+` AES-128 | `+` AES-128 | `++` TLS + 802.1X | `−` réseau tiers | `++` TLS |
| **Sécurité bouton** | `+` AES-128 | `−` **rejouable** | `−` **rejouable** | `−` **rejouable** | `−` **rejouable** |
| **Accusé opérateur** | `++` LED au bouton | `−` aucun | `−` aucun | `−` aucun | `−` aucun |
| **Simplicité de mise en place** | `++` | `+` | `−` accord IT | `~` | `~` |
| **Délai de mise en œuvre** | `~` PCB 5 sem. | `~` PCB 5 sem. | `−` IT 2–15 j + UniPi | `−` PCB + SIM | `−` PCB + SIM |
| **Réversibilité** | `++` carte intacte | `++` carte intacte | `−` firmware écrasé | `++` carte intacte | `++` carte intacte |
| **Maintenance** | `~` piles 5–8 ans | `++` aucune | `++` aucune | `++` aucune | `++` aucune |
| **Évolutivité** | `+` +60 €/station | `++` +46 €/station | `+` +50 €/station | `+` +50 €/station | `+` +50 €/station |
| **Conformité** | `~` ERC 70-03 | `~` ERC 70-03 | `+` rien à monter | `++` rien à monter | `++` rien à monter |
| **Pérennité** | `++` 10 ans+ | `++` 10 ans+ | `~` 2,4 GHz saturé | `−` 2G/3G éteints | `+` LTE-M longue durée |
| **Développement restant** | `−` dossier à compléter | `−` dossier à compléter | `+` 108 tests verts | `+` 112 tests verts | `+` 112 tests verts |

---

## 4. Critère par critère

### 4.1 Coût — ✅ établi (prix catalogue 📐)

Base 2 stations, outillage inclus, prix **HT** 2026. Les nomenclatures
détaillées sont en **TTC** (TVA 20 %, récupérable pour une entreprise) :
multiplier par 1,20 pour retrouver leurs totaux.

| | Matériel | Récurrent | **10 ans** | Par station |
|---|---:|---:|---:|---:|
| LoRa pur (A1) | 279 € | 0 €/an | **291 €** | +60 € |
| LoRa + EnOcean (A3) | 378 € | 0 €/an | **378 €** | +46 € |
| Wi-Fi + EnOcean | 692 € | 0 €/an | **692 €** | +50 € |
| LTE-M / MQTT | 406 € | 96 €/an | **1 366 €** | +50 € |
| SMS | 625 € | 1 500 €/an | **15 625 €** | +50 € |

Trois observations qui ne sautent pas aux yeux :

1. **La carte AGV du Wi-Fi n'est pas gratuite.** La nomenclature extraite du
   projet KiCad montre qu'elle est **fabriquée** : Mega2560 Pro, ESP32-DevKitC,
   23 MOSFET IRF520 et un convertisseur isolé, soit 129 € HT. Le poste a en
   revanche été allégé — une passerelle Unipi Gate G100 (~200 €) au lieu d'un
   automate à entrées/sorties inutilisées, puisque les boutons sont EnOcean.
2. **Le récurrent domine dès qu'un opérateur entre dans la boucle.** Le SMS
   coûte 56 fois le LoRa pur sur dix ans, pour un service inférieur.
3. **A1 et A3 se croisent à 8 stations** : en dessous A1 est moins chère, au-delà
   A3 prend l'avantage grâce à des boutons à 46 € au lieu de 60 €.

Poste évitable dans trois solutions : remplacer l'UniPi (439–461 €) par un poste
ESP32 (~148 €) économise ~300 €, au prix de l'historique long terme.

### 4.2 Latence et déterminisme — ✅ calculé et testé pour LoRa

| Solution | Typique | Pire cas | Bornée ? |
|---|---:|---:|:---:|
| Wi-Fi (réseau local) | ~50 ms | 2–5 s au handover | Partiellement |
| **LoRa SF7** | ~92 ms | ~280 ms (3 essais) | **Oui** |
| **LoRa SF9** (profil actuel) | ~330 ms | ~990 ms | **Oui** |
| LoRa + EnOcean | +50 ms sur les valeurs ci-dessus | | **Oui** |
| LTE-M / MQTT | 0,5–2 s | ~10 s (reconnexion) | Partiellement |
| **SMS** | **4–8 s** | **plusieurs minutes** | **Non, par conception** |

Le temps d'antenne LoRa est calculé par le code (formule Semtech AN1200.13) et
vérifié par test. **SF9 ne tient pas la cible de 200 ms** annoncée au brief ;
SF7 la tient, au prix de la portée. C'est un arbitrage à trancher — voir
[`LoRa/docs/latence_lora.md`](LoRa/docs/latence_lora.md).

Le SMS n'a pas seulement une mauvaise latence : **il n'a aucun ordre de remise**.
Un `STOP` peut arriver avant le `GOTO` qu'il annule. Sur un engin mobile, c'est
un problème de sécurité fonctionnelle, pas de confort. Le logiciel s'en protège
(rejet des trames désordonnées et périmées), mais au prix de commandes
silencieusement refusées.

### 4.3 Portée et fiabilité selon la distance — ❓ à mesurer

C'est le critère le plus dépendant du site, et **aucun relevé n'existe**.

| Technologie | Portée intérieure typique 📐 | Comportement en structure métallique | Correction possible |
|---|---|---|---|
| **LoRa 868 MHz** | 100–500 m, SF9 | **Bon** — le sub-GHz contourne et pénètre | Augmenter le SF, déporter l'antenne |
| **EnOcean 868 MHz** | **~30 m** | Moyen — bursts très courts, faible puissance | Répéteur (~120 €), antenne déportée |
| **Wi-Fi 2,4 GHz** | 30–50 m par AP | **Médiocre** — forte atténuation, multi-trajets | Ajouter des AP (à la charge du client) |
| **Cellulaire** | Dépend de l'opérateur | **Imprévisible en intérieur** | **Aucune** — on ne pose pas de répéteur opérateur |

Trois conséquences pratiques :

- **Le LoRa est la seule technologie qui progresse avec la distance** au prix de
  la latence : passer de SF7 à SF12 multiplie la portée, et le temps d'antenne
  par 28. Le logiciel gère les deux.
- **L'EnOcean est le maillon court** des architectures A3 et Wi-Fi : 30 m entre
  le bouton et le poste, pas entre le bouton et l'AGV. Si les points d'appel sont
  dispersés, il faut plusieurs postes ou des répéteurs.
- **Le cellulaire est le seul cas où l'on ne peut rien faire.** Une zone morte
  dans une allée entre racks reste une zone morte. C'est pourquoi son relevé est
  **éliminatoire** : un point sous −110 dBm disqualifie la solution.

En mouvement, deux effets s'ajoutent :

| | Effet du déplacement |
|---|---|
| LoRa | Aucun — pas d'association, chaque trame est indépendante |
| Wi-Fi | **Handover entre AP : 2 à 5 s de coupure**, sauf AP unique. IP statique élimine le délai DHCP mais pas la réassociation |
| Cellulaire | Handover cellulaire + variation forte du niveau reçu dans une structure métallique |

### 4.4 Sécurité face à une attaque ou une intrusion

Ce critère se décompose en **trois surfaces distinctes**, et la plus faible n'est
pas celle qu'on croit.

#### a) Surface transport — commande vers l'AGV

| Solution | Chiffrement | Authentification | Anti-rejeu | Surface exposée |
|---|---|---|---|---|
| LoRa (A1, A3) | AES-128-CTR applicatif | clé partagée | fenêtre `seq` 16 | Portée radio du site |
| Wi-Fi | **TLS** + WPA2 ou 802.1X | certificats + ACL par topic | fenêtre `seq` + horodatage | **Réseau d'entreprise** |
| LTE-M / MQTT | **TLS** | certificats + ACL | idem | Internet, si broker sur VPS |
| SMS | AES-128-CTR applicatif | clé partagée | fenêtre `seq` + péremption | **Tout le réseau mobile** |

Points d'attention par solution :

- **LoRa** : sans clé, une commande est inexploitable. Mais le chiffrement CTR
  avec CRC-16 **protège de l'écoute, pas de la falsification** — CTR est
  malléable et un CRC n'est pas une signature. Un AES-CMAC tronqué est
  implémenté et désactivé par défaut : **l'activer coûte 4 octets par trame** et
  ferme cette brèche. À faire si le site est accessible au public.
- **SMS** : n'importe qui connaissant le numéro de la SIM peut **inonder** l'AGV
  de messages. Le contenu reste protégé par la clé, mais le déni de service est
  gratuit, et facturé au client.
- **Wi-Fi** : la meilleure sécurité de transport, **mais elle place l'AGV sur le
  réseau d'entreprise**. C'est un équipement OT non maintenu par l'IT sur un
  réseau IT : le VLAN dédié et les règles de pare-feu ne sont pas optionnels,
  c'est ce qui empêche un AGV compromis de servir de point d'entrée.
- **MQTT sur VPS** : un broker exposé sur Internet est une surface permanente.
  Héberger Mosquitto sur un serveur usine supprime ce point **et** la dépendance
  au tiers.

#### b) Surface bouton — la faiblesse commune ⚠️

**Les télégrammes EnOcean d'un PTM 210 ne sont ni chiffrés ni horodatés.** Ils
contiennent un identifiant 32 bits gravé en usine, en clair.

Conséquence concrète : **quelqu'un disposant d'un récepteur EnOcean à ~30 € peut
capter l'appui d'un bouton, puis le rejouer plus tard pour appeler l'AGV.** La
déduplication du logiciel ne filtre que les copies dans une fenêtre de 100 ms ;
un rejeu différé est indiscernable d'un appui légitime.

Cela concerne **les trois architectures à boutons EnOcean** : A3, Wi-Fi et
cellulaire. Ce n'est pas un défaut du logiciel — c'est le protocole du bouton.

| Parade | Effet | Coût |
|---|---|---|
| Accepter le risque | Il faut être à ~30 m avec du matériel dédié pour appeler un chariot | 0 € |
| Émetteurs à sécurité EnOcean (rolling code AES) | Ferme la brèche | +~15 €/bouton, à valider avec le TCM 515 |
| Boutons LoRa (**A1**) | **Chiffrés et anti-rejeu de bout en bout** | inclus |

**C'est un argument de sécurité en faveur du LoRa pur** qui n'apparaît dans
aucune autre comparaison : c'est la seule solution où le bouton lui-même est
authentifié.

#### c) Surface physique et disponibilité

| Solution | Ce qui peut couper le service depuis l'extérieur |
|---|---|
| LoRa (A1, A3) | Brouillage 868 MHz volontaire — détectable, et le repli s'active |
| Wi-Fi | Panne réseau, **maintenance IT non notifiée**, changement de politique |
| Cellulaire | Panne opérateur, saturation locale, résiliation, changement tarifaire |

Dans tous les cas, la perte de liaison provoque un **arrêt au point d'arrêt
suivant**, jamais un état indéterminé. C'est vérifié par test dans les deux
dossiers compilables.

> Rappel : aucune de ces solutions n'est un organe de sécurité. L'arrêt
> d'urgence, les bumpers et le scrutateur laser restent dans une chaîne
> indépendante conforme à l'ISO 3691-4. Une intrusion réussie peut appeler un
> chariot au mauvais endroit ; elle ne peut pas désactiver ses protections.

### 4.5 Dépendances et disponibilité

| Solution | Dépend de | Qui contrôle | Délai de résolution d'une panne |
|---|---|---|---|
| LoRa pur | rien | **vous** | immédiat |
| LoRa + EnOcean | poste fixe (alimentation) | **vous** | immédiat |
| Wi-Fi + EnOcean | AP, VLAN, DHCP, broker, poste, **service IT** | client / IT | heures à jours |
| Cellulaire | opérateur, SMSC ou broker, poste | **tiers** | hors de tout contrôle |

C'est le critère qui sépare le plus nettement les deux familles. Une phrase à
poser au client : *« l'usine ne peut plus appeler son AGV parce que le réseau
mobile est tombé »* est une situation difficile à expliquer — et elle vaut aussi
pour une maintenance Wi-Fi non notifiée un lundi matin.

### 4.6 Simplicité de mise en place et délai

| Solution | Charge 📐 | Chemin critique | Dépendance externe |
|---|---|---|---|
| **LoRa pur** | 5–8 j | Fabrication PCB, 3–5 sem. | Aucune |
| **LoRa + EnOcean** | 5–8 j | Fabrication PCB, 3–5 sem. | Aucune |
| **Wi-Fi + EnOcean** | 3–5 j | **Accord du service IT, 2 à 15 jours calendaires** | **Élevée** |
| **Cellulaire** | 6–9 j | PCB + souscription SIM | Moyenne |

Le Wi-Fi demande **le moins de travail technique** — aucun matériel à fabriquer —
mais c'est le seul dont le chemin critique ne dépend pas de l'équipe projet. Un
refus du service informatique bloque tout, et ce refus n'est pas rare pour un
équipement OT non géré.

À l'inverse, le LoRa demande plus de travail mais **n'a besoin de l'accord de
personne**.

### 4.7 Réversibilité

| Solution | Retour arrière | Coût du retour |
|---|---|---|
| LoRa, cellulaire | Reposer la carte V5.0.1, **jamais modifiée** | quelques heures |
| **Wi-Fi** | **Reflasher les firmwares d'origine — s'ils ont pu être sauvegardés** | ⚠️ potentiellement impossible |

C'est un risque spécifique au Wi-Fi, et il est asymétrique : les autres solutions
laissent la carte d'origine intacte comme filet de sécurité. Le Wi-Fi l'écrase.

Si les bits de protection interdisent la relecture des flash, **le retour arrière
n'existe plus**. La procédure impose une décision écrite du client à ce stade
(phase 2 de [`Wifi/DEPLOY.md`](Wifi/DEPLOY.md)), et une carte de rechange devient
une assurance à chiffrer.

### 4.8 Maintenance et diagnostic

| Solution | Maintenance périodique | Diagnostic |
|---|---|---|
| LoRa pur | **Piles des boutons, tous les 5–8 ans** | `/agvdump`, RSSI, budget d'émission |
| LoRa + EnOcean | Aucune | idem + compteurs EnOcean |
| Wi-Fi + EnOcean | Aucune | idem + RSSI Wi-Fi, état MQTT |
| Cellulaire | Aucune | idem + niveau réseau, état modem |

Les quatre servent la page `/agvdump` au format historique du client, ce qui
préserve les procédures d'atelier existantes.

Le remplacement des piles de A1 est peu contraignant — 6 € et cinq minutes tous
les 5 à 8 ans — mais c'est une **tâche récurrente à ne pas oublier**, et un
bouton à pile morte est muet. La LED rouge et le compteur au poste le rendent
détectable.

### 4.9 Évolutivité

| Solution | Station de plus | Second AGV | Limite |
|---|---:|---|---|
| LoRa pur | +60 € | +103 € (carte) | Budget de rapport cyclique partagé |
| LoRa + EnOcean | +46 € | +103 € | Portée EnOcean vers le poste |
| Wi-Fi + EnOcean | +50 € | **0 € si la carte existe** | Couverture Wi-Fi |
| Cellulaire | +50 € | +114 € **et une SIM de plus** | Coût récurrent qui double |

Le bus d'adresse de l'AGV code **1 024 stations** : aucune limite matérielle du
côté MEIDEN.

### 4.10 Conformité réglementaire

| Solution | À monter | Contrainte d'exploitation |
|---|---|---|
| LoRa | **Dossier RED** si les cartes sont mises sur le marché ; rien si usage interne | **1 % de rapport cyclique** (ERC 70-03), imposé par le firmware |
| Wi-Fi, cellulaire | Rien — modules certifiés, bandes gérées par l'opérateur ou le client | Aucune |

Le budget de rapport cyclique n'est pas qu'une formalité : il **plafonne le
nombre d'émissions par heure** (218 à SF9). Le firmware refuse d'émettre au-delà
et remonte le refus, ce qui est le comportement correct, mais impose de choisir
la période de télémétrie en conséquence.

### 4.11 Pérennité

| Solution | Horizon 📐 | Risque |
|---|---|---|
| LoRa 868 MHz | 10 ans et plus | Bande ISM pérenne, composants largement diffusés |
| EnOcean | 10 ans et plus | Standard stable, écosystème industriel |
| **Wi-Fi 2,4 GHz** | Incertain | **La saturation du 2,4 GHz est le problème d'origine du projet**. L'ESP32-WROOM ne fait pas de 5 GHz |
| **SMS / 2G-3G** | **Décroissant** | Calendrier d'extinction opérateur en cours |
| LTE-M | 10 ans et plus | Bande dédiée IoT, engagement long des opérateurs |

Le point le plus inconfortable de la solution Wi-Fi : elle **ne résout pas le
problème qui a lancé le projet**. Elle supprime l'émission permanente en point
d'accès et permet au client d'améliorer sa couverture, mais reste sur la bande
saturée. Mesurer l'occupation avant et après est la seule façon d'objectiver le
gain.

### 4.12 Charge de développement restante — ✅ état réel du dépôt

| Solution | État du logiciel |
|---|---|
| **Wi-Fi + EnOcean** | **108 tests verts.** Deux firmwares écrits, jamais compilés pour leur cible |
| **Cellulaire (SMS et LTE-M)** | **112 tests verts.** Firmware et poste écrits, jamais compilés pour leur cible |
| **LoRa pur et hybride** | **10 tests verts**, mais le dossier n'est **pas encore autonome** : une phase de complétion d'un jour est nécessaire |

Le paradoxe mérite d'être dit : les deux solutions les mieux avancées
logiciellement sont celles qui sortent le moins bien du comparatif technique.

---

## 5. Fiches par solution

### 5.1 LoRa pur (A1)

**Points forts**

- Le **coût le plus bas** de toutes les solutions : 291 € sur dix ans.
- **Aucune dépendance** : ni opérateur, ni service informatique, ni infrastructure.
- **Latence bornée**, et la seule à pouvoir être ajustée par un paramètre.
- **Portée sub-GHz**, la plus adaptée à une structure métallique.
- **Le seul accusé visuel à l'opérateur** : LED verte = ordre reçu par l'AGV.
- **Le seul bouton authentifié** : chiffré, anti-rejeu de bout en bout.
- Ajout d'un bouton sans toucher à l'AGV.

**Points faibles**

- **Piles à remplacer** tous les 5 à 8 ans, et un bouton à pile morte est muet.
- **Boutons à fabriquer** : PCB, boîtier IP65, assemblage — 60 € l’unité et un
  délai de PCB en parallèle de la carte AGV.
- **Budget de rapport cyclique** à respecter : 218 émissions/h à SF9.
- Dossier logiciel **à compléter** avant de pouvoir construire.
- Dossier RED à monter si les cartes sont mises sur le marché.

### 5.2 Hybride LoRa + EnOcean (A3) — *retenue*

**Points forts**

- **Boutons sans pile, sans maintenance**, à vie.
- Coût très bas : 378 € sur dix ans, et **le moins cher par station ajoutée**.
- Indépendance totale, comme A1.
- Latence bornée, portée sub-GHz sur le tronçon long.
- Boutons du commerce disponibles, pas de PCB bouton à faire.

**Points faibles**

- **Aucun retour à l'opérateur** — le TCM 515 est en réception seule. C'est la
  faiblesse ergonomique principale.
- **Bouton rejouable** : télégramme EnOcean en clair.
- **Portée EnOcean de ~30 m** vers le poste : contrainte d'implantation qui peut
  imposer un répéteur ou plusieurs postes.
- Un **poste fixe supplémentaire** à alimenter et à maintenir.
- **Deux antennes 868 MHz** sur le même boîtier : à espacer, sous peine de
  désensibiliser le récepteur EnOcean.
- Même dossier logiciel à compléter que A1.

### 5.3 Wi-Fi + EnOcean

**Points forts**

- **Réutilise les modules existants** — Mega2560 Pro et ESP32-DevKitC sur
  supports — donc aucun composant exotique et un remplacement immédiat en cas de
  panne d'un module.
- **La meilleure latence** : réseau local, ~50 ms hors handover.
- **La meilleure sécurité de transport** : TLS, 802.1X, ACL par topic.
- Infrastructure existante, maintenue par le client.
- Boutons sans pile.
- Logiciel le plus avancé, avec un repli de sécurité matériel indépendant du
  réseau (heartbeat ESP32 → ATmega).

**Points faibles**

- **Dépend du service informatique**, qui devient le chemin critique du projet et
  n'est pas sous votre contrôle. Un refus bloque tout.
- **Ne résout pas la saturation du 2,4 GHz**, qui est le motif du projet.
- **Réversibilité incertaine** : les firmwares d'origine sont écrasés, et rien ne
  garantit qu'ils soient relisibles.
- **Handover** entre points d'accès : 2 à 5 s de coupure en mouvement.
- **Plus chère que le LoRa** : 692 € contre 291 €. Sa carte AGV est fabriquée, pas réutilisée.
- Une maintenance IT non notifiée peut arrêter la production.
- Bouton rejouable, pas d'accusé opérateur.

### 5.4 Cellulaire — SMS et LTE-M

**Points forts**

- **Portée illimitée** : pertinent si le site s'étend ou couvre plusieurs
  bâtiments.
- **Notification hors site native** — le seul avantage réellement différenciant,
  et il reste accessible aux autres solutions par une passerelle d'alerte à
  ~10 €/an.
- Aucune infrastructure radio à déployer ni à étudier.
- Couche physique robuste : handover, contrôle de puissance, correction d'erreurs
  gérés nativement.
- Aucun dossier de conformité radio à monter.

**Points faibles**

- **Coût récurrent** : 15 625 € sur dix ans en SMS, 1 366 € en LTE-M, contre 291 €
  en LoRa.
- **Couverture intérieure incertaine et non corrigeable.**
- **Dépendance à un tiers** : panne, saturation, résiliation, changement tarifaire.
- **Obsolescence 2G/3G** pour le SMS.
- **Le SMS n'a ni latence bornée, ni ordre de remise, ni garantie de remise.**
- Consommation en pics de 2 A à l'émission : réservoir capacitif obligatoire, et
  boutons sur pile exclus.
- Complexité de mise au point : AT, roaming, APN, PIN — difficile à diagnostiquer
  sur site.

---

## 6. Arbre de décision

```
Le client refuse-t-il toute nouvelle carte ?
├── OUI ──▶ Le service IT accepte-t-il un équipement OT sur son réseau ?
│           ├── OUI ──▶ WI-FI + ENOCEAN
│           │           (vérifier d'abord : sauvegarde des flash possible ?)
│           └── NON ──▶ impasse : reprendre la discussion « nouvelle carte »
│
└── NON ──▶ Le sans-pile est-il une exigence réelle ?
            ├── OUI ──▶ Plus de 8 points d'appel ?
            │           ├── OUI ──▶ HYBRIDE LORA + ENOCEAN
            │           └── NON ──▶ HYBRIDE LORA + ENOCEAN
            │                       (A1 serait moins cher, mais l'exigence tranche)
            │
            └── NON ──▶ Un accusé visuel à l'opérateur est-il utile ?
                        ├── OUI ──▶ LORA PUR (A1)
                        └── NON ──▶ Plus de 8 points d'appel ?
                                    ├── OUI ──▶ HYBRIDE (A3)
                                    └── NON ──▶ LORA PUR (A1)

Le cellulaire est-il imposé par le client ?
└── OUI ──▶ LTE-M / MQTT, jamais le SMS
            + relevé RSRP éliminatoire avant tout engagement
```

---

## 7. Ce qui doit être mesuré avant de trancher

Aucune décision définitive ne devrait être prise avant ces relevés. Chacun peut
disqualifier une solution.

| # | Mesure | Disqualifie | Charge |
|---|---|---|---|
| 1 | **Couverture LoRa** le long du parcours, à hauteur d'antenne AGV | A1, A3 si trous | 0,5 j |
| 2 | **Couverture Wi-Fi** idem, plus comptage des handovers | Wi-Fi | 1 j |
| 3 | **Couverture cellulaire** RSRP/RSRQ — un point sous −110 dBm suffit | Cellulaire | 0,5 j |
| 4 | **Portée EnOcean** depuis chaque emplacement définitif de bouton | A3, Wi-Fi, cellulaire | 0,5 j |
| 5 | **Occupation de la bande 868 MHz** (RTL-SDR, 30 €) | objective la crainte de collision | 0,5 j |
| 6 | **Amplitude des lignes Y et chronogrammes du bus** | aucune — commun aux quatre | 1 j |
| 7 | **Accord de principe du service informatique** | Wi-Fi | 2 à 15 j calendaires |
| 8 | **Lecture des flash d'origine** (`esptool`, `avrdude`) | conditionne la réversibilité du Wi-Fi | 0,5 j |

Les mesures 1 à 5 se font en une journée avec le bon matériel, pour ~45 € de
sondes. **C'est l'investissement le plus rentable du projet** : il évite de
choisir une architecture qui ne fonctionnera pas sur ce site.

---

## 8. Questions à faire trancher par le client

1. **Le sans-pile est-il une exigence contractuelle, ou un confort ?** C'est la
   question qui départage A1 et A3, et elle vaut ~84 € plus un accusé opérateur.
2. **Un opérateur doit-il savoir que son appel est parti ?** Si oui, seul A1 le
   rend nativement ; les autres imposent un voyant au poste ou un actionneur
   EnOcean (+100 à 160 €).
3. **Quel seuil de latence rend un appel « raté » aux yeux d'un opérateur ?**
   À fixer avant les essais, sinon le critère de recette est le vôtre et non le
   sien.
4. **Le service informatique acceptera-t-il un équipement OT sur son réseau,**
   avec VLAN dédié et notification des changements ? Réponse à obtenir **avant**
   d'engager la solution Wi-Fi.
5. **Un historique consultable sur plusieurs semaines est-il attendu ?** Sinon,
   remplacer l'UniPi par un ESP32 économise ~300 €.
6. **Combien de points d'appel à terme ?** C'est ce qui place le curseur entre
   A1 et A3.
7. **Le coût récurrent du cellulaire est-il accepté ?** 15 625 € sur dix ans en
   SMS, 1 366 € en LTE-M, contre 291 € sans opérateur.

---

## 9. Conclusion

**L'hybride LoRa + EnOcean reste le meilleur compromis** dès lors que le
sans-pile est exigé : coût le plus bas de sa catégorie, indépendance totale,
latence bornée, portée adaptée à un environnement métallique, et aucune
maintenance.

**Le LoRa pur mérite d'être remis sur la table** si le sans-pile n'est qu'un
confort. Il est moins cher jusqu'à huit stations, et il est le seul à répondre à
deux besoins que personne n'a formulés mais qui comptent en exploitation :
**dire à l'opérateur que son appel est passé**, et **empêcher qu'un tiers rejoue
un appui de bouton**.

**Le Wi-Fi est une option légitime mais coûteuse en risques**, pas en euros
seulement. Elle échange un travail matériel contre une dépendance organisationnelle
et une réversibilité incertaine, sans résoudre le problème de bande qui a motivé
le projet.

**Le SMS est à écarter.** Le remède est plus dangereux que le mal supposé : la
crainte porte sur des collisions LoRa dont l'effet serait une retransmission de
100 ms, et la solution proposée introduit une latence non bornée et un désordre
de remise possible — pour 15 625 € sur dix ans. Si le cellulaire est un souhait
politique, **LTE-M/MQTT le satisfait pour un onzième du prix**, et une passerelle
d'alerte SMS à ~10 €/an apporte la notification hors site à n'importe laquelle
des autres architectures.
