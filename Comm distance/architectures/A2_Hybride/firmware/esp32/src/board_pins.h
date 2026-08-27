// Brochage de l'ESP32 sur la carte AIO AGV Control V5.0.1.
//
// ⚠ PROVISOIRE : le câblage entre l'ESP32 et l'ATmega n'est pas documenté
// (planification 0.4/0.5). Ces numéros sont l'hypothèse de travail la plus
// probable pour un module ESP32-WROOM-32E ; ils DOIVENT être confirmés au
// relevé de continuité avant le premier flash sur la carte.
#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>

namespace agv::board {

// Liaison série vers l'ATmega2560 (UART1 de l'ESP32).
constexpr gpio_num_t kLinkTx = GPIO_NUM_17;
constexpr gpio_num_t kLinkRx = GPIO_NUM_16;

// Contact ILS (aimant) d'ouverture de la fenêtre de maintenance (§9.4).
constexpr gpio_num_t kMaintenanceReed = GPIO_NUM_25;

// Signalisation locale, si la carte en dispose.
constexpr gpio_num_t kLedFault = GPIO_NUM_2;
constexpr gpio_num_t kLedLink = GPIO_NUM_15;

// --- Radio LoRa — RELEVÉ sur le projet KiCad de la V6.0 --------------------
//
// Le RFM95W-868S2 (U2) est câblé sur le SPI que l'ESP32 laissait libre.
constexpr spi_host_device_t kLoraSpiHost = VSPI_HOST;
constexpr gpio_num_t kLoraSck = GPIO_NUM_18;    // U2.SCK  -> U1 pad 30
constexpr gpio_num_t kLoraMiso = GPIO_NUM_19;   // U2.MISO -> U1 pad 31
constexpr gpio_num_t kLoraMosi = GPIO_NUM_23;   // U2.MOSI -> U1 pad 37
constexpr gpio_num_t kLoraNss = GPIO_NUM_5;     // U2.NSS  -> U1 pad 29
constexpr uint8_t kLoraDio0 = 26;               // U2.DIO0 -> U1 pad 10
// ⚠ U2.RESET n'est PAS câblée sur la V6.0 : aucun reset matériel n'est
// possible, un module figé ne se récupère qu'en coupant l'alimentation carte.
constexpr uint8_t kLoraReset = 0xFF;
constexpr uint32_t kLoraSpiHz = 5000000;

}  // namespace agv::board
