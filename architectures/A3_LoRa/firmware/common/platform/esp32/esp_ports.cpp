#include "platform/esp32/esp_ports.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace agv::esp32 {

bool EspI2c::begin(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz) {
  port_ = port;
  i2c_config_t cfg = {};
  cfg.mode = I2C_MODE_MASTER;
  cfg.sda_io_num = sda;
  cfg.scl_io_num = scl;
  cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.master.clk_speed = freq_hz;
  if (i2c_param_config(port_, &cfg) != ESP_OK) return false;
  return i2c_driver_install(port_, I2C_MODE_MASTER, 0, 0, 0) == ESP_OK;
}

bool EspI2c::write_reg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
  uint8_t buf[34];
  if (len + 1 > sizeof(buf)) return false;
  buf[0] = reg;
  for (size_t i = 0; i < len; ++i) buf[i + 1] = data[i];
  return i2c_master_write_to_device(port_, addr, buf, len + 1, pdMS_TO_TICKS(20)) == ESP_OK;
}

bool EspI2c::read_reg(uint8_t addr, uint8_t reg, uint8_t* out, size_t len) {
  return i2c_master_write_read_device(port_, addr, &reg, 1, out, len, pdMS_TO_TICKS(20)) == ESP_OK;
}

bool EspSpi::begin(spi_host_device_t host, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t miso,
                   gpio_num_t cs, uint32_t freq_hz) {
  spi_bus_config_t bus = {};
  bus.mosi_io_num = mosi;
  bus.miso_io_num = miso;
  bus.sclk_io_num = sclk;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = 64;
  if (spi_bus_initialize(host, &bus, SPI_DMA_DISABLED) != ESP_OK) return false;

  spi_device_interface_config_t dev = {};
  dev.clock_speed_hz = static_cast<int>(freq_hz);
  dev.mode = 0;
  dev.spics_io_num = cs;
  dev.queue_size = 1;
  return spi_bus_add_device(host, &dev, &dev_) == ESP_OK;
}

bool EspSpi::transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
  if (dev_ == nullptr || len == 0) return false;
  spi_transaction_t t = {};
  t.length = len * 8;
  t.tx_buffer = tx;
  t.rx_buffer = rx;
  return spi_device_polling_transmit(dev_, &t) == ESP_OK;
}

void EspGpio::configure_output(uint8_t pin) {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = 1ULL << pin;
  cfg.mode = GPIO_MODE_OUTPUT;
  gpio_config(&cfg);
}

void EspGpio::configure_input(uint8_t pin, bool pullup) {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = 1ULL << pin;
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  gpio_config(&cfg);
}

void EspGpio::set(uint8_t pin, bool level) {
  gpio_set_level(static_cast<gpio_num_t>(pin), level ? 1 : 0);
}

bool EspGpio::get(uint8_t pin) const {
  return gpio_get_level(static_cast<gpio_num_t>(pin)) != 0;
}

bool EspUart::begin(uart_port_t port, gpio_num_t tx, gpio_num_t rx, int baud) {
  port_ = port;
  uart_config_t cfg = {};
  cfg.baud_rate = baud;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity = UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.source_clk = UART_SCLK_DEFAULT;
  if (uart_param_config(port_, &cfg) != ESP_OK) return false;
  if (uart_set_pin(port_, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) return false;
  return uart_driver_install(port_, 2048, 2048, 0, nullptr, 0) == ESP_OK;
}

size_t EspUart::write(const uint8_t* data, size_t len) {
  const int written = uart_write_bytes(port_, reinterpret_cast<const char*>(data), len);
  return (written < 0) ? 0 : static_cast<size_t>(written);
}

size_t EspUart::read(uint8_t* out, size_t capacity) {
  const int n = uart_read_bytes(port_, out, capacity, 0);
  return (n < 0) ? 0 : static_cast<size_t>(n);
}

size_t EspUart::available() const {
  size_t n = 0;
  uart_get_buffered_data_len(port_, &n);
  return n;
}

void EspModemPower::begin() {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << pwrkey_) | (1ULL << watchdog_done_);
  cfg.mode = GPIO_MODE_OUTPUT;
  gpio_config(&cfg);
  gpio_set_level(pwrkey_, 0);
  gpio_set_level(watchdog_done_, 0);
}

void EspModemPower::pulse_pwrkey(uint32_t duration_ms) {
  // 1 000 ms pour allumer, 2 500 ms pour éteindre proprement (brief §8.1).
  gpio_set_level(pwrkey_, 1);
  vTaskDelay(pdMS_TO_TICKS(duration_ms));
  gpio_set_level(pwrkey_, 0);
}

void EspModemPower::kick_hardware_watchdog() {
  // TPL5010 : impulsion sur DONE. Le chien de garde matériel est indispensable,
  // la pile AT peut se bloquer sans que le watchdog logiciel ne le voie.
  gpio_set_level(watchdog_done_, 1);
  ets_delay_us(50);
  gpio_set_level(watchdog_done_, 0);
}

}  // namespace agv::esp32
