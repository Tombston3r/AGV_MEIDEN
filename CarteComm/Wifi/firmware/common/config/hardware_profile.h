// Profil matériel/applicatif — point d'entrée unique de la configuration.
//
// Le header généré ne fournit que les VALEURS PAR DÉFAUT. Tout le code exploite
// la structure `HardwareProfile` en runtime : c'est ce qui permet aux tests du
// §11 de prouver qu'aucun paramètre du §12 n'est figé dans la logique.
#pragma once

#include <cstdint>

#include "config/generated_profile.h"

namespace agv {

// Sur cette carte, le bus est posé par les ports de l'ATmega2560 : il n'y a
// pas d'arbitrage §12.10 à faire, le matériel est celui de la V5.0.1.
enum class DriverVariant : uint8_t {
  Sim = CFG_DRIVER_SIM,
  AvrPort = CFG_DRIVER_AVR_PORT,
};

// Repli de sécurité porté par l'ATmega (planification §2). Il ne dépend NI du
// Wi-Fi, NI du réseau d'entreprise, NI du poste fixe : c'est ce qui le rend
// utile le jour où l'infrastructure tombe.
struct HeartbeatConfig {
  uint32_t period_ms;   // cadence d'émission côté ESP32
  uint32_t timeout_ms;  // au-delà, l'ATmega s'arrête au point d'arrêt suivant
};

// Liaison série inter-MCU sur la carte.
struct LinkConfig {
  uint32_t baud;
  uint32_t reply_timeout_ms;
  uint32_t state_poll_ms;
};

struct WifiConfig {
  const char* ssid;
  const char* password;
  bool use_static_ip;
  const char* static_ip;
  const char* gateway;
  const char* netmask;
  uint32_t reconnect_backoff_ms;
  uint32_t reconnect_backoff_max_ms;
  int16_t rssi_warn_dbm;
};

struct MqttConfig {
  const char* host;
  uint16_t port;
  uint8_t qos;
  uint32_t keepalive_s;
  const char* client_id;
  const char* agv_id;
  const char* username;
  const char* password;
  bool tls;
  uint32_t state_period_ms;
};

struct BusConfig {
  bool x_active_high;      // §12.3 PNP/NPN
  bool y_active_high;      // §12.3
  uint32_t t_setup_us;     // §12.4 stabilisation avant strobe X93
  uint32_t t_strobe_us;    // §12.4 largeur du strobe
  uint32_t t_hold_us;      // §12.4 maintien après strobe
  uint32_t y_debounce_us;  // §12.1 dépend de l'amplitude réelle des Y
  uint32_t mcp_ab_skew_us; // §12.10 décalage GPIOA/GPIOB résiduel
  DriverVariant variant;   // §12.10
};

struct TimeoutConfig {
  uint32_t y22_write_ack_ms;  // §12.5
  uint32_t y05_start_ack_ms;  // §12.5
  uint32_t y10_arrival_ms;    // §12.5
  uint32_t write_max_tries;
  uint32_t start_max_tries;
  uint32_t stop_max_tries;
};

struct SafetyConfig {
  uint32_t link_watchdog_s;      // absence de trame valide -> état sûr
  uint32_t max_command_age_s;    // commande périmée jamais exécutée (§8.1)
  bool safe_stop_at_next_station;
};

struct QueueConfig {
  uint32_t max_courses;          // 5 (§4.5)
  uint32_t course_validity_min;  // au-delà, la course persistée est écartée
  bool persist_to_nvs;
};

struct ProtocolConfig {
  uint8_t version;
  uint16_t node_id;
  uint32_t replay_window;  // 16 derniers (node_id, seq) (§5.1)
  bool aes_enabled;
};

struct MaintenanceConfig {
  bool wifi_enabled_at_boot;  // false par défaut (§9.4)
  uint32_t wifi_window_s;
  const char* wifi_ssid;
};

struct HardwareProfile {
  const char* name;
  BusConfig bus;
  TimeoutConfig timeouts;
  SafetyConfig safety;
  QueueConfig queue;
  HeartbeatConfig heartbeat;
  LinkConfig link;
  WifiConfig wifi;
  MqttConfig mqtt;
  ProtocolConfig protocol;
  MaintenanceConfig maintenance;
};

// Profil compilé depuis profiles/*.yaml. Les tests en prennent une copie et la
// modifient : aucun composant ne doit relire les macros directement.
const HardwareProfile& default_profile();

}  // namespace agv
