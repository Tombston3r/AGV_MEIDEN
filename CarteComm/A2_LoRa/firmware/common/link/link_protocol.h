// Protocole série ESP32 <-> ATmega2560 sur la carte V5.0.1 (planification §2.4).
//
// RÉPARTITION DES RÔLES — c'est la décision structurante de cette architecture :
//
//   ATmega2560 : séquenceur X/Y, file de 5 courses, décodage de position.
//                Il pose les 22 lignes en un cycle (`PORTx = valeur`), propriété
//                que la carte d'origine avait gratuitement.
//   ESP32      : client Wi-Fi + MQTT. Il ne touche JAMAIS au bus MEIDEN.
//
// Conséquence voulue : si le Wi-Fi, le réseau d'entreprise ou le poste fixe
// tombent, l'AGV reste piloté par un microcontrôleur qui n'en dépend pas. Le
// heartbeat (ci-dessous) est ce qui rend ce repli observable.
//
// Trame, dans les deux sens :
//
//   SOF | cmd | len | payload[len] | crc16_hi | crc16_lo
//
//   SOF = 0xA5 pour ESP32 -> MEGA, 0x5A pour MEGA -> ESP32. Deux valeurs
//   distinctes : une trame réfléchie par un câblage douteux ne peut pas être
//   prise pour une commande.
//   CRC-16/CCITT-FALSE sur `cmd | len | payload`.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv::link {

constexpr uint8_t kSofToMega = 0xA5;
constexpr uint8_t kSofToEsp = 0x5A;
constexpr size_t kPayloadMax = 16;
constexpr size_t kFrameMax = kPayloadMax + 5;

enum class Cmd : uint8_t {
  // ESP32 -> MEGA
  Heartbeat = 0x01,   // payload : vide. Rearme le repli de sécurité.
  Goto = 0x02,        // payload : seq(1) station(2) speed(1) flags(1)
  Stop = 0x03,        // payload : flags(1) — bit 0 : purge de la file
  GetState = 0x04,    // payload : vide
  ClearFault = 0x05,  // payload : vide — acquittement opérateur
  Ping = 0x06,        // payload : vide

  // MEGA -> ESP32
  Ack = 0x81,     // payload : seq(1) result(1)
  State = 0x82,   // payload : voir LinkState
  Pong = 0x86,    // payload : version(1)
};

// Résultat d'une commande, renvoyé dans un Ack.
enum class CmdResult : uint8_t {
  Accepted = 0,
  Duplicate = 1,     // même seq déjà traitée : ré-acquittée, PAS ré-exécutée
  QueueFull = 2,
  SafeStopActive = 3,  // heartbeat perdu : le MEGA refuse toute course
  Fault = 4,
  BadPayload = 5,
};

// État remonté par l'ATmega. Tenu volontairement compact : il transite toutes
// les `state_poll_ms` sur une liaison partagée avec les commandes.
struct LinkState {
  uint16_t station = 0;        // position courante décodée (Y23…Y34)
  uint8_t speed = 0;           // vitesse courante (Y11…Y14)
  uint8_t seq_state = 0;       // SeqState du séquenceur
  uint8_t fault = 0;           // FaultCause
  uint8_t flags = 0;           // voir state_flag ci-dessous
  uint8_t queue_len = 0;       // nb_courses_programmed
  uint8_t write_tries = 0;
  uint8_t start_tries = 0;
  uint8_t stop_tries = 0;
  uint8_t last_seq = 0;        // dernière séquence traitée
};

namespace state_flag {
constexpr uint8_t kMoving = 0x01;
constexpr uint8_t kInStation = 0x02;
constexpr uint8_t kPlcFault = 0x04;
constexpr uint8_t kNoDestination = 0x08;
constexpr uint8_t kSafeStop = 0x10;      // repli heartbeat actif
constexpr uint8_t kHeartbeatOk = 0x20;
}  // namespace state_flag

constexpr size_t kStatePayloadSize = 12;

// --- Encodage / décodage ---------------------------------------------------

size_t encode(uint8_t sof, Cmd cmd, const uint8_t* payload, uint8_t len, uint8_t* out,
              size_t capacity);

size_t encode_goto(uint8_t seq, uint16_t station, uint8_t speed, uint8_t flags, uint8_t* out,
                   size_t capacity);
size_t encode_ack(uint8_t seq, CmdResult result, uint8_t* out, size_t capacity);
size_t encode_state(const LinkState& state, uint8_t* out, size_t capacity);
bool decode_state(const uint8_t* payload, uint8_t len, LinkState& out);

// Analyseur incrémental : un octet à la fois, resynchronisation automatique.
class Parser {
 public:
  explicit Parser(uint8_t expected_sof) : sof_(expected_sof) {}

  // Retourne true quand une trame complète et valide est disponible.
  bool feed(uint8_t byte, Cmd& cmd, uint8_t* payload, uint8_t& len);

  uint32_t crc_errors() const { return crc_errors_; }
  uint32_t resyncs() const { return resyncs_; }
  void reset() { index_ = 0; state_ = State::Sync; }

 private:
  enum class State : uint8_t { Sync, Cmd, Len, Payload, CrcHi, CrcLo };

  uint8_t sof_;
  State state_ = State::Sync;
  uint8_t cmd_ = 0;
  uint8_t len_ = 0;
  uint8_t index_ = 0;
  uint8_t buffer_[kPayloadMax] = {};
  uint16_t crc_received_ = 0;
  uint32_t crc_errors_ = 0;
  uint32_t resyncs_ = 0;
};

}  // namespace agv::link
