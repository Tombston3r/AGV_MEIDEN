# Timer — planning journalier des déplacements AGV

Déclenchement **automatique de missions à horaires programmés**, validé chaque
jour par un opérateur. Cadrage complet :
[`Spec Planning Journalier.md`](Spec%20Planning%20Journalier.md).

> ⚠️ Changement de nature : on passe d'un appel à la demande à un déclenchement
> autonome sur horloge. La sécurité machine (§9 de la spec, EN ISO 3691-4) est
> à traiter **avant** la mise en service — sélecteur physique d'autorisation,
> authentification de l'IHM.

## Ce qui existe : le moteur (spec §4)

C++ pur, horloge injectée, zéro dépendance ESP-IDF/Arduino — même approche que
le codec LoRa de `Comm distance/`. Il couvre :

- occurrences par **masque de jours**, bornes de validité, exceptions (fériés) ;
- **validation quotidienne** (§3.2) : rien ne part sans autorisation du jour,
  la validation d'hier ne vaut pas, traçabilité `valide_par`/`valide_le` ;
- **fenêtre de grâce** (5 min par défaut) : rattrapage borné, jamais de rejeu
  de la journée au redémarrage ;
- **idempotence** par `(id, date locale)` — le 02:30 double de l'automne ne
  part qu'une fois ;
- **heure d'été de printemps** : exécution au premier instant existant
  (03:00), décalage signalé — ou saut, par configuration *(décision produit du
  2026-08-27 : exécuter)* ;
- **gel sur heure non fiable** (§2.3) : rien ne part, rien n'est consommé ;
- pause globale, saut sur demande, priorités sur occurrences simultanées ;
- journal borné motivé, liste des **prochaines occurrences calculées** (la vue
  IHM la plus utile).

## Lancer les tests

```bash
make test
```

**Attendu : `18 tests, 0 echec(s)`.** Aucun matériel requis : fuseau
Europe/Paris forcé, dates d'heure d'été réelles de 2026 (29 mars, 25 octobre).

## Structure

| Chemin | Rôle |
|---|---|
| `Spec Planning Journalier.md` | cadrage — décisions, points ouverts |
| `moteur/planning.{h,cpp}` | le moteur, C++ pur |
| `test/test_planning.cpp` | 18 tests, horloge simulée |
| `docs/ALIGNEMENT_COMM_DISTANCE.md` | **confrontation de la spec au dépôt** — à lire avant d'implémenter la suite |
| `docs/ETAT_PROJET.md` | état, kanban, journal |

## Ce qui reste (voir le kanban)

Persistance LittleFS (§3.3), pilote DS3231 et états de confiance (§2), API web
REST + WebSocket (§6), intégration dans `A4_Wifi` — le séquenceur du §5
**existe déjà, testé**, le moteur n'a qu'à produire des missions vers la
liaison existante.
