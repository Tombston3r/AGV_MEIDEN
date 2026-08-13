// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) — brief §5.1.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t seed = 0xFFFFu);

// CRC8 EnOcean ESP3 (poly 0x07) — utilisé par le décodeur du §7.
uint8_t crc8_enocean(const uint8_t* data, size_t len, uint8_t seed = 0x00u);

}  // namespace agv
