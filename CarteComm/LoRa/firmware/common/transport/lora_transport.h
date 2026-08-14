// Transport LoRa P2P 868 MHz — architecture 1 (brief §6).
//
// Trois responsabilités que le reste du logiciel n'a pas à connaître :
//  1. ORDONNANCEMENT HALF-DUPLEX : le RFM95W ne peut pas écouter en émettant.
//     Après chaque émission on ouvre explicitement une fenêtre d'écoute d'ACK,
//     puis on retourne en écoute de télémétrie. Rien n'est improvisé.
//  2. RETRANSMISSIONS : jusqu'à `max_tries`, fenêtre `ack_timeout_ms`.
//  3. BUDGET DE RAPPORT CYCLIQUE : 1 % sur 1 h glissante (EN 300 220 /
//     ERC 70-03). Au-delà, l'émission est REFUSÉE et remontée en défaut.
#pragma once

#include <cstdint>

#include "config/hardware_profile.h"
#include "config/lora_config.h"
#include "proto/secure_channel.h"
#include "transport/duty_cycle.h"
#include "transport/itransport.h"
#include "transport/lora_radio.h"

namespace agv {

class LoraTransport final : public ITransport {
 public:
  enum class TxState : uint8_t {
    Idle,
    Pending,    // trame en attente d'un créneau (budget, radio occupée)
    Sending,    // émission en cours
    AwaitAck,   // fenêtre d'écoute d'ACK ouverte
  };

  // `lora` est passé séparément : le cœur métier est partagé entre
  // architectures et ne transporte aucun paramètre radio (voir lora_config.h).
  LoraTransport(const HardwareProfile& profile, const LoraConfig& lora, ILoraRadio& radio)
      : profile_(profile),
        lora_(lora),
        radio_(radio),
        duty_(lora.duty_cycle_permille, lora.duty_window_ms) {}

  SecureChannel& channel() { return channel_; }

  bool begin() override;
  bool send(const Frame& f) override;
  bool poll(Frame& out) override;
  void tick() override;
  LinkHealth health() const override;
  const char* name() const override { return "lora"; }
  bool ordered() const override { return true; }
  // Latence bornée (~200 ms, pire cas ~800 ms) : le contrôle de fraîcheur du
  // §8.1 n'a pas lieu d'être ici, contrairement au SMS.
  uint32_t max_command_age_s() const override { return 0; }

  TxState tx_state() const { return tx_state_; }
  uint32_t retries() const { return tries_; }
  const DutyCycleBudget& duty() const { return duty_; }
  bool duty_blocked() const { return duty_blocked_; }

 private:
  static bool needs_ack(FrameType t);
  bool try_transmit(uint32_t now_ms);

  const HardwareProfile& profile_;
  LoraConfig lora_;
  ILoraRadio& radio_;
  SecureChannel channel_;
  DutyCycleBudget duty_;
  LinkHealth health_{};

  TxState tx_state_ = TxState::Idle;
  uint8_t tx_buf_[kSecurePacketMax] = {};
  size_t tx_len_ = 0;
  uint8_t tx_seq_ = 0;
  uint32_t tries_ = 0;
  uint32_t last_tx_ms_ = 0;
  uint32_t pending_airtime_us_ = 0;
  bool duty_blocked_ = false;
  bool ack_expected_ = false;

  // Trames reçues en attente de lecture par poll().
  static constexpr size_t kRxQueue = 4;
  Frame rx_queue_[kRxQueue] = {};
  size_t rx_count_ = 0;
};

}  // namespace agv
