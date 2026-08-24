// Brochage et paramètres communs aux deux essais ESP32.
//
// Les broches sont celles relevées comme LIBRES sur la carte AIO AGV Control
// V5.0.1 : le projet KiCad montre que l'ESP32-DEVKITC n'utilise que 4 de ses
// 38 broches — deux d'alimentation, et deux vers le Mega2560 Pro (IO16/IO17).
// Tout le VSPI est donc disponible pour greffer le RFM95W.
#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>

namespace test {

constexpr spi_host_device_t kSpiHost = VSPI_HOST;
constexpr gpio_num_t kPinSck = GPIO_NUM_18;
constexpr gpio_num_t kPinMiso = GPIO_NUM_19;
constexpr gpio_num_t kPinMosi = GPIO_NUM_23;
constexpr gpio_num_t kPinNss = GPIO_NUM_5;
constexpr uint8_t kPinReset = 14;
constexpr uint8_t kPinDio0 = 26;

// ⚠ NE PAS utiliser IO16 ni IO17 : elles portent la liaison vers le MEGA.
static_assert(kPinNss != GPIO_NUM_16 && kPinNss != GPIO_NUM_17, "IO16/IO17 reservees");

constexpr uint32_t kSpiFreqHz = 5000000;
constexpr uint16_t kNodeIdTx = 0x0001;
constexpr uint16_t kNodeIdRx = 0x0002;

}  // namespace test
