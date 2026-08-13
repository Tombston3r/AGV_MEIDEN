// Transport SMS — architecture 2 variante A (brief §8.1).
//
// AVERTISSEMENT PORTÉ PAR LE CODE, PAS SEULEMENT PAR LA DOC :
// le SMS n'offre ni latence bornée, ni ordre de remise, ni garantie de remise,
// ni protection contre les doublons. Ce transport déclare donc
// `ordered() == false` et `max_command_age_s() != 0` : la couche applicative
// active automatiquement le rejet des trames désordonnées et périmées.
// Il n'est pas recommandé comme liaison principale (voir Archi_2, §8).
//
// Format du corps de SMS : "AGV:" suivi du paquet scellé en hexadécimal
// majuscule. Le texte lisible du type « GOTO;02;SPD=08;SEQ=41 » a été écarté :
// il ne transporte ni CRC ni chiffrement, et impose un second analyseur.
#pragma once

#include <cstddef>
#include <cstdint>

#include "config/hardware_profile.h"
#include "hal/byte_stream.h"
#include "proto/secure_channel.h"
#include "transport/at_engine.h"
#include "transport/itransport.h"

namespace agv {

class SmsTransport final : public ITransport, public IAtUrcHandler {
 public:
  enum class State : uint8_t {
    PowerOff,
    PowerOn,     // impulsion PWRKEY en cours
    Init,        // séquence AT d'initialisation
    Ready,
    ReadSms,     // AT+CMGR en cours
    DeleteSms,   // AT+CMGD en cours
    SendPrompt,  // AT+CMGS envoyé, attente de « > »
    SendBody,    // corps envoyé, attente de OK
    Recovering,  // modem muet : cycle d'alimentation
  };

  SmsTransport(const HardwareProfile& profile, IByteStream& uart, IModemPower& power)
      : profile_(profile), uart_(uart), power_(power),
        at_(uart, profile.cellular.at_timeout_ms) {
    at_.set_urc_handler(this);
  }

  SecureChannel& channel() { return channel_; }

  bool begin() override;
  bool send(const Frame& f) override;
  bool poll(Frame& out) override;
  void tick() override;
  LinkHealth health() const override { return health_; }
  const char* name() const override { return "sms"; }

  // Le SMSC ne garantit aucun ordre : le verdict OutOfOrder doit être actif.
  bool ordered() const override { return false; }
  uint32_t max_command_age_s() const override { return profile_.safety.max_command_age_s; }

  void on_urc(const char* line) override;

  State state() const { return state_; }
  uint32_t recoveries() const { return recoveries_; }

  // Conversions exposées pour les tests unitaires.
  static size_t to_hex(const uint8_t* data, size_t len, char* out, size_t capacity);
  static size_t from_hex(const char* text, uint8_t* out, size_t capacity);
  static bool extract_payload(const char* line, uint8_t* out, size_t capacity, size_t& len);

 private:
  void advance_init();
  void enter(State s);

  const HardwareProfile& profile_;
  IByteStream& uart_;
  IModemPower& power_;
  AtEngine at_;
  SecureChannel channel_;
  LinkHealth health_{};

  State state_ = State::PowerOff;
  uint8_t init_step_ = 0;
  uint32_t state_since_ms_ = 0;
  uint32_t recoveries_ = 0;

  // Index des SMS annoncés par +CMTI, en attente de lecture.
  static constexpr size_t kPendingMax = 8;
  uint16_t pending_idx_[kPendingMax] = {};
  size_t pending_count_ = 0;
  uint16_t reading_idx_ = 0;

  static constexpr size_t kRxQueue = 4;
  Frame rx_queue_[kRxQueue] = {};
  size_t rx_count_ = 0;

  uint8_t tx_buf_[kSecurePacketMax] = {};
  size_t tx_len_ = 0;
  bool tx_pending_ = false;
};

}  // namespace agv
