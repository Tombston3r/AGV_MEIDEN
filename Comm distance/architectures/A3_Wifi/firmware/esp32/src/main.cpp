// Firmware ESP32 de la carte AIO AGV Control V5.0.1 : RÉÉCRIT.
//
// Changement de rôle par rapport à l'existant (planification §1) :
//   avant : point d'accès `agv_atelier`, piloté par l'application mobile
//   après : CLIENT du réseau Wi-Fi d'entreprise, piloté en MQTT par le poste
//
// Responsabilités (planification §2.6 à §2.8) :
//   - client Wi-Fi STA avec reconnexion et IP statique ;
//   - client MQTT : publication `state`/`ack`, abonnement `cmd`, LWT ;
//   - heartbeat vers l'ATmega : c'est ce qui autorise l'AGV à rouler ;
//   - point d'accès de maintenance à la demande, servant `/agvdump`.
//
// L'ESP32 NE TOUCHE JAMAIS AU BUS MEIDEN. Toute la commande passe par
// l'ATmega, qui reste maître de la sécurité de repli.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include <nvs_flash.h>

#include <cstring>

#include "app/agvdump.h"
#include "app/gateway_app.h"
#include "board_pins.h"
#include "config/hardware_profile.h"
#include "maintenance_ap.h"
#include "platform/esp32/esp_ports.h"

namespace {

const char* TAG = "agv-esp32";

agv::esp32::EspClock g_clock;
agv::esp32::EspUart g_link_uart;
agv::GatewayApp* g_gateway = nullptr;
esp_mqtt_client_handle_t g_mqtt = nullptr;
bool g_wifi_up = false;
bool g_mqtt_up = false;

// --- Ports de la passerelle ------------------------------------------------

class UartLinkPort final : public agv::ILinkPort {
 public:
  void write(const uint8_t* data, size_t len) override { g_link_uart.write(data, len); }
};

class EspMqttPublisher final : public agv::IMqttPublisher {
 public:
  bool publish(const char* topic, const char* payload, bool retain) override {
    if (g_mqtt == nullptr || !g_mqtt_up) return false;
    const int id = esp_mqtt_client_publish(g_mqtt, topic, payload, 0,
                                           agv::default_profile().mqtt.qos, retain ? 1 : 0);
    return id >= 0;
  }
  bool connected() const override { return g_mqtt_up; }
};

UartLinkPort g_link_port;
EspMqttPublisher g_publisher;

// --- Wi-Fi -----------------------------------------------------------------

void on_wifi_event(void*, esp_event_base_t base, int32_t id, void* data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    g_wifi_up = false;
    // Reconnexion immédiate : en mouvement, chaque seconde de coupure est une
    // commande potentiellement perdue. Le heartbeat vers l'ATmega continue
    // pendant ce temps : la carte n'est pas en défaut, c'est le réseau.
    ESP_LOGW(TAG, "Wi-Fi perdu, reconnexion");
    esp_wifi_connect();
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    g_wifi_up = true;
    const auto* event = static_cast<ip_event_got_ip_t*>(data);
    ESP_LOGI(TAG, "Wi-Fi OK, IP=" IPSTR, IP2STR(&event->ip_info.ip));
  }
}

void start_wifi(const agv::HardwareProfile& profile) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_t* netif = esp_netif_create_default_wifi_sta();

  if (profile.wifi.use_static_ip) {
    // IP statique : supprime le délai DHCP à chaque handover entre AP
    // (planification §1.4). Sur un AGV en mouvement, ce délai est la première
    // cause de commande perdue.
    esp_netif_dhcpc_stop(netif);
    esp_netif_ip_info_t ip = {};
    ip.ip.addr = esp_ip4addr_aton(profile.wifi.static_ip);
    ip.gw.addr = esp_ip4addr_aton(profile.wifi.gateway);
    ip.netmask.addr = esp_ip4addr_aton(profile.wifi.netmask);
    esp_netif_set_ip_info(netif, &ip);
  }

  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&init);
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, nullptr,
                                      nullptr);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi_event, nullptr,
                                      nullptr);

  wifi_config_t cfg = {};
  std::strncpy(reinterpret_cast<char*>(cfg.sta.ssid), profile.wifi.ssid,
               sizeof(cfg.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(cfg.sta.password), profile.wifi.password,
               sizeof(cfg.sta.password) - 1);
  // Roaming : on accepte de changer d'AP dès que le niveau se dégrade.
  cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &cfg);
  esp_wifi_set_ps(WIFI_PS_NONE);  // pas d'économie d'énergie : latence d'abord
  esp_wifi_start();
}

int16_t wifi_rssi() {
  wifi_ap_record_t ap;
  if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return 0;
  return static_cast<int16_t>(ap.rssi);
}

// --- MQTT ------------------------------------------------------------------

void on_mqtt_event(void*, esp_event_base_t, int32_t id, void* data) {
  auto* event = static_cast<esp_mqtt_event_handle_t>(data);
  const agv::HardwareProfile& profile = agv::default_profile();

  switch (static_cast<esp_mqtt_event_id_t>(id)) {
    case MQTT_EVENT_CONNECTED: {
      g_mqtt_up = true;
      esp_mqtt_client_subscribe(g_mqtt, g_gateway->topic_cmd(), profile.mqtt.qos);
      // Contrepartie du Last Will : on annonce explicitement la présence.
      esp_mqtt_client_publish(g_mqtt, g_gateway->topic_status(), "online", 0, profile.mqtt.qos,
                              1);
      ESP_LOGI(TAG, "MQTT connecté, abonné à %s", g_gateway->topic_cmd());
      break;
    }
    case MQTT_EVENT_DISCONNECTED:
      g_mqtt_up = false;
      ESP_LOGW(TAG, "MQTT déconnecté");
      break;
    case MQTT_EVENT_DATA: {
      // Charge utile terminée par un zéro avant analyse : esp-mqtt ne le
      // garantit pas, et l'analyseur JSON travaille sur une chaîne C.
      static char payload[512];
      const int n = (event->data_len < static_cast<int>(sizeof(payload)) - 1)
                        ? event->data_len
                        : static_cast<int>(sizeof(payload)) - 1;
      std::memcpy(payload, event->data, n);
      payload[n] = '\0';
      g_gateway->on_mqtt_command(payload);
      break;
    }
    default:
      break;
  }
}

void start_mqtt(const agv::HardwareProfile& profile) {
  esp_mqtt_client_config_t cfg = {};
  static char uri[96];
  std::snprintf(uri, sizeof(uri), "%s://%s:%u", profile.mqtt.tls ? "mqtts" : "mqtt",
                profile.mqtt.host, static_cast<unsigned>(profile.mqtt.port));
  cfg.broker.address.uri = uri;
  cfg.credentials.client_id = profile.mqtt.client_id;
  cfg.credentials.username = profile.mqtt.username;
  cfg.credentials.authentication.password = profile.mqtt.password;
  cfg.session.keepalive = static_cast<int>(profile.mqtt.keepalive_s);
  // Last Will and Testament : le broker annonce la perte de l'AGV sans
  // attendre l'expiration d'un chien de garde applicatif (planification §2).
  cfg.session.last_will.topic = g_gateway->topic_status();
  cfg.session.last_will.msg = "offline";
  cfg.session.last_will.qos = profile.mqtt.qos;
  cfg.session.last_will.retain = 1;

  g_mqtt = esp_mqtt_client_init(&cfg);
  esp_mqtt_client_register_event(g_mqtt, MQTT_EVENT_ANY, on_mqtt_event, nullptr);
  esp_mqtt_client_start(g_mqtt);
}

// --- Tâches ----------------------------------------------------------------

// Liaison série vers l'ATmega : lecture continue + heartbeat + interrogation.
void link_task(void*) {
  uint8_t buf[64];
  for (;;) {
    const size_t n = g_link_uart.read(buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) g_gateway->on_link_byte(buf[i]);
    g_gateway->tick();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Point d'accès de maintenance, ouvert sur contact ILS (planification §2.8).
void maintenance_task(void*) {
  const agv::HardwareProfile& profile = agv::default_profile();
  agv::maintenance::AccessPoint ap(profile, g_clock);
  ap.set_dump_source([](char* out, size_t cap) {
    const agv::GatewayStats& st = g_gateway->stats();
    agv::AgvDumpInput in;
    in.state = &g_gateway->state();
    in.profile_name = agv::default_profile().name;
    in.uptime_s = g_clock.now_ms() / 1000u;
    in.link_up = g_gateway->link_up();
    in.wifi_up = g_wifi_up;
    in.mqtt_up = g_mqtt_up;
    in.rssi_dbm = wifi_rssi();
    in.ssid = agv::default_profile().wifi.ssid;
    in.heartbeats_sent = st.heartbeats_sent;
    in.link_timeouts = st.link_timeouts;
    in.cmd_expired = st.cmd_expired;
    in.cmd_duplicate = st.cmd_duplicate;
    return agv::render_agvdump(in, out, cap);
  });

  agv::esp32::EspGpio gpio;
  gpio.configure_input(agv::board::kMaintenanceReed, true);
  for (;;) {
    if (!gpio.get(agv::board::kMaintenanceReed)) ap.request_open();
    ap.tick();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

}  // namespace

extern "C" void app_main() {
  const agv::HardwareProfile& profile = agv::default_profile();
  ESP_LOGI(TAG, "ESP32 carte V5.0.1 : client Wi-Fi, profil=%s", profile.name);

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  g_link_uart.begin(UART_NUM_1, agv::board::kLinkTx, agv::board::kLinkRx,
                    static_cast<int>(profile.link.baud));

  static agv::GatewayApp gateway(profile, g_clock, g_publisher, g_link_port);
  g_gateway = &gateway;
  gateway.begin();

  // La liaison série démarre AVANT le Wi-Fi : le heartbeat vers l'ATmega ne
  // doit pas attendre l'association au réseau d'entreprise, sinon l'AGV reste
  // en repli de sécurité tant que le service informatique n'a pas fini.
  xTaskCreatePinnedToCore(link_task, "link", 6144, nullptr, 6, nullptr, 1);

  start_wifi(profile);
  start_mqtt(profile);

  xTaskCreatePinnedToCore(maintenance_task, "maint", 8192, nullptr, 3, nullptr, 0);

  // Surveillance du niveau reçu : sous le seuil du profil, la couverture n'est
  // plus tenable en mouvement et il faut le savoir avant de perdre des appels.
  for (;;) {
    if (g_wifi_up) {
      const int16_t rssi = wifi_rssi();
      if (rssi != 0 && rssi < profile.wifi.rssi_warn_dbm) {
        ESP_LOGW(TAG, "niveau Wi-Fi faible : %d dBm (seuil %d)", rssi,
                 profile.wifi.rssi_warn_dbm);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
