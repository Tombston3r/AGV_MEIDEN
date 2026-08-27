// Signaux du bus MEIDEN : numérotation OCTALE (convention Meiden/Mitsubishi).
//
// Piège principal (brief §4.2) : Y23…Y34 ne compte pas 12 signaux mais 10, car
// après Y27 vient Y30. Toute énumération de plage doit passer par les
// utilitaires octaux de ce fichier, jamais par une arithmétique décimale.
#pragma once

#include <cstddef>
#include <cstdint>

#include "config/generated_profile.h"

namespace agv {

// --- Utilitaires de numérotation octale -----------------------------------

// Convertit un repère écrit en octal (ex. 0o34 saisi « 34 ») en rang linéaire.
constexpr uint16_t octal_to_linear(uint16_t octal_digits) {
  uint16_t value = 0;
  uint16_t scale = 1;
  uint16_t rest = octal_digits;
  while (rest > 0) {
    value = static_cast<uint16_t>(value + (rest % 10) * scale);
    rest = static_cast<uint16_t>(rest / 10);
    scale = static_cast<uint16_t>(scale * 8);
  }
  return value;
}

// Réciproque : rang linéaire -> repère octal lisible (34 pour 0o34).
constexpr uint16_t linear_to_octal(uint16_t linear) {
  uint16_t value = 0;
  uint16_t scale = 1;
  uint16_t rest = linear;
  while (rest > 0) {
    value = static_cast<uint16_t>(value + (rest % 8) * scale);
    rest = static_cast<uint16_t>(rest / 8);
    scale = static_cast<uint16_t>(scale * 10);
  }
  return value;
}

// n-ième repère d'une plage octale : octal_step(23, 5) == 30, pas 28.
constexpr uint16_t octal_step(uint16_t start_octal, uint16_t steps) {
  return linear_to_octal(static_cast<uint16_t>(octal_to_linear(start_octal) + steps));
}

// Nombre de repères entre deux bornes octales incluses : Y23..Y34 -> 10.
constexpr uint16_t octal_span(uint16_t first_octal, uint16_t last_octal) {
  return static_cast<uint16_t>(octal_to_linear(last_octal) - octal_to_linear(first_octal) + 1);
}

// --- Positions dans les mots bus (issues de profiles/*.yaml, §12.2) -------

namespace x {
enum Bit : uint8_t {
  X82 = CFG_PIN_X82,  // standby release / start
  X83 = CFG_PIN_X83,  // standby stop
  X84 = CFG_PIN_X84,  // aiguillage
  X85 = CFG_PIN_X85,  // sens
  X86 = CFG_PIN_X86,  // vitesse b0
  X87 = CFG_PIN_X87,  // vitesse b1
  X90 = CFG_PIN_X90,  // vitesse b2
  X91 = CFG_PIN_X91,  // vitesse b3
  X92 = CFG_PIN_X92,  // instruction data input switch
  X93 = CFG_PIN_X93,  // write strobe
  X94 = CFG_PIN_X94,  // type de donnée (station / comptage de marqueurs)
  X95 = CFG_PIN_X95,  // frein externe
  X96 = CFG_PIN_X96,  // destination b0
  X97 = CFG_PIN_X97,  // destination b1
  XA0 = CFG_PIN_XA0,  // destination b2
  XA1 = CFG_PIN_XA1,
  XA2 = CFG_PIN_XA2,
  XA3 = CFG_PIN_XA3,
  XA4 = CFG_PIN_XA4,
  XA5 = CFG_PIN_XA5,
  XA6 = CFG_PIN_XA6,
  XA7 = CFG_PIN_XA7,  // destination b9
};
}  // namespace x

namespace y {
enum Bit : uint8_t {
  Y03 = CFG_PIN_Y03,  // défaut
  Y05 = CFG_PIN_Y05,  // moving flag
  Y10 = CFG_PIN_Y10,  // in station flag
  Y11 = CFG_PIN_Y11,  // vitesse courante b0
  Y12 = CFG_PIN_Y12,
  Y13 = CFG_PIN_Y13,
  Y14 = CFG_PIN_Y14,  // vitesse courante b3
  Y15 = CFG_PIN_Y15,  // écho aiguillage
  Y20 = CFG_PIN_Y20,  // écho sens
  Y21 = CFG_PIN_Y21,  // pas de destination programmée
  Y22 = CFG_PIN_Y22,  // instruction reading complete
  Y23 = CFG_PIN_Y23,  // position courante b0
  Y24 = CFG_PIN_Y24,
  Y25 = CFG_PIN_Y25,
  Y26 = CFG_PIN_Y26,
  Y27 = CFG_PIN_Y27,
  Y30 = CFG_PIN_Y30,  // ...suite octale : après Y27 vient Y30
  Y31 = CFG_PIN_Y31,
  Y32 = CFG_PIN_Y32,
  Y33 = CFG_PIN_Y33,
  Y34 = CFG_PIN_Y34,  // position courante b9
};
}  // namespace y

constexpr size_t kStationBits = 10;  // 1024 valeurs
constexpr size_t kSpeedBits = 4;     // 16 niveaux
constexpr uint16_t kStationMax = (1u << kStationBits) - 1u;
constexpr uint8_t kSpeedMax = (1u << kSpeedBits) - 1u;

// Ordre des bits d'adresse/vitesse sur les bus.
// PROVISOIRE §12.6 : la correspondance poids fort / poids faible avec les
// repères sérigraphiés T9…T24 de la V5.0.1 n'est pas relevée. L'ordre vit ici,
// modifiable sans toucher au séquenceur.
struct BusLayout {
  uint8_t x_station_bits[kStationBits];  // LSB en premier
  uint8_t x_speed_bits[kSpeedBits];
  uint8_t y_station_bits[kStationBits];
  uint8_t y_speed_bits[kSpeedBits];
};

constexpr BusLayout kDefaultLayout = {
    {x::X96, x::X97, x::XA0, x::XA1, x::XA2, x::XA3, x::XA4, x::XA5, x::XA6, x::XA7},
    {x::X86, x::X87, x::X90, x::X91},
    {y::Y23, y::Y24, y::Y25, y::Y26, y::Y27, y::Y30, y::Y31, y::Y32, y::Y33, y::Y34},
    {y::Y11, y::Y12, y::Y13, y::Y14},
};

// --- Encodage / décodage ---------------------------------------------------

inline uint32_t encode_field(uint32_t word, const uint8_t* bits, size_t count, uint32_t value) {
  for (size_t i = 0; i < count; ++i) {
    const uint32_t mask = 1u << bits[i];
    word = ((value >> i) & 1u) ? (word | mask) : (word & ~mask);
  }
  return word;
}

inline uint32_t decode_field(uint32_t word, const uint8_t* bits, size_t count) {
  uint32_t value = 0;
  for (size_t i = 0; i < count; ++i) {
    if ((word >> bits[i]) & 1u) {
      value |= (1u << i);
    }
  }
  return value;
}

inline uint32_t encode_station(uint32_t word, const BusLayout& l, uint16_t station) {
  return encode_field(word, l.x_station_bits, kStationBits, station & kStationMax);
}

inline uint32_t encode_speed(uint32_t word, const BusLayout& l, uint8_t speed) {
  return encode_field(word, l.x_speed_bits, kSpeedBits, speed & kSpeedMax);
}

inline uint16_t decode_position(uint32_t y_word, const BusLayout& l) {
  return static_cast<uint16_t>(decode_field(y_word, l.y_station_bits, kStationBits));
}

inline uint8_t decode_current_speed(uint32_t y_word, const BusLayout& l) {
  return static_cast<uint8_t>(decode_field(y_word, l.y_speed_bits, kSpeedBits));
}

inline bool bit_set(uint32_t word, uint8_t bit) { return ((word >> bit) & 1u) != 0u; }

inline uint32_t with_bit(uint32_t word, uint8_t bit, bool value) {
  const uint32_t mask = 1u << bit;
  return value ? (word | mask) : (word & ~mask);
}

}  // namespace agv
