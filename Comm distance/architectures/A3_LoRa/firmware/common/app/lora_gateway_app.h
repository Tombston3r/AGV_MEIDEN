// Passerelle ESP32 : LoRa <-> liaison série vers l'ATmega.
//
// C'est l'équivalent LoRa de la passerelle MQTT de l'architecture A4, et elle
// remplit exactement le même contrat côté ATmega : mêmes trames de liaison,
// même heartbeat, même repli de sécurité. Seule la source des ordres change.
//
// L'ESP32 ne touche JAMAIS au bus MEIDEN. Son travail :
//   - entretenir le heartbeat vers l'ATmega : c'est ce qui autorise l'AGV à
//     accepter des courses, et cela ne dépend d'aucune radio ;
//   - traduire les trames applicatives reçues en LoRa en commandes série ;
//   - renvoyer un accusé par la même voie ;
//   - refuser les doublons AVANT de les transmettre.
//
// Toute cette logique est testable en natif : la radio et l'UART sont derrière
// des interfaces.
#pragma once

#include <cstdint>

#include "app/clock.h"
#include "bus/bus_signals.h"  // kStationMax / kSpeedMax
#include "config/hardware_profile.h"
#include "link/link_protocol.h"
#include "proto/frame.h"
#include "transport/itransport.h"

namespace agv {

// Écriture sur la liaison série vers l'ATmega.
class ILinkPort {
 public:
  virtual ~ILinkPort() = default;
  virtual void write(const uint8_t* data, size_t len) = 0;
};

struct LoraGatewayStats {
  uint32_t cmd_received = 0;
  uint32_t cmd_forwarded = 0;
  uint32_t cmd_out_of_order = 0;
  uint32_t cmd_duplicate = 0;
  uint32_t cmd_malformed = 0;
  uint32_t acks_sent = 0;
  uint32_t acks_refused_duty = 0;  // budget de rapport cyclique épuisé
  uint32_t telemetry_sent = 0;
  uint32_t heartbeats_sent = 0;
  uint32_t link_timeouts = 0;      // l'ATmega ne répond plus
};

class LoraGatewayApp {
 public:
  LoraGatewayApp(const HardwareProfile& profile, IClock& clock, ITransport& radio,
                 ILinkPort& link)
      : profile_(profile), clock_(clock), radio_(radio), link_(link),
        parser_(link::kSofToEsp) {}

  void begin();

  // À appeler en boucle : heartbeat, écoute radio, interrogation d'état.
  void tick();

  // Octet reçu de l'ATmega.
  void on_link_byte(uint8_t byte);

  const link::LinkState& state() const { return state_; }
  bool link_up() const { return link_up_; }
  const LoraGatewayStats& stats() const { return stats_; }

  // Rendu de l'état, partagé avec la page de maintenance.
  size_t render_state_json(char* out, size_t capacity) const;

 private:
  void send_heartbeat();
  void request_state();
  void handle_frame(const Frame& f);
  void send_ack(uint8_t seq, link::CmdResult result);
  void send_telemetry();

  const HardwareProfile& profile_;
  IClock& clock_;
  ITransport& radio_;
  ILinkPort& link_;
  link::Parser parser_;
  link::LinkState state_{};
  LoraGatewayStats stats_{};

  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_poll_ms_ = 0;
  uint32_t last_telemetry_ms_ = 0;
  uint32_t last_link_rx_ms_ = 0;
  // Drapeau explicite : `last_link_rx_ms_ == 0` est un instant VALIDE au
  // démarrage, pas une sentinelle. S'en servir comme telle rendait la
  // détection de silence inopérante pendant les premières millisecondes.
  bool have_link_rx_ = false;
  bool link_up_ = false;

  // Idempotence : une même (node_id, seq) est RÉ-ACQUITTÉE sans être
  // ré-exécutée. Sans cela, un accusé perdu déclenche une course en double.
  static constexpr size_t kNodes = 8;
  struct LastSeq {
    uint16_t node_id = 0;
    uint8_t seq = 0;
    bool used = false;
  };
  LastSeq seen_[kNodes]{};
  bool already_seen(uint16_t node_id, uint8_t seq);

  // Dernière commande transmise, pour associer l'accusé de l'ATmega.
  uint16_t pending_node_ = 0;
  uint8_t pending_seq_ = 0;
  bool pending_ = false;
};

}  // namespace agv
