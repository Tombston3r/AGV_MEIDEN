// Tests du moteur de planning — horloge simulée, fuseau Europe/Paris forcé.
//
// Les dates d'heure d'été utilisées sont réelles : en 2026, passage à l'heure
// d'été le 29 mars (02:00 -> 03:00), retour le 25 octobre (03:00 -> 02:00).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "moteur/planning.h"

using namespace agv::planning;

namespace {

int g_tests = 0;
int g_echecs = 0;

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::printf("    ECHEC %s:%d — %s\n", __FILE__, __LINE__, #cond);    \
      ++g_echecs;                                                          \
    }                                                                      \
  } while (0)

#define RUN(f)                       \
  do {                               \
    std::printf("[ RUN  ] %s\n", #f); \
    ++g_tests;                       \
    f();                             \
  } while (0)

// Instant local Europe/Paris. `isdst` explicite pour viser l'une des deux
// occurrences d'un horaire ambigu d'automne.
time_t T(int a, int mo, int j, int h, int mi, int isdst = -1) {
  tm local{};
  local.tm_year = a - 1900;
  local.tm_mon = mo - 1;
  local.tm_mday = j;
  local.tm_hour = h;
  local.tm_min = mi;
  local.tm_isdst = isdst;
  return mktime(&local);
}

Entree entree_simple(const char* id, uint8_t h, uint8_t mi,
                     uint8_t jours = jour::kTous) {
  Entree e;
  e.id = id;
  e.heure = h;
  e.minute = mi;
  e.jours = jours;
  e.station = 42;
  return e;
}

// Un moteur validé pour le jour de `now` : l'état de départ de la plupart
// des scénarios.
Moteur moteur_valide(std::vector<Entree> entrees, time_t now) {
  Moteur m;
  m.definir_entrees(std::move(entrees));
  m.valider_journee(date_locale(now), "test", now);
  return m;
}

// --- Calendrier ------------------------------------------------------------

void jours_ouvres_seulement() {
  const Entree e = entree_simple("liv", 6, 0, jour::kOuvres);
  // Samedi 29 août 2026 -> prochaine : lundi 31 à 06:00.
  const auto occ = prochaine_occurrence(e, T(2026, 8, 29, 12, 0), {});
  CHECK(occ.has_value());
  CHECK(occ->quand == T(2026, 8, 31, 6, 0));
  CHECK(!occ->decalee_dst);
}

void bornes_de_validite() {
  Entree e = entree_simple("tmp", 6, 0);
  e.debut = 20260901;
  e.fin = 20260905;
  const auto avant = prochaine_occurrence(e, T(2026, 8, 20, 0, 0), {});
  CHECK(avant.has_value());
  CHECK(avant->quand == T(2026, 9, 1, 6, 0));
  const auto apres = prochaine_occurrence(e, T(2026, 9, 6, 0, 0), {});
  CHECK(!apres.has_value());
}

void exception_exclut_la_date() {
  Entree e = entree_simple("fer", 6, 0);
  e.exceptions = {20260828};  // vendredi exclu
  const auto occ = prochaine_occurrence(e, T(2026, 8, 28, 0, 0), {});
  CHECK(occ.has_value());
  CHECK(occ->quand == T(2026, 8, 29, 6, 0));
}

// --- Heure d'été -----------------------------------------------------------

void printemps_execute_au_premier_instant() {
  // 02:30 n'existe pas le 29 mars 2026 : la mission part à 03:00 CEST.
  const Entree e = entree_simple("nuit", 2, 30);
  const auto occ = prochaine_occurrence(e, T(2026, 3, 29, 0, 0), {});
  CHECK(occ.has_value());
  CHECK(occ->decalee_dst);
  tm local{};
  localtime_r(&occ->quand, &local);
  CHECK(local.tm_hour == 3);
  CHECK(local.tm_min == 0);
  CHECK(local.tm_isdst == 1);
}

void printemps_politique_saut() {
  const Entree e = entree_simple("nuit", 2, 30);
  Config cfg;
  cfg.dst = DstPrintemps::Sauter;
  // La journée du 29 est sautée : prochaine occurrence le 30 à 02:30.
  const auto occ = prochaine_occurrence(e, T(2026, 3, 29, 0, 0), cfg);
  CHECK(occ.has_value());
  CHECK(occ->quand == T(2026, 3, 30, 2, 30));

  // Et le moteur journalise le saut au passage de la journée.
  Config cfg_saut;
  cfg_saut.dst = DstPrintemps::Sauter;
  Moteur m(cfg_saut);
  m.definir_entrees({e});
  const time_t now = T(2026, 3, 29, 12, 0);
  m.valider_journee(date_locale(now), "test", now);
  CHECK(!m.tick(now).has_value());
  bool journalise = false;
  for (const auto& ev : m.journal()) {
    if (ev.motif == Motif::SauteeDst && ev.id == "nuit") journalise = true;
  }
  CHECK(journalise);
}

void automne_deux_occurrences_une_seule_mission() {
  // Le 25 octobre 2026, 02:30 existe deux fois (CEST puis CET, à une heure
  // d'écart). L'idempotence par (id, date locale) n'en laisse passer qu'une.
  const time_t premiere = T(2026, 10, 25, 2, 30, /*isdst=*/1);
  const time_t seconde = T(2026, 10, 25, 2, 30, /*isdst=*/0);
  CHECK(seconde - premiere == 3600);

  Moteur m = moteur_valide({entree_simple("nuit", 2, 30)}, premiere);
  int missions = 0;
  if (m.tick(premiere).has_value()) ++missions;
  if (m.tick(seconde).has_value()) ++missions;
  CHECK(missions == 1);
}

// --- Grâce et rattrapage ---------------------------------------------------

void rattrapage_dans_la_grace() {
  const time_t now = T(2026, 8, 27, 14, 3);
  Moteur m = moteur_valide({entree_simple("apm", 14, 0)}, now);
  const auto mission = m.tick(now);
  CHECK(mission.has_value());
  CHECK(mission->station == 42);
  CHECK(mission->prevu == T(2026, 8, 27, 14, 0));
}

void saut_au_dela_de_la_grace() {
  const time_t now = T(2026, 8, 27, 14, 7);  // grâce par défaut : 5 min
  Moteur m = moteur_valide({entree_simple("apm", 14, 0)}, now);
  CHECK(!m.tick(now).has_value());
  CHECK(!m.journal().empty());
  CHECK(m.journal().back().motif == Motif::SauteeGrace);
  // Et l'occurrence est bel et bien consommée : rien au tick suivant.
  CHECK(!m.tick(now + 10).has_value());
}

void boot_ne_rejoue_pas_la_journee() {
  // Redémarrage à midi : les missions du matin sont sautées, pas rejouées.
  const time_t now = T(2026, 8, 27, 12, 0);
  Moteur m = moteur_valide(
      {entree_simple("m1", 6, 0), entree_simple("m2", 7, 0)}, now);
  CHECK(!m.tick(now).has_value());
  CHECK(!m.tick(now + 1).has_value());
  int sauts = 0;
  for (const auto& ev : m.journal()) {
    if (ev.motif == Motif::SauteeGrace) ++sauts;
  }
  CHECK(sauts == 2);
}

void grace_a_cheval_sur_minuit() {
  // Mission d'hier 23:58, vue à 00:01 : dans la grâce, elle part — sous la
  // validation du JOUR COURANT (la règle §3.2 porte sur la date du jour).
  const time_t now = T(2026, 8, 28, 0, 1);
  Moteur m = moteur_valide({entree_simple("nuit", 23, 58)}, now);
  const auto mission = m.tick(now);
  CHECK(mission.has_value());
  CHECK(mission->prevu == T(2026, 8, 27, 23, 58));
}

// --- Validation quotidienne ------------------------------------------------

void rien_sans_validation() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m;
  m.definir_entrees({entree_simple("mat", 6, 0)});

  // Non validée : rien ne part, mais l'occurrence reste EN ATTENTE tant que
  // la grâce court.
  CHECK(!m.tick(occ + 60).has_value());
  // Validée dans la fenêtre : elle part.
  m.valider_journee(date_locale(occ), "dupont", occ + 120);
  const auto mission = m.tick(occ + 180);
  CHECK(mission.has_value());
  CHECK(m.validation().valide_par == "dupont");
}

void validation_de_la_veille_ne_vaut_pas() {
  const time_t hier = T(2026, 8, 26, 18, 0);
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m;
  m.definir_entrees({entree_simple("mat", 6, 0)});
  m.valider_journee(date_locale(hier), "dupont", hier);  // validation d'HIER

  CHECK(!m.tick(occ + 60).has_value());
  CHECK(!m.tick(occ + 600).has_value());  // au-delà de la grâce
  CHECK(m.journal().back().motif == Motif::SauteeNonValidee);
}

// --- Commandes d'exploitation ----------------------------------------------

void pause_puis_reprise_dans_la_grace() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  m.pause(true);
  CHECK(!m.tick(occ + 60).has_value());
  m.pause(false);
  CHECK(m.tick(occ + 120).has_value());
}

void sauter_la_prochaine_sur_demande() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  m.sauter_prochaine();
  CHECK(!m.tick(occ + 30).has_value());
  CHECK(m.journal().back().motif == Motif::SauteeSurDemande);
  CHECK(!m.tick(occ + 60).has_value());  // consommée, pas simplement différée
}

void priorite_ordonne_les_simultanees() {
  const time_t occ = T(2026, 8, 27, 8, 0);
  Entree urgente = entree_simple("urgente", 8, 0);
  urgente.priorite = 1;
  Entree normale = entree_simple("normale", 8, 0);
  normale.priorite = 5;
  Moteur m = moteur_valide({normale, urgente}, occ);

  const auto premiere = m.tick(occ);
  const auto seconde = m.tick(occ + 1);
  CHECK(premiere.has_value());
  CHECK(premiere->id == "urgente");
  CHECK(seconde.has_value());
  CHECK(seconde->id == "normale");
}

void heure_non_fiable_gele_tout() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);

  // Heure douteuse : rien ne part, rien n'est consommé.
  CHECK(!m.tick(occ + 30, /*heure_fiable=*/false).has_value());
  // L'heure revient dans la grâce : la mission part.
  const auto mission = m.tick(occ + 90, true);
  CHECK(mission.has_value());

  bool gel = false, retour = false;
  for (const auto& ev : m.journal()) {
    if (ev.motif == Motif::HeureNonFiable) gel = true;
    if (ev.motif == Motif::HeureRedevenueFiable) retour = true;
  }
  CHECK(gel);
  CHECK(retour);
}

void entree_suspendue_ignoree() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Entree e = entree_simple("off", 6, 0);
  e.enabled = false;
  Moteur m = moteur_valide({e}, occ);
  CHECK(!m.tick(occ + 60).has_value());
  // Ni exécutée, ni « sautée » : une entrée suspendue n'existe pas.
  for (const auto& ev : m.journal()) {
    CHECK(ev.motif == Motif::JourneeValidee);
  }
}

void invalidation_revoque_l_autorisation() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  m.invalider_journee();
  CHECK(!m.tick(occ + 60).has_value());
  CHECK(!m.journee_validee(occ + 60));
}

// --- Arrêt de sécurité pendant un déplacement ------------------------------

void interruption_en_deplacement_leve_une_alerte() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  const auto mission = m.tick(occ);
  CHECK(mission.has_value());
  CHECK(m.mission_en_cours());

  m.interruption_agv(/*en_deplacement=*/true, occ + 30);
  CHECK(m.alerte().active);
  CHECK(m.alerte().mission == "mat");
  CHECK(m.alerte().station == 42);
  CHECK(!m.mission_en_cours());
  CHECK(m.journal().back().motif == Motif::MissionInterrompue);
}

void interruption_a_l_arret_ne_leve_rien() {
  // LA règle demandée : hors déplacement, une coupure est banale — fin de
  // poste, maintenance, quelqu'un devant un AGV à quai. Crier au loup à chaque
  // fois viderait l'alerte de son sens.
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  m.tick(occ);
  m.interruption_agv(/*en_deplacement=*/false, occ + 30);
  CHECK(!m.alerte().active);
  CHECK(m.journal().back().motif == Motif::ArretHorsDeplacement);
}

void arrivee_confirmee_ferme_la_mission() {
  const time_t occ = T(2026, 8, 27, 6, 0);
  Moteur m = moteur_valide({entree_simple("mat", 6, 0)}, occ);
  m.tick(occ);
  m.mission_arrivee(occ + 200);
  CHECK(!m.mission_en_cours());
  CHECK(m.journal().back().motif == Motif::MissionArrivee);
  // Une coupure APRÈS l'arrivée n'est plus une interruption de trajet.
  m.interruption_agv(true, occ + 400);
  CHECK(m.alerte().active);          // le drapeau se lève quand même…
  CHECK(m.alerte().mission.empty()); // …mais sans mission à incriminer
}

void l_alerte_bloque_les_departs_autonomes() {
  // Repartir seul vers l'obstacle qui vient d'arrêter l'AGV est une boucle
  // que personne ne surveille.
  const time_t occ1 = T(2026, 8, 27, 6, 0);
  const time_t occ2 = T(2026, 8, 27, 7, 0);
  Moteur m = moteur_valide({entree_simple("a", 6, 0), entree_simple("b", 7, 0)}, occ1);
  m.tick(occ1);
  m.interruption_agv(true, occ1 + 30);

  CHECK(!m.tick(occ2).has_value());
  CHECK(!m.tick(occ2 + 600).has_value());   // au-delà de la grâce : saut motivé
  CHECK(m.journal().back().motif == Motif::SauteeAlerte);
}

void l_acquittement_rend_la_main() {
  const time_t occ1 = T(2026, 8, 27, 6, 0);
  const time_t occ2 = T(2026, 8, 27, 7, 0);
  Moteur m = moteur_valide({entree_simple("a", 6, 0), entree_simple("b", 7, 0)}, occ1);
  m.tick(occ1);
  m.interruption_agv(true, occ1 + 30);

  CHECK(!m.acquitter_alerte("", occ1 + 60));       // un nom est exigé
  CHECK(m.acquitter_alerte("dupont", occ1 + 60));  // traçabilité
  CHECK(!m.alerte().active);
  CHECK(m.tick(occ2).has_value());                 // le planning repart
}

void chevauchement_signale_dans_la_liste() {
  // Un trajet dure au plus 5 min : deux départs à 3 min d'intervalle se
  // chevauchent, et le second sera refusé par le séquenceur (§5).
  Moteur m;
  m.definir_entrees({entree_simple("a", 8, 0), entree_simple("b", 8, 3),
                     entree_simple("c", 9, 0)});
  const auto liste = m.prochaines(T(2026, 8, 27, 7, 0), 3);
  CHECK(liste.size() == 3);
  CHECK(!liste[0].conflit);   // 08:00, rien avant
  CHECK(liste[1].conflit);    // 08:03, l'AGV est encore en route
  CHECK(!liste[2].conflit);   // 09:00, largement après
}

void liste_des_prochaines_occurrences() {
  Moteur m;
  m.definir_entrees(
      {entree_simple("mat", 6, 0, jour::kOuvres), entree_simple("apm", 14, 0, jour::kOuvres)});
  // Jeudi 27 août 2026, 12:00 : 14:00 du jour, puis 06:00 et 14:00 du vendredi.
  const auto liste = m.prochaines(T(2026, 8, 27, 12, 0), 3);
  CHECK(liste.size() == 3);
  CHECK(liste[0].quand == T(2026, 8, 27, 14, 0));
  CHECK(liste[1].quand == T(2026, 8, 28, 6, 0));
  CHECK(liste[2].quand == T(2026, 8, 28, 14, 0));
}

}  // namespace

int main() {
  // Europe/Paris, règles DST comprises — identique à la cible (§2.4).
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  RUN(jours_ouvres_seulement);
  RUN(bornes_de_validite);
  RUN(exception_exclut_la_date);
  RUN(printemps_execute_au_premier_instant);
  RUN(printemps_politique_saut);
  RUN(automne_deux_occurrences_une_seule_mission);
  RUN(rattrapage_dans_la_grace);
  RUN(saut_au_dela_de_la_grace);
  RUN(boot_ne_rejoue_pas_la_journee);
  RUN(grace_a_cheval_sur_minuit);
  RUN(rien_sans_validation);
  RUN(validation_de_la_veille_ne_vaut_pas);
  RUN(pause_puis_reprise_dans_la_grace);
  RUN(sauter_la_prochaine_sur_demande);
  RUN(priorite_ordonne_les_simultanees);
  RUN(heure_non_fiable_gele_tout);
  RUN(entree_suspendue_ignoree);
  RUN(invalidation_revoque_l_autorisation);
  RUN(interruption_en_deplacement_leve_une_alerte);
  RUN(interruption_a_l_arret_ne_leve_rien);
  RUN(arrivee_confirmee_ferme_la_mission);
  RUN(l_alerte_bloque_les_departs_autonomes);
  RUN(l_acquittement_rend_la_main);
  RUN(chevauchement_signale_dans_la_liste);
  RUN(liste_des_prochaines_occurrences);

  std::printf("\n%d tests, %d echec(s)\n", g_tests, g_echecs);
  return g_echecs == 0 ? 0 : 1;
}
