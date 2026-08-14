// Port matériel du module RFM95W / SX1276 (brief §6).
//
// Isolé pour que `LoraTransport` — ordonnanceur half-duplex, retransmissions et
// budget de rapport cyclique — soit testable en natif avec une radio factice.
#pragma once

#include <cstddef>
#include <cstdint>

#include "config/lora_config.h"

namespace agv {

class ILoraRadio {
 public:
  virtual ~ILoraRadio() = default;
  virtual bool begin(const LoraConfig& cfg) = 0;

  // Démarre une émission. Le RFM95W est HALF-DUPLEX : pendant l'émission la
  // réception est impossible, d'où l'ordonnancement explicite du transport.
  virtual bool transmit(const uint8_t* data, size_t len) = 0;
  virtual bool tx_busy() const = 0;

  // Repasse en écoute continue.
  virtual void listen() = 0;

  // Récupère une trame reçue. False si rien en attente.
  virtual bool receive(uint8_t* buf, size_t capacity, size_t& len, int16_t& rssi_dbm,
                       int8_t& snr_db) = 0;

  virtual uint32_t now_ms() const = 0;
};

}  // namespace agv
