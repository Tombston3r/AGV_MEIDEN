#include "app/agvdump.h"

#include <cstdio>

namespace agv {
namespace {

// Noms d'états et de défauts : repris littéralement de ceux du séquenceur, qui
// tourne sur l'ATmega. L'ESP32 ne fait que les remettre en forme.
const char* seq_state_str(uint8_t v) {
  static const char* kNames[] = {"BOOT",  "IDLE",          "WRITE_SETUP", "WRITE_STROBE",
                                 "WRITE_RELEASE", "START_PULSE", "START_RELEASE", "TRANSIT",
                                 "ARRIVED", "STOP_PULSE", "SAFE_STOP", "FAULT"};
  return (v < sizeof(kNames) / sizeof(kNames[0])) ? kNames[v] : "?";
}

const char* fault_str(uint8_t v) {
  static const char* kNames[] = {"NONE",           "WRITE_TIMEOUT", "START_TIMEOUT",
                                 "STOP_TIMEOUT",   "ARRIVAL_TIMEOUT", "PLC_FAULT",
                                 "NO_DESTINATION", "BUS_WRITE_ERROR", "LINK_LOST"};
  return (v < sizeof(kNames) / sizeof(kNames[0])) ? kNames[v] : "?";
}

}  // namespace

size_t render_agvdump(const AgvDumpInput& in, char* out, size_t capacity) {
  if (out == nullptr || capacity == 0) return 0;
  static const link::LinkState kEmpty{};
  const link::LinkState& s = (in.state != nullptr) ? *in.state : kEmpty;

  const int n = std::snprintf(
      out, capacity,
      "AIO AGV CONTROL - DUMP\n"
      "profile=%s\n"
      "uptime_s=%u\n"
      "\n[AGV STATE]\n"
      "state=%s\n"
      "fault=%s\n"
      "current_station=%u\n"
      "current_speed=%u\n"
      "moving=%u\n"
      "in_station=%u\n"
      "plc_fault=%u\n"
      "no_destination=%u\n"
      "safe_stop=%u\n"
      "heartbeat_ok=%u\n"
      "write_tries=%u\n"
      "start_tries=%u\n"
      "stop_tries=%u\n"
      "last_seq=%u\n"
      "battery_mv=%u\n"
      "\n[QUEUE]\n"
      "nb_courses_programmed=%u\n"
      "\n[LINK]\n"
      "mcu_link_up=%u\n"
      "link_timeouts=%u\n"
      "heartbeats_sent=%u\n"
      "wifi_up=%u\n"
      "ssid=%s\n"
      "rssi_dbm=%d\n"
      "mqtt_up=%u\n"
      "cmd_expired=%u\n"
      "cmd_duplicate=%u\n",
      in.profile_name, in.uptime_s, seq_state_str(s.seq_state), fault_str(s.fault), s.station,
      s.speed, (s.flags & link::state_flag::kMoving) ? 1u : 0u,
      (s.flags & link::state_flag::kInStation) ? 1u : 0u,
      (s.flags & link::state_flag::kPlcFault) ? 1u : 0u,
      (s.flags & link::state_flag::kNoDestination) ? 1u : 0u,
      (s.flags & link::state_flag::kSafeStop) ? 1u : 0u,
      (s.flags & link::state_flag::kHeartbeatOk) ? 1u : 0u, s.write_tries, s.start_tries,
      s.stop_tries, s.last_seq, in.battery_mv, s.queue_len, in.link_up ? 1u : 0u,
      in.link_timeouts, in.heartbeats_sent, in.wifi_up ? 1u : 0u, in.ssid, in.rssi_dbm,
      in.mqtt_up ? 1u : 0u, in.cmd_expired, in.cmd_duplicate);

  if (n < 0) return 0;
  const size_t len = (static_cast<size_t>(n) < capacity) ? static_cast<size_t>(n) : capacity - 1;
  out[len] = '\0';
  return len;
}

}  // namespace agv
