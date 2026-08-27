// Brochage de l'ESP32 sur la carte AIO AGV Control V5.0.1.
//
// ⚠ PROVISOIRE : le câblage entre l'ESP32 et l'ATmega n'est pas documenté
// (planification 0.4/0.5). Ces numéros sont l'hypothèse de travail la plus
// probable pour un module ESP32-WROOM-32E ; ils DOIVENT être confirmés au
// relevé de continuité avant le premier flash sur la carte.
#pragma once

#include <driver/gpio.h>

namespace agv::board {

// Liaison série vers l'ATmega2560 (UART1 de l'ESP32).
constexpr gpio_num_t kLinkTx = GPIO_NUM_17;
constexpr gpio_num_t kLinkRx = GPIO_NUM_16;

// Contact ILS (aimant) d'ouverture de la fenêtre de maintenance (§9.4).
constexpr gpio_num_t kMaintenanceReed = GPIO_NUM_25;

// Signalisation locale, si la carte en dispose.
constexpr gpio_num_t kLedFault = GPIO_NUM_2;
constexpr gpio_num_t kLedLink = GPIO_NUM_15;

}  // namespace agv::board
