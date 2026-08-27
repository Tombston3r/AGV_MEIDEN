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
| 2026-08-27 | **`DEPLOY.md` manquants ajoutés** — la convention du dépôt (un `README.md` **et** un `DEPLOY.md` par dossier livrable) n'avait pas été appliquée au chantier Timer. `banc_api/DEPLOY.md` : construction, lancement, recette en neuf gestes, vérification en ligne de commande, limites assumées et diagnostic. `Timer/DEPLOY.md` : le chemin ordonné vers la mise en service, dont une **phase 0 de verrous non logiciels** (sécurité machine ISO 3691-4 et sélecteur physique, mesure de `t_setup`, capacité I²C) et une recette sur chariot. **Cause corrigée** : `ORGANISATION.md` vivait dans `Comm distance/docs/` et ne couvrait pas le dépôt unifié — remonté à `docs/` racine, élargi aux chantiers, et la paire `README`+`DEPLOY` y est désormais explicitement obligatoire, **même quand rien n'est déployable**. | conventions, 8 dossiers conformes |
| 2026-08-27 | **Banc local de l'API** (`banc_api/`) : le VRAI moteur derrière les routes de la spec §6 — pas une doublure — avec horloge simulée pilotable (`/api/sim/*`, absent de la cible) : se placer à 05:59, accélérer ×600, franchir minuit, simuler une heure douteuse. IHM avec bandeau de validation non contournable, prochaines occurrences, missions, journal motivé. **Codec JSON strict** du document (clé inconnue = erreur : une faute de frappe ignorée deviendrait une mission mal paramétrée), 13 tests de rejet/aller-retour — il servira tel quel à la persistance §3.3. **10 tests de contrat** (subprocess + HTTP réel) : verrou optimiste 428/409, mission à l'heure une seule fois, expiration de la validation à minuit, gel §2.3, révocation. **Décision** : toute modification du planning (PUT) révoque la validation du jour — ce qui a été validé n'est plus ce qui est en mémoire. ⚠️ Piège documenté : l'horloge simulée ne va que vers le futur, revenir en arrière ne dé-consomme rien. | banc, moteur, 42 tests |
| 2026-08-27 | **Moteur d'ordonnancement écrit et testé** — occurrences (masque de jours, validité, exceptions), validation quotidienne, grâce 5 min sans rejeu au boot, idempotence `(id, date locale)` couvrant le 02:30 double d'automne, DST printemps configurable (défaut : exécution au premier instant), gel sur heure non fiable, pause/saut/priorités, journal motivé, liste des prochaines occurrences. Un point de conception attrapé par les tests : le balayage de la veille (grâce à cheval sur minuit) journalisait comme « sautées » des missions d'hier qu'un redémarrage aurait pu avoir déjà exécutées — **le journal aurait menti**. La veille n'est plus balayée que dans la fenêtre de grâce ; le reste attend la persistance des clés (T1). | moteur, 18 tests |
| 2026-08-27 | Spec confrontée au dépôt : le séquenceur du §5 **existe déjà** (A4), pas de MCP23017 sur la V5.0.1 relevée, stations 0–1023. Voir `ALIGNEMENT_COMM_DISTANCE.md`. | périmètre réduit |
