// Variante B du §4.4 : 3× 74HC595 (sorties) + 3× 74HC165 (entrées) sur SPI.
//
// C'est la variante PRÉFÉRABLE : ~3 µs de pose, et surtout SIMULTANÉE — les 22
// lignes basculent au même front de latch RCLK, propriété que le MEGA d'origine
// avait gratuitement avec `PORTA = x` et que les MCP23017 n'ont pas.
//
// Ordre imposé : décalage complet des 3 octets, PUIS impulsion RCLK. Ne jamais
// latcher entre deux octets, sinon on recrée un décalage entre groupes de bits.
#pragma once

#include <cstdint>

#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "hal/bus_ports.h"

namespace agv {

struct ShiftPins {
  uint8_t rclk = 0;  // latch commun des 74HC595 (storage register clock)
  uint8_t pl = 1;    // parallel load des 74HC165 (actif bas)
  uint8_t oe = 2;    // output enable des 595 (actif bas), maintien à 0 au repos
};

class ShiftBus final : public IBusDriver {
 public:
  ShiftBus(const HardwareProfile& profile, ISpiBus& spi, IGpio& gpio, IMicroClock& clock,
           const ShiftPins& pins = ShiftPins{})
      : profile_(profile), spi_(spi), gpio_(gpio), clock_(clock), pins_(pins),
        debouncer_(profile.bus.y_debounce_us) {}

  bool begin() override;
  bool writeX(uint32_t word) override;
  uint32_t lastX() const override { return last_x_; }
  uint32_t readY() override;
  bool pulse(uint8_t x_bit, uint32_t duration_us) override;
  const char* name() const override { return "shift595"; }
  const BusStats& stats() const override { return stats_; }
  uint64_t now_us() const override { return clock_.now_us(); }

  uint32_t latch_count() const { return latch_count_; }

 private:
  const HardwareProfile& profile_;
  ISpiBus& spi_;
  IGpio& gpio_;
  IMicroClock& clock_;
  ShiftPins pins_;
  YDebouncer debouncer_;
  BusStats stats_{};
  uint32_t last_x_ = 0;
  uint32_t latch_count_ = 0;
};

}  // namespace agv
