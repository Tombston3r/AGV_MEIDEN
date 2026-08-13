#include "config/hardware_profile.h"

namespace agv {

const HardwareProfile& default_profile() {
  // Seul endroit du dépôt autorisé à lire les macros CFG_*.
  static const HardwareProfile profile = {
      CFG_PROFILE_NAME,
      BusConfig{
          CFG_BUS_X_ACTIVE_HIGH,
          CFG_BUS_Y_ACTIVE_HIGH,
          CFG_BUS_T_SETUP_US,
          CFG_BUS_T_STROBE_US,
          CFG_BUS_T_HOLD_US,
          CFG_BUS_Y_DEBOUNCE_US,
          CFG_BUS_MCP_AB_SKEW_US,
          static_cast<DriverVariant>(CFG_BUS_DRIVER_VARIANT),
      },
      TimeoutConfig{
          CFG_TIMEOUTS_Y22_WRITE_ACK_MS,
          CFG_TIMEOUTS_Y05_START_ACK_MS,
          CFG_TIMEOUTS_Y10_ARRIVAL_MS,
          CFG_TIMEOUTS_WRITE_MAX_TRIES,
          CFG_TIMEOUTS_START_MAX_TRIES,
          CFG_TIMEOUTS_STOP_MAX_TRIES,
      },
      SafetyConfig{
          CFG_SAFETY_LINK_WATCHDOG_S,
          CFG_SAFETY_MAX_COMMAND_AGE_S,
          CFG_SAFETY_SAFE_STOP_AT_NEXT_STATION,
      },
      QueueConfig{
          CFG_QUEUE_MAX_COURSES,
          CFG_QUEUE_COURSE_VALIDITY_MIN,
          CFG_QUEUE_PERSIST_TO_NVS,
      },
      ProtocolConfig{
          static_cast<uint8_t>(CFG_PROTOCOL_VERSION),
          static_cast<uint16_t>(CFG_PROTOCOL_NODE_ID),
          CFG_PROTOCOL_REPLAY_WINDOW,
          CFG_PROTOCOL_AES_ENABLED,
      },
      LoraConfig{
          CFG_LORA_FREQUENCY_HZ,
          static_cast<uint8_t>(CFG_LORA_SPREADING_FACTOR),
          CFG_LORA_BANDWIDTH_HZ,
          static_cast<uint8_t>(CFG_LORA_CODING_RATE),
          static_cast<uint8_t>(CFG_LORA_SYNC_WORD),
          static_cast<int8_t>(CFG_LORA_TX_POWER_DBM),
          CFG_LORA_ACK_TIMEOUT_MS,
          CFG_LORA_MAX_TRIES,
          CFG_LORA_DUTY_CYCLE_PERMILLE,
          CFG_LORA_DUTY_WINDOW_MS,
      },
      EnoceanConfig{
          CFG_ENOCEAN_DEDUP_WINDOW_MS,
          CFG_ENOCEAN_RX_ONLY,
          CFG_ENOCEAN_PAIRING_MODE_TIMEOUT_S,
          CFG_ENOCEAN_MAX_PAIRINGS,
      },
      CellularConfig{
          CFG_CELLULAR_APN,
          CFG_CELLULAR_SIM_PIN,
          CFG_CELLULAR_PEER_MSISDN,
          CFG_CELLULAR_ALERT_MSISDN,
          CFG_CELLULAR_ALERTS_PER_DAY_MAX,
          CFG_CELLULAR_PWRKEY_ON_MS,
          CFG_CELLULAR_PWRKEY_OFF_MS,
          CFG_CELLULAR_AT_TIMEOUT_MS,
          CFG_CELLULAR_MODEM_MUTE_TIMEOUT_MS,
          CFG_CELLULAR_MQTT_HOST,
          static_cast<uint16_t>(CFG_CELLULAR_MQTT_PORT),
          static_cast<uint8_t>(CFG_CELLULAR_MQTT_QOS),
          CFG_CELLULAR_MQTT_KEEPALIVE_S,
          CFG_CELLULAR_MQTT_CLIENT_ID,
      },
      MaintenanceConfig{
          CFG_MAINTENANCE_WIFI_ENABLED_AT_BOOT,
          CFG_MAINTENANCE_WIFI_WINDOW_S,
          CFG_MAINTENANCE_WIFI_SSID,
      },
  };
  return profile;
}

}  // namespace agv
