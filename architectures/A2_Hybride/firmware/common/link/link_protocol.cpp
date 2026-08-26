#include "link/link_protocol.h"

#include "proto/crc16.h"

namespace agv::link {

size_t encode(uint8_t sof, Cmd cmd, const uint8_t* payload, uint8_t len, uint8_t* out,
              size_t capacity) {
  if (out == nullptr || len > kPayloadMax || capacity < static_cast<size_t>(len) + 5u) return 0;
  size_t i = 0;
  out[i++] = sof;
  out[i++] = static_cast<uint8_t>(cmd);
  out[i++] = len;
  for (uint8_t k = 0; k < len; ++k) out[i++] = payload[k];
  const uint16_t crc = crc16_ccitt(out + 1, i - 1);
  out[i++] = static_cast<uint8_t>(crc >> 8);
  out[i++] = static_cast<uint8_t>(crc & 0xFFu);
  return i;
}

size_t encode_goto(uint8_t seq, uint16_t station, uint8_t speed, uint8_t flags, uint8_t* out,
                   size_t capacity) {
  const uint8_t payload[5] = {seq, static_cast<uint8_t>(station >> 8),
                              static_cast<uint8_t>(station & 0xFFu), speed, flags};
  return encode(kSofToMega, Cmd::Goto, payload, 5, out, capacity);
}

size_t encode_ack(uint8_t seq, CmdResult result, uint8_t* out, size_t capacity) {
  const uint8_t payload[2] = {seq, static_cast<uint8_t>(result)};
  return encode(kSofToEsp, Cmd::Ack, payload, 2, out, capacity);
}

size_t encode_state(const LinkState& s, uint8_t* out, size_t capacity) {
  const uint8_t payload[kStatePayloadSize] = {
      static_cast<uint8_t>(s.station >> 8), static_cast<uint8_t>(s.station & 0xFFu),
      s.speed,       s.seq_state, s.fault,       s.flags,
      s.queue_len,   s.write_tries, s.start_tries, s.stop_tries,
      s.last_seq,    0 /* réservé : garde la charge utile alignée */};
  return encode(kSofToEsp, Cmd::State, payload, kStatePayloadSize, out, capacity);
}

bool decode_state(const uint8_t* payload, uint8_t len, LinkState& out) {
  if (payload == nullptr || len < kStatePayloadSize) return false;
  out.station = static_cast<uint16_t>((payload[0] << 8) | payload[1]);
  out.speed = payload[2];
  out.seq_state = payload[3];
  out.fault = payload[4];
  out.flags = payload[5];
  out.queue_len = payload[6];
  out.write_tries = payload[7];
  out.start_tries = payload[8];
  out.stop_tries = payload[9];
  out.last_seq = payload[10];
  return true;
}

bool Parser::feed(uint8_t byte, Cmd& cmd, uint8_t* payload, uint8_t& len) {
  switch (state_) {
    case State::Sync:
      if (byte == sof_) state_ = State::Cmd;
      break;

    case State::Cmd:
      cmd_ = byte;
      state_ = State::Len;
      break;

    case State::Len:
      len_ = byte;
      if (len_ > kPayloadMax) {
        // Longueur impossible : on repart en synchronisation plutôt que de
        // consommer aveuglément des octets qui ne sont peut-être pas à nous.
        ++resyncs_;
        state_ = State::Sync;
        break;
      }
      index_ = 0;
      state_ = (len_ == 0) ? State::CrcHi : State::Payload;
      break;

    case State::Payload:
      buffer_[index_++] = byte;
      if (index_ == len_) state_ = State::CrcHi;
      break;

    case State::CrcHi:
      crc_received_ = static_cast<uint16_t>(byte << 8);
      state_ = State::CrcLo;
      break;

    case State::CrcLo: {
      crc_received_ = static_cast<uint16_t>(crc_received_ | byte);
      state_ = State::Sync;

      uint8_t header[2 + kPayloadMax];
      header[0] = cmd_;
      header[1] = len_;
      for (uint8_t k = 0; k < len_; ++k) header[2 + k] = buffer_[k];
      if (crc16_ccitt(header, static_cast<size_t>(len_) + 2u) != crc_received_) {
        // Trame corrompue : SILENCE. L'émetteur gère son timeout ; réémettre un
        // NACK sur une liaison déjà douteuse ne ferait qu'ajouter du bruit.
        ++crc_errors_;
        return false;
      }
      cmd = static_cast<Cmd>(cmd_);
      len = len_;
      for (uint8_t k = 0; k < len_; ++k) payload[k] = buffer_[k];
      return true;
    }
  }
  return false;
}

}  // namespace agv::link
