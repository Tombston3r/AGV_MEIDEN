// Implémentations ESP-IDF des ports matériels.
//
// Ce répertoire est le SEUL endroit du dépôt autorisé à inclure des en-têtes
// ESP-IDF. Tout le reste de /common compile en natif : c'est ce qui rend le
// simulateur et les tests possibles.
#pragma once

#ifndef ESP_PLATFORM
#error "platform/esp32 ne se compile que pour la cible ESP32 (ESP_PLATFORM)."
#endif

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <driver/uart.h>
#include <esp_timer.h>
#include <rom/ets_sys.h>

#include "app/clock.h"
#include "hal/bus_ports.h"
#include "hal/byte_stream.h"

namespace agv::esp32 {

class EspClock final : public IMicroClock, public IClock {
 public:
  uint64_t now_us() const override { return static_cast<uint64_t>(esp_timer_get_time()); }
  void delay_us(uint32_t us) override { ets_delay_us(us); }
  uint32_t now_ms() const override { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }
  // Horloge murale : 0 tant qu'aucune synchronisation n'a eu lieu (SNTP côté
  // poste fixe, ou horodatage reçu dans une trame). Retourner une valeur fausse
  // désactiverait silencieusement les contrôles de fraîcheur du §8.1.
  uint32_t now_s() const override { return wall_offset_s_ ? wall_offset_s_ + now_ms() / 1000 : 0; }
  void set_wall_clock(uint32_t unix_s) {
    wall_offset_s_ = (unix_s > now_ms() / 1000) ? unix_s - now_ms() / 1000 : 0;
  }

 private:
  uint32_t wall_offset_s_ = 0;
};

class EspI2c final : public II2cBus {
 public:
  bool begin(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz);
  bool write_reg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) override;
  bool read_reg(uint8_t addr, uint8_t reg, uint8_t* out, size_t len) override;

 private:
  i2c_port_t port_ = I2C_NUM_0;
};

class EspSpi final : public ISpiBus {
 public:
  bool begin(spi_host_device_t host, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t miso,
             gpio_num_t cs, uint32_t freq_hz);
  bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) override;
  spi_device_handle_t handle() const { return dev_; }

 private:
  spi_device_handle_t dev_ = nullptr;
};

class EspGpio final : public IGpio {
 public:
  void configure_output(uint8_t pin);
  void configure_input(uint8_t pin, bool pullup);
  void set(uint8_t pin, bool level) override;
  bool get(uint8_t pin) const override;
};

class EspUart final : public IByteStream {
 public:
  bool begin(uart_port_t port, gpio_num_t tx, gpio_num_t rx, int baud);
  size_t write(const uint8_t* data, size_t len) override;
  size_t read(uint8_t* out, size_t capacity) override;
  size_t available() const override;
  uint32_t now_ms() const override { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

 private:
  uart_port_t port_ = UART_NUM_1;
};

// PWRKEY du modem cellulaire + entretien du chien de garde matériel TPL5010.
class EspModemPower final : public IModemPower {
 public:
  EspModemPower(gpio_num_t pwrkey, gpio_num_t watchdog_done)
      : pwrkey_(pwrkey), watchdog_done_(watchdog_done) {}
  void begin();
  void pulse_pwrkey(uint32_t duration_ms) override;
  void kick_hardware_watchdog() override;

 private:
  gpio_num_t pwrkey_;
  gpio_num_t watchdog_done_;
};

}  // namespace agv::esp32
