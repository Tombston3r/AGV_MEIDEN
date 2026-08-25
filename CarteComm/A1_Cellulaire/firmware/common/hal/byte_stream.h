// Port flux d'octets (UART) — permet de tester les piles AT et ESP3 en natif.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

class IByteStream {
 public:
  virtual ~IByteStream() = default;
  virtual size_t write(const uint8_t* data, size_t len) = 0;
  virtual size_t read(uint8_t* out, size_t capacity) = 0;
  virtual size_t available() const = 0;
  virtual void flush() {}
  virtual uint32_t now_ms() const = 0;
};

// Commande matérielle du modem : PWRKEY, alimentation, chien de garde externe.
class IModemPower {
 public:
  virtual ~IModemPower() = default;
  // Maintient PWRKEY actif pendant `duration_ms` (1 000 ms allumage,
  // 2 500 ms extinction propre — brief §8.1).
  virtual void pulse_pwrkey(uint32_t duration_ms) = 0;
  // Chien de garde matériel TPL5010 : la pile AT peut se bloquer sans que le
  // watchdog logiciel ne le voie (brief §8.1).
  virtual void kick_hardware_watchdog() {}
};

}  // namespace agv
