// Pose du bus MEIDEN par les ports de l'ATmega2560 (carte V5.0.1 conservée).
//
// C'est la propriété que la carte d'origine avait gratuitement et qu'il ne faut
// surtout pas perdre : `PORTx = valeur` pose 8 lignes en UN cycle, et trois
// écritures consécutives en section critique posent les 22 sorties de façon
// strictement simultanée. Aucune interface I²C ou SPI ne sait faire ça.
//
// ⚠ CORRESPONDANCE PORT <-> SIGNAL <-> BROCHE SUB-D : PROVISOIRE.
// Le câblage entre les ports de l'ATmega et les SUB-D 25 est imposé par le PCB
// de la V5.0.1 et n'est PAS documenté (planification 0.4 et 0.5 : rétro-ingénierie
// et tentative de lecture des flash existantes). La table ci-dessous est une
// hypothèse de travail ; elle DOIT être vérifiée par relevé de continuité avant
// tout branchement sur l'automate.
//
// Le firmware MEGA embarque un MODE DÉCOUVERTE (voir firmware/mega/src) qui
// active une ligne à la fois pour permettre ce relevé au multimètre.
#pragma once

#include <cstdint>

#include "bus/bus_signals.h"
#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "hal/bus_ports.h"

namespace agv {

// Un port 8 bits de l'ATmega : registre de données, de direction, et d'entrée.
// Abstrait pour que la logique soit testable en natif, sans AVR.
struct PortRegisters {
  volatile uint8_t* out;   // PORTx
  volatile uint8_t* dir;   // DDRx
  volatile uint8_t* in;    // PINx
};

// Répartition des 22 sorties X et des 21 entrées Y sur les ports.
//
//   X : bits 0..7 -> port_x[0], bits 8..15 -> port_x[1], bits 16..21 -> port_x[2]
//   Y : bits 0..7 -> port_y[0], bits 8..15 -> port_y[1], bits 16..20 -> port_y[2]
struct AvrBusPorts {
  PortRegisters port_x[3];
  PortRegisters port_y[3];
};

// Verrou de section critique. Sur AVR : cli()/sei(). En test : compteur.
class ICriticalSection {
 public:
  virtual ~ICriticalSection() = default;
  virtual void enter() = 0;
  virtual void leave() = 0;
};

class AvrPortBus final : public IBusDriver {
 public:
  AvrPortBus(const HardwareProfile& profile, const AvrBusPorts& ports, ICriticalSection& critical,
             IMicroClock& clock)
      : profile_(profile), ports_(ports), critical_(critical), clock_(clock),
        debouncer_(profile.bus.y_debounce_us) {}

  bool begin() override;
  bool writeX(uint32_t word) override;
  uint32_t lastX() const override { return last_x_; }
  uint32_t readY() override;
  bool pulse(uint8_t x_bit, uint32_t duration_us) override;
  const char* name() const override { return "avr_port"; }
  const BusStats& stats() const override { return stats_; }
  uint64_t now_us() const override { return clock_.now_us(); }

  // Mode découverte : active exactement une sortie X, toutes les autres au
  // repos. Permet de relever la correspondance signal <-> broche SUB-D au
  // multimètre, sans automate branché.
  bool drive_single(uint8_t x_bit);

 private:
  const HardwareProfile& profile_;
  AvrBusPorts ports_;
  ICriticalSection& critical_;
  IMicroClock& clock_;
  YDebouncer debouncer_;
  BusStats stats_{};
  uint32_t last_x_ = 0;
};

}  // namespace agv
