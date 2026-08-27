// Pont ATmega2560 — variante C du §4.4 (« ATmega2560 conservé »).
//
// Le MEGA garde ce qu'il fait mieux que tout le reste : poser 22 lignes en un
// cycle (`PORTx = valeur`, < 1 µs, strictement simultané). Il ne fait RIEN
// d'autre : pas de séquenceur, pas de timeout, pas de polarité. Toute la
// logique — donc tous les paramètres du §12 — reste dans l'ESP32, sinon deux
// firmwares porteraient la même vérité et divergeraient.
//
// Protocole (voir docs/protocole_mega.md) :
//   ESP32 -> MEGA : A5 | cmd | len | payload | crc16
//   MEGA  -> ESP32: 5A | cmd | len | payload | crc16
#include <Arduino.h>

namespace {

constexpr uint8_t kSofRequest = 0xA5;
constexpr uint8_t kSofReply = 0x5A;
constexpr uint8_t kCmdSetX = 0x01;
constexpr uint8_t kCmdGetY = 0x02;
constexpr uint8_t kCmdPulse = 0x03;
constexpr uint8_t kCmdPing = 0x04;
constexpr uint8_t kFirmwareVersion = 1;

// ⚠ PROVISOIRE §12.2 : affectation des ports à confirmer sur le routage réel.
// Bus X : PORTA (bits 0..7), PORTC (8..15), PORTL (16..21).
// Bus Y : PINK (0..7), PINF (8..15), PINB (16..20).

uint16_t crc16_ccitt(const uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t b = 0; b < 8; ++b) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

void write_x(uint32_t word) {
  // Pose simultanée : trois écritures de port consécutives, sans instruction
  // intercalée. C'est la propriété que les MCP23017 ne peuvent pas offrir.
  const uint8_t a = static_cast<uint8_t>(word & 0xFF);
  const uint8_t c = static_cast<uint8_t>((word >> 8) & 0xFF);
  const uint8_t l = static_cast<uint8_t>((word >> 16) & 0x3F);
  noInterrupts();
  PORTA = a;
  PORTC = c;
  PORTL = l;
  interrupts();
}

uint32_t read_y() {
  noInterrupts();
  const uint8_t k = PINK;
  const uint8_t f = PINF;
  const uint8_t b = PINB;
  interrupts();
  return static_cast<uint32_t>(k) | (static_cast<uint32_t>(f) << 8) |
         (static_cast<uint32_t>(b & 0x1F) << 16);
}

void reply(uint8_t cmd, const uint8_t* payload, uint8_t len) {
  uint8_t frame[16];
  uint8_t i = 0;
  frame[i++] = kSofReply;
  frame[i++] = cmd;
  frame[i++] = len;
  for (uint8_t k = 0; k < len; ++k) frame[i++] = payload[k];
  const uint16_t crc = crc16_ccitt(frame + 1, static_cast<uint8_t>(i - 1));
  frame[i++] = static_cast<uint8_t>(crc >> 8);
  frame[i++] = static_cast<uint8_t>(crc & 0xFF);
  Serial1.write(frame, i);
}

uint8_t buffer[16];
uint8_t buffer_len = 0;

void handle(const uint8_t* frame, uint8_t total) {
  const uint8_t cmd = frame[1];
  const uint16_t crc_calc = crc16_ccitt(frame + 1, static_cast<uint8_t>(total - 3));
  const uint16_t crc_recv =
      static_cast<uint16_t>((frame[total - 2] << 8) | frame[total - 1]);
  if (crc_calc != crc_recv) return;  // trame corrompue : silence, l'ESP32 gère
                                     // le timeout et compte la désynchro

  switch (cmd) {
    case kCmdSetX: {
      const uint32_t word = (static_cast<uint32_t>(frame[3]) << 16) |
                            (static_cast<uint32_t>(frame[4]) << 8) | frame[5];
      write_x(word);
      reply(cmd, nullptr, 0);
      break;
    }
    case kCmdGetY: {
      const uint32_t y = read_y();
      const uint8_t payload[3] = {static_cast<uint8_t>((y >> 16) & 0xFF),
                                  static_cast<uint8_t>((y >> 8) & 0xFF),
                                  static_cast<uint8_t>(y & 0xFF)};
      reply(cmd, payload, 3);
      break;
    }
    case kCmdPulse: {
      // Impulsion courte pilotée par l'ESP32 : le MEGA n'en décide pas la durée.
      const uint8_t bit = frame[3];
      const uint16_t us = static_cast<uint16_t>((frame[4] << 8) | frame[5]);
      uint32_t word = (static_cast<uint32_t>(PORTL & 0x3F) << 16) |
                      (static_cast<uint32_t>(PORTC) << 8) | PORTA;
      write_x(word | (1UL << bit));
      delayMicroseconds(us);
      write_x(word & ~(1UL << bit));
      reply(cmd, nullptr, 0);
      break;
    }
    case kCmdPing: {
      const uint8_t version = kFirmwareVersion;
      reply(cmd, &version, 1);
      break;
    }
    default:
      break;
  }
}

}  // namespace

void setup() {
  // Bus X en sortie, forcé à zéro AVANT toute autre chose (brief §3.1).
  DDRA = 0xFF;
  DDRC = 0xFF;
  DDRL |= 0x3F;
  PORTA = 0x00;
  PORTC = 0x00;
  PORTL &= ~0x3F;

  // Bus Y en entrée.
  DDRK = 0x00;
  DDRF = 0x00;
  DDRB &= ~0x1F;

  Serial1.begin(500000);
}

void loop() {
  while (Serial1.available() > 0) {
    const uint8_t byte = static_cast<uint8_t>(Serial1.read());
    if (buffer_len == 0 && byte != kSofRequest) continue;  // resynchronisation
    if (buffer_len < sizeof(buffer)) buffer[buffer_len++] = byte;
    if (buffer_len >= 3) {
      const uint8_t total = static_cast<uint8_t>(buffer[2] + 5);
      if (total > sizeof(buffer)) {
        buffer_len = 0;
        continue;
      }
      if (buffer_len == total) {
        handle(buffer, total);
        buffer_len = 0;
      }
    }
  }
}
