# État du projet Timer — planning journalier

> Tenu à jour à chaque modification, comme les `ETAT_PROJET.md` de
> `Comm distance/`.

## État

**Moteur d'ordonnancement livré et testé** (18 tests natifs). Ni persistance,
ni matériel, ni IHM : le moteur est la fondation dont tout le reste dépend.

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
| T2 | Persistance LittleFS : JSON + CRC32 + écriture atomique + version N-1 (§3.3) | Le moteur est prêt : `definir_entrees()` |
| T3 | Pilote DS3231 + machine d'états de confiance (§2.3) + séquence de boot (§2.4) | Prototypable sur le banc T-Beam (I²C actif) avant la carte AGV |
| T4 | API web REST + WebSocket + ETag/409 (§6) | S'appuyer sur le serveur A4 existant, pas créer de zéro |
| T5 | Bandeau de validation + rappel actif 06:30 (§6) | Non contournable |
| T6 | Intégration A4 : mission → `Cmd::Goto` sur la liaison existante | Le séquenceur §5 est déjà écrit et testé — voir `ALIGNEMENT_COMM_DISTANCE.md` |
| T7 | Reboot quotidien 03:00 + verrous (§7.3) + instrumentation heap (§7.4) | Interdit si mission en cours ou AGV hors quai |
| T8 | Sécurité machine §9 : sélecteur physique (impact **nomenclature**), authentification web | **Avant mise en service** |
| T9 | Mesure physique de `t_setup` | Déjà au kanban de A4 — même mesure |

## Journal

| Date | Modification | Impact |
|---|---|---|
| 2026-08-27 | **Moteur d'ordonnancement écrit et testé** — occurrences (masque de jours, validité, exceptions), validation quotidienne, grâce 5 min sans rejeu au boot, idempotence `(id, date locale)` couvrant le 02:30 double d'automne, DST printemps configurable (défaut : exécution au premier instant), gel sur heure non fiable, pause/saut/priorités, journal motivé, liste des prochaines occurrences. Un point de conception attrapé par les tests : le balayage de la veille (grâce à cheval sur minuit) journalisait comme « sautées » des missions d'hier qu'un redémarrage aurait pu avoir déjà exécutées — **le journal aurait menti**. La veille n'est plus balayée que dans la fenêtre de grâce ; le reste attend la persistance des clés (T1). | moteur, 18 tests |
| 2026-08-27 | Spec confrontée au dépôt : le séquenceur du §5 **existe déjà** (A4), pas de MCP23017 sur la V5.0.1 relevée, stations 0–1023. Voir `ALIGNEMENT_COMM_DISTANCE.md`. | périmètre réduit |
