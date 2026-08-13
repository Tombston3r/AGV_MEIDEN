// Brochage ESP32 de la carte AGV.
//
// ⚠ PROVISOIRE §12.2 : le brochage définitif dépend du routage de la nouvelle
// carte et de la divergence non tranchée sur les SUB-D 25. Ces numéros sont
// ceux du prototype ; ils ne conditionnent PAS le mapping signal -> bit, qui
// vit dans profiles/*.yaml.
#pragma once

#include <driver/gpio.h>

namespace agv::board {

// I²C (variante MCP23017)
constexpr gpio_num_t kI2cSda = GPIO_NUM_21;
constexpr gpio_num_t kI2cScl = GPIO_NUM_22;

// SPI partagé : RFM95W et, en variante 74HC595/165, les registres à décalage.
constexpr gpio_num_t kSpiSclk = GPIO_NUM_18;
constexpr gpio_num_t kSpiMosi = GPIO_NUM_23;
constexpr gpio_num_t kSpiMiso = GPIO_NUM_19;
constexpr gpio_num_t kLoraCs = GPIO_NUM_5;
constexpr gpio_num_t kLoraReset = GPIO_NUM_14;
constexpr gpio_num_t kLoraDio0 = GPIO_NUM_26;

// Registres à décalage (variante 74HC595 + 74HC165)
constexpr gpio_num_t kShiftRclk = GPIO_NUM_27;
constexpr gpio_num_t kShiftPl = GPIO_NUM_33;
constexpr gpio_num_t kShiftOe = GPIO_NUM_32;

// Modem cellulaire (variantes A2)
constexpr gpio_num_t kModemTx = GPIO_NUM_17;
constexpr gpio_num_t kModemRx = GPIO_NUM_16;
constexpr gpio_num_t kModemPwrkey = GPIO_NUM_4;
constexpr gpio_num_t kWatchdogDone = GPIO_NUM_13;  // TPL5010

// Liaison inter-MCU (variante ATmega2560 conservé)
constexpr gpio_num_t kMegaTx = GPIO_NUM_17;
constexpr gpio_num_t kMegaRx = GPIO_NUM_16;

// Signalisation et maintenance
constexpr gpio_num_t kLedFault = GPIO_NUM_2;
constexpr gpio_num_t kLedLink = GPIO_NUM_15;
constexpr gpio_num_t kMaintenanceReed = GPIO_NUM_25;  // contact ILS (aimant)

// Mesure de tension batterie — ⚠ traverse la barrière d'isolation, voir schéma.
constexpr gpio_num_t kBatteryAdc = GPIO_NUM_34;

}  // namespace agv::board
