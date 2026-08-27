// Décodeur EnOcean Serial Protocol 3 (ESP3) — architecture 3 (brief §7).
//
// Chaîne : bouton PTM 210 (sans pile) -> récepteur TCM 515 (UART) -> ESP32.
//
// Trame ESP3 :
//   0x55 | DataLen(2, BE) | OptLen(1) | PacketType(1) | CRC8H |
//   Data[DataLen] | OptData[OptLen] | CRC8D
//
// ⚠ Le TCM 515 est RÉCEPTION SEULE : aucun retour visuel n'est possible vers le
// bouton EnOcean. Si un accusé côté opérateur est exigé, il faut un TCM 310
// (bidirectionnel) ou un voyant déporté câblé — point ouvert §12.8.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

constexpr uint8_t kEsp3Sync = 0x55;
constexpr uint8_t kEsp3TypeRadioErp1 = 0x01;
constexpr uint8_t kRorgRps = 0xF6;  // télégramme des interrupteurs PTM
constexpr size_t kEsp3DataMax = 64;
constexpr size_t kEsp3OptMax = 16;

struct Esp3Packet {
  uint8_t packet_type = 0;
  uint8_t data[kEsp3DataMax] = {};
  uint16_t data_len = 0;
  uint8_t opt[kEsp3OptMax] = {};
  uint8_t opt_len = 0;
  int8_t rssi_dbm = 0;  // dBm (issu du champ dBm des OptData, valeur négative)
};

// Télégramme RPS décodé (appui sur un PTM 210).
struct RpsTelegram {
  uint32_t sender_id = 0;  // identifiant 32 bits gravé en usine
  uint8_t data = 0;        // octet d'état du bascule
  uint8_t status = 0;
  int8_t rssi_dbm = 0;
  bool pressed = false;    // true = appui, false = relâchement
  uint8_t rocker = 0;      // 0..3 : position du bascule (A0, AI, B0, BI)
};

class Esp3Decoder {
 public:
  // Injecte un octet reçu. Retourne true quand un paquet complet et valide
  // (CRC8 header ET CRC8 data) est disponible dans `out`.
  bool feed(uint8_t byte, Esp3Packet& out);

  uint32_t crc_header_errors() const { return crc_header_errors_; }
  uint32_t crc_data_errors() const { return crc_data_errors_; }
  uint32_t packets_ok() const { return packets_ok_; }
  void reset() { state_ = State::Sync; }

  // Extrait un télégramme RPS d'un paquet RADIO_ERP1. False si ce n'en est pas.
  static bool parse_rps(const Esp3Packet& p, RpsTelegram& out);

 private:
  enum class State : uint8_t { Sync, Header, CrcHeader, Payload, CrcData };

  State state_ = State::Sync;
  uint8_t header_[4] = {};
  size_t index_ = 0;
  Esp3Packet current_{};
  size_t payload_expected_ = 0;
  uint32_t crc_header_errors_ = 0;
  uint32_t crc_data_errors_ = 0;
  uint32_t packets_ok_ = 0;
};

// Déduplication des sous-télégrammes : le PTM 210 émet 3 copies identiques par
// appui (§7). Sans ce filtre, un appui déclenche trois courses.
class EnoceanDeduplicator {
 public:
  explicit EnoceanDeduplicator(uint32_t window_ms) : window_ms_(window_ms) {}

  // True si ce télégramme doit être TRAITÉ, false si c'est une copie.
  bool accept(uint32_t sender_id, uint8_t data, uint32_t now_ms);
  void set_window_ms(uint32_t v) { window_ms_ = v; }
  uint32_t duplicates() const { return duplicates_; }

 private:
  static constexpr size_t kSlots = 8;
  struct Slot {
    uint32_t sender_id;
    uint8_t data;
    uint32_t at_ms;
    bool used;
  };
  Slot slots_[kSlots] = {};
  size_t next_ = 0;
  uint32_t window_ms_;
  uint32_t duplicates_ = 0;
};

}  // namespace agv
