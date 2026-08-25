#include "config/hardware_profile.h"

namespace agv {

const HardwareProfile& default_profile() {
  // Seul endroit du dossier autorisé à lire les macros CFG_*.
  static const HardwareProfile profile = {
      CFG_PROFILE_NAME,
      BusConfig{
          CFG_BUS_X_ACTIVE_HIGH,
          CFG_BUS_Y_ACTIVE_HIGH,
          CFG_BUS_T_SETUP_US,
          CFG_BUS_T_STROBE_US,
          CFG_BUS_T_HOLD_US,
          CFG_BUS_Y_DEBOUNCE_US,
          CFG_BUS_Y_PULLUPS,
          CFG_BUS_X_OPEN_DRAIN,
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
      HeartbeatConfig{
          CFG_HEARTBEAT_PERIOD_MS,
          CFG_HEARTBEAT_TIMEOUT_MS,
      },
      LinkConfig{
          CFG_LINK_BAUD,
          CFG_LINK_REPLY_TIMEOUT_MS,
          CFG_LINK_STATE_POLL_MS,
      },
      WifiConfig{
          CFG_WIFI_SSID,
          CFG_WIFI_PASSWORD,
          CFG_WIFI_USE_STATIC_IP,
          CFG_WIFI_STATIC_IP,
          CFG_WIFI_GATEWAY,
          CFG_WIFI_NETMASK,
          CFG_WIFI_RECONNECT_BACKOFF_MS,
          CFG_WIFI_RECONNECT_BACKOFF_MAX_MS,
          static_cast<int16_t>(CFG_WIFI_RSSI_WARN_DBM),
      },
      MqttConfig{
          CFG_MQTT_HOST,
          static_cast<uint16_t>(CFG_MQTT_PORT),
          static_cast<uint8_t>(CFG_MQTT_QOS),
          CFG_MQTT_KEEPALIVE_S,
          CFG_MQTT_CLIENT_ID,
          CFG_MQTT_AGV_ID,
          CFG_MQTT_USERNAME,
          CFG_MQTT_PASSWORD,
          CFG_MQTT_TLS,
          CFG_MQTT_STATE_PERIOD_MS,
      },
      ProtocolConfig{
          static_cast<uint8_t>(CFG_PROTOCOL_VERSION),
          static_cast<uint16_t>(CFG_PROTOCOL_NODE_ID),
          CFG_PROTOCOL_REPLAY_WINDOW,
          CFG_PROTOCOL_AES_ENABLED,
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
