#include "bus/shift_bus.h"

namespace agv {

bool ShiftBus::begin() {
  gpio_.set(pins_.oe, true);    // sorties en haute impédance le temps du reset
  gpio_.set(pins_.rclk, false);
  gpio_.set(pins_.pl, true);

  // §3.1 : bus X à zéro AVANT d'autoriser les sorties.
  if (!writeX(0)) return false;
  gpio_.set(pins_.oe, false);   // OE actif bas : sorties validées
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  return true;
}

bool ShiftBus::writeX(uint32_t word) {
  const uint64_t started = clock_.now_us();

  // Le dernier 595 de la chaîne reçoit le premier octet décalé : on envoie donc
  // les poids forts en tête.
  const uint8_t tx[3] = {static_cast<uint8_t>((word >> 16) & 0x3Fu),
                         static_cast<uint8_t>((word >> 8) & 0xFFu),
                         static_cast<uint8_t>(word & 0xFFu)};
  if (!spi_.transfer(tx, nullptr, 3)) {
    ++stats_.write_errors;
    return false;
  }

  // UN SEUL front de latch pour les 22 lignes : c'est toute la valeur de cette
  // variante. Le déplacer ou le dédoubler ferait perdre la simultanéité.
  gpio_.set(pins_.rclk, true);
  clock_.delay_us(1);
  gpio_.set(pins_.rclk, false);
  ++latch_count_;

  last_x_ = word;
  ++stats_.x_writes;
  stats_.last_write_us = static_cast<uint32_t>(clock_.now_us() - started);
  if (stats_.last_write_us > stats_.max_write_us) stats_.max_write_us = stats_.last_write_us;
  return true;
}

uint32_t ShiftBus::readY() {
  // Capture parallèle des 165 : PL bas, puis décalage série.
  gpio_.set(pins_.pl, false);
  clock_.delay_us(1);
  gpio_.set(pins_.pl, true);

  uint8_t rx[3] = {};
  const uint8_t zero[3] = {0, 0, 0};
  if (!spi_.transfer(zero, rx, 3)) {
    ++stats_.write_errors;
    return debouncer_.stable();
  }
  ++stats_.y_reads;
  const uint32_t raw = (static_cast<uint32_t>(rx[0]) << 16) |
                       (static_cast<uint32_t>(rx[1]) << 8) | static_cast<uint32_t>(rx[2]);
  return debouncer_.update(raw & ((1u << 21) - 1u), clock_.now_us());
}

bool ShiftBus::pulse(uint8_t x_bit, uint32_t duration_us) {
  const bool active = profile_.bus.x_active_high;
  if (!writeX(with_bit(last_x_, x_bit, active))) return false;
  clock_.delay_us(duration_us);
  return writeX(with_bit(last_x_, x_bit, !active));
}

}  // namespace agv
