#include "bus/mcp23017_bus.h"

namespace agv {

bool Mcp23017Bus::begin() {
  // Sorties X : IODIR = 0 sur les deux ports.
  const uint8_t out_dir[2] = {0x00, 0x00};
  if (!i2c_.write_reg(addr_.x_low, kMcpIodirA, out_dir, 2)) return false;
  if (!i2c_.write_reg(addr_.x_high, kMcpIodirA, out_dir, 2)) return false;

  // Entrées Y : IODIR = 1, pull-ups activées.
  const uint8_t in_dir[2] = {0xFF, 0xFF};
  if (!i2c_.write_reg(addr_.y_low, kMcpIodirA, in_dir, 2)) return false;
  if (!i2c_.write_reg(addr_.y_high, kMcpIodirA, in_dir, 2)) return false;
  if (!i2c_.write_reg(addr_.y_low, kMcpGppuA, in_dir, 2)) return false;
  if (!i2c_.write_reg(addr_.y_high, kMcpGppuA, in_dir, 2)) return false;

  // §3.1 : bus X à zéro avant toute autre action.
  const uint8_t zero[2] = {0x00, 0x00};
  if (!i2c_.write_reg(addr_.x_low, kMcpOlatA, zero, 2)) return false;
  if (!i2c_.write_reg(addr_.x_high, kMcpOlatA, zero, 2)) return false;
  last_x_ = 0;
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  return true;
}

bool Mcp23017Bus::writeX(uint32_t word) {
  const uint64_t started = clock_.now_us();

  // Deux octets par composant en UNE transaction : GPIOA puis GPIOB via
  // l'auto-incrément d'adresse. C'est ce qui ramène le décalage A/B au seul
  // temps d'un octet I²C (~25 µs à 400 kHz) au lieu de deux transactions.
  const uint8_t low[2] = {static_cast<uint8_t>(word & 0xFFu),
                          static_cast<uint8_t>((word >> 8) & 0xFFu)};
  const uint8_t high[2] = {static_cast<uint8_t>((word >> 16) & 0xFFu),
                           static_cast<uint8_t>((word >> 24) & 0x3Fu)};

  if (!i2c_.write_reg(addr_.x_low, kMcpGpioA, low, 2)) {
    ++stats_.write_errors;
    return false;
  }
  if (!i2c_.write_reg(addr_.x_high, kMcpGpioA, high, 2)) {
    ++stats_.write_errors;
    return false;
  }

  last_x_ = word;
  ++stats_.x_writes;
  stats_.last_write_us = static_cast<uint32_t>(clock_.now_us() - started);
  if (stats_.last_write_us > stats_.max_write_us) stats_.max_write_us = stats_.last_write_us;
  return true;
}

uint32_t Mcp23017Bus::readY() {
  uint8_t low[2] = {};
  uint8_t high[2] = {};
  if (!i2c_.read_reg(addr_.y_low, kMcpGpioA, low, 2)) {
    ++stats_.write_errors;
    return debouncer_.stable();
  }
  if (!i2c_.read_reg(addr_.y_high, kMcpGpioA, high, 2)) {
    ++stats_.write_errors;
    return debouncer_.stable();
  }
  ++stats_.y_reads;
  const uint32_t raw = static_cast<uint32_t>(low[0]) | (static_cast<uint32_t>(low[1]) << 8) |
                       (static_cast<uint32_t>(high[0]) << 16) |
                       (static_cast<uint32_t>(high[1] & 0x1Fu) << 24);
  return debouncer_.update(raw, clock_.now_us());
}

bool Mcp23017Bus::pulse(uint8_t x_bit, uint32_t duration_us) {
  const bool active = profile_.bus.x_active_high;
  if (!writeX(with_bit(last_x_, x_bit, active))) return false;
  clock_.delay_us(duration_us);
  return writeX(with_bit(last_x_, x_bit, !active));
}

}  // namespace agv
