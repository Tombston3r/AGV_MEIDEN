// Transport LTE-M / NB-IoT + MQTT — architecture 2 variante B (brief §8.2).
//
// « Si le cellulaire est imposé par le client, c'est cette variante qu'il faut
// coder, jamais le SMS. » Elle apporte l'ordre (TCP), QoS 1, et surtout le
// Last Will and Testament : la perte de l'AGV est détectée par le broker, sans
// attendre l'expiration d'un chien de garde applicatif.
//
// Modem SIM7080G piloté en AT (jeu de commandes SMxxx).
// Topics : agv/<id>/cmd, agv/<id>/ack, agv/<id>/telemetry, agv/<id>/status.
#pragma once

#include <cstddef>
#include <cstdint>

#include "config/hardware_profile.h"
#include "hal/byte_stream.h"
#include "proto/secure_channel.h"
#include "transport/at_engine.h"
#include "transport/itransport.h"

namespace agv {

class MqttLteTransport final : public ITransport, public IAtUrcHandler {
 public:
  enum class State : uint8_t {
    PowerOff,
    PowerOn,
    Attach,      // attachement réseau (CGATT / CNACT)
    Configure,   // paramètres MQTT dont le LWT
    Connecting,  // SMCONN
    Subscribing,
    Online,
    Publishing,
    Recovering,
  };

  MqttLteTransport(const HardwareProfile& profile, IByteStream& uart, IModemPower& power)
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
  const char* name() const override { return "mqtt-lte"; }
  // TCP garantit l'ordre à l'intérieur d'une connexion.
  bool ordered() const override { return true; }
  // Latence 0,5–2 s, pire cas ~10 s à la reconnexion : le contrôle de fraîcheur
  // reste utile, avec la marge du profil.
  uint32_t max_command_age_s() const override { return profile_.safety.max_command_age_s; }

  void on_urc(const char* line) override;
  State state() const { return state_; }

 private:
  void advance_step();
  void enter(State s);
  const char* topic_for(FrameType t) const;

  const HardwareProfile& profile_;
  IByteStream& uart_;
  IModemPower& power_;
  AtEngine at_;
  SecureChannel channel_;
  LinkHealth health_{};

  State state_ = State::PowerOff;
  uint8_t step_ = 0;
  uint32_t state_since_ms_ = 0;

  static constexpr size_t kRxQueue = 4;
  Frame rx_queue_[kRxQueue] = {};
  size_t rx_count_ = 0;

  uint8_t tx_buf_[kSecurePacketMax] = {};
  size_t tx_len_ = 0;
  bool tx_pending_ = false;
  FrameType tx_type_ = FrameType::Ping;
  char topic_buf_[64] = {};
};

}  // namespace agv
