// Passerelle ESP32 : MQTT <-> liaison série vers l'ATmega (planification §2.6-2.7).
//
// L'ESP32 ne touche JAMAIS au bus MEIDEN. Son travail :
//   - entretenir le heartbeat vers l'ATmega (c'est ce qui autorise l'AGV à
//     accepter des courses) ;
//   - traduire `agv/<id>/cmd` (JSON) en commandes série ;
//   - publier `agv/<id>/state` et `agv/<id>/ack` ;
//   - refuser les commandes périmées et les doublons AVANT de les transmettre.
//
// Toute cette logique est testable en natif : le client MQTT et l'UART sont
// derrière des interfaces.
#pragma once

#include <cstdint>

#include "app/clock.h"
#include "bus/bus_signals.h"  // kStationMax / kSpeedMax
#include "config/hardware_profile.h"
#include "link/link_protocol.h"

namespace agv {

// Publication MQTT. Implémentée par esp-mqtt sur cible, par une doublure en test.
class IMqttPublisher {
 public:
  virtual ~IMqttPublisher() = default;
  // `retain` : vrai pour `state` et `status` (planification §2).
  virtual bool publish(const char* topic, const char* payload, bool retain) = 0;
  virtual bool connected() const = 0;
};

class ILinkPort {
 public:
  virtual ~ILinkPort() = default;
  virtual void write(const uint8_t* data, size_t len) = 0;
};

struct GatewayStats {
  uint32_t cmd_received = 0;
  uint32_t cmd_forwarded = 0;
  uint32_t cmd_expired = 0;     // horodatage plus vieux que max_command_age_s
  uint32_t cmd_out_of_order = 0;
  uint32_t cmd_duplicate = 0;
  uint32_t cmd_malformed = 0;
  uint32_t acks_published = 0;
  uint32_t states_published = 0;
  uint32_t heartbeats_sent = 0;
  uint32_t link_timeouts = 0;   // le MEGA ne répond plus
};

class GatewayApp {
 public:
  GatewayApp(const HardwareProfile& profile, IClock& clock, IMqttPublisher& mqtt, ILinkPort& link)
      : profile_(profile), clock_(clock), mqtt_(mqtt), link_(link),
        parser_(link::kSofToEsp) {}

  void begin();
  // À appeler en boucle : heartbeat, interrogation d'état, publication.
  void tick();

  // Charge utile reçue sur `agv/<id>/cmd`.
  void on_mqtt_command(const char* payload);
  // Octet reçu de l'ATmega.
  void on_link_byte(uint8_t byte);

  const link::LinkState& state() const { return state_; }
  bool link_up() const { return link_up_; }
  const GatewayStats& stats() const { return stats_; }

  // Topics construits une fois pour toutes (planification §2).
  const char* topic_state() const { return topic_state_; }
  const char* topic_cmd() const { return topic_cmd_; }
  const char* topic_ack() const { return topic_ack_; }
  const char* topic_status() const { return topic_status_; }

  // Rendu de l'état en JSON, partagé entre MQTT et la page de maintenance.
  size_t render_state_json(char* out, size_t capacity) const;

 private:
  void send_heartbeat();
  void request_state();
  void publish_state();
  void publish_ack(uint8_t seq, link::CmdResult result);

  const HardwareProfile& profile_;
  IClock& clock_;
  IMqttPublisher& mqtt_;
  ILinkPort& link_;
  link::Parser parser_;
  link::LinkState state_{};
  GatewayStats stats_{};

  char topic_state_[48] = {};
  char topic_cmd_[48] = {};
  char topic_ack_[48] = {};
  char topic_status_[48] = {};

  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_poll_ms_ = 0;
  uint32_t last_publish_ms_ = 0;
  uint32_t last_link_rx_ms_ = 0;
  bool link_up_ = false;
  bool have_last_seq_ = false;
  uint8_t last_cmd_seq_ = 0;
};

}  // namespace agv
