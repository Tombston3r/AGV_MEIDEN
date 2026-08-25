#include "app/mega_app.h"

namespace agv {

bool MegaApp::begin(uint32_t now_ms) {
  last_heartbeat_ms_ = now_ms;
  heartbeat_ok_ = false;   // aucun heartbeat reçu : on ne roule pas encore
  safe_stop_ = true;       // état sûr par défaut au démarrage
  started_ = seq_.begin();
  return started_;
}

bool MegaApp::already_seen(uint8_t seq) const {
  for (uint8_t i = 0; i < seen_count_; ++i) {
    if (seen_[i] == seq) return true;
  }
  return false;
}

void MegaApp::remember(uint8_t seq) {
  seen_[seen_next_] = seq;
  seen_next_ = static_cast<uint8_t>((seen_next_ + 1) % kSeenMax);
  if (seen_count_ < kSeenMax) ++seen_count_;
  last_seq_ = seq;
}

void MegaApp::send_ack(uint8_t seq, link::CmdResult result) {
  uint8_t frame[link::kFrameMax];
  const size_t n = link::encode_ack(seq, result, frame, sizeof(frame));
  if (n > 0) {
    out_.write(frame, n);
    ++stats_.frames_out;
  }
}

void MegaApp::send_state() {
  uint8_t frame[link::kFrameMax];
  const link::LinkState s = snapshot();
  const size_t n = link::encode_state(s, frame, sizeof(frame));
  if (n > 0) {
    out_.write(frame, n);
    ++stats_.frames_out;
  }
}

link::LinkState MegaApp::snapshot() const {
  link::LinkState s;
  const SeqCounters& c = seq_.counters();
  s.station = c.current_station;
  s.speed = c.current_speed;
  s.seq_state = static_cast<uint8_t>(seq_.state());
  s.fault = static_cast<uint8_t>(seq_.fault_cause());
  s.queue_len = c.nb_courses_programmed;
  s.write_tries = static_cast<uint8_t>(c.write_tries);
  s.start_tries = static_cast<uint8_t>(c.start_tries);
  s.stop_tries = static_cast<uint8_t>(c.stop_tries);
  s.last_seq = last_seq_;
  s.flags = 0;
  if (seq_.moving()) s.flags |= link::state_flag::kMoving;
  if (seq_.in_station()) s.flags |= link::state_flag::kInStation;
  if (seq_.plc_fault()) s.flags |= link::state_flag::kPlcFault;
  if (seq_.no_destination_flag()) s.flags |= link::state_flag::kNoDestination;
  if (safe_stop_) s.flags |= link::state_flag::kSafeStop;
  if (heartbeat_ok_) s.flags |= link::state_flag::kHeartbeatOk;
  return s;
}

void MegaApp::handle(link::Cmd cmd, const uint8_t* payload, uint8_t len, uint32_t now_ms) {
  ++stats_.frames_in;

  switch (cmd) {
    case link::Cmd::Heartbeat:
      last_heartbeat_ms_ = now_ms;
      if (!heartbeat_ok_) {
        heartbeat_ok_ = true;
        // Le retour du heartbeat ne relance RIEN de lui-même : il rouvre
        // seulement la possibilité d'accepter de nouvelles courses. Une course
        // abandonnée pendant la coupure ne repart pas toute seule.
        safe_stop_ = false;
        seq_.clear_fault();
      }
      break;

    case link::Cmd::Ping: {
      const uint8_t version = 1;
      uint8_t frame[link::kFrameMax];
      const size_t n = link::encode(link::kSofToEsp, link::Cmd::Pong, &version, 1, frame,
                                    sizeof(frame));
      if (n > 0) {
        out_.write(frame, n);
        ++stats_.frames_out;
      }
      break;
    }

    case link::Cmd::GetState:
      send_state();
      break;

    case link::Cmd::Goto: {
      if (len < 5) {
        send_ack(0, link::CmdResult::BadPayload);
        break;
      }
      const uint8_t seq = payload[0];
      const uint16_t station = static_cast<uint16_t>((payload[1] << 8) | payload[2]);
      const uint8_t speed = payload[3];
      const uint8_t flags = payload[4];

      // Idempotence : ré-acquitter SANS ré-exécuter. Sans cela, un ACK perdu
      // sur la liaison série déclencherait une course en double.
      if (already_seen(seq)) {
        ++stats_.goto_duplicate;
        send_ack(seq, link::CmdResult::Duplicate);
        break;
      }
      // Repli actif : on refuse explicitement plutôt que d'empiler une course
      // qui partirait au retour du réseau, longtemps après l'appui.
      if (safe_stop_ || !heartbeat_ok_) {
        ++stats_.goto_refused_safe_stop;
        send_ack(seq, link::CmdResult::SafeStopActive);
        break;
      }

      Course c;
      c.station = station & kStationMax;
      c.speed = speed & kSpeedMax;
      c.node_id = profile_.protocol.node_id;
      c.seq = seq;
      c.flags = flags;
      const bool ok = (flags & 0x01u) ? queue_.push_front(c) : queue_.push(c);
      if (!ok) {
        ++stats_.goto_refused_full;
        send_ack(seq, link::CmdResult::QueueFull);
        break;
      }
      remember(seq);
      ++stats_.goto_accepted;
      send_ack(seq, link::CmdResult::Accepted);
      break;
    }

    case link::Cmd::Stop: {
      const uint8_t flags = (len >= 1) ? payload[0] : 0u;
      if (flags & 0x01u) queue_.clear();
      seq_.request_safe_stop();
      ++stats_.stops;
      send_ack(last_seq_, link::CmdResult::Accepted);
      break;
    }

    case link::Cmd::ClearFault:
      seq_.clear_fault();
      safe_stop_ = !heartbeat_ok_;
      send_ack(last_seq_, link::CmdResult::Accepted);
      break;

    default:
      break;  // commande MEGA -> ESP32 reçue à l'envers : ignorée
  }
}

void MegaApp::feed(uint8_t byte, uint32_t now_ms) {
  link::Cmd cmd;
  uint8_t payload[link::kPayloadMax];
  uint8_t len = 0;
  if (parser_.feed(byte, cmd, payload, len)) {
    handle(cmd, payload, len, now_ms);
  }
  stats_.link_crc_errors = parser_.crc_errors();
}

void MegaApp::tick(uint32_t now_ms) {
  // Surveillance du heartbeat AVANT le séquenceur : si la liaison est perdue,
  // la demande d'arrêt doit être posée sur le tick courant, pas le suivant.
  if (heartbeat_ok_ && (now_ms - last_heartbeat_ms_) > profile_.heartbeat.timeout_ms) {
    heartbeat_ok_ = false;
    safe_stop_ = true;
    ++stats_.heartbeat_losses;
    // Arrêt sûr AU POINT D'ARRÊT SUIVANT : la course engagée va au bout, on
    // n'immobilise jamais l'AGV en pleine allée.
    seq_.request_safe_stop(FaultCause::LinkLost);
  }

  seq_.tick();
}

}  // namespace agv
