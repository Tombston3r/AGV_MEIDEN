// Doublures de test : radio LoRa, UART modem, alimentation modem.
#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "app/clock.h"
#include "hal/byte_stream.h"

namespace agv::test {

// UART factice : ce que le firmware écrit est capturé, ce que le modem
// « répond » est injecté par le test.
class FakeUart final : public agv::IByteStream {
 public:
  explicit FakeUart(agv::FakeClock& clock) : clock_(clock) {}

  size_t write(const uint8_t* data, size_t len) override {
    written.append(reinterpret_cast<const char*>(data), len);
    return len;
  }
  size_t read(uint8_t* out, size_t capacity) override {
    const size_t n = (rx.size() < capacity) ? rx.size() : capacity;
    std::memcpy(out, rx.data(), n);
    rx.erase(0, n);
    return n;
  }
  size_t available() const override { return rx.size(); }
  uint32_t now_ms() const override { return clock_.now_ms(); }

  void inject(const std::string& text) { rx += text; }
  bool wrote(const std::string& needle) const {
    return written.find(needle) != std::string::npos;
  }
  void clear() { written.clear(); }

  std::string written;
  std::string rx;

 private:
  agv::FakeClock& clock_;
};

class FakeModemPower final : public agv::IModemPower {
 public:
  void pulse_pwrkey(uint32_t duration_ms) override {
    ++pulses;
    last_duration_ms = duration_ms;
  }
  void kick_hardware_watchdog() override { ++kicks; }

  uint32_t pulses = 0;
  uint32_t kicks = 0;
  uint32_t last_duration_ms = 0;
};

}  // namespace agv::test
