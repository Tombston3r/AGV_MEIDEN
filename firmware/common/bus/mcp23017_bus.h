// Variante A du §4.4 : 4× MCP23017 sur I²C.
//
// Pose du bus ~150 µs, et surtout GPIOA/GPIOB décalés d'environ 25 µs.
// Conséquences appliquées ici :
//  - GPIOA et GPIOB sont écrits en UNE SEULE transaction (adressage séquentiel
//    du MCP23017 : écrire à 0x12 avec 2 octets touche GPIOA puis GPIOB) ;
//  - le décalage résiduel est documenté et exposé (`ab_skew_us`) : il doit
//    rester très inférieur au `t_setup_us` retenu, sinon le strobe X93 peut
//    tomber alors que la moitié du mot n'est pas encore posée.
//
// C'est la variante la moins bonne des trois pour le déterminisme temporel.
#pragma once

#include <cstdint>

#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "hal/bus_ports.h"

namespace agv {

// Registres MCP23017 (IOCON.BANK = 0).
constexpr uint8_t kMcpIodirA = 0x00;
constexpr uint8_t kMcpIodirB = 0x01;
constexpr uint8_t kMcpGppuA = 0x0C;
constexpr uint8_t kMcpGpioA = 0x12;
constexpr uint8_t kMcpGpioB = 0x13;
constexpr uint8_t kMcpOlatA = 0x14;

struct McpAddresses {
  // 22 sorties X : 2 composants (32 lignes disponibles, 22 utilisées).
  uint8_t x_low = 0x20;   // bits 0..15
  uint8_t x_high = 0x21;  // bits 16..21
  // 21 entrées Y : 2 composants.
  uint8_t y_low = 0x22;   // bits 0..15
  uint8_t y_high = 0x23;  // bits 16..20
};

class Mcp23017Bus final : public IBusDriver {
 public:
  Mcp23017Bus(const HardwareProfile& profile, II2cBus& i2c, IMicroClock& clock,
              const McpAddresses& addr = McpAddresses{})
      : profile_(profile), i2c_(i2c), clock_(clock), addr_(addr),
        debouncer_(profile.bus.y_debounce_us) {}

  bool begin() override;
  bool writeX(uint32_t word) override;
  uint32_t lastX() const override { return last_x_; }
  uint32_t readY() override;
  bool pulse(uint8_t x_bit, uint32_t duration_us) override;
  const char* name() const override { return "mcp23017"; }
  const BusStats& stats() const override { return stats_; }
  uint64_t now_us() const override { return clock_.now_us(); }

  uint32_t ab_skew_us() const { return profile_.bus.mcp_ab_skew_us; }

 private:
  const HardwareProfile& profile_;
  II2cBus& i2c_;
  IMicroClock& clock_;
  McpAddresses addr_;
  YDebouncer debouncer_;
  BusStats stats_{};
  uint32_t last_x_ = 0;
};

}  // namespace agv
