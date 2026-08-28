// Tests d'intégration du séquenceur contre le simulateur d'automate (§11).
//
// Couvre les quatre phases, tous les chemins de timeout, la dégradation et la
// dépendance effective aux paramètres §12 (aucun n'est figé dans la logique).
#include "app/course_queue.h"
#include "app/persistent_store.h"
#include "app/sequencer.h"
#include "sim_bus_driver.h"
#include "test_framework.h"
#include "timing_profile.h"

using namespace agv;
using agv::sim::SimBusDriver;
using agv::sim::TimingProfile;

namespace {

// Banc d'essai : profil modifiable, simulateur, file et séquenceur assemblés.
struct Bench {
  HardwareProfile profile = default_profile();
  RamStore store;
  TimingProfile timings = TimingProfile::fast();

  // Construction différée : le profil doit pouvoir être ajusté avant `build()`.
  SimBusDriver* bus = nullptr;
  CourseQueue* queue = nullptr;
  Sequencer* seq = nullptr;

  void build() {
    bus = new SimBusDriver(profile, timings);
    queue = new CourseQueue(profile.queue, &store);
    seq = new Sequencer(profile, *bus, *queue);
    seq->begin();
  }
  ~Bench() {
    delete seq;
    delete queue;
    delete bus;
  }

  // Fait tourner le séquenceur en temps simulé. Pas de 100 µs : très en deçà
  // du plus court paramètre temporel du profil.
  void run(uint64_t duration_us) {
    for (uint64_t t = 0; t < duration_us; t += 100) {
      seq->tick();
      bus->advance(100);
    }
    seq->tick();
  }

  void enqueue(uint16_t station, uint8_t speed = 4, uint8_t seq_no = 1) {
    Course c;
    c.station = station;
    c.speed = speed;
    c.node_id = 1;
    c.seq = seq_no;
    c.enqueued_at_s = 1000;
    queue->push(c);
  }
};

}  // namespace

TEST(course_simple_les_quatre_phases_aboutissent) {
  Bench b;
  b.build();
  b.bus->set_position(0);
  b.enqueue(5, 4);

  b.run(5000000);  // 5 s simulées

  CHECK(b.seq->state() == SeqState::Idle);
  CHECK(b.seq->fault_cause() == FaultCause::None);
  CHECK_EQ(b.bus->position(), 5u);
  CHECK_EQ(b.seq->counters().current_station, 5u);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
  CHECK(b.seq->counters().write_op_return == OpReturn::Ok);
  CHECK(b.seq->counters().start_op_return == OpReturn::Ok);
  CHECK(b.seq->counters().stop_op_return == OpReturn::Ok);
  CHECK_EQ(b.seq->counters().write_tries, 1u);
  CHECK_EQ(b.seq->counters().start_tries, 1u);
  CHECK_EQ(b.bus->setup_violations(), 0u);  // t_setup toujours respecté
}

TEST(file_de_cinq_courses_enchainees_dans_l_ordre) {
  Bench b;
  b.build();
  b.bus->set_position(0);
  for (uint8_t i = 1; i <= 5; ++i) b.enqueue(i, 4, i);
  CHECK_EQ(b.queue->size(), 5u);

  b.run(20000000);

  CHECK_EQ(b.seq->counters().courses_completed, 5u);
  CHECK_EQ(b.bus->position(), 5u);
  CHECK(b.queue->empty());
  CHECK(b.seq->state() == SeqState::Idle);
}

TEST(file_pleine_refuse_la_sixieme_course) {
  Bench b;
  b.build();
  for (uint8_t i = 0; i < 5; ++i) b.enqueue(static_cast<uint16_t>(i + 1), 4, i);
  Course extra;
  extra.station = 9;
  CHECK(!b.queue->push(extra));
  CHECK_EQ(b.queue->size(), 5u);
}

TEST(timeout_Y22_declenche_des_reessais_puis_un_defaut) {
  Bench b;
  b.timings.drop_every_nth_y22 = 1;  // l'automate n'accuse jamais
  b.build();
  b.enqueue(3);

  b.run(5000000);

  CHECK(b.seq->state() == SeqState::Fault);
  CHECK(b.seq->fault_cause() == FaultCause::WriteTimeout);
  CHECK(b.seq->counters().write_op_return == OpReturn::Timeout);
  CHECK_EQ(b.seq->counters().write_tries, b.profile.timeouts.write_max_tries);
  CHECK_EQ(b.seq->counters().y22_timeouts, b.profile.timeouts.write_max_tries);
  // État sûr : toutes les sorties au repos.
  CHECK_EQ(b.bus->lastX(), 0u);
}

TEST(un_accuse_Y22_sur_deux_perdu_reste_rattrapable) {
  Bench b;
  b.timings.drop_every_nth_y22 = 2;  // un strobe sur deux ignoré
  b.build();
  b.enqueue(2);

  b.run(5000000);

  CHECK(b.seq->fault_cause() == FaultCause::None);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
  CHECK(b.seq->counters().write_tries >= 1u);
}

TEST(timeout_Y05_declenche_un_defaut_de_demarrage) {
  Bench b;
  b.timings.drop_every_nth_y05 = 1;  // le moving flag ne monte jamais
  b.build();
  b.enqueue(4);

  b.run(20000000);

  CHECK(b.seq->state() == SeqState::Fault);
  CHECK(b.seq->fault_cause() == FaultCause::StartTimeout);
  CHECK(b.seq->counters().start_op_return == OpReturn::Timeout);
  CHECK_EQ(b.bus->lastX(), 0u);
}

TEST(defaut_automate_Y03_met_en_defaut_immediatement) {
  Bench b;
  b.build();
  b.enqueue(3);
  b.run(500000);
  b.bus->timings().force_fault_y03 = true;
  b.run(500000);

  CHECK(b.seq->state() == SeqState::Fault);
  CHECK(b.seq->fault_cause() == FaultCause::PlcFault);
  CHECK_EQ(b.bus->lastX(), 0u);
}

TEST(acquittement_de_defaut_permet_de_repartir) {
  Bench b;
  b.timings.drop_every_nth_y22 = 1;
  b.build();
  b.enqueue(3);
  b.run(3000000);
  CHECK(b.seq->state() == SeqState::Fault);

  b.bus->timings().drop_every_nth_y22 = 0;  // l'automate répond de nouveau
  b.seq->clear_fault();
  b.enqueue(3, 4, 2);
  b.run(5000000);

  CHECK(b.seq->fault_cause() == FaultCause::None);
  CHECK_EQ(b.bus->position(), 3u);
}

TEST(arret_sur_perte_de_liaison_termine_la_course_en_cours) {
  // §3.1 : arrêt sûr au point d'arrêt SUIVANT, jamais d'état indéterminé.
  Bench b;
  b.build();
  b.bus->set_position(0);
  b.enqueue(3, 4, 1);
  b.enqueue(7, 4, 2);

  b.run(100000);  // l'AGV est en route vers la station 3
  CHECK(b.seq->state() == SeqState::Transit);
  b.seq->request_safe_stop(FaultCause::LinkLost);
  b.run(10000000);

  CHECK(b.seq->state() == SeqState::SafeStop);
  CHECK_EQ(b.bus->position(), 3u);       // la course en cours est allée au bout
  CHECK_EQ(b.queue->size(), 1u);         // la suivante n'a PAS été lancée
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
  CHECK_EQ(b.bus->lastX(), 0u);
}

TEST(automate_lent_le_sequenceur_tient_avec_des_timeouts_adaptes) {
  // Le simulateur doit pouvoir rejouer un automate lent (brief §10). Avec les
  // timeouts par défaut, un automate lent échoue : c'est le résultat attendu,
  // et c'est exactement pourquoi le §12.5 doit être relevé et non deviné.
  Bench slow_default;
  slow_default.timings = TimingProfile::slow();
  slow_default.build();
  slow_default.enqueue(2);
  slow_default.run(30000000);
  CHECK(slow_default.seq->counters().courses_completed == 0u);

  // Avec des timeouts issus du profil et non du code, le même automate passe.
  Bench slow_tuned;
  slow_tuned.timings = TimingProfile::slow();
  slow_tuned.profile.timeouts.y22_write_ack_ms = 1000;
  slow_tuned.profile.timeouts.y05_start_ack_ms = 5000;
  slow_tuned.profile.timeouts.y10_arrival_ms = 300000;
  slow_tuned.build();
  slow_tuned.enqueue(2);
  slow_tuned.run(30000000);
  CHECK_EQ(slow_tuned.seq->counters().courses_completed, 1u);
}

TEST(t_setup_trop_court_fait_perdre_la_premiere_ecriture) {
  // §12.4 : un strobe posé avant stabilisation est ignoré par l'automate. La
  // première écriture est perdue et rattrapée par le réessai, au prix d'un
  // aller-retour de plus. C'est exactement le coût d'un t_setup mal réglé.
  Bench b;
  b.timings.required_setup_us = 500;   // l'automate exige 500 µs
  b.profile.bus.t_setup_us = 100;      // le firmware n'en attend que 100
  b.build();
  b.enqueue(2);
  b.run(5000000);

  CHECK(b.bus->setup_violations() > 0u);
  CHECK(b.seq->counters().write_tries > 1u);
  CHECK_EQ(b.seq->counters().y22_timeouts, 1u);
}

TEST(t_setup_conforme_au_profil_passe) {
  Bench b;
  b.timings.required_setup_us = 500;
  b.profile.bus.t_setup_us = 600;  // paramètre relevé, pas une constante
  b.build();
  b.enqueue(2);
  b.run(5000000);

  CHECK_EQ(b.bus->setup_violations(), 0u);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
}

TEST(logique_NPN_inverse_toutes_les_voies_X) {
  // §12.3 : bus_x_active_high est un paramètre. En logique inverse, le mot
  // électrique au repos est le complément, et la séquence doit rester valide.
  Bench b;
  b.profile.bus.x_active_high = false;
  b.profile.bus.y_active_high = false;
  b.build();
  b.bus->set_position(0);
  b.enqueue(6);

  b.run(5000000);

  CHECK_EQ(b.bus->position(), 6u);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
  // Au repos, les signaux de commande sont électriquement hauts (= logique 0
  // en NPN). Les lignes d'adresse et de vitesse conservent la dernière valeur
  // écrite, comme sur le firmware d'origine : seules les commandes retombent.
  const uint32_t last = b.bus->lastX();
  CHECK(bit_set(last, x::X92));
  CHECK(bit_set(last, x::X93));
  CHECK(bit_set(last, x::X82));
  CHECK(bit_set(last, x::X83));
}

TEST(rebonds_sur_les_entrees_Y_ne_perturbent_pas_la_sequence) {
  Bench b;
  b.timings.y_bounce_us = 1500;
  b.profile.bus.y_debounce_us = 3000;  // §12.1, paramétrable
  b.build();
  b.enqueue(2);
  b.run(8000000);

  CHECK_EQ(b.seq->counters().courses_completed, 1u);
  CHECK(b.seq->fault_cause() == FaultCause::None);
}

TEST(bus_a_zero_au_demarrage_avant_toute_commande) {
  // §3.1 : à la mise sous tension et après tout reset, le bus X est à zéro.
  Bench b;
  b.build();
  CHECK_EQ(b.bus->lastX(), 0u);
  CHECK(b.seq->state() == SeqState::Idle);
  b.run(1000000);
  CHECK_EQ(b.bus->lastX(), 0u);  // file vide : aucune sortie activée
}

TEST(compteur_nb_courses_programmed_reflete_la_file) {
  Bench b;
  b.build();
  b.enqueue(1, 4, 1);
  b.enqueue(2, 4, 2);
  b.seq->tick();
  CHECK_EQ(b.seq->counters().nb_courses_programmed, 2u);
  b.run(6000000);
  CHECK_EQ(b.seq->counters().nb_courses_programmed, 0u);
  CHECK_EQ(b.seq->counters().courses_completed, 2u);
}
