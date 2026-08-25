// Interface d'accès au bus MEIDEN (brief §4.4).
//
// Le séquenceur ne connaît QUE cette interface. Les trois variantes matérielles
// en discussion (MCP23017, 74HC595+165, ATmega2560 conservé) et le simulateur
// en sont des implémentations interchangeables : le choix matériel du §12.10
// n'est pas présupposé.
#pragma once

#include <cstdint>

#include "bus/bus_signals.h"

namespace agv {

struct BusStats {
  uint32_t x_writes = 0;
  uint32_t y_reads = 0;
  uint32_t write_errors = 0;   // NACK I²C, SPI muet, désynchro UART…
  uint32_t max_write_us = 0;   // pire durée de pose observée
  uint32_t last_write_us = 0;
};

class IBusDriver {
 public:
  virtual ~IBusDriver() = default;

  // Prépare le matériel. À l'appel, le bus X est supposé déjà à zéro par le
  // matériel (brief §3.1 : bus physiquement à zéro avant reprise firmware).
  virtual bool begin() = 0;

  // Pose les 22 sorties en une transaction. L'implémentation doit rendre la
  // pose aussi atomique que le matériel le permet et documenter le décalage
  // résiduel (cf. skew GPIOA/GPIOB des MCP23017).
  virtual bool writeX(uint32_t word) = 0;

  // Dernier mot X posé, tel que le driver le croit présent sur les lignes.
  virtual uint32_t lastX() const = 0;

  // Lit les 21 entrées, déjà debouncées selon `y_debounce_us` du profil.
  virtual uint32_t readY() = 0;

  // Impulsion sur un signal unique, largeur en microsecondes. Utilisé pour le
  // strobe X93 et les fronts X82/X83.
  virtual bool pulse(uint8_t x_bit, uint32_t duration_us) = 0;

  // Force les 22 sorties à l'état de repos (état sûr).
  virtual bool clearX() { return writeX(0); }

  virtual const char* name() const = 0;
  virtual const BusStats& stats() const = 0;

  // Horloge monotone en microsecondes ; injectée par l'implémentation pour que
  // le séquenceur reste testable en temps simulé.
  virtual uint64_t now_us() const = 0;
};

}  // namespace agv
