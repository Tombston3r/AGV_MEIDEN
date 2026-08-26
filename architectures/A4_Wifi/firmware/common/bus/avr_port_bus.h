// Pose du bus MEIDEN par les ports de l'ATmega2560.
//
// LE CÂBLAGE RÉEL EST RELEVÉ (voir docs/subd25_atmega.md) et il impose la forme
// de ce driver. Trois faits qu'on ne peut pas contourner :
//
//  1. Les 43 signaux sont répartis sur ONZE ports, par bits épars. Il n'existe
//     aucun `PORTx = valeur` qui poserait un champ complet : la table est
//     bit à bit, et les écritures sont regroupées par port.
//
//  2. TROIS PORTS SONT MIXTES — PORTA, PORTB et PORTG portent à la fois des
//     sorties X et des entrées Y. Écrire un registre de direction complet
//     (`DDRA = 0xFF`) mettrait en sortie des broches sur lesquelles l'automate
//     pilote : conflit électrique franc. Toute manipulation de DDR et de PORT
//     se fait donc EN MASQUE, jamais en octet plein.
//
//  3. Les 10 bits d'adresse sont répartis sur 4 ports : leur pose demande 4
//     écritures consécutives, soit ~0,25 µs à 16 MHz. Ce n'est pas la
//     simultanéité stricte d'un `PORTA = x`, mais c'est trois ordres de
//     grandeur sous le `t_setup` attendu — et très loin devant des MCP23017.
//     Le décalage résiduel est exposé par `port_writes_per_pose()`.
#pragma once

#include <cstdint>

#include "bus/bus_signals.h"
#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "hal/bus_ports.h"

namespace agv {

// Registres d'un port 8 bits de l'ATmega. Abstraits pour rester testable en
// natif : en test ce sont de simples octets, sur cible ce sont PORTx/DDRx/PINx.
struct PortRegisters {
  volatile uint8_t* out = nullptr;  // PORTx
  volatile uint8_t* dir = nullptr;  // DDRx
  volatile uint8_t* in = nullptr;   // PINx
};

// Emplacement physique d'un signal : quel port, quel bit.
struct BitLocation {
  uint8_t port = 0xFF;  // index dans AvrBusMap::ports ; 0xFF = non câblé
  uint8_t bit = 0;      // 0..7
};

constexpr uint8_t kMaxPorts = 12;
constexpr uint8_t kUnwired = 0xFF;

// Table complète du câblage. Le brochage vit à un seul endroit
// (firmware/mega/src/board_ports.h) et se corrige sans toucher au code.
struct AvrBusMap {
  PortRegisters ports[kMaxPorts];
  uint8_t port_count = 0;
  BitLocation x[22];  // index = position dans le mot logique (profiles/pinmap)
  BitLocation y[21];
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
  AvrPortBus(const HardwareProfile& profile, const AvrBusMap& map, ICriticalSection& critical,
             IMicroClock& clock)
      : profile_(profile), map_(map), critical_(critical), clock_(clock),
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
  // repos. Sert au contrôle du brochage au multimètre, automate débranché.
  bool drive_single(uint8_t x_bit);

  // Nombre d'écritures de port nécessaires à une pose complète : c'est la
  // mesure du décalage résiduel entre les premières et les dernières lignes.
  uint8_t port_writes_per_pose() const { return port_writes_; }

 private:
  void compute_masks();

  const HardwareProfile& profile_;
  AvrBusMap map_;
  ICriticalSection& critical_;
  IMicroClock& clock_;
  YDebouncer debouncer_;
  BusStats stats_{};

  uint8_t x_mask_[kMaxPorts] = {};  // bits de sortie X portés par chaque port
  uint8_t y_mask_[kMaxPorts] = {};  // bits d'entrée Y portés par chaque port
  uint8_t port_writes_ = 0;
  uint32_t last_x_ = 0;
};

}  // namespace agv
