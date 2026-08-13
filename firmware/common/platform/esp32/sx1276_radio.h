// Pilote RFM95W / SX1276 (LoRa 868 MHz) — implémentation de ILoraRadio.
//
// Registres et séquences issus de la fiche technique SX1276/77/78/79 (rév. 7).
// Le module est HALF-DUPLEX : ce pilote ne fait que ce qu'on lui demande, tout
// l'ordonnancement écoute/émission appartient à LoraTransport.
#pragma once

#ifndef ESP_PLATFORM
#error "platform/esp32 ne se compile que pour la cible ESP32."
#endif

#include "platform/esp32/esp_ports.h"
#include "transport/lora_radio.h"

namespace agv::esp32 {

class Sx1276Radio final : public ILoraRadio {
 public:
  Sx1276Radio(EspSpi& spi, EspGpio& gpio, EspClock& clock, uint8_t reset_pin, uint8_t dio0_pin)
      : spi_(spi), gpio_(gpio), clock_(clock), reset_pin_(reset_pin), dio0_pin_(dio0_pin) {}

  bool begin(const LoraConfig& cfg) override;
  bool transmit(const uint8_t* data, size_t len) override;
  bool tx_busy() const override;
  void listen() override;
  bool receive(uint8_t* buf, size_t capacity, size_t& len, int16_t& rssi_dbm,
               int8_t& snr_db) override;
  uint32_t now_ms() const override { return clock_.now_ms(); }

  uint8_t version() const { return version_; }

 private:
  uint8_t read_reg(uint8_t reg) const;
  void write_reg(uint8_t reg, uint8_t value);
  void set_mode(uint8_t mode);

  EspSpi& spi_;
  EspGpio& gpio_;
  EspClock& clock_;
  uint8_t reset_pin_;
  uint8_t dio0_pin_;
  uint8_t version_ = 0;
  mutable bool transmitting_ = false;
};

}  // namespace agv::esp32
