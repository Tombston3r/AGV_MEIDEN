#include "transport/lora_transport.h"

namespace agv {

bool LoraTransport::begin() {
  if (!radio_.begin(lora_)) return false;
  radio_.listen();
  tx_state_ = TxState::Idle;
  health_ = LinkHealth{};
  return true;
}

bool LoraTransport::needs_ack(FrameType t) {
  // Télémétrie et PING ne sont pas acquittés : les acquitter saturerait le
  // budget de rapport cyclique sans rien apporter.
  return t == FrameType::CmdGoto || t == FrameType::CmdStop || t == FrameType::Pair;
}

bool LoraTransport::send(const Frame& f) {
  if (tx_state_ != TxState::Idle) return false;  // non bloquant : refus franc
  tx_len_ = channel_.seal(f, tx_buf_, sizeof(tx_buf_));
  if (tx_len_ == 0) return false;
  tx_seq_ = f.seq;
  tries_ = 0;
  tx_state_ = TxState::Pending;
  pending_airtime_us_ = lora_airtime_us(tx_len_, lora_.spreading_factor,
                                        lora_.bandwidth_hz, lora_.coding_rate);
  ack_expected_ = needs_ack(f.type);
  return true;
}

bool LoraTransport::try_transmit(uint32_t now_ms) {
  if (radio_.tx_busy()) return false;
  if (!duty_.can_transmit(pending_airtime_us_, now_ms)) {
    // Refus réglementaire : remonté en défaut applicatif visible, jamais
    // silencieux (brief §6).
    if (!duty_blocked_) {
      duty_.note_refusal();
      ++health_.tx_refused_duty;
      duty_blocked_ = true;
    }
    return false;
  }
  duty_blocked_ = false;
  if (!radio_.transmit(tx_buf_, tx_len_)) {
    ++health_.tx_failed;
    return false;
  }
  duty_.record(pending_airtime_us_, now_ms);
  ++health_.tx_ok;
  ++tries_;
  last_tx_ms_ = now_ms;
  tx_state_ = TxState::Sending;
  return true;
}

void LoraTransport::tick() {
  const uint32_t now_ms = radio_.now_ms();

  // 1. Réception — toujours drainée en premier : une trame reçue pendant la
  //    fenêtre d'ACK doit être vue avant d'armer une retransmission.
  uint8_t buf[kSecurePacketMax];
  size_t len = 0;
  int16_t rssi = 0;
  int8_t snr = 0;
  while (radio_.receive(buf, sizeof(buf), len, rssi, snr)) {
    Frame f;
    if (!channel_.open(buf, len, f, profile_.protocol.version)) {
      ++health_.rx_bad_crc;
      continue;
    }
    ++health_.rx_ok;
    health_.rssi_dbm = rssi;
    health_.snr_db = snr;
    health_.last_rx_ms = now_ms;
    health_.up = true;

    if (f.type == FrameType::Ack && tx_state_ == TxState::AwaitAck && f.seq == tx_seq_) {
      health_.last_ack_ms = now_ms;
      tx_state_ = TxState::Idle;
      radio_.listen();
      continue;  // l'ACK est consommé par le transport, pas remonté au métier
    }
    if (rx_count_ < kRxQueue) rx_queue_[rx_count_++] = f;
  }

  // 2. Ordonnancement d'émission.
  switch (tx_state_) {
    case TxState::Idle:
      break;
    case TxState::Pending:
      try_transmit(now_ms);
      break;
    case TxState::Sending:
      if (!radio_.tx_busy()) {
        if (ack_expected_) {
          radio_.listen();  // fenêtre d'écoute d'ACK
          tx_state_ = TxState::AwaitAck;
        } else {
          radio_.listen();
          tx_state_ = TxState::Idle;
        }
      }
      break;
    case TxState::AwaitAck:
      if ((now_ms - last_tx_ms_) >= lora_.ack_timeout_ms) {
        if (tries_ >= lora_.max_tries) {
          ++health_.tx_failed;
          health_.up = false;  // pire cas ~800 ms après 3 retransmissions
          tx_state_ = TxState::Idle;
        } else {
          tx_state_ = TxState::Pending;  // retransmission, budget revérifié
        }
      }
      break;
  }
}

bool LoraTransport::poll(Frame& out) {
  if (rx_count_ == 0) return false;
  out = rx_queue_[0];
  for (size_t i = 1; i < rx_count_; ++i) rx_queue_[i - 1] = rx_queue_[i];
  --rx_count_;
  return true;
}

LinkHealth LoraTransport::health() const {
  LinkHealth h = health_;
  h.duty_used_permille = duty_.used_permille_of_budget(radio_.now_ms());
  return h;
}

}  // namespace agv
