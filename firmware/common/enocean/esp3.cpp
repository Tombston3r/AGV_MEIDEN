#include "enocean/esp3.h"

#include "proto/crc16.h"

namespace agv {

bool Esp3Decoder::feed(uint8_t byte, Esp3Packet& out) {
  switch (state_) {
    case State::Sync:
      if (byte == kEsp3Sync) {
        index_ = 0;
        state_ = State::Header;
      }
      break;

    case State::Header:
      header_[index_++] = byte;
      if (index_ == 4) state_ = State::CrcHeader;
      break;

    case State::CrcHeader: {
      if (crc8_enocean(header_, 4) != byte) {
        ++crc_header_errors_;
        state_ = State::Sync;  // resynchronisation sur le prochain 0x55
        break;
      }
      current_ = Esp3Packet{};
      current_.data_len = static_cast<uint16_t>((header_[0] << 8) | header_[1]);
      current_.opt_len = header_[2];
      current_.packet_type = header_[3];
      if (current_.data_len > kEsp3DataMax || current_.opt_len > kEsp3OptMax) {
        state_ = State::Sync;  // trame plus longue que ce qu'on sait traiter
        break;
      }
      payload_expected_ = current_.data_len + current_.opt_len;
      index_ = 0;
      state_ = (payload_expected_ == 0) ? State::CrcData : State::Payload;
      break;
    }

    case State::Payload:
      if (index_ < current_.data_len) {
        current_.data[index_] = byte;
      } else {
        current_.opt[index_ - current_.data_len] = byte;
      }
      ++index_;
      if (index_ == payload_expected_) state_ = State::CrcData;
      break;

    case State::CrcData: {
      uint8_t buf[kEsp3DataMax + kEsp3OptMax];
      size_t n = 0;
      for (size_t i = 0; i < current_.data_len; ++i) buf[n++] = current_.data[i];
      for (size_t i = 0; i < current_.opt_len; ++i) buf[n++] = current_.opt[i];
      state_ = State::Sync;
      if (crc8_enocean(buf, n) != byte) {
        ++crc_data_errors_;
        return false;
      }
      // OptData d'un RADIO_ERP1 : [subTelNum][destinationID×4][dBm][security]
      if (current_.packet_type == kEsp3TypeRadioErp1 && current_.opt_len >= 6) {
        current_.rssi_dbm = -static_cast<int8_t>(current_.opt[5]);
      }
      ++packets_ok_;
      out = current_;
      return true;
    }
  }
  return false;
}

bool Esp3Decoder::parse_rps(const Esp3Packet& p, RpsTelegram& out) {
  // RPS : RORG(1) + data(1) + senderID(4) + status(1) = 7 octets.
  if (p.packet_type != kEsp3TypeRadioErp1) return false;
  if (p.data_len < 7 || p.data[0] != kRorgRps) return false;

  out.data = p.data[1];
  out.sender_id = (static_cast<uint32_t>(p.data[2]) << 24) |
                  (static_cast<uint32_t>(p.data[3]) << 16) |
                  (static_cast<uint32_t>(p.data[4]) << 8) | static_cast<uint32_t>(p.data[5]);
  out.status = p.data[6];
  out.rssi_dbm = p.rssi_dbm;
  // Bit 4 (energy bow) : 1 = appui, 0 = relâchement.
  out.pressed = (out.data & 0x10u) != 0u;
  // Bits 7..5 : identifiant du bascule actionné (R1).
  out.rocker = static_cast<uint8_t>((out.data >> 5) & 0x03u);
  return true;
}

bool EnoceanDeduplicator::accept(uint32_t sender_id, uint8_t data, uint32_t now_ms) {
  for (const auto& slot : slots_) {
    if (slot.used && slot.sender_id == sender_id && slot.data == data &&
        (now_ms - slot.at_ms) <= window_ms_) {
      ++duplicates_;
      return false;
    }
  }
  slots_[next_] = Slot{sender_id, data, now_ms, true};
  next_ = (next_ + 1) % kSlots;
  return true;
}

}  // namespace agv
