// Trame applicative commune à tous les transports (brief §5.1).
//
// Disposition sur le fil, exactement les champs du §5.1 :
//
//   octet 0      : ver (4 bits) | type (4 bits)
//   octets 1-2   : node_id (16 bits, big endian)
//   octet 3      : seq (8 bits)
//   octets 4-5   : station (10 bits) | speed (4 bits) | 2 bits réservés
//   octet 6      : flags (8 bits)
//   [octets 7-10]: ts_s (32 bits) — présent seulement si flags & Timestamped
//   2 derniers   : crc16 CCITT sur tout ce qui précède
//
// L'horodatage est une EXTENSION optionnelle, signalée par un bit de `flags`.
// Elle est indispensable aux transports cellulaires : le §8.1 impose de refuser
// une commande plus vieille que `max_command_age_s`, ce qu'aucun des champs de
// base ne permet. Elle reste facultative en LoRa où la latence est bornée.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

enum class FrameType : uint8_t {
  CmdGoto = 0,
  CmdStop = 1,
  Ack = 2,
  Telemetry = 3,
  Ping = 4,
  Pair = 5,
};

namespace flag {
constexpr uint8_t kPriority = 0x01;     // course prioritaire, insérée en tête
constexpr uint8_t kPurgeQueue = 0x02;   // vide la file avant empilement
constexpr uint8_t kTimestamped = 0x04;  // extension ts_s présente
constexpr uint8_t kNack = 0x08;         // ACK négatif (commande refusée)
constexpr uint8_t kRetry = 0x10;        // retransmission d'une trame déjà émise
// Bits d'état pour les trames Telemetry.
constexpr uint8_t kStatusMoving = 0x20;
constexpr uint8_t kStatusInStation = 0x40;
constexpr uint8_t kStatusFault = 0x80;
}  // namespace flag

constexpr size_t kFrameBaseSize = 9;       // sans horodatage
constexpr size_t kFrameMaxSize = 13;       // avec horodatage
constexpr uint8_t kProtocolVersionMax = 0x0F;

struct Frame {
  uint8_t ver = 1;
  FrameType type = FrameType::Ping;
  uint16_t node_id = 0;
  uint8_t seq = 0;
  uint16_t station = 0;  // 10 bits utiles
  uint8_t speed = 0;     // 4 bits utiles
  uint8_t flags = 0;
  uint32_t ts_s = 0;     // significatif si flags & kTimestamped

  bool timestamped() const { return (flags & flag::kTimestamped) != 0; }
};

enum class FrameError : uint8_t {
  Ok = 0,
  TooShort,
  BadCrc,
  BadVersion,
  BadLength,
  UnknownType,
};

// Sérialise. Retourne le nombre d'octets écrits, 0 si `capacity` insuffisante.
size_t encode_frame(const Frame& f, uint8_t* out, size_t capacity);

// Désérialise et vérifie le CRC. `expected_version` = 0 : toute version admise.
FrameError decode_frame(const uint8_t* data, size_t len, Frame& out,
                        uint8_t expected_version = 0);

const char* frame_error_str(FrameError e);
const char* frame_type_str(FrameType t);

}  // namespace agv
