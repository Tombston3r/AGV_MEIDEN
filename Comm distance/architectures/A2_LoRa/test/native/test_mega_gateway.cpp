// Applications des deux microcontrôleurs de la carte V5.0.1.
//
// Le test central de cette architecture est le REPLI HEARTBEAT : l'ATmega doit
// s'arrêter au point d'arrêt suivant et refuser toute course quand l'ESP32 se
// tait, sans dépendre du Wi-Fi ni du poste fixe (planification §2).
#include <string>
#include <vector>

#include "app/agvdump.h"
#include "app/course_queue.h"
#include "app/lora_gateway_app.h"
#include "app/mega_app.h"
#include "app/persistent_store.h"
#include "app/sequencer.h"
#include "proto/json.h"
#include "sim_bus_driver.h"
#include "test_framework.h"
#include "timing_profile.h"

using namespace agv;
using agv::sim::SimBusDriver;
using agv::sim::TimingProfile;

namespace {

// --- Banc ATmega -----------------------------------------------------------

class CaptureWriter final : public ILinkWriter {
 public:
  void write(const uint8_t* data, size_t len) override {
    for (size_t i = 0; i < len; ++i) bytes.push_back(data[i]);
  }
  // Décode toutes les trames émises vers l'ESP32.
  std::vector<std::pair<link::Cmd, std::vector<uint8_t>>> frames() {
    std::vector<std::pair<link::Cmd, std::vector<uint8_t>>> out;
    link::Parser parser(link::kSofToEsp);
    link::Cmd cmd;
    uint8_t payload[link::kPayloadMax];
    uint8_t len = 0;
    for (uint8_t b : bytes) {
      if (parser.feed(b, cmd, payload, len)) {
        out.emplace_back(cmd, std::vector<uint8_t>(payload, payload + len));
      }
    }
    return out;
  }
  std::vector<uint8_t> bytes;
};

struct MegaBench {
  HardwareProfile profile = default_profile();
  RamStore store;
  TimingProfile timings = TimingProfile::fast();
  CaptureWriter writer;

  SimBusDriver* bus = nullptr;
  CourseQueue* queue = nullptr;
  Sequencer* seq = nullptr;
  MegaApp* app = nullptr;
  uint32_t now_ms = 0;

  void build() {
    bus = new SimBusDriver(profile, timings);
    queue = new CourseQueue(profile.queue, &store);
    seq = new Sequencer(profile, *bus, *queue);
    app = new MegaApp(profile, *seq, *queue, writer);
    app->begin(now_ms);
  }
  ~MegaBench() {
    delete app;
    delete queue;
    delete seq;
    delete bus;
  }

  void feed_frame(const uint8_t* frame, size_t n) {
    for (size_t i = 0; i < n; ++i) app->feed(frame[i], now_ms);
  }
  void heartbeat() {
    uint8_t frame[link::kFrameMax];
    const size_t n = link::encode(link::kSofToMega, link::Cmd::Heartbeat, nullptr, 0, frame,
                                  sizeof(frame));
    feed_frame(frame, n);
  }
  void send_goto(uint8_t seq_no, uint16_t station, uint8_t speed = 4, uint8_t flags = 0) {
    uint8_t frame[link::kFrameMax];
    const size_t n = link::encode_goto(seq_no, station, speed, flags, frame, sizeof(frame));
    feed_frame(frame, n);
  }
  // Fait tourner l'ATmega ET le heartbeat, comme en fonctionnement nominal.
  void run(uint32_t duration_ms, bool with_heartbeat = true) {
    for (uint32_t t = 0; t < duration_ms; ++t) {
      if (with_heartbeat && (now_ms % profile.heartbeat.period_ms) == 0) heartbeat();
      app->tick(now_ms);
      bus->advance(1000);
      ++now_ms;
    }
  }
  link::CmdResult last_ack() {
    link::CmdResult r = link::CmdResult::BadPayload;
    for (const auto& f : writer.frames()) {
      if (f.first == link::Cmd::Ack && f.second.size() >= 2) {
        r = static_cast<link::CmdResult>(f.second[1]);
      }
    }
    return r;
  }
};

// --- Banc ESP32 ------------------------------------------------------------

// Radio factice : capte ce que la passerelle émet, injecte ce qu'elle reçoit.
class FakeRadio final : public ITransport {
 public:
  bool begin() override { return true; }
  bool send(const Frame& f) override {
    if (!budget_) {          // budget de rapport cyclique épuisé
      ++refused;
      return false;
    }
    sent.push_back(f);
    return true;
  }
  bool poll(Frame& out) override {
    if (inbox_.empty()) return false;
    out = inbox_.front();
    inbox_.erase(inbox_.begin());
    return true;
  }
  LinkHealth health() const override { return health_; }
  const char* name() const override { return "fake-lora"; }
  void tick() override { ++ticks; }

  void receive(const Frame& f) { inbox_.push_back(f); }
  void set_budget(bool ok) { budget_ = ok; }
  size_t count(FrameType t) const {
    size_t n = 0;
    for (const auto& f : sent) {
      if (f.type == t) ++n;
    }
    return n;
  }

  std::vector<Frame> sent;
  uint32_t refused = 0;
  uint32_t ticks = 0;

 private:
  std::vector<Frame> inbox_;
  LinkHealth health_{};
  bool budget_ = true;
};

class CaptureLink final : public ILinkPort {
 public:
  void write(const uint8_t* data, size_t len) override {
    for (size_t i = 0; i < len; ++i) bytes.push_back(data[i]);
  }
  std::vector<std::pair<link::Cmd, std::vector<uint8_t>>> frames() {
    std::vector<std::pair<link::Cmd, std::vector<uint8_t>>> out;
    link::Parser parser(link::kSofToMega);
    link::Cmd cmd;
    uint8_t payload[link::kPayloadMax];
    uint8_t len = 0;
    for (uint8_t b : bytes) {
      if (parser.feed(b, cmd, payload, len)) {
        out.emplace_back(cmd, std::vector<uint8_t>(payload, payload + len));
      }
    }
    return out;
  }
  size_t count(link::Cmd wanted) {
    size_t n = 0;
    for (const auto& f : frames()) {
      if (f.first == wanted) ++n;
    }
    return n;
  }
  std::vector<uint8_t> bytes;
};

struct GatewayBench {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeRadio radio;
  CaptureLink link_port;
  LoraGatewayApp app{profile, clock, radio, link_port};

  GatewayBench() {
    clock.set_wall_s(1'700'000'000u);
    app.begin();
  }
  void run(uint32_t duration_ms, uint32_t step_ms = 10) {
    for (uint32_t t = 0; t < duration_ms; t += step_ms) {
      app.tick();
      clock.advance_ms(step_ms);
    }
    app.tick();
  }
  // Une commande arrivant par la radio, comme depuis un bouton ou le poste.
  Frame goto_frame(uint8_t seq, uint16_t station, uint8_t speed = 4,
                   uint16_t node = 0x0042) {
    Frame f;
    f.type = FrameType::CmdGoto;
    f.node_id = node;
    f.seq = seq;
    f.station = station;
    f.speed = speed;
    return f;
  }
  // Réponse de l'ATmega, injectée octet par octet.
  void mega_ack(uint8_t seq, link::CmdResult r) {
    uint8_t buf[link::kFrameMax];
    const size_t n = link::encode_ack(seq, r, buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) app.on_link_byte(buf[i]);
  }
  void mega_state(const link::LinkState& st) {
    uint8_t buf[link::kFrameMax];
    const size_t n = link::encode_state(st, buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) app.on_link_byte(buf[i]);
  }
};

}  // namespace

// ===========================================================================
//  ATmega2560 : repli de sécurité
// ===========================================================================

TEST(mega_refuse_toute_course_tant_qu_aucun_heartbeat_n_est_recu) {
  // Au démarrage, l'ESP32 n'a encore rien dit : l'AGV ne doit pas bouger.
  MegaBench b;
  b.build();
  CHECK(b.app->safe_stop_active());
  CHECK(!b.app->heartbeat_ok());

  b.send_goto(1, 5);
  CHECK(b.last_ack() == link::CmdResult::SafeStopActive);
  CHECK_EQ(b.queue->size(), 0u);
  CHECK_EQ(b.app->stats().goto_refused_safe_stop, 1u);
}

TEST(mega_accepte_une_course_apres_le_premier_heartbeat) {
  MegaBench b;
  b.build();
  b.heartbeat();
  CHECK(b.app->heartbeat_ok());
  CHECK(!b.app->safe_stop_active());

  b.send_goto(1, 5);
  CHECK(b.last_ack() == link::CmdResult::Accepted);
  CHECK_EQ(b.app->stats().goto_accepted, 1u);

  b.run(4000);
  CHECK_EQ(b.bus->position(), 5u);
}

TEST(mega_perte_de_heartbeat_arrete_au_point_d_arret_suivant) {
  // LE test de cette architecture (planification §2, non négociable).
  MegaBench b;
  // Trajet réaliste : 1 s par station, donc plus long que le timeout de
  // heartbeat (2 s). Avec un AGV instantané, la coupure ne prouverait rien.
  b.timings.travel_per_station_us = 1000000;
  b.build();
  b.heartbeat();
  b.send_goto(1, 4);
  b.send_goto(2, 9);
  b.run(60);  // l'AGV s'élance vers la station 4
  CHECK(b.seq->state() == SeqState::Transit);

  // L'ESP32 se tait : plus aucun heartbeat. Le repli tombe en pleine course.
  b.run(8000, /*with_heartbeat=*/false);

  CHECK(!b.app->heartbeat_ok());
  CHECK(b.app->safe_stop_active());
  CHECK_EQ(b.app->stats().heartbeat_losses, 1u);
  // La course engagée est allée jusqu'au bout, jamais d'arrêt en pleine allée.
  CHECK_EQ(b.bus->position(), 4u);
  // La course suivante n'a PAS été lancée.
  CHECK(b.seq->state() == SeqState::SafeStop);
  CHECK_EQ(b.queue->size(), 1u);
}

TEST(mega_refuse_les_courses_pendant_le_repli) {
  MegaBench b;
  b.build();
  b.heartbeat();
  b.run(3000, /*with_heartbeat=*/false);
  CHECK(b.app->safe_stop_active());

  b.writer.bytes.clear();
  b.send_goto(7, 3);
  CHECK(b.last_ack() == link::CmdResult::SafeStopActive);
  CHECK_EQ(b.app->stats().goto_refused_safe_stop, 1u);
}

TEST(mega_retour_du_heartbeat_ne_relance_rien_tout_seul) {
  // Le retour de la liaison rouvre la possibilité de commander ; il ne remet
  // pas l'AGV en marche de lui-même. Un redémarrage spontané après une coupure
  // réseau serait le pire comportement possible pour un opérateur à proximité.
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(1, 3);
  b.run(3000, /*with_heartbeat=*/false);
  const uint16_t position_pendant_coupure = b.bus->position();

  b.heartbeat();
  b.run(500, /*with_heartbeat=*/false);
  CHECK(b.app->heartbeat_ok());
  CHECK(!b.app->safe_stop_active());
  CHECK_EQ(b.bus->position(), position_pendant_coupure);

  // En revanche, une nouvelle commande est de nouveau acceptée.
  b.send_goto(2, 6);
  CHECK(b.last_ack() == link::CmdResult::Accepted);
  b.run(5000);
  CHECK_EQ(b.bus->position(), 6u);
}

TEST(mega_un_heartbeat_manque_isolement_n_immobilise_pas_l_agv) {
  // timeout = 4 × période : rater un battement ne doit rien déclencher.
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(1, 6);
  // Un seul battement toutes les 1,5 périodes : sous le timeout.
  for (uint32_t t = 0; t < 4000; ++t) {
    if (b.now_ms % 750 == 0) b.heartbeat();
    b.app->tick(b.now_ms);
    b.bus->advance(1000);
    ++b.now_ms;
  }
  CHECK(b.app->heartbeat_ok());
  CHECK_EQ(b.app->stats().heartbeat_losses, 0u);
  CHECK_EQ(b.bus->position(), 6u);
}

// ===========================================================================
//  ATmega2560 : file et idempotence
// ===========================================================================

TEST(mega_commande_rejouee_est_reacquittee_sans_seconde_course) {
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(5, 3);
  b.send_goto(5, 3);  // ACK perdu côté ESP32, l'émetteur retente

  CHECK_EQ(b.app->stats().goto_accepted, 1u);
  CHECK_EQ(b.app->stats().goto_duplicate, 1u);
  CHECK(b.last_ack() == link::CmdResult::Duplicate);
  b.run(4000);
  CHECK_EQ(b.seq->counters().courses_completed, 1u);
}

TEST(mega_file_pleine_refuse_explicitement) {
  MegaBench b;
  b.build();
  b.heartbeat();
  for (uint8_t i = 1; i <= 6; ++i) b.send_goto(i, i);
  CHECK_EQ(b.app->stats().goto_refused_full, 1u);
  CHECK(b.last_ack() == link::CmdResult::QueueFull);
}

TEST(mega_course_prioritaire_passe_devant) {
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(1, 2);
  b.send_goto(2, 8, 4, /*flags=*/0x01);  // priorité
  Course first;
  CHECK(b.queue->peek(first));
  CHECK_EQ(first.station, 8u);
}

TEST(mega_stop_avec_purge_vide_la_file) {
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(1, 2);
  b.send_goto(2, 3);
  uint8_t frame[link::kFrameMax];
  const uint8_t flags = 0x01;
  const size_t n = link::encode(link::kSofToMega, link::Cmd::Stop, &flags, 1, frame,
                                sizeof(frame));
  b.feed_frame(frame, n);
  CHECK_EQ(b.queue->size(), 0u);
  CHECK_EQ(b.app->stats().stops, 1u);
}

TEST(mega_repond_a_get_state_avec_un_etat_coherent) {
  MegaBench b;
  b.build();
  b.heartbeat();
  b.send_goto(1, 7);
  b.run(4000);

  b.writer.bytes.clear();
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode(link::kSofToMega, link::Cmd::GetState, nullptr, 0, frame,
                                sizeof(frame));
  b.feed_frame(frame, n);

  const auto frames = b.writer.frames();
  REQUIRE(frames.size() == 1u);
  CHECK(frames[0].first == link::Cmd::State);
  link::LinkState s;
  CHECK(link::decode_state(frames[0].second.data(),
                           static_cast<uint8_t>(frames[0].second.size()), s));
  CHECK_EQ(s.station, 7u);
  CHECK((s.flags & link::state_flag::kHeartbeatOk) != 0);
  CHECK((s.flags & link::state_flag::kSafeStop) == 0);
}

// ===========================================================================
//  ESP32 : passerelle LoRa
// ===========================================================================

TEST(gateway_commande_recue_en_lora_est_transmise_a_l_atmega) {
  GatewayBench b;
  b.radio.receive(b.goto_frame(1, 42, 6));
  b.app.tick();

  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 1u);
  CHECK_EQ(b.app.stats().cmd_forwarded, 1u);
}

TEST(gateway_commande_malformee_est_comptee_et_rejetee) {
  GatewayBench b;
  // Station hors plage : elle ne doit jamais atteindre le bus.
  b.radio.receive(b.goto_frame(1, kStationMax + 1));
  b.app.tick();

  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 0u);
  CHECK_EQ(b.app.stats().cmd_malformed, 1u);
  CHECK_EQ(b.radio.count(FrameType::Ack), 1u);
}

TEST(gateway_doublon_est_reacquitte_sans_seconde_course) {
  // La règle d'idempotence : sans elle, un accusé perdu déclenche une course
  // en double, ce qui envoie l'AGV deux fois au même endroit.
  GatewayBench b;
  b.radio.receive(b.goto_frame(7, 12));
  b.app.tick();
  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 1u);

  b.radio.receive(b.goto_frame(7, 12));
  b.app.tick();

  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 1u);   // PAS de seconde course
  CHECK_EQ(b.app.stats().cmd_duplicate, 1u);
  CHECK_EQ(b.radio.count(FrameType::Ack), 1u);        // mais bien ré-acquittée
}

TEST(gateway_deux_boutons_ont_des_sequences_independantes) {
  GatewayBench b;
  b.radio.receive(b.goto_frame(3, 10, 4, /*node=*/0x0001));
  b.radio.receive(b.goto_frame(3, 20, 4, /*node=*/0x0002));
  b.app.tick();

  // Même seq, nœuds différents : ce n'est pas un doublon.
  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 2u);
  CHECK_EQ(b.app.stats().cmd_duplicate, 0u);
}

TEST(gateway_heartbeat_est_emis_meme_sans_radio) {
  // La carte va très bien ; c'est la liaison radio qui est absente. Couper le
  // heartbeat immobiliserait l'AGV pour rien.
  GatewayBench b;
  b.radio.set_budget(false);          // plus rien ne peut partir en LoRa
  b.run(2000);

  CHECK(b.link_port.count(link::Cmd::Heartbeat) >= 3u);
  CHECK(b.app.stats().heartbeats_sent >= 3u);
}

TEST(gateway_accuse_chaque_reponse_de_l_atmega) {
  GatewayBench b;
  b.radio.receive(b.goto_frame(5, 8));
  b.app.tick();
  b.mega_ack(5, link::CmdResult::Accepted);

  CHECK_EQ(b.radio.count(FrameType::Ack), 1u);
  CHECK_EQ(b.app.stats().acks_sent, 1u);
}

TEST(gateway_refus_de_l_atmega_part_en_ack_negatif) {
  GatewayBench b;
  b.radio.receive(b.goto_frame(6, 8));
  b.app.tick();
  b.mega_ack(6, link::CmdResult::QueueFull);

  bool nack = false;
  for (const auto& f : b.radio.sent) {
    if (f.type == FrameType::Ack && (f.flags & flag::kNack)) nack = true;
  }
  CHECK(nack);
}

TEST(gateway_accuse_refuse_par_le_budget_est_compte_pas_perdu) {
  // Un récepteur qui acquitte est un émetteur : l'accusé consomme du budget de
  // rapport cyclique. S'il est refusé, cela doit se voir.
  GatewayBench b;
  b.radio.set_budget(false);
  b.radio.receive(b.goto_frame(9, 3));
  b.app.tick();
  b.mega_ack(9, link::CmdResult::Accepted);

  CHECK_EQ(b.app.stats().acks_sent, 0u);
  CHECK(b.app.stats().acks_refused_duty >= 1u);
}

TEST(gateway_detecte_le_silence_de_l_atmega) {
  GatewayBench b;
  link::LinkState st;
  st.station = 4;
  b.mega_state(st);
  CHECK(b.app.link_up());

  b.run(5000);
  CHECK(!b.app.link_up());
  CHECK_EQ(b.app.stats().link_timeouts, 1u);
}

TEST(gateway_etat_json_porte_le_repli_de_securite) {
  GatewayBench b;
  link::LinkState st;
  st.station = 11;
  st.flags = link::state_flag::kSafeStop;
  b.mega_state(st);

  char buf[256];
  const size_t n = b.app.render_state_json(buf, sizeof(buf));
  CHECK(n > 0);
  CHECK(std::string(buf).find("\"safe_stop\":true") != std::string::npos);
  CHECK(std::string(buf).find("\"station\":11") != std::string::npos);
}

TEST(gateway_emet_de_la_telemetrie_espacee) {
  // Elle ne doit pas manger le budget réservé aux accusés.
  GatewayBench b;
  b.run(6000);
  CHECK(b.radio.count(FrameType::Telemetry) >= 1u);
  CHECK(b.radio.count(FrameType::Telemetry) <= 5u);
}

TEST(agvdump_reste_au_format_atelier) {
  // §3.3 : les noms de compteurs sont ceux du firmware d'origine, même si les
  // données traversent maintenant une liaison série.
  link::LinkState s;
  s.station = 17;
  s.write_tries = 2;
  s.flags = link::state_flag::kMoving | link::state_flag::kHeartbeatOk;

  AgvDumpInput in;
  in.state = &s;
  in.profile_name = "wifi";
  in.link_up = true;
  in.wifi_up = true;
  in.ssid = "USINE-OT";
  in.rssi_dbm = -62;

  char buf[2048];
  const size_t len = render_agvdump(in, buf, sizeof(buf));
  REQUIRE(len > 0u);
  const std::string dump(buf, len);
  for (const char* field : {"AGV STATE", "current_station", "write_tries", "start_tries",
                            "stop_tries", "nb_courses_programmed", "safe_stop", "heartbeat_ok",
                            "ssid", "rssi_dbm"}) {
    CHECK(dump.find(field) != std::string::npos);
  }
}
