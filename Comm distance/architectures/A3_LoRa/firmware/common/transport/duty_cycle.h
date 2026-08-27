// Budget de rapport cyclique 868 MHz — OBLIGATION RÉGLEMENTAIRE (brief §6).
//
// EN 300 220 / ERC 70-03 imposent 1 % de temps d'émission sur la bande. Ce
// budget glissant sur 1 h REFUSE l'émission au-delà et remonte le refus en
// défaut applicatif visible. Ce n'est pas une option de confort.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

// Temps d'antenne d'une trame LoRa, en microsecondes (Semtech AN1200.13).
uint32_t lora_airtime_us(size_t payload_bytes, uint8_t spreading_factor, uint32_t bandwidth_hz,
                         uint8_t coding_rate, uint8_t preamble_symbols = 8,
                         bool explicit_header = true, bool crc_on = true,
                         bool low_data_rate_optimize = false);

class DutyCycleBudget {
 public:
  static constexpr size_t kMaxEvents = 64;

  // `permille` : 10 ‰ = 1 %. `window_ms` : 3 600 000 pour une heure glissante.
  DutyCycleBudget(uint32_t permille, uint32_t window_ms)
      : permille_(permille), window_ms_(window_ms) {}

  // L'émission de `airtime_us` est-elle autorisée à l'instant `now_ms` ?
  bool can_transmit(uint32_t airtime_us, uint32_t now_ms) const;

  // Enregistre une émission effectuée. À n'appeler qu'après un envoi réel.
  void record(uint32_t airtime_us, uint32_t now_ms);

  // Temps d'antenne consommé dans la fenêtre glissante, en microsecondes.
  uint64_t used_us(uint32_t now_ms) const;
  // Budget total de la fenêtre, en microsecondes.
  uint64_t budget_us() const {
    return (static_cast<uint64_t>(window_ms_) * 1000ull * permille_) / 1000ull;
  }
  // Consommation en ‰ du budget (0…1000+), pour l'affichage de supervision.
  uint32_t used_permille_of_budget(uint32_t now_ms) const;
  // Attente nécessaire avant que `airtime_us` redevienne émissible, en ms.
  uint32_t wait_ms(uint32_t airtime_us, uint32_t now_ms) const;

  uint32_t refusals() const { return refusals_; }
  void note_refusal() { ++refusals_; }
  void reset();

 private:
  struct Event {
    uint32_t at_ms;
    uint32_t airtime_us;
  };
  void prune(uint32_t now_ms) const;

  mutable Event events_[kMaxEvents] = {};
  mutable size_t count_ = 0;
  uint32_t permille_;
  uint32_t window_ms_;
  uint32_t refusals_ = 0;
};

}  // namespace agv
