#include "app/agvdump.h"

#include <cstdio>

namespace agv {

size_t render_agvdump(const AgvDumpInput& in, char* out, size_t capacity) {
  if (out == nullptr || capacity == 0) return 0;
  int n = 0;
  const auto append = [&](const char* fmt, auto... args) {
    if (n < 0 || static_cast<size_t>(n) >= capacity) return;
    const int written = std::snprintf(out + n, capacity - static_cast<size_t>(n), fmt, args...);
    if (written > 0) n += written;
  };

  static const SeqCounters kEmpty{};
  const SeqCounters& c = (in.counters != nullptr) ? *in.counters : kEmpty;

  append("AIO AGV CONTROL - DUMP\n");
  append("profile=%s\n", in.profile_name);
  append("uptime_s=%u\n", in.uptime_s);
  append("\n[AGV STATE]\n");
  append("state=%s\n",
         (in.sequencer != nullptr) ? Sequencer::state_str(in.sequencer->state()) : "?");
  append("fault=%s\n",
         (in.sequencer != nullptr) ? Sequencer::fault_str(in.sequencer->fault_cause()) : "?");
  append("current_station=%u\n", c.current_station);
  append("current_speed=%u\n", c.current_speed);
  append("moving=%u\n", (in.sequencer != nullptr && in.sequencer->moving()) ? 1u : 0u);
  append("in_station=%u\n", (in.sequencer != nullptr && in.sequencer->in_station()) ? 1u : 0u);
  append("plc_fault=%u\n", (in.sequencer != nullptr && in.sequencer->plc_fault()) ? 1u : 0u);
  append("no_destination=%u\n",
         (in.sequencer != nullptr && in.sequencer->no_destination_flag()) ? 1u : 0u);
  append("switch_monitor=%u\n",
         (in.sequencer != nullptr && in.sequencer->switch_echo()) ? 1u : 0u);
  append("direction_monitor=%u\n",
         (in.sequencer != nullptr && in.sequencer->direction_echo()) ? 1u : 0u);
  append("write_tries=%u\n", c.write_tries);
  append("write_op_return=%s\n", Sequencer::op_return_str(c.write_op_return));
  append("start_tries=%u\n", c.start_tries);
  append("start_op_return=%s\n", Sequencer::op_return_str(c.start_op_return));
  append("stop_tries=%u\n", c.stop_tries);
  append("stop_op_return=%s\n", Sequencer::op_return_str(c.stop_op_return));
  append("courses_completed=%u\n", c.courses_completed);
  append("faults=%u\n", c.faults);
  append("y22_timeouts=%u\n", c.y22_timeouts);
  append("y05_timeouts=%u\n", c.y05_timeouts);
  append("y10_timeouts=%u\n", c.y10_timeouts);
  append("battery_mv=%u\n", in.battery_mv);

  append("\n[QUEUE]\n");
  append("nb_courses_programmed=%u\n", c.nb_courses_programmed);
  if (in.queue != nullptr) {
    for (size_t i = 0; i < kMaxCourses; ++i) {
      if (i < in.queue->size()) {
        const Course& course = in.queue->at(i);
        append("programmed_courses[%u]=%u,%u,%u\n", static_cast<unsigned>(i), course.station,
               course.speed, course.node_id);
      } else {
        append("programmed_courses[%u]=-\n", static_cast<unsigned>(i));
      }
    }
  }

  append("\n[LINK]\n");
  append("transport=%s\n", in.transport_name);
  append("link_up=%u\n", in.link_up ? 1u : 0u);
  if (in.link != nullptr) {
    append("rssi_dbm=%d\n", in.link->rssi_dbm);
    append("snr_db=%d\n", in.link->snr_db);
    append("last_rx_ms=%u\n", in.link->last_rx_ms);
    append("last_ack_ms=%u\n", in.link->last_ack_ms);
    append("tx_ok=%u\n", in.link->tx_ok);
    append("tx_failed=%u\n", in.link->tx_failed);
    append("rx_ok=%u\n", in.link->rx_ok);
    append("rx_bad_crc=%u\n", in.link->rx_bad_crc);
    append("tx_refused_duty=%u\n", in.link->tx_refused_duty);
    append("duty_used_permille=%u\n", in.link->duty_used_permille);
  }

  if (n < 0) return 0;
  const size_t len = static_cast<size_t>(n) < capacity ? static_cast<size_t>(n) : capacity - 1;
  out[len] = '\0';
  return len;
}

}  // namespace agv
