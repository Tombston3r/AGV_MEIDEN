#include "app/lora_gateway_app.h"

#include <cstdio>
#include <cstring>

namespace agv {

void LoraGatewayApp::begin() {
  radio_.begin();
  parser_.reset();
  last_heartbeat_ms_ = 0;
  last_poll_ms_ = 0;
  last_telemetry_ms_ = 0;
  have_link_rx_ = false;
  link_up_ = false;
}

void LoraGatewayApp::send_heartbeat() {
  uint8_t buf[link::kFrameMax];
  const size_t n = link::encode(link::kSofToMega, link::Cmd::Heartbeat, nullptr, 0, buf,
                                sizeof(buf));
  if (n > 0) {
    link_.write(buf, n);
    ++stats_.heartbeats_sent;
  }
}

void LoraGatewayApp::request_state() {
  uint8_t buf[link::kFrameMax];
  const size_t n = link::encode(link::kSofToMega, link::Cmd::GetState, nullptr, 0, buf,
                                sizeof(buf));
  if (n > 0) link_.write(buf, n);
}

void LoraGatewayApp::tick() {
  const uint32_t now = clock_.now_ms();

  // Le heartbeat part QUOI QU'IL ARRIVE côté radio. La carte va très bien ;
  // c'est la liaison radio qui est éventuellement absente. Le couper
  // immobiliserait l'AGV pour rien.
  if (now - last_heartbeat_ms_ >= profile_.heartbeat.period_ms) {
    last_heartbeat_ms_ = now;
    send_heartbeat();
  }

  if (now - last_poll_ms_ >= profile_.link.state_poll_ms) {
    last_poll_ms_ = now;
    request_state();
  }

  // L'ATmega ne répond plus : on le signale, sans rien décider à sa place,
  // c'est LUI qui porte le repli de sécurité, sur perte du heartbeat.
  if (have_link_rx_ && now - last_link_rx_ms_ > profile_.link.reply_timeout_ms * 4) {
    if (link_up_) ++stats_.link_timeouts;
    link_up_ = false;
  }

  radio_.tick();

  Frame f;
  while (radio_.poll(f)) handle_frame(f);

  // Télémétrie espacée : elle consomme du budget de rapport cyclique, qui
  // doit rester disponible pour les accusés. Dix fois la cadence d'état.
  if (now - last_telemetry_ms_ >= profile_.link.state_poll_ms * 10) {
    last_telemetry_ms_ = now;
    send_telemetry();
  }
}

bool LoraGatewayApp::already_seen(uint16_t node_id, uint8_t seq) {
  for (auto& e : seen_) {
    if (e.used && e.node_id == node_id) {
      if (e.seq == seq) return true;
      e.seq = seq;
      return false;
    }
  }
  for (auto& e : seen_) {
    if (!e.used) {
      e.used = true;
      e.node_id = node_id;
      e.seq = seq;
      return false;
    }
  }
  return false;  // table pleine : on laisse passer, l'ATmega refera le tri
}

void LoraGatewayApp::handle_frame(const Frame& f) {
  switch (f.type) {
    case FrameType::CmdGoto: {
      ++stats_.cmd_received;
      if (f.station > kStationMax || f.speed > kSpeedMax) {
        ++stats_.cmd_malformed;
        send_ack(f.seq, link::CmdResult::BadPayload);
        return;
      }
      // Ré-acquittée sans être ré-exécutée : c'est toute la règle.
      if (already_seen(f.node_id, f.seq)) {
        ++stats_.cmd_duplicate;
        send_ack(f.seq, link::CmdResult::Duplicate);
        return;
      }
      uint8_t buf[link::kFrameMax];
      const size_t n = link::encode_goto(f.seq, f.station, f.speed, f.flags, buf, sizeof(buf));
      if (n > 0) {
        link_.write(buf, n);
        ++stats_.cmd_forwarded;
        pending_node_ = f.node_id;
        pending_seq_ = f.seq;
        pending_ = true;
      }
      return;
    }
    case FrameType::CmdStop: {
      ++stats_.cmd_received;
      const uint8_t flags = f.flags;
      uint8_t buf[link::kFrameMax];
      const size_t n = link::encode(link::kSofToMega, link::Cmd::Stop, &flags, 1, buf,
                                    sizeof(buf));
      if (n > 0) {
        link_.write(buf, n);
        ++stats_.cmd_forwarded;
        pending_node_ = f.node_id;
        pending_seq_ = f.seq;
        pending_ = true;
      }
      return;
    }
    case FrameType::Ping:
      send_ack(f.seq, link::CmdResult::Accepted);
      return;
    default:
      ++stats_.cmd_malformed;
      return;
  }
}

void LoraGatewayApp::send_ack(uint8_t seq, link::CmdResult result) {
  Frame ack;
  ack.type = FrameType::Ack;
  ack.node_id = profile_.protocol.node_id;
  ack.seq = seq;
  ack.station = state_.station;
  ack.speed = state_.speed;
  if (result != link::CmdResult::Accepted && result != link::CmdResult::Duplicate) {
    ack.flags |= flag::kNack;
  }

  // Un récepteur qui acquitte est un émetteur : l'accusé consomme du budget de
  // rapport cyclique, et peut donc être refusé. On le compte plutôt que de le
  // perdre en silence.
  if (radio_.send(ack)) {
    ++stats_.acks_sent;
  } else {
    ++stats_.acks_refused_duty;
  }
}

void LoraGatewayApp::send_telemetry() {
  Frame t;
  t.type = FrameType::Telemetry;
  t.node_id = profile_.protocol.node_id;
  t.station = state_.station;
  t.speed = state_.speed;
  if (state_.flags & link::state_flag::kMoving) t.flags |= flag::kStatusMoving;
  if (state_.flags & link::state_flag::kInStation) t.flags |= flag::kStatusInStation;
  if (state_.fault != 0) t.flags |= flag::kStatusFault;
  if (radio_.send(t)) ++stats_.telemetry_sent;
}

void LoraGatewayApp::on_link_byte(uint8_t byte) {
  link::Cmd cmd{};
  uint8_t payload[link::kPayloadMax];
  uint8_t len = 0;
  if (!parser_.feed(byte, cmd, payload, len)) return;

  last_link_rx_ms_ = clock_.now_ms();
  have_link_rx_ = true;
  link_up_ = true;

  switch (cmd) {
    case link::Cmd::Ack:
      if (len >= 2) {
        const uint8_t seq = payload[0];
        const auto result = static_cast<link::CmdResult>(payload[1]);
        if (pending_ && seq == pending_seq_) pending_ = false;
        if (result == link::CmdResult::Duplicate) ++stats_.cmd_duplicate;
        send_ack(seq, result);
      }
      return;
    case link::Cmd::State:
      link::decode_state(payload, len, state_);
      return;
    default:
      return;
  }
}

size_t LoraGatewayApp::render_state_json(char* out, size_t capacity) const {
  if (out == nullptr || capacity == 0) return 0;
  const LinkHealth h = radio_.health();
  const int n = snprintf(
      out, capacity,
      "{\"station\":%u,\"speed\":%u,\"seq_state\":%u,\"fault\":%u,\"queue\":%u,"
      "\"safe_stop\":%s,\"link_up\":%s,\"rssi\":%d,\"snr\":%d,\"duty\":%u}",
      static_cast<unsigned>(state_.station), static_cast<unsigned>(state_.speed),
      static_cast<unsigned>(state_.seq_state), static_cast<unsigned>(state_.fault),
      static_cast<unsigned>(state_.queue_len),
      (state_.flags & link::state_flag::kSafeStop) ? "true" : "false",
      link_up_ ? "true" : "false", static_cast<int>(h.rssi_dbm), static_cast<int>(h.snr_db),
      static_cast<unsigned>(h.duty_used_permille));
  return (n > 0 && static_cast<size_t>(n) < capacity) ? static_cast<size_t>(n) : 0;
}

}  // namespace agv
