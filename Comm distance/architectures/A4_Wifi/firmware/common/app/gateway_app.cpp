#include "app/gateway_app.h"

#include <cstdio>

#include "proto/json.h"

namespace agv {
namespace {

// Compare deux séquences 8 bits roulantes : true si `a` est postérieur à `b`.
bool seq_after(uint8_t a, uint8_t b) {
  return static_cast<int8_t>(static_cast<uint8_t>(a - b)) > 0;
}

}  // namespace

void GatewayApp::begin() {
  const char* id = profile_.mqtt.agv_id;
  std::snprintf(topic_state_, sizeof(topic_state_), "agv/%s/state", id);
  std::snprintf(topic_cmd_, sizeof(topic_cmd_), "agv/%s/cmd", id);
  std::snprintf(topic_ack_, sizeof(topic_ack_), "agv/%s/ack", id);
  std::snprintf(topic_status_, sizeof(topic_status_), "agv/%s/status", id);
  last_heartbeat_ms_ = clock_.now_ms();
  last_link_rx_ms_ = clock_.now_ms();
}

void GatewayApp::send_heartbeat() {
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode(link::kSofToMega, link::Cmd::Heartbeat, nullptr, 0, frame,
                                sizeof(frame));
  if (n > 0) {
    link_.write(frame, n);
    ++stats_.heartbeats_sent;
  }
}

void GatewayApp::request_state() {
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode(link::kSofToMega, link::Cmd::GetState, nullptr, 0, frame,
                                sizeof(frame));
  if (n > 0) link_.write(frame, n);
}

size_t GatewayApp::render_state_json(char* out, size_t capacity) const {
  json::Writer w(out, capacity);
  w.field("station", static_cast<uint32_t>(state_.station));
  w.field("speed", static_cast<uint32_t>(state_.speed));
  w.field("moving", (state_.flags & link::state_flag::kMoving) != 0);
  w.field("in_station", (state_.flags & link::state_flag::kInStation) != 0);
  w.field("fault", state_.fault != 0);
  w.field("fault_code", static_cast<uint32_t>(state_.fault));
  w.field("seq_state", static_cast<uint32_t>(state_.seq_state));
  w.field("queue_len", static_cast<uint32_t>(state_.queue_len));
  // `safe_stop` est l'information la plus importante de cette architecture :
  // elle dit que l'AGV a perdu son heartbeat et refuse toute nouvelle course.
  w.field("safe_stop", (state_.flags & link::state_flag::kSafeStop) != 0);
  w.field("heartbeat_ok", (state_.flags & link::state_flag::kHeartbeatOk) != 0);
  w.field("link_up", link_up_);
  w.field("write_tries", static_cast<uint32_t>(state_.write_tries));
  w.field("start_tries", static_cast<uint32_t>(state_.start_tries));
  w.field("stop_tries", static_cast<uint32_t>(state_.stop_tries));
  w.field("ts", clock_.now_s());
  return w.end();
}

void GatewayApp::publish_state() {
  char payload[512];
  const size_t n = render_state_json(payload, sizeof(payload));
  if (n == 0) return;
  // `retain` : un poste qui se connecte doit connaître l'état sans attendre la
  // publication suivante (planification §2).
  if (mqtt_.publish(topic_state_, payload, true)) ++stats_.states_published;
}

void GatewayApp::publish_ack(uint8_t seq, link::CmdResult result) {
  char payload[160];
  json::Writer w(payload, sizeof(payload));
  w.field("seq", static_cast<uint32_t>(seq));
  w.field("status", static_cast<uint32_t>(result));
  w.field("ok", result == link::CmdResult::Accepted || result == link::CmdResult::Duplicate);
  w.field("ts", clock_.now_s());
  if (w.end() == 0) return;
  if (mqtt_.publish(topic_ack_, payload, false)) ++stats_.acks_published;
}

void GatewayApp::on_mqtt_command(const char* payload) {
  ++stats_.cmd_received;

  int32_t seq = 0;
  int32_t dest = 0;
  if (!json::get_int(payload, "seq", seq) || !json::get_int(payload, "dest", dest)) {
    ++stats_.cmd_malformed;
    return;
  }
  int32_t speed = 0;
  json::get_int(payload, "speed", speed);
  int32_t ts = 0;
  const bool has_ts = json::get_int(payload, "ts", ts);

  if (dest < 0 || dest > static_cast<int32_t>(kStationMax) || speed < 0 ||
      speed > static_cast<int32_t>(kSpeedMax)) {
    ++stats_.cmd_malformed;
    publish_ack(static_cast<uint8_t>(seq), link::CmdResult::BadPayload);
    return;
  }

  const uint8_t seq8 = static_cast<uint8_t>(seq);

  // Péremption (planification §2) : une commande retardée par le réseau ne
  // s'exécute JAMAIS. Sans horloge murale sûre, on ne peut pas trancher : on
  // laisse passer plutôt que de tout refuser, et l'absence d'horodatage est
  // visible dans les compteurs.
  const uint32_t now_s = clock_.now_s();
  if (has_ts && now_s != 0 && profile_.safety.max_command_age_s != 0) {
    const uint32_t age = (now_s > static_cast<uint32_t>(ts)) ? now_s - static_cast<uint32_t>(ts)
                                                             : 0u;
    if (age > profile_.safety.max_command_age_s) {
      ++stats_.cmd_expired;
      publish_ack(seq8, link::CmdResult::Fault);
      return;
    }
  }

  // Doublon et désordre. Le MEGA refait le contrôle d'idempotence de son côté :
  // ici on évite surtout d'encombrer la liaison série.
  if (have_last_seq_) {
    if (seq8 == last_cmd_seq_) {
      ++stats_.cmd_duplicate;
      publish_ack(seq8, link::CmdResult::Duplicate);
      return;
    }
    if (!seq_after(seq8, last_cmd_seq_)) {
      ++stats_.cmd_out_of_order;
      publish_ack(seq8, link::CmdResult::Fault);
      return;
    }
  }

  int32_t purge = 0;
  const bool is_stop = json::get_int(payload, "stop", purge);

  uint8_t frame[link::kFrameMax];
  size_t n = 0;
  if (is_stop) {
    const uint8_t flags = static_cast<uint8_t>(purge != 0 ? 0x01 : 0x00);
    n = link::encode(link::kSofToMega, link::Cmd::Stop, &flags, 1, frame, sizeof(frame));
  } else {
    int32_t priority = 0;
    json::get_int(payload, "priority", priority);
    n = link::encode_goto(seq8, static_cast<uint16_t>(dest), static_cast<uint8_t>(speed),
                          static_cast<uint8_t>(priority != 0 ? 0x01 : 0x00), frame,
                          sizeof(frame));
  }
  if (n == 0) {
    ++stats_.cmd_malformed;
    return;
  }
  link_.write(frame, n);
  last_cmd_seq_ = seq8;
  have_last_seq_ = true;
  ++stats_.cmd_forwarded;
}

void GatewayApp::on_link_byte(uint8_t byte) {
  link::Cmd cmd;
  uint8_t payload[link::kPayloadMax];
  uint8_t len = 0;
  if (!parser_.feed(byte, cmd, payload, len)) return;

  last_link_rx_ms_ = clock_.now_ms();
  link_up_ = true;

  switch (cmd) {
    case link::Cmd::State:
      link::decode_state(payload, len, state_);
      break;
    case link::Cmd::Ack:
      if (len >= 2) {
        publish_ack(payload[0], static_cast<link::CmdResult>(payload[1]));
      }
      break;
    default:
      break;
  }
}

void GatewayApp::tick() {
  const uint32_t now = clock_.now_ms();

  // 1. Heartbeat : la seule chose qui autorise l'ATmega à rouler. Émis même
  //    quand MQTT est déconnecté : le Wi-Fi peut tomber sans que la carte soit
  //    en danger, et couper le heartbeat immobiliserait l'AGV pour rien.
  if ((now - last_heartbeat_ms_) >= profile_.heartbeat.period_ms) {
    last_heartbeat_ms_ = now;
    send_heartbeat();
  }

  // 2. Interrogation d'état de l'ATmega.
  if ((now - last_poll_ms_) >= profile_.link.state_poll_ms) {
    last_poll_ms_ = now;
    request_state();
  }

  // 3. Le MEGA ne répond plus : c'est un défaut de la carte, pas du réseau.
  const uint32_t link_timeout_ms = profile_.link.state_poll_ms * 5u;
  if (link_up_ && (now - last_link_rx_ms_) > link_timeout_ms) {
    link_up_ = false;
    ++stats_.link_timeouts;
  }

  // 4. Publication périodique de l'état (1 s par défaut).
  if (mqtt_.connected() && (now - last_publish_ms_) >= profile_.mqtt.state_period_ms) {
    last_publish_ms_ = now;
    publish_state();
  }
}

}  // namespace agv
