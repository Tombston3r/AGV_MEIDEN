// Ports matériels bas niveau : I²C, SPI, GPIO, horloge microseconde.
//
// Les drivers de bus (MCP23017, 74HC595+165, ATmega2560 conservé) sont écrits
// contre ces ports et sont donc compilables et testables en natif. Les
// implémentations ESP-IDF vivent dans firmware/*/platform/.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

class II2cBus {
 public:
  virtual ~II2cBus() = default;
  // Écrit `len` octets à partir du registre `reg` d'un composant. Retourne
  // false sur NACK : une écriture I²C perdue ne doit jamais passer inaperçue.
  virtual bool write_reg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) = 0;
  virtual bool read_reg(uint8_t addr, uint8_t reg, uint8_t* out, size_t len) = 0;
};

class ISpiBus {
 public:
  virtual ~ISpiBus() = default;
  // Transaction full duplex. `rx` peut être nul si la réponse est ignorée.
  virtual bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) = 0;
};

class IGpio {
 public:
  virtual ~IGpio() = default;
  virtual void set(uint8_t pin, bool level) = 0;
  virtual bool get(uint8_t pin) const = 0;
};

class IMicroClock {
 public:
  virtual ~IMicroClock() = default;
  virtual uint64_t now_us() const = 0;
  // Attente courte, réservée aux fronts de quelques microsecondes.
  virtual void delay_us(uint32_t us) = 0;
};

}  // namespace agv
