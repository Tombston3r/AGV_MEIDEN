#include "bus/mega_uart_bus.h"

#include "proto/crc16.h"

namespace agv {

size_t MegaUartBus::encode_request(uint8_t cmd, const uint8_t* payload, uint8_t len, uint8_t* out,
                                   size_t capacity) {
  if (capacity < static_cast<size_t>(len) + 5u) return 0;
  size_t i = 0;
  out[i++] = kMegaSofRequest;
  out[i++] = cmd;
  out[i++] = len;
  for (uint8_t k = 0; k < len; ++k) out[i++] = payload[k];
  const uint16_t crc = crc16_ccitt(out + 1, i - 1);
  out[i++] = static_cast<uint8_t>(crc >> 8);
  out[i++] = static_cast<uint8_t>(crc & 0xFFu);
  return i;
}

bool MegaUartBus::exchange(uint8_t cmd, const uint8_t* payload, uint8_t len, uint8_t* reply,
                           uint8_t& reply_len) {
  uint8_t frame[16];
  const size_t n = encode_request(cmd, payload, len, frame, sizeof(frame));
  if (n == 0) return false;
  uart_.write(frame, n);

  // Attente de réponse bornée : un MEGA muet ne doit pas figer la tâche bus.
  uint8_t buf[16];
  size_t got = 0;
  const uint64_t deadline = clock_.now_us() + reply_timeout_us_;
  while (clock_.now_us() < deadline) {
    got += uart_.read(buf + got, sizeof(buf) - got);
    if (got >= 5) {
      if (buf[0] != kMegaSofReply) {
        ++desyncs_;
        return false;
      }
      const uint8_t rlen = buf[2];
      const size_t total = static_cast<size_t>(rlen) + 5u;
      if (total > sizeof(buf)) {
        ++desyncs_;
        return false;
      }
      if (got >= total) {
        const uint16_t crc_calc = crc16_ccitt(buf + 1, total - 3);
        const uint16_t crc_recv =
            static_cast<uint16_t>((buf[total - 2] << 8) | buf[total - 1]);
        if (crc_calc != crc_recv || buf[1] != cmd) {
          ++desyncs_;
          return false;
        }
        for (uint8_t k = 0; k < rlen; ++k) reply[k] = buf[3 + k];
        reply_len = rlen;
        return true;
      }
    }
    clock_.delay_us(50);
  }
  ++desyncs_;
  return false;
}

bool MegaUartBus::begin() {
  uint8_t reply[8] = {};
  uint8_t reply_len = 0;
  if (!exchange(kMegaCmdPing, nullptr, 0, reply, reply_len)) return false;
  peer_version_ = (reply_len > 0) ? reply[0] : 0;
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  return writeX(0);  // §3.1 : bus X à zéro avant toute commande
}

bool MegaUartBus::writeX(uint32_t word) {
  const uint64_t started = clock_.now_us();
  const uint8_t payload[3] = {static_cast<uint8_t>((word >> 16) & 0x3Fu),
                              static_cast<uint8_t>((word >> 8) & 0xFFu),
                              static_cast<uint8_t>(word & 0xFFu)};
  uint8_t reply[8] = {};
  uint8_t reply_len = 0;
  if (!exchange(kMegaCmdSetX, payload, 3, reply, reply_len)) {
    ++stats_.write_errors;
    return false;
  }
  last_x_ = word;
  ++stats_.x_writes;
  stats_.last_write_us = static_cast<uint32_t>(clock_.now_us() - started);
  if (stats_.last_write_us > stats_.max_write_us) stats_.max_write_us = stats_.last_write_us;
  return true;
}

uint32_t MegaUartBus::readY() {
  uint8_t reply[8] = {};
  uint8_t reply_len = 0;
  if (!exchange(kMegaCmdGetY, nullptr, 0, reply, reply_len) || reply_len < 3) {
    return debouncer_.stable();
  }
  ++stats_.y_reads;
  const uint32_t raw = (static_cast<uint32_t>(reply[0]) << 16) |
                       (static_cast<uint32_t>(reply[1]) << 8) | static_cast<uint32_t>(reply[2]);
  return debouncer_.update(raw & ((1u << 21) - 1u), clock_.now_us());
}

bool MegaUartBus::pulse(uint8_t x_bit, uint32_t duration_us) {
  const uint8_t payload[3] = {x_bit, static_cast<uint8_t>(duration_us >> 8),
                              static_cast<uint8_t>(duration_us & 0xFFu)};
  uint8_t reply[8] = {};
  uint8_t reply_len = 0;
  return exchange(kMegaCmdPulse, payload, 3, reply, reply_len);
}

}  // namespace agv
