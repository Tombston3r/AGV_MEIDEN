#include "app/poste_app.h"

#include <cstdio>

namespace agv {

bool PosteApp::begin() {
  pairings_.load();
  dedup_.set_window_ms(profile_.enocean.dedup_window_ms);
  return transport_.begin();
}

bool PosteApp::send_command(FrameType type, uint16_t station, uint8_t speed, uint8_t flags) {
  Frame f;
  f.ver = profile_.protocol.version;
  f.type = type;
  f.node_id = profile_.protocol.node_id;
  f.seq = tx_seq_++;
  f.station = station;
  f.speed = speed;
  f.flags = flags;
  f.ts_s = clock_.now_s();
  if (f.ts_s != 0) f.flags |= flag::kTimestamped;

  if (!transport_.send(f)) {
    // Transport occupé, ou budget de rapport cyclique épuisé : refus FRANC,
    // jamais une mise en file silencieuse qui partirait dix minutes plus tard.
    ++stats_.commands_refused;
    return false;
  }
  ++stats_.commands_sent;
  return true;
}

bool PosteApp::request_goto(uint16_t station, uint8_t speed, uint8_t flags) {
  return send_command(FrameType::CmdGoto, station & kStationMax, speed & kSpeedMax, flags);
}

bool PosteApp::request_stop(bool purge_queue) {
  return send_command(FrameType::CmdStop, 0, 0, purge_queue ? flag::kPurgeQueue : 0);
}

void PosteApp::start_pairing(uint16_t station, uint8_t speed) {
  pairings_.start_pairing(station, speed, clock_.now_s(), profile_.enocean.pairing_mode_timeout_s);
}

bool PosteApp::feed_enocean(uint8_t byte) {
  Esp3Packet packet;
  if (!decoder_.feed(byte, packet)) return false;

  RpsTelegram rps;
  if (!Esp3Decoder::parse_rps(packet, rps)) return false;
  if (!rps.pressed) return false;  // relâchement : ignoré

  // Le PTM 210 émet 3 sous-télégrammes identiques par appui (§7).
  if (!dedup_.accept(rps.sender_id, rps.data, clock_.now_ms())) {
    ++stats_.enocean_duplicates;
    return false;
  }
  ++stats_.enocean_telegrams;

  if (pairings_.pairing_active(clock_.now_s())) {
    if (pairings_.complete_pairing(rps.sender_id, rps.rocker, clock_.now_s())) {
      ++stats_.pairings_done;
      return true;
    }
    return false;
  }

  Pairing p;
  if (!pairings_.lookup(rps.sender_id, rps.rocker, p)) {
    // Bouton inconnu : on ne devine JAMAIS une station. L'appui est compté pour
    // que l'exploitant voie qu'un bouton non appairé est utilisé.
    ++stats_.enocean_unpaired;
    return false;
  }
  return request_goto(p.station, p.speed);
}

void PosteApp::tick() {
  transport_.tick();

  Frame f;
  while (transport_.poll(f)) {
    switch (f.type) {
      case FrameType::Telemetry:
        snapshot_.station = f.station;
        snapshot_.speed = f.speed;
        snapshot_.moving = (f.flags & flag::kStatusMoving) != 0;
        snapshot_.in_station = (f.flags & flag::kStatusInStation) != 0;
        snapshot_.fault = (f.flags & flag::kStatusFault) != 0;
        snapshot_.last_update_ms = clock_.now_ms();
        snapshot_.valid = true;
        break;
      case FrameType::Ack:
        if ((f.flags & flag::kNack) != 0) {
          ++stats_.nacks_received;
        } else {
          ++stats_.acks_received;
        }
        break;
      default:
        break;
    }
  }
}

size_t PosteApp::render_agvdump(char* out, size_t capacity) const {
  if (out == nullptr || capacity == 0) return 0;
  const LinkHealth link = transport_.health();
  const uint32_t age = telemetry_age_ms();
  const int n = std::snprintf(
      out, capacity,
      "AIO AGV CONTROL - DUMP (source: poste fixe, telemetrie)\n"
      "profile=%s\n"
      "\n[AGV STATE]\n"
      "current_station=%u\n"
      "current_speed=%u\n"
      "moving=%u\n"
      "in_station=%u\n"
      "fault=%u\n"
      "telemetry_valid=%u\n"
      "telemetry_age_ms=%u\n"
      "\n[LINK]\n"
      "transport=%s\n"
      "rssi_dbm=%d\n"
      "tx_ok=%u\n"
      "tx_failed=%u\n"
      "rx_ok=%u\n"
      "rx_bad_crc=%u\n"
      "tx_refused_duty=%u\n"
      "duty_used_permille=%u\n"
      "\n[POSTE]\n"
      "enocean_telegrams=%u\n"
      "enocean_duplicates=%u\n"
      "enocean_unpaired=%u\n"
      "commands_sent=%u\n"
      "commands_refused=%u\n"
      "acks_received=%u\n"
      "nacks_received=%u\n"
      "pairings_done=%u\n"
      "operator_feedback=%u\n",
      profile_.name, snapshot_.station, snapshot_.speed, snapshot_.moving ? 1u : 0u,
      snapshot_.in_station ? 1u : 0u, snapshot_.fault ? 1u : 0u, snapshot_.valid ? 1u : 0u,
      (age == UINT32_MAX) ? 0u : age, transport_.name(), link.rssi_dbm, link.tx_ok,
      link.tx_failed, link.rx_ok, link.rx_bad_crc, link.tx_refused_duty,
      link.duty_used_permille, stats_.enocean_telegrams, stats_.enocean_duplicates,
      stats_.enocean_unpaired, stats_.commands_sent, stats_.commands_refused,
      stats_.acks_received, stats_.nacks_received, stats_.pairings_done,
      operator_feedback_available() ? 1u : 0u);
  if (n < 0) return 0;
  return (static_cast<size_t>(n) < capacity) ? static_cast<size_t>(n) : capacity - 1;
}

uint32_t PosteApp::telemetry_age_ms() const {
  if (!snapshot_.valid) return UINT32_MAX;
  return clock_.now_ms() - snapshot_.last_update_ms;
}

}  // namespace agv
