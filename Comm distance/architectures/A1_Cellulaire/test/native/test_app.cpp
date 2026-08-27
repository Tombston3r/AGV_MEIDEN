// Tests applicatifs : persistance de la file, chien de garde de liaison,
// idempotence de bout en bout et compatibilité agvdump (brief §11).
#include <cstring>
#include <deque>
#include <string>

#include "app/agv_app.h"
#include "app/alert_gateway.h"
#include "app/course_queue.h"
#include "app/persistent_store.h"
#include "app/sequencer.h"
#include "fakes.h"
#include "sim_bus_driver.h"
#include "test_framework.h"

using namespace agv;
using agv::sim::SimBusDriver;
using agv::sim::TimingProfile;

namespace {

// Transport en boucle : ce que l'AGV émet est capturé, ce que le test injecte
// est remis à l'application.
class LoopTransport final : public ITransport {
 public:
  explicit LoopTransport(bool ordered_transport, uint32_t max_age_s = 0)
      : ordered_(ordered_transport), max_age_s_(max_age_s) {}

  bool begin() override { return true; }
  bool send(const Frame& f) override {
    sent.push_back(f);
    return true;
  }
  bool poll(Frame& out) override {
    if (inbox.empty()) return false;
    out = inbox.front();
    inbox.pop_front();
    return true;
  }
  LinkHealth health() const override { return health_; }
  const char* name() const override { return "loop"; }
  bool ordered() const override { return ordered_; }
  uint32_t max_command_age_s() const override { return max_age_s_; }

  void deliver(const Frame& f) { inbox.push_back(f); }
  size_t ack_count(bool positive) const {
    size_t n = 0;
    for (const auto& f : sent) {
      if (f.type != FrameType::Ack) continue;
      const bool nack = (f.flags & flag::kNack) != 0;
      if (nack != positive) ++n;
    }
    return n;
  }

  std::deque<Frame> inbox;
  std::vector<Frame> sent;

 private:
  LinkHealth health_{};
  bool ordered_;
  uint32_t max_age_s_;
};

struct AppBench {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  RamStore store;
  TimingProfile timings = TimingProfile::fast();
  LoopTransport transport;

  SimBusDriver* bus = nullptr;
  CourseQueue* queue = nullptr;
  Sequencer* seq = nullptr;
  AgvApp* app = nullptr;

  explicit AppBench(bool ordered_transport = true, uint32_t max_age_s = 0)
      : transport(ordered_transport, max_age_s) {}

  void build() {
    clock.set_wall_s(1'700'000'000u);
    bus = new SimBusDriver(profile, timings);
    queue = new CourseQueue(profile.queue, &store);
    seq = new Sequencer(profile, *bus, *queue);
    app = new AgvApp(profile, clock, transport, *seq, *queue);
    app->set_telemetry_period_ms(0);  // désactivée sauf test dédié
    app->begin();
  }
  ~AppBench() {
    delete app;
    delete queue;
    delete seq;
    delete bus;
  }

  void run(uint32_t duration_ms) {
    for (uint32_t t = 0; t < duration_ms; ++t) {
      app->tick();
      clock.advance_ms(1);
      bus->advance(1000);
    }
    app->tick();
  }

  Frame goto_frame(uint8_t seq_no, uint16_t station, uint8_t flags = 0) const {
    Frame f;
    f.ver = profile.protocol.version;
    f.type = FrameType::CmdGoto;
    f.node_id = 77;
    f.seq = seq_no;
    f.station = station;
    f.speed = 4;
    f.flags = flags;
    f.ts_s = clock.now_s();
    if (f.ts_s != 0) f.flags |= flag::kTimestamped;
    return f;
  }
};

}  // namespace

// --- File de courses --------------------------------------------------------

TEST(file_persistee_est_restauree_au_boot) {
  // La V5.0.1 perdait la file à chaque coupure : c'est l'amélioration du §4.5.
  HardwareProfile profile = default_profile();
  RamStore store;
  {
    CourseQueue q(profile.queue, &store);
    Course c;
    c.station = 12;
    c.speed = 5;
    c.node_id = 3;
    c.enqueued_at_s = 1000;
    q.push(c);
    c.station = 34;
    q.push(c);
    CHECK(q.save(1000));
  }
  CourseQueue restored(profile.queue, &store);
  size_t dropped = 0;
  CHECK_EQ(restored.restore(1100, &dropped), 2u);
  CHECK_EQ(dropped, 0u);
  Course out;
  CHECK(restored.pop(out));
  CHECK_EQ(out.station, 12u);  // ordre préservé
}

TEST(course_perimee_est_ecartee_au_boot) {
  HardwareProfile profile = default_profile();
  profile.queue.course_validity_min = 30;  // paramètre, pas constante
  RamStore store;
  {
    CourseQueue q(profile.queue, &store);
    Course c;
    c.station = 12;
    c.enqueued_at_s = 1000;
    q.push(c);
    c.station = 13;
    c.enqueued_at_s = 5000;
    q.push(c);
    q.save(5000);
  }
  CourseQueue restored(profile.queue, &store);
  size_t dropped = 0;
  // 1 h plus tard : la course de t=1000 a plus de 30 min, celle de t=5000 non.
  CHECK_EQ(restored.restore(5000 + 1500, &dropped), 1u);
  CHECK_EQ(dropped, 1u);
  Course out;
  restored.pop(out);
  CHECK_EQ(out.station, 13u);
}

TEST(file_persistee_corrompue_donne_une_file_vide) {
  HardwareProfile profile = default_profile();
  RamStore store;
  {
    CourseQueue q(profile.queue, &store);
    Course c;
    c.station = 7;
    c.enqueued_at_s = 10;
    q.push(c);
    q.save(10);
  }
  uint8_t blob[128];
  const size_t len = store.read("courses", blob, sizeof(blob));
  blob[2] ^= 0xFF;
  store.write("courses", blob, len);

  CourseQueue restored(profile.queue, &store);
  CHECK_EQ(restored.restore(20, nullptr), 0u);  // jamais de course inventée
}

TEST(sans_horloge_murale_la_validite_ne_peut_pas_trancher) {
  HardwareProfile profile = default_profile();
  RamStore store;
  {
    CourseQueue q(profile.queue, &store);
    Course c;
    c.station = 7;
    c.enqueued_at_s = 1000;
    q.push(c);
    q.save(1000);
  }
  CourseQueue restored(profile.queue, &store);
  size_t dropped = 0;
  // now_s == 0 : horloge non sûre. On restaure plutôt que d'écarter à tort.
  CHECK_EQ(restored.restore(0, &dropped), 1u);
  CHECK_EQ(dropped, 0u);
}

TEST(course_prioritaire_passe_en_tete_de_file) {
  HardwareProfile profile = default_profile();
  RamStore store;
  CourseQueue q(profile.queue, &store);
  Course a;
  a.station = 1;
  Course b;
  b.station = 2;
  q.push(a);
  q.push_front(b);
  Course out;
  q.pop(out);
  CHECK_EQ(out.station, 2u);
}

// --- Application ------------------------------------------------------------

TEST(commande_goto_est_empilee_et_acquittee) {
  AppBench b;
  b.build();
  b.transport.deliver(b.goto_frame(1, 9));
  b.run(5);

  CHECK_EQ(b.app->stats().cmd_accepted, 1u);
  CHECK_EQ(b.transport.ack_count(true), 1u);
  b.run(3000);
  CHECK_EQ(b.bus->position(), 9u);
}

TEST(commande_rejouee_est_reacquittee_sans_seconde_course) {
  // Le scénario exact du §5.1 : l'ACK est perdu, l'émetteur retransmet.
  AppBench b;
  b.build();
  const Frame cmd = b.goto_frame(5, 4);
  b.transport.deliver(cmd);
  b.run(5);
  b.transport.deliver(cmd);  // même (node_id, seq)
  b.run(5);

  CHECK_EQ(b.app->stats().cmd_accepted, 1u);
  CHECK_EQ(b.app->stats().cmd_duplicate, 1u);
  CHECK_EQ(b.transport.ack_count(true), 2u);  // ré-acquitté deux fois
  b.run(3000);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);  // UNE seule course
}

TEST(file_pleine_repond_par_un_nack) {
  AppBench b;
  b.build();
  for (uint8_t i = 0; i < 7; ++i) {
    b.transport.deliver(b.goto_frame(static_cast<uint8_t>(i + 1), static_cast<uint16_t>(i + 1)));
    b.app->tick();
  }
  CHECK(b.app->stats().cmd_rejected_full > 0u);
  CHECK(b.transport.ack_count(false) > 0u);
}

TEST(commande_perimee_ne_fait_pas_bouger_l_agv) {
  // Transport non ordonné avec contrôle de fraîcheur : cas SMS (§8.1).
  AppBench b(/*ordered=*/false, /*max_age_s=*/15);
  b.build();
  Frame old = b.goto_frame(3, 6);
  old.ts_s = b.clock.now_s() - 180;  // sortie du SMSC 3 minutes plus tard
  b.transport.deliver(old);
  b.run(2000);

  CHECK_EQ(b.app->stats().cmd_expired, 1u);
  CHECK_EQ(b.app->stats().cmd_accepted, 0u);
  CHECK_EQ(b.bus->position(), 0u);
  CHECK(b.transport.ack_count(false) > 0u);
}

TEST(stop_arrive_avant_le_goto_est_rejete_sur_transport_non_ordonne) {
  AppBench b(/*ordered=*/false, /*max_age_s=*/15);
  b.build();
  b.transport.deliver(b.goto_frame(10, 5));
  b.run(5);
  Frame stop = b.goto_frame(9, 0);  // séquence ANTÉRIEURE
  stop.type = FrameType::CmdStop;
  b.transport.deliver(stop);
  b.run(5);

  CHECK_EQ(b.app->stats().cmd_out_of_order, 1u);
  CHECK(b.seq->state() != SeqState::SafeStop);
}

TEST(chien_de_garde_de_liaison_declenche_l_arret_sur) {
  AppBench b;
  b.build();
  b.transport.deliver(b.goto_frame(1, 4));
  b.run(10);
  CHECK(b.app->link_up());

  // Plus aucune trame pendant link_watchdog_s (paramètre, pas constante).
  b.run(b.profile.safety.link_watchdog_s * 1000u + 500u);

  CHECK(!b.app->link_up());
  CHECK_EQ(b.app->stats().link_losses, 1u);
  CHECK(b.seq->state() == SeqState::SafeStop);
  // La course engagée est allée jusqu'au point d'arrêt : pas d'état indéterminé.
  CHECK_EQ(b.bus->position(), 4u);
}

TEST(chien_de_garde_utilise_la_valeur_du_profil) {
  AppBench b;
  b.profile.safety.link_watchdog_s = 5;  // au lieu de 30
  b.build();
  b.transport.deliver(b.goto_frame(1, 2));
  b.run(10);
  b.run(6000);
  CHECK(!b.app->link_up());

  AppBench slow;
  slow.profile.safety.link_watchdog_s = 60;
  slow.build();
  slow.transport.deliver(slow.goto_frame(1, 2));
  slow.run(10);
  slow.run(6000);
  CHECK(slow.app->link_up());
}

TEST(telemetrie_periodique_est_emise) {
  AppBench b;
  b.build();
  b.app->set_telemetry_period_ms(100);
  b.run(450);
  CHECK(b.app->stats().telemetry_sent >= 4u);
  bool found = false;
  for (const auto& f : b.transport.sent) {
    if (f.type == FrameType::Telemetry) found = true;
  }
  CHECK(found);
}

TEST(agvdump_expose_les_champs_attendus_par_l_atelier) {
  // §3.3 : sans ces noms, les procédures d'atelier du client deviennent
  // caduques.
  AppBench b;
  b.build();
  b.transport.deliver(b.goto_frame(1, 3));
  b.run(50);

  char buf[4096];
  const size_t len = b.app->render_agvdump(buf, sizeof(buf));
  CHECK(len > 0u);
  const std::string dump(buf, len);
  for (const char* field :
       {"AGV STATE", "write_tries", "write_op_return", "start_tries", "start_op_return",
        "stop_tries", "stop_op_return", "current_station", "nb_courses_programmed",
        "programmed_courses[0]", "programmed_courses[4]"}) {
    CHECK(dump.find(field) != std::string::npos);
  }
}

TEST(agvdump_tronque_proprement_sur_un_petit_tampon) {
  AppBench b;
  b.build();
  char buf[64];
  const size_t len = b.app->render_agvdump(buf, sizeof(buf));
  CHECK(len < sizeof(buf));
  CHECK_EQ(buf[len], '\0');
}

// --- Passerelle d'alerte ----------------------------------------------------

TEST(alerte_sms_respecte_le_quota_journalier) {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  agv::test::FakeUart uart(clock);
  AlertGateway gw(profile.cellular, uart);

  uint32_t sent = 0;
  for (uint32_t i = 0; i < profile.cellular.alerts_per_day_max + 3; ++i) {
    if (gw.raise(AlertKind::BlockingFault, "Y03", 1000)) {
      ++sent;
      // Déroulement complet de l'échange AT.
      for (int k = 0; k < 6; ++k) {
        gw.tick();
        uart.inject("\r\nOK\r\n");
        gw.tick();
        uart.inject("\r\n> ");
        gw.tick();
        uart.inject("\r\nOK\r\n");
        gw.tick();
      }
    }
  }
  CHECK_EQ(sent, profile.cellular.alerts_per_day_max);
  CHECK(gw.suppressed() >= 3u);
}
