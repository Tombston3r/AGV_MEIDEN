# État du projet Timer — planning journalier

> Tenu à jour à chaque modification, comme les `ETAT_PROJET.md` de
> `Comm distance/`.

## État

**Moteur livré et testé** (19 tests), **codec JSON** du document (13 tests —
le même schéma servira l'API et la persistance §3.3), et **banc local de
l'API** : le vrai moteur derrière les routes de la spec §6, horloge simulée
pilotable, IHM avec bandeau de validation, 10 tests de contrat. Restent le
matériel (DS3231), la couche fichier et le portage ESP32.

## Décisions actées

| Date | Décision |
|---|---|
| 2026-08-27 | DST printemps : **exécution au premier instant existant** (03:00), décalage signalé. Le saut reste configurable. |
| 2026-08-27 | Fenêtre de grâce par défaut : **5 min** (suggestion spec §4.2 adoptée). |
| 2026-08-27 | Versionnage : **dépôt unique à la racine `AGV_MEIDEN/`**, `.git` remonté depuis `Comm distance/` (historique conservé). |

## Kanban

| # | Tâche | Note |
|---|---|---|
| T1 | ⚠️ **Persister les clés consommées** avec le planning (§3.3) | Sans cela, un redémarrage **dans la fenêtre de grâce** rejoue une mission déjà partie, et le journal de boot peut mentir sur ce qui a réellement tourné |
| T2 | Persistance LittleFS : couche **fichier** — CRC32, écriture atomique, version N-1 (§3.3) | Le **codec JSON existe** (`serialisation.{h,cpp}`, 13 tests) |
| T3 | Pilote DS3231 + machine d'états de confiance (§2.3) + séquence de boot (§2.4) | Prototypable sur le banc T-Beam (I²C actif) avant la carte AGV |
| T4 | **Portage ESP32** de l'API — le contrat est fixé et testé par `banc_api/` (ETag/409/428 compris) | S'appuyer sur le serveur A4 existant ; WebSocket à trancher à ce moment-là (le banc sonde) |
| T5 | Bandeau de validation + rappel actif 06:30 (§6) | Non contournable |
| T6 | Intégration A4 : mission → `Cmd::Goto` sur la liaison existante | Le séquenceur §5 est déjà écrit et testé — voir `ALIGNEMENT_COMM_DISTANCE.md` |
| T7 | Reboot quotidien 03:00 + verrous (§7.3) + instrumentation heap (§7.4) | Interdit si mission en cours ou AGV hors quai |
| T8 | Sécurité machine §9 : sélecteur physique (impact **nomenclature**), authentification web | **Avant mise en service** |
| T9 | Mesure physique de `t_setup` | Déjà au kanban de A4 — même mesure |

## Journal

| Date | Modification | Impact |
|---|---|---|
| 2026-08-27 | **La frise saturait en texte** : sa hauteur et ses trois rangées étaient figées dans la feuille de style. Toute la géométrie verticale passe dans un objet `GEO` du script, et **la hauteur suit les rangées réellement occupées** (jusqu'à dix, plancher à trois). Vérifié sur une journée chargée : **onze départs sur quatre rangées, sans chevauchement**. Ordre de peinture rendu explicite (`z-index` occupation < étiquette < curseur) — sans lui, l'ordre du DOM décidait et le bloc d'occupation recouvrait l'heure du curseur. | IHM du banc |
| 2026-08-27 | **La frise mentait sur l'occupation de l'AGV.** L'étiquette d'un départ mesurait ~92 px sur une frise de 1030 px, soit **2,1 h d'immobilisation apparente** pour un trajet qui en dure **5 min** (0,35 %, soit 3,6 px). Corrigé en séparant deux objets : un **bloc d'occupation à l'échelle réelle sur l'axe**, et une **étiquette détachée** reliée par une amorce d'un pixel, qui n'engage aucune durée. La durée devient `Config::duree_mission_s` (défaut 300 s), exposée par `/api/time` — une seule source de vérité pour le moteur et l'IHM, pas une constante de CSS. Le moteur marque désormais les **chevauchements** (`Prochaine::conflit`, test dédié), l'API les expose, l'IHM les cerne d'ambre, et **poser un départ à moins de 5 min d'un autre est refusé à la saisie** : le séquenceur refusant d'empiler sans accusé (§5), le départ serait perdu. Placement des étiquettes sur trois rangées **par détection de chevauchement** au lieu d'une alternance aveugle. | moteur, API, IHM |
| 2026-08-27 | **Deux défauts de rendu corrigés sur l'IHM, constatés par l'exploitant.** (1) Drapeaux invisibles : `.fanion` posait `color:#fff` **et** `background:currentColor` — `currentColor` se résout contre la couleur de l'élément lui-même, d'où un fanion blanc-sur-blanc, seul le mât de 2 px restait. La couleur d'état vit sur `.drapeau`, le blanc du texte sur le `span`. (2) `rendrePostes()` n'était **jamais appelé au démarrage** : aucune pastille, donc aucun geste possible. Rendu synchrone avant le premier échange réseau. **La vérification a changé de nature** : capture Firefox headless d'une page peuplée, via une page-enveloppe au `load` retardé — une capture au chargement précède le contenu asynchrone, c'est ce qui avait masqué les deux défauts. Palette par défaut : **Poste 1 / Poste 2** (les postes réels, clé de stockage versionnée) ; l'ajout inline devient la fenêtre **⚙ Gestion des postes**. Recette : douze gestes. | IHM du banc |
| 2026-08-27 | **IHM du banc, itération 2** sur directives client : thème blanc/noir/**bleu outremer**, logo AIO, la journée en **frise horizontale** 00h–24h avec curseur d'heure courante. **Pose de départ au drapeau** : choisir un poste, prendre le drapeau, cliquer la frise (pas de 5 min, fantôme sous la souris, refus des heures passées) — le drapeau posé est un départ à **date unique** (`debut=fin=jour`), la frise représentant la journée et non une récurrence. Retrait au clic avec confirmation explicite si l'entrée est récurrente. **Logs des départs** : ✓ confirmés avec heure prévue, ✗ sautés avec motif. **Bouton d'appel** : `POST /api/appel`, mission immédiate **hors validation** — c'est un geste opérateur, pas un déclenchement autonome, la décision est documentée. Nouvelle route `GET /api/planning/jour` : occurrences d'aujourd'hui **passées comprises**, calculées par le moteur (DST compris) plutôt que recalculées en JavaScript. L'éditeur JSON brut disparaît de la page (l'API PUT reste, testée). 12 tests de contrat. | banc, serveur, 44 tests |
| 2026-08-27 | **`DEPLOY.md` manquants ajoutés** — la convention du dépôt (un `README.md` **et** un `DEPLOY.md` par dossier livrable) n'avait pas été appliquée au chantier Timer. `banc_api/DEPLOY.md` : construction, lancement, recette en neuf gestes, vérification en ligne de commande, limites assumées et diagnostic. `Timer/DEPLOY.md` : le chemin ordonné vers la mise en service, dont une **phase 0 de verrous non logiciels** (sécurité machine ISO 3691-4 et sélecteur physique, mesure de `t_setup`, capacité I²C) et une recette sur chariot. **Cause corrigée** : `ORGANISATION.md` vivait dans `Comm distance/docs/` et ne couvrait pas le dépôt unifié — remonté à `docs/` racine, élargi aux chantiers, et la paire `README`+`DEPLOY` y est désormais explicitement obligatoire, **même quand rien n'est déployable**. | conventions, 8 dossiers conformes |
| 2026-08-27 | **Banc local de l'API** (`banc_api/`) : le VRAI moteur derrière les routes de la spec §6 — pas une doublure — avec horloge simulée pilotable (`/api/sim/*`, absent de la cible) : se placer à 05:59, accélérer ×600, franchir minuit, simuler une heure douteuse. IHM avec bandeau de validation non contournable, prochaines occurrences, missions, journal motivé. **Codec JSON strict** du document (clé inconnue = erreur : une faute de frappe ignorée deviendrait une mission mal paramétrée), 13 tests de rejet/aller-retour — il servira tel quel à la persistance §3.3. **10 tests de contrat** (subprocess + HTTP réel) : verrou optimiste 428/409, mission à l'heure une seule fois, expiration de la validation à minuit, gel §2.3, révocation. **Décision** : toute modification du planning (PUT) révoque la validation du jour — ce qui a été validé n'est plus ce qui est en mémoire. ⚠️ Piège documenté : l'horloge simulée ne va que vers le futur, revenir en arrière ne dé-consomme rien. | banc, moteur, 42 tests |
| 2026-08-27 | **Moteur d'ordonnancement écrit et testé** — occurrences (masque de jours, validité, exceptions), validation quotidienne, grâce 5 min sans rejeu au boot, idempotence `(id, date locale)` couvrant le 02:30 double d'automne, DST printemps configurable (défaut : exécution au premier instant), gel sur heure non fiable, pause/saut/priorités, journal motivé, liste des prochaines occurrences. Un point de conception attrapé par les tests : le balayage de la veille (grâce à cheval sur minuit) journalisait comme « sautées » des missions d'hier qu'un redémarrage aurait pu avoir déjà exécutées — **le journal aurait menti**. La veille n'est plus balayée que dans la fenêtre de grâce ; le reste attend la persistance des clés (T1). | moteur, 18 tests |
| 2026-08-27 | Spec confrontée au dépôt : le séquenceur du §5 **existe déjà** (A4), pas de MCP23017 sur la V5.0.1 relevée, stations 0–1023. Voir `ALIGNEMENT_COMM_DISTANCE.md`. | périmètre réduit |
