// Abstraction transport (brief §5).
//
// Toute la logique métier passe par cette interface : c'est ce qui permet de
// basculer entre les architectures A1 (LoRa), A2 (SMS / LTE-M) et A3 (EnOcean
// + LoRa) sans retoucher le séquenceur.
#pragma once

#include <cstdint>

#include "proto/frame.h"

namespace agv {

struct LinkHealth {
  bool up = false;             // liaison considérée établie
  int16_t rssi_dbm = 0;        // LoRa : RSSI ; cellulaire : RSRP
  int8_t snr_db = 0;
  uint32_t last_rx_ms = 0;     // date de la dernière trame reçue
  uint32_t last_ack_ms = 0;    // date du dernier ACK applicatif
  uint32_t tx_ok = 0;
  uint32_t tx_failed = 0;
  uint32_t rx_ok = 0;
  uint32_t rx_bad_crc = 0;
  uint32_t tx_refused_duty = 0;  // émissions refusées par le budget légal
  uint32_t duty_used_permille = 0;
};

class ITransport {
 public:
  virtual ~ITransport() = default;
  virtual bool begin() = 0;
  virtual bool send(const Frame& f) = 0;   // non bloquant
  virtual bool poll(Frame& out) = 0;       // une trame reçue ?
  virtual LinkHealth health() const = 0;
  virtual const char* name() const = 0;

  // Le transport garantit-il l'ordre de remise ? Détermine l'activation du
  // verdict OutOfOrder de la fenêtre anti-rejeu (§8.1). Faux pour le SMS.
  virtual bool ordered() const { return true; }

  // Âge maximal toléré pour une commande reçue par ce transport, en secondes.
  // 0 = contrôle désactivé (latence bornée par construction).
  virtual uint32_t max_command_age_s() const { return 0; }

  // À appeler périodiquement : retransmissions, fenêtres d'écoute, machine AT.
  virtual void tick() {}
};

}  // namespace agv
