#include "proto/crc16.h"

namespace agv {

uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t seed) {
  uint16_t crc = seed;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000u) ? static_cast<uint16_t>((crc << 1) ^ 0x1021u)
                            : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

uint8_t crc8_enocean(const uint8_t* data, size_t len, uint8_t seed) {
  uint8_t crc = seed;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1) ^ 0x07u)
                          : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

}  // namespace agv
