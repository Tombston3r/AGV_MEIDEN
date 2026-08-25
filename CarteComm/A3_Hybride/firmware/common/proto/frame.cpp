#include "proto/frame.h"

#include "proto/crc16.h"

namespace agv {

size_t encode_frame(const Frame& f, uint8_t* out, size_t capacity) {
  const bool ts = f.timestamped();
  const size_t size = ts ? kFrameMaxSize : kFrameBaseSize;
  if (out == nullptr || capacity < size) return 0;

  size_t i = 0;
  out[i++] = static_cast<uint8_t>(((f.ver & 0x0Fu) << 4) |
                                  (static_cast<uint8_t>(f.type) & 0x0Fu));
  out[i++] = static_cast<uint8_t>(f.node_id >> 8);
  out[i++] = static_cast<uint8_t>(f.node_id & 0xFFu);
  out[i++] = f.seq;

  // station sur 10 bits, speed sur 4, alignés à gauche du champ 16 bits.
  const uint16_t packed = static_cast<uint16_t>(((f.station & 0x03FFu) << 6) |
                                                ((f.speed & 0x0Fu) << 2));
  out[i++] = static_cast<uint8_t>(packed >> 8);
  out[i++] = static_cast<uint8_t>(packed & 0xFFu);
  out[i++] = f.flags;

  if (ts) {
    out[i++] = static_cast<uint8_t>(f.ts_s >> 24);
    out[i++] = static_cast<uint8_t>(f.ts_s >> 16);
    out[i++] = static_cast<uint8_t>(f.ts_s >> 8);
    out[i++] = static_cast<uint8_t>(f.ts_s & 0xFFu);
  }

  const uint16_t crc = crc16_ccitt(out, i);
  out[i++] = static_cast<uint8_t>(crc >> 8);
  out[i++] = static_cast<uint8_t>(crc & 0xFFu);
  return i;
}

FrameError decode_frame(const uint8_t* data, size_t len, Frame& out, uint8_t expected_version) {
  if (data == nullptr || len < kFrameBaseSize) return FrameError::TooShort;

  const uint8_t flags = data[6];
  const bool ts = (flags & flag::kTimestamped) != 0;
  const size_t expected = ts ? kFrameMaxSize : kFrameBaseSize;
  if (len != expected) return FrameError::BadLength;

  const uint16_t crc_calc = crc16_ccitt(data, expected - 2);
  const uint16_t crc_recv = static_cast<uint16_t>((data[expected - 2] << 8) | data[expected - 1]);
  if (crc_calc != crc_recv) return FrameError::BadCrc;

  Frame f;
  f.ver = static_cast<uint8_t>(data[0] >> 4);
  const uint8_t type_raw = static_cast<uint8_t>(data[0] & 0x0Fu);
  if (type_raw > static_cast<uint8_t>(FrameType::Pair)) return FrameError::UnknownType;
  f.type = static_cast<FrameType>(type_raw);
  if (expected_version != 0 && f.ver != expected_version) return FrameError::BadVersion;

  f.node_id = static_cast<uint16_t>((data[1] << 8) | data[2]);
  f.seq = data[3];
  const uint16_t packed = static_cast<uint16_t>((data[4] << 8) | data[5]);
  f.station = static_cast<uint16_t>((packed >> 6) & 0x03FFu);
  f.speed = static_cast<uint8_t>((packed >> 2) & 0x0Fu);
  f.flags = flags;
  if (ts) {
    f.ts_s = (static_cast<uint32_t>(data[7]) << 24) | (static_cast<uint32_t>(data[8]) << 16) |
             (static_cast<uint32_t>(data[9]) << 8) | static_cast<uint32_t>(data[10]);
  }
  out = f;
  return FrameError::Ok;
}

const char* frame_error_str(FrameError e) {
  switch (e) {
    case FrameError::Ok: return "OK";
    case FrameError::TooShort: return "TOO_SHORT";
    case FrameError::BadCrc: return "BAD_CRC";
    case FrameError::BadVersion: return "BAD_VERSION";
    case FrameError::BadLength: return "BAD_LENGTH";
    case FrameError::UnknownType: return "UNKNOWN_TYPE";
  }
  return "?";
}

const char* frame_type_str(FrameType t) {
  switch (t) {
    case FrameType::CmdGoto: return "CMD_GOTO";
    case FrameType::CmdStop: return "CMD_STOP";
    case FrameType::Ack: return "ACK";
    case FrameType::Telemetry: return "TELEMETRY";
    case FrameType::Ping: return "PING";
    case FrameType::Pair: return "PAIR";
  }
  return "?";
}

}  // namespace agv
