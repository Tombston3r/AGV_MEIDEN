#include "bus/avr_port_bus.h"

namespace agv {
namespace {
constexpr uint32_t kXMask = (1u << 22) - 1u;
}

void AvrPortBus::compute_masks() {
  for (uint8_t p = 0; p < kMaxPorts; ++p) {
    x_mask_[p] = 0;
    y_mask_[p] = 0;
  }
  for (size_t i = 0; i < 22; ++i) {
    const BitLocation& loc = map_.x[i];
    if (loc.port < map_.port_count) x_mask_[loc.port] |= static_cast<uint8_t>(1u << loc.bit);
  }
  for (size_t i = 0; i < 21; ++i) {
    const BitLocation& loc = map_.y[i];
    if (loc.port < map_.port_count) y_mask_[loc.port] |= static_cast<uint8_t>(1u << loc.bit);
  }
  port_writes_ = 0;
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (x_mask_[p] != 0) ++port_writes_;
  }
}

bool AvrPortBus::begin() {
  compute_masks();
  const uint32_t idle = profile_.bus.x_active_high ? 0u : kXMask;

  critical_.enter();

  // §3.1 : l'état de repos est posé AVANT de passer les broches en sortie.
  // L'ordre inverse produirait une impulsion parasite sur les 22 lignes au
  // démarrage, que l'automate pourrait interpréter comme une commande.
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (map_.ports[p].out == nullptr || x_mask_[p] == 0) continue;
    uint8_t value = *map_.ports[p].out;
    for (size_t i = 0; i < 22; ++i) {
      const BitLocation& loc = map_.x[i];
      if (loc.port != p) continue;
      const uint8_t mask = static_cast<uint8_t>(1u << loc.bit);
      value = ((idle >> i) & 1u) ? static_cast<uint8_t>(value | mask)
                                 : static_cast<uint8_t>(value & ~mask);
    }
    *map_.ports[p].out = value;
  }

  // Directions EN MASQUE. PORTA, PORTB et PORTG sont mixtes : écrire un octet
  // de direction complet mettrait en sortie des broches pilotées par
  // l'automate — conflit électrique franc.
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (map_.ports[p].dir == nullptr) continue;
    uint8_t dir = *map_.ports[p].dir;
    dir = static_cast<uint8_t>(dir | x_mask_[p]);   // sorties X
    dir = static_cast<uint8_t>(dir & ~y_mask_[p]);  // entrées Y
    *map_.ports[p].dir = dir;
  }

  // Pull-ups des entrées Y. Si les sorties de l'automate sont à collecteur
  // ouvert, un pull-up est indispensable ; si elles sont poussées, il injecte
  // du courant à contresens. Le choix n'est pas devinable : il vient du profil
  // (§12.1, à confirmer par le relevé d'amplitude sur Y05).
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (map_.ports[p].out == nullptr || y_mask_[p] == 0) continue;
    uint8_t value = *map_.ports[p].out;
    value = profile_.bus.y_pullups ? static_cast<uint8_t>(value | y_mask_[p])
                                   : static_cast<uint8_t>(value & ~y_mask_[p]);
    *map_.ports[p].out = value;
  }

  critical_.leave();

  last_x_ = idle & kXMask;
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  return port_writes_ > 0;
}

bool AvrPortBus::writeX(uint32_t word) {
  const uint64_t started = clock_.now_us();
  const uint32_t masked = word & kXMask;

  // Les valeurs sont calculées HORS section critique : celle-ci ne contient que
  // les écritures de registre, pour la garder aussi courte que possible.
  uint8_t values[kMaxPorts] = {};
  bool touched[kMaxPorts] = {};
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (map_.ports[p].out == nullptr || x_mask_[p] == 0) continue;
    // Lecture-modification-écriture obligatoire : sur un port mixte, les bits
    // voisins sont des entrées Y (et leur réglage de pull-up).
    values[p] = static_cast<uint8_t>(*map_.ports[p].out & ~x_mask_[p]);
    touched[p] = true;
  }
  for (size_t i = 0; i < 22; ++i) {
    const BitLocation& loc = map_.x[i];
    if (loc.port >= map_.port_count || !touched[loc.port]) continue;
    if ((masked >> i) & 1u) {
      values[loc.port] = static_cast<uint8_t>(values[loc.port] | (1u << loc.bit));
    }
  }

  critical_.enter();
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (touched[p]) *map_.ports[p].out = values[p];
  }
  critical_.leave();

  last_x_ = masked;
  ++stats_.x_writes;
  stats_.last_write_us = static_cast<uint32_t>(clock_.now_us() - started);
  if (stats_.last_write_us > stats_.max_write_us) stats_.max_write_us = stats_.last_write_us;
  return true;
}

uint32_t AvrPortBus::readY() {
  uint8_t snapshot[kMaxPorts] = {};

  // Toutes les entrées sont échantillonnées dans la même section critique :
  // sinon deux signaux lus à quelques microsecondes d'écart pourraient décrire
  // deux états différents de l'automate (Y05 retombé mais Y10 pas encore vu).
  critical_.enter();
  for (uint8_t p = 0; p < map_.port_count; ++p) {
    if (map_.ports[p].in != nullptr && y_mask_[p] != 0) snapshot[p] = *map_.ports[p].in;
  }
  critical_.leave();

  uint32_t raw = 0;
  for (size_t i = 0; i < 21; ++i) {
    const BitLocation& loc = map_.y[i];
    if (loc.port >= map_.port_count) continue;
    if ((snapshot[loc.port] >> loc.bit) & 1u) raw |= (1u << i);
  }

  ++stats_.y_reads;
  return debouncer_.update(raw, clock_.now_us());
}

bool AvrPortBus::pulse(uint8_t x_bit, uint32_t duration_us) {
  const bool active = profile_.bus.x_active_high;
  if (!writeX(with_bit(last_x_, x_bit, active))) return false;
  clock_.delay_us(duration_us);
  return writeX(with_bit(last_x_, x_bit, !active));
}

bool AvrPortBus::drive_single(uint8_t x_bit) {
  if (x_bit >= 22 || map_.x[x_bit].port >= map_.port_count) return false;
  const uint32_t idle = profile_.bus.x_active_high ? 0u : kXMask;
  return writeX(with_bit(idle, x_bit, profile_.bus.x_active_high));
}

}  // namespace agv
