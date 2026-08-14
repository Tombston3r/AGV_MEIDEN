#include "bus/avr_port_bus.h"

namespace agv {
namespace {
constexpr uint32_t kXMask = (1u << 22) - 1u;
constexpr uint32_t kYMask = (1u << 21) - 1u;
}  // namespace

bool AvrPortBus::begin() {
  // §3.1 : le bus X doit être physiquement à zéro AVANT que le firmware ne
  // prenne la main. On force donc l'état de repos avant de passer les broches
  // en sortie — l'ordre inverse produirait une impulsion parasite sur les 22
  // lignes au démarrage, que l'automate pourrait interpréter.
  const uint32_t idle = profile_.bus.x_active_high ? 0u : kXMask;
  for (int i = 0; i < 3; ++i) {
    if (ports_.port_x[i].out != nullptr) {
      *ports_.port_x[i].out = static_cast<uint8_t>((idle >> (8 * i)) & 0xFFu);
    }
  }
  for (int i = 0; i < 3; ++i) {
    if (ports_.port_x[i].dir != nullptr) {
      *ports_.port_x[i].dir = (i == 2) ? 0x3Fu : 0xFFu;  // 22 bits : 8 + 8 + 6
    }
  }
  // Entrées Y : direction en entrée, pull-ups laissées au matériel (les
  // optocoupleurs de la carte imposent déjà un niveau de repos).
  for (int i = 0; i < 3; ++i) {
    if (ports_.port_y[i].dir != nullptr) {
      *ports_.port_y[i].dir = 0x00u;
    }
  }

  last_x_ = 0;
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  return true;
}

bool AvrPortBus::writeX(uint32_t word) {
  const uint64_t started = clock_.now_us();
  const uint32_t masked = word & kXMask;

  // Section critique : les trois écritures de port ne doivent pas être
  // séparées par une interruption, sinon la simultanéité est perdue et le
  // strobe X93 peut tomber sur un mot incomplet.
  critical_.enter();
  if (ports_.port_x[0].out != nullptr) *ports_.port_x[0].out = static_cast<uint8_t>(masked & 0xFFu);
  if (ports_.port_x[1].out != nullptr) {
    *ports_.port_x[1].out = static_cast<uint8_t>((masked >> 8) & 0xFFu);
  }
  if (ports_.port_x[2].out != nullptr) {
    // Les 6 bits de poids fort seulement : le reste du port peut servir à
    // autre chose sur la carte, on ne l'écrase pas.
    const uint8_t high = static_cast<uint8_t>((masked >> 16) & 0x3Fu);
    *ports_.port_x[2].out = static_cast<uint8_t>((*ports_.port_x[2].out & 0xC0u) | high);
  }
  critical_.leave();

  last_x_ = masked;
  ++stats_.x_writes;
  stats_.last_write_us = static_cast<uint32_t>(clock_.now_us() - started);
  if (stats_.last_write_us > stats_.max_write_us) stats_.max_write_us = stats_.last_write_us;
  return true;
}

uint32_t AvrPortBus::readY() {
  uint32_t raw = 0;
  critical_.enter();
  if (ports_.port_y[0].in != nullptr) raw |= static_cast<uint32_t>(*ports_.port_y[0].in);
  if (ports_.port_y[1].in != nullptr) raw |= static_cast<uint32_t>(*ports_.port_y[1].in) << 8;
  if (ports_.port_y[2].in != nullptr) {
    raw |= static_cast<uint32_t>(*ports_.port_y[2].in & 0x1Fu) << 16;
  }
  critical_.leave();

  ++stats_.y_reads;
  return debouncer_.update(raw & kYMask, clock_.now_us());
}

bool AvrPortBus::pulse(uint8_t x_bit, uint32_t duration_us) {
  const bool active = profile_.bus.x_active_high;
  if (!writeX(with_bit(last_x_, x_bit, active))) return false;
  clock_.delay_us(duration_us);
  return writeX(with_bit(last_x_, x_bit, !active));
}

bool AvrPortBus::drive_single(uint8_t x_bit) {
  if (x_bit >= 22) return false;
  const uint32_t idle = profile_.bus.x_active_high ? 0u : kXMask;
  return writeX(with_bit(idle, x_bit, profile_.bus.x_active_high));
}

}  // namespace agv
