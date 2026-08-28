// Applications des deux microcontrôleurs de la carte V5.0.1.
//
// Le test central de cette architecture est le REPLI HEARTBEAT : l'ATmega doit
// s'arrêter au point d'arrêt suivant et refuser toute course quand l'ESP32 se
// tait, sans dépendre du Wi-Fi ni du poste fixe (planification §2).
#include <string>
#include <vector>

#include "app/agvdump.h"
#include "app/course_queue.h"
#include "app/gateway_app.h"
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

class FakeMqtt final : public IMqttPublisher {
 public:
  bool publish(const char* topic, const char* payload, bool retain) override {
    if (!connected_) return false;
    published.push_back({topic, payload, retain});
    return true;
  }
  bool connected() const override { return connected_; }
  void set_connected(bool v) { connected_ = v; }

  struct Message {
    std::string topic;
    std::string payload;
    bool retain;
  };
  std::vector<Message> published;

 private:
  bool connected_ = true;
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
  FakeMqtt mqtt;
  CaptureLink link_port;
  GatewayApp app{profile, clock, mqtt, link_port};

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
  std::string cmd_json(uint32_t seq, uint32_t dest, uint32_t speed, uint32_t ts) {
    char buf[160];
    json::Writer w(buf, sizeof(buf));
    w.field("seq", seq);
    w.field("dest", dest);
    w.field("speed", speed);
    w.field("ts", ts);
    w.end();
    return std::string(buf);
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
//  ESP32 : passerelle MQTT
// ===========================================================================

TEST(gateway_commande_json_est_transmise_a_l_atmega) {
  GatewayBench b;
  b.app.on_mqtt_command(b.cmd_json(1, 42, 6, b.clock.now_s()).c_str());

  CHECK_EQ(b.app.stats().cmd_forwarded, 1u);
  const auto frames = b.link_port.frames();
  bool found = false;
  for (const auto& f : frames) {
    if (f.first == link::Cmd::Goto && f.second.size() >= 5) {
      CHECK_EQ(f.second[0], 1u);
      CHECK_EQ(static_cast<uint16_t>((f.second[1] << 8) | f.second[2]), 42u);
      CHECK_EQ(f.second[3], 6u);
      found = true;
    }
  }
  CHECK(found);
}

TEST(gateway_commande_perimee_n_est_jamais_transmise) {
  // Planification §2 : horodatage + péremption 30 s. Une commande retardée par
  // le réseau d'entreprise ne doit jamais faire bouger l'AGV.
  GatewayBench b;
  const uint32_t vieux = b.clock.now_s() - 120;
  b.app.on_mqtt_command(b.cmd_json(1, 42, 6, vieux).c_str());

  CHECK_EQ(b.app.stats().cmd_expired, 1u);
  CHECK_EQ(b.app.stats().cmd_forwarded, 0u);
  CHECK_EQ(b.link_port.count(link::Cmd::Goto), 0u);
}

TEST(gateway_seuil_de_peremption_vient_du_profil) {
  GatewayBench b;
  b.profile.safety.max_command_age_s = 300;  // paramètre, pas constante
  const uint32_t vieux = b.clock.now_s() - 120;
  b.app.on_mqtt_command(b.cmd_json(1, 42, 6, vieux).c_str());
  CHECK_EQ(b.app.stats().cmd_expired, 0u);
  CHECK_EQ(b.app.stats().cmd_forwarded, 1u);
}

TEST(gateway_doublon_et_desordre_sont_filtres_avant_la_liaison_serie) {
  GatewayBench b;
  b.app.on_mqtt_command(b.cmd_json(10, 5, 4, b.clock.now_s()).c_str());
  b.app.on_mqtt_command(b.cmd_json(10, 5, 4, b.clock.now_s()).c_str());
  b.app.on_mqtt_command(b.cmd_json(9, 5, 4, b.clock.now_s()).c_str());

  CHECK_EQ(b.app.stats().cmd_duplicate, 1u);
  CHECK_EQ(b.app.stats().cmd_out_of_order, 1u);
  CHECK_EQ(b.app.stats().cmd_forwarded, 1u);
}

TEST(gateway_commande_malformee_est_comptee_et_rejetee) {
  GatewayBench b;
  b.app.on_mqtt_command("{\"seq\":1}");                 // pas de destination
  b.app.on_mqtt_command("pas du json");
  b.app.on_mqtt_command(b.cmd_json(2, 5000, 4, b.clock.now_s()).c_str());  // hors bornes
  CHECK_EQ(b.app.stats().cmd_malformed, 3u);
  CHECK_EQ(b.app.stats().cmd_forwarded, 0u);
}

TEST(gateway_heartbeat_est_emis_meme_sans_mqtt) {
  // Couper le heartbeat parce que le Wi-Fi est tombé immobiliserait l'AGV pour
  // rien : la carte va très bien, c'est le réseau qui est absent.
  GatewayBench b;
  b.mqtt.set_connected(false);
  b.run(2000);
  CHECK(b.app.stats().heartbeats_sent >= 3u);
  CHECK(b.link_port.count(link::Cmd::Heartbeat) >= 3u);
}

TEST(gateway_publie_l_etat_avec_retain) {
  GatewayBench b;
  b.run(3500);
  size_t states = 0;
  for (const auto& m : b.mqtt.published) {
    if (m.topic == std::string("agv/1/state")) {
      ++states;
      CHECK(m.retain);  // un poste qui se connecte doit connaître l'état
    }
  }
  CHECK(states >= 3u);  // ~1 publication par seconde
}

TEST(gateway_publie_un_ack_pour_chaque_reponse_de_l_atmega) {
  GatewayBench b;
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode_ack(9, link::CmdResult::QueueFull, frame, sizeof(frame));
  for (size_t i = 0; i < n; ++i) b.app.on_link_byte(frame[i]);

  bool found = false;
  for (const auto& m : b.mqtt.published) {
    if (m.topic == std::string("agv/1/ack")) {
      found = true;
      CHECK(m.payload.find("\"seq\":9") != std::string::npos);
      CHECK(m.payload.find("\"ok\":false") != std::string::npos);
      CHECK(!m.retain);  // un ACK ne se rejoue pas à la reconnexion
    }
  }
  CHECK(found);
}

TEST(gateway_detecte_le_silence_de_l_atmega) {
  // Un ATmega muet est un défaut de la CARTE, pas du réseau : il doit être
  // distingué d'une coupure Wi-Fi dans la supervision.
  GatewayBench b;
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode_state(link::LinkState{}, frame, sizeof(frame));
  for (size_t i = 0; i < n; ++i) b.app.on_link_byte(frame[i]);
  CHECK(b.app.link_up());

  b.run(3000);
  CHECK(!b.app.link_up());
  CHECK_EQ(b.app.stats().link_timeouts, 1u);
}

TEST(gateway_etat_json_porte_le_repli_de_securite) {
  GatewayBench b;
  link::LinkState s;
  s.station = 12;
  s.flags = link::state_flag::kSafeStop;
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode_state(s, frame, sizeof(frame));
  for (size_t i = 0; i < n; ++i) b.app.on_link_byte(frame[i]);

  char json[512];
  const size_t len = b.app.render_state_json(json, sizeof(json));
  REQUIRE(len > 0u);
  const std::string payload(json, len);
  CHECK(payload.find("\"station\":12") != std::string::npos);
  CHECK(payload.find("\"safe_stop\":true") != std::string::npos);
  CHECK(payload.find("\"heartbeat_ok\":false") != std::string::npos);
}

TEST(gateway_topics_suivent_l_identifiant_du_profil) {
  GatewayBench b;
  CHECK_STR_EQ(b.app.topic_state(), "agv/1/state");
  CHECK_STR_EQ(b.app.topic_cmd(), "agv/1/cmd");
  CHECK_STR_EQ(b.app.topic_ack(), "agv/1/ack");
  CHECK_STR_EQ(b.app.topic_status(), "agv/1/status");
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
