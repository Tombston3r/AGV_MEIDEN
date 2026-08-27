// Brochage et paramètres des essais radio.
//
// Deux cartes sont gérées, choisies par un drapeau de compilation :
//
//   CARTE_V6      carte AIO AGV Control V6.0 du projet   (défaut)
//   CARTE_TBEAM   LILYGO T-Beam 868 MHz, révision v1.1
//
// Le reste du code ne connaît que les constantes de ce fichier : ajouter une
// carte ne demande pas de toucher aux essais eux-mêmes.
#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>

#if !defined(CARTE_V6) && !defined(CARTE_TBEAM)
#define CARTE_V6
#endif

namespace test {

constexpr uint32_t kSpiFreqHz = 5000000;
constexpr uint16_t kNodeIdTx = 0x0001;
constexpr uint16_t kNodeIdRx = 0x0002;

#if defined(CARTE_V6)

// --- Carte AIO AGV Control V6.0 -------------------------------------------
//
// Relevé sur le projet KiCad : le RFM95W-868S2 (U2) est câblé sur le SPI que
// l'ESP32-DEVKITC laissait libre.
constexpr const char* kNomCarte = "AIO AGV Control V6.0";
constexpr spi_host_device_t kSpiHost = VSPI_HOST;
constexpr gpio_num_t kPinSck = GPIO_NUM_18;    // U2.SCK  -> U1 pad 30
constexpr gpio_num_t kPinMiso = GPIO_NUM_19;   // U2.MISO -> U1 pad 31
constexpr gpio_num_t kPinMosi = GPIO_NUM_23;   // U2.MOSI -> U1 pad 37
constexpr gpio_num_t kPinNss = GPIO_NUM_5;     // U2.NSS  -> U1 pad 29
constexpr uint8_t kPinDio0 = 26;               // U2.DIO0 -> U1 pad 10
// ⚠ U2.RESET n'est PAS câblée sur la V6.0 : aucun reset matériel n'est
// possible, un module figé ne se récupère qu'en coupant l'alimentation carte.
constexpr uint8_t kPinReset = 0xFF;
constexpr bool kAlimentationGeree = false;     // la radio est alimentée en dur

// ⚠ NE PAS utiliser IO16 ni IO17 : elles portent la liaison vers le MEGA.
static_assert(kPinNss != GPIO_NUM_16 && kPinNss != GPIO_NUM_17, "IO16/IO17 reservees");

#elif defined(CARTE_TBEAM)

// --- LILYGO T-Beam 868 MHz, révision v1.1 ---------------------------------
//
// ⚠ LE BROCHAGE CHANGE D'UNE RÉVISION À L'AUTRE. Celui-ci est celui de la
// v1.1, la plus répandue. Sur une v0.7 ou une v1.2, vérifier avant de flasher :
// le numéro de révision est sérigraphié près du connecteur USB.
//
// ⚠ La radio est alimentée PAR LE GESTIONNAIRE AXP192, pas directement. Sans
// l'avoir activée, le SX1276 ne répond pas et `RegVersion` lit 0x00. C'est la
// cause n°1 des « la radio est morte » sur cette carte — voir tbeam_power.h.
constexpr const char* kNomCarte = "LILYGO T-Beam v1.1";
constexpr spi_host_device_t kSpiHost = VSPI_HOST;
constexpr gpio_num_t kPinSck = GPIO_NUM_5;
constexpr gpio_num_t kPinMiso = GPIO_NUM_19;
constexpr gpio_num_t kPinMosi = GPIO_NUM_27;
constexpr gpio_num_t kPinNss = GPIO_NUM_18;
constexpr uint8_t kPinDio0 = 26;
// La T-Beam câble RESET, contrairement à la V6.0 : un module figé se récupère
// sans couper l'alimentation. C'est un avantage réel au banc.
constexpr uint8_t kPinReset = 23;
constexpr bool kAlimentationGeree = true;      // AXP192 à initialiser d'abord

// I²C partagé par l'AXP192 (0x34) et l'écran OLED (0x3C).
constexpr gpio_num_t kPinSda = GPIO_NUM_21;
constexpr gpio_num_t kPinScl = GPIO_NUM_22;
constexpr gpio_num_t kPinBouton = GPIO_NUM_38;  // bouton utilisateur

#endif

}  // namespace test
