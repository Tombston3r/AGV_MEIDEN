// Ports matériels bas niveau de cette architecture.
//
// La carte V5.0.1 ne comporte ni expandeur I²C ni registre à décalage : les 43
// lignes sont câblées directement sur les ports de l'ATmega2560. Il ne reste
// donc qu'une horloge microseconde à abstraire, pour que le séquenceur et le
// driver de bus soient testables en natif.
#pragma once

#include <cstdint>

namespace agv {

class IMicroClock {
 public:
  virtual ~IMicroClock() = default;
  virtual uint64_t now_us() const = 0;
  // Attente courte, réservée aux fronts de quelques microsecondes.
  virtual void delay_us(uint32_t us) = 0;
};

}  // namespace agv
