// Profil matériel/applicatif — point d'entrée unique de la configuration.
//
// Le header généré ne fournit que les VALEURS PAR DÉFAUT. Tout le code exploite
// la structure `HardwareProfile` en runtime : c'est ce qui permet aux tests du
// §11 de prouver qu'aucun paramètre du §12 n'est figé dans la logique.
#pragma once

#include <cstdint>

#include "config/generated_profile.h"

namespace agv {

// Variante d'interface bus retenue (§12.10) — pas encore tranchée.
enum class DriverVariant : uint8_t {
  Sim = CFG_DRIVER_SIM,
  Mcp23017 = CFG_DRIVER_MCP23017,
  Shift595 = CFG_DRIVER_SHIFT595,
  MegaUart = CFG_DRIVER_MEGA_UART,
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

struct EnoceanConfig {
  uint32_t dedup_window_ms;       // PTM 210 : 3 sous-télégrammes (§7)
  // §12.8 : TCM 515 (Rx seul) ou TCM 310 (bidirectionnel). Tant que c'est vrai,
  // aucun accusé n'est possible vers le bouton EnOcean — l'IHM ne doit donc
  // rien promettre à l'opérateur de ce côté.
  bool rx_only;
  uint32_t pairing_mode_timeout_s;
  uint32_t max_pairings;
};

struct CellularConfig {
  const char* apn;
  const char* sim_pin;
  const char* peer_msisdn;   // variante SMS : numéro du pair
  const char* alert_msisdn;  // AlertGateway (§8.3), hors chaîne de commande
  uint32_t alerts_per_day_max;
  uint32_t pwrkey_on_ms;
  uint32_t pwrkey_off_ms;
  uint32_t at_timeout_ms;
  uint32_t modem_mute_timeout_ms;
  const char* mqtt_host;
  uint16_t mqtt_port;
  uint8_t mqtt_qos;
  uint32_t mqtt_keepalive_s;
  const char* mqtt_client_id;
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
  ProtocolConfig protocol;
  EnoceanConfig enocean;
  CellularConfig cellular;
  MaintenanceConfig maintenance;
};

// Profil compilé depuis profiles/*.yaml. Les tests en prennent une copie et la
// modifient : aucun composant ne doit relire les macros directement.
const HardwareProfile& default_profile();

}  // namespace agv
