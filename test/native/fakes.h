// Doublures de test : radio LoRa, UART modem, alimentation modem.
#pragma once

#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "app/clock.h"
#include "hal/byte_stream.h"
#include "transport/lora_radio.h"

namespace agv::test {

// Radio LoRa factice : temps d'émission simulé, canal partagé entre deux
// instances pour rejouer un échange poste <-> AGV.
class FakeRadio final : public agv::ILoraRadio {
 public:
  explicit FakeRadio(agv::FakeClock& clock) : clock_(clock) {}

  bool begin(const agv::LoraConfig& cfg) override {
    cfg_ = cfg;
    started = true;
    return true;
  }

  bool transmit(const uint8_t* data, size_t len) override {
    if (tx_busy_) return false;
    last_tx.assign(data, data + len);
    ++tx_count;
    tx_busy_ = true;
    tx_end_ms_ = clock_.now_ms() + tx_duration_ms;
    listening = false;
    if (peer != nullptr && !drop_next) peer->deliver(last_tx);
    drop_next = false;
    return true;
  }

  bool tx_busy() const override {
    if (tx_busy_ && clock_.now_ms() >= tx_end_ms_) tx_busy_ = false;
    return tx_busy_;
  }

  void listen() override { listening = true; }

  bool receive(uint8_t* buf, size_t capacity, size_t& len, int16_t& rssi, int8_t& snr) override {
    if (inbox.empty()) return false;
    const auto& front = inbox.front();
    if (front.size() > capacity) {
      inbox.pop_front();
      return false;
    }
    std::memcpy(buf, front.data(), front.size());
    len = front.size();
    rssi = rssi_dbm;
    snr = snr_db;
    inbox.pop_front();
    return true;
  }

  uint32_t now_ms() const override { return clock_.now_ms(); }

  void deliver(const std::vector<uint8_t>& packet) { inbox.push_back(packet); }

  FakeRadio* peer = nullptr;
  bool started = false;
  bool listening = false;
  bool drop_next = false;
  uint32_t tx_count = 0;
  uint32_t tx_duration_ms = 60;
  int16_t rssi_dbm = -95;
  int8_t snr_db = 7;
  std::vector<uint8_t> last_tx;
  std::deque<std::vector<uint8_t>> inbox;

 private:
  agv::FakeClock& clock_;
  agv::LoraConfig cfg_{};
  mutable bool tx_busy_ = false;
  uint32_t tx_end_ms_ = 0;
};

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
