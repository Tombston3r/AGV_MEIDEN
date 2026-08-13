#include "app/agv_app.h"

namespace agv {

bool AgvApp::begin() {
  // Restauration de la file persistée (§4.5) : c'est ce qui distingue cette
  // carte de la V5.0.1, où une coupure effaçait la mémoire de mission.
  restored_courses_ = queue_.restore(clock_.now_s(), &dropped_courses_);
  if (!transport_.begin()) return false;
  last_valid_frame_ms_ = clock_.now_ms();
  return seq_.begin();
}

void AgvApp::send_ack(const Frame& request, bool positive) {
  Frame ack;
  ack.ver = profile_.protocol.version;
  ack.type = FrameType::Ack;
  ack.node_id = profile_.protocol.node_id;
  ack.seq = request.seq;  // l'ACK reprend la séquence acquittée
  ack.station = request.station;
  ack.flags = positive ? 0u : flag::kNack;
  ack.ts_s = clock_.now_s();
  if (ack.ts_s != 0) ack.flags |= flag::kTimestamped;
  transport_.send(ack);
}

void AgvApp::handle_frame(const Frame& f) {
  last_valid_frame_ms_ = clock_.now_ms();

  if (f.type == FrameType::Ping) {
    send_ack(f, true);
    return;
  }
  if (f.type == FrameType::Ack || f.type == FrameType::Telemetry) {
    return;  // rien à faire côté AGV
  }

  const uint32_t max_age = transport_.max_command_age_s();
  const FrameVerdict verdict =
      replay_.classify(f.node_id, f.seq, f.timestamped(), f.ts_s, clock_.now_s(), max_age);

  switch (verdict) {
    case FrameVerdict::Duplicate:
      // Ré-acquitter SANS ré-exécuter : sans cela, un ACK perdu déclenche une
      // course en double (brief §5.1).
      ++stats_.cmd_duplicate;
      send_ack(f, true);
      return;
    case FrameVerdict::OutOfOrder:
      ++stats_.cmd_out_of_order;
      send_ack(f, false);
      return;
    case FrameVerdict::Expired:
      // Une commande périmée ne fait JAMAIS bouger l'AGV (§8.1).
      ++stats_.cmd_expired;
      send_ack(f, false);
      return;
    case FrameVerdict::Accept:
      break;
  }

  if (f.type == FrameType::CmdStop) {
    if ((f.flags & flag::kPurgeQueue) != 0) queue_.clear();
    seq_.request_safe_stop();
    replay_.remember(f.node_id, f.seq);
    ++stats_.cmd_accepted;
    queue_.save(clock_.now_s());
    send_ack(f, true);
    return;
  }

  if (f.type == FrameType::CmdGoto) {
    if ((f.flags & flag::kPurgeQueue) != 0) queue_.clear();
    Course c;
    c.station = f.station;
    c.speed = f.speed;
    c.node_id = f.node_id;
    c.seq = f.seq;
    c.enqueued_at_s = clock_.now_s();
    c.flags = f.flags;

    const bool priority = (f.flags & flag::kPriority) != 0;
    const bool ok = priority ? queue_.push_front(c) : queue_.push(c);
    if (!ok) {
      // File pleine : NACK explicite. Un refus silencieux ferait retenter
      // l'émetteur indéfiniment.
      ++stats_.cmd_rejected_full;
      send_ack(f, false);
      return;
    }
    replay_.remember(f.node_id, f.seq);
    ++stats_.cmd_accepted;
    queue_.save(clock_.now_s());
    send_ack(f, true);
    return;
  }
}

void AgvApp::send_telemetry() {
  Frame t;
  t.ver = profile_.protocol.version;
  t.type = FrameType::Telemetry;
  t.node_id = profile_.protocol.node_id;
  t.seq = tx_seq_++;
  t.station = seq_.counters().current_station;
  t.speed = seq_.counters().current_speed;
  t.flags = 0;
  if (seq_.moving()) t.flags |= flag::kStatusMoving;
  if (seq_.in_station()) t.flags |= flag::kStatusInStation;
  if (seq_.state() == SeqState::Fault) t.flags |= flag::kStatusFault;
  if (transport_.send(t)) ++stats_.telemetry_sent;
}

void AgvApp::tick() {
  transport_.tick();
  seq_.tick();

  Frame f;
  while (transport_.poll(f)) handle_frame(f);

  const uint32_t now_ms = clock_.now_ms();

  // Chien de garde applicatif (§3.1) : absence de trame valide pendant N
  // secondes -> état sûr + LED FAULT. N est un paramètre, pas une constante.
  const uint32_t watchdog_ms = profile_.safety.link_watchdog_s * 1000u;
  const bool link_ok = (now_ms - last_valid_frame_ms_) < watchdog_ms;
  if (!link_ok && link_up_) {
    ++stats_.link_losses;
    // Arrêt sûr AU POINT D'ARRÊT SUIVANT, jamais d'état indéterminé.
    seq_.request_safe_stop(FaultCause::LinkLost);
  }
  link_up_ = link_ok;

  if (indicators_ != nullptr) {
    indicators_->set_link(link_up_);
    indicators_->set_fault(seq_.state() == SeqState::Fault || seq_.state() == SeqState::SafeStop);
  }

  if (telemetry_period_ms_ != 0 && (now_ms - last_telemetry_ms_) >= telemetry_period_ms_) {
    last_telemetry_ms_ = now_ms;
    send_telemetry();
  }
}

size_t AgvApp::render_agvdump(char* out, size_t capacity) const {
  const LinkHealth link = transport_.health();
  AgvDumpInput in;
  in.counters = &seq_.counters();
  in.queue = &queue_;
  in.sequencer = &seq_;
  in.link = &link;
  in.transport_name = transport_.name();
  in.profile_name = profile_.name;
  in.uptime_s = clock_.now_ms() / 1000u;
  in.link_up = link_up_;
  return agv::render_agvdump(in, out, capacity);
}

}  // namespace agv
