// Firmware ESP32 de la carte AIO AGV Control V6.0 : architecture LoRa.
//
// La V6.0 est la V5.0.1 augmentée d'un RFM95W-868S2 câblé sur le SPI libre de
// l'ESP32. Ce firmware pilote cette radio et rien d'autre.
//
// Responsabilités :
//   - radio LoRa 868 MHz : réception des ordres, émission des accusés ;
//   - budget de rapport cyclique 1 %/1 h, appliqué par le transport ;
//   - heartbeat vers l'ATmega : c'est ce qui autorise l'AGV à rouler ;
//   - point d'accès de maintenance à la demande, servant `/agvdump`.
//
// Il n'y a NI Wi-Fi client, NI MQTT, NI broker : l'AGV et ses boutons se
// parlent directement. C'est la différence avec l'architecture A3.
//
// L'ESP32 NE TOUCHE JAMAIS AU BUS MEIDEN. Toute la commande passe par
// l'ATmega, qui reste maître du repli de sécurité.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <nvs_flash.h>

#include "app/agvdump.h"
#include "app/lora_gateway_app.h"
#include "board_pins.h"
#include "config/hardware_profile.h"
#include "config/lora_config.h"
#include "maintenance_ap.h"
#include "platform/esp32/esp_ports.h"
#include "platform/esp32/sx1276_radio.h"
#include "transport/lora_transport.h"

namespace {

const char* TAG = "agv-esp32-lora";

agv::esp32::EspClock g_clock;
agv::esp32::EspUart g_link_uart;
agv::esp32::EspSpi g_spi;
agv::esp32::EspGpio g_gpio;
agv::LoraGatewayApp* g_gateway = nullptr;
agv::LoraTransport* g_radio = nullptr;

class UartLinkPort final : public agv::ILinkPort {
 public:
  void write(const uint8_t* data, size_t len) override { g_link_uart.write(data, len); }
};

UartLinkPort g_link_port;

// --- Tâches ----------------------------------------------------------------

// Liaison série vers l'ATmega, écoute radio, heartbeat.
void link_task(void*) {
  uint8_t buf[64];
  for (;;) {
    const size_t n = g_link_uart.read(buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) g_gateway->on_link_byte(buf[i]);
    g_gateway->tick();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Point d'accès de maintenance, ouvert sur contact ILS.
void maintenance_task(void*) {
  const agv::HardwareProfile& profile = agv::default_profile();
  agv::maintenance::AccessPoint ap(profile, g_clock);
  ap.set_dump_source([](char* out, size_t cap) {
    const agv::LoraGatewayStats& st = g_gateway->stats();
    const agv::LinkHealth h = g_radio->health();
    agv::AgvDumpInput in;
    in.state = &g_gateway->state();
    in.profile_name = agv::default_profile().name;
    in.uptime_s = g_clock.now_ms() / 1000u;
    in.link_up = g_gateway->link_up();
    // Le format `/agvdump` est celui de l'atelier (§3.3) : on garde les mêmes
    // champs, en y plaçant les grandeurs radio équivalentes.
    in.wifi_up = h.up;
    in.mqtt_up = h.up;
    in.rssi_dbm = h.rssi_dbm;
    in.ssid = "lora-868";
    in.heartbeats_sent = st.heartbeats_sent;
    in.link_timeouts = st.link_timeouts;
    in.cmd_expired = st.acks_refused_duty;
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
  const agv::LoraConfig lora = agv::lora_config_from_profile();
  ESP_LOGI(TAG, "ESP32 carte V6.0 : LoRa %.1f MHz SF%u, profil=%s",
           lora.frequency_hz / 1e6, lora.spreading_factor, profile.name);

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  g_link_uart.begin(UART_NUM_1, agv::board::kLinkTx, agv::board::kLinkRx,
                    static_cast<int>(profile.link.baud));

  if (!g_spi.begin(agv::board::kLoraSpiHost, agv::board::kLoraSck, agv::board::kLoraMosi,
                   agv::board::kLoraMiso, agv::board::kLoraNss, agv::board::kLoraSpiHz)) {
    ESP_LOGE(TAG, "bus SPI non initialise : la radio restera muette");
  }

  static agv::esp32::Sx1276Radio radio(g_spi, g_gpio, g_clock, agv::board::kLoraReset,
                                       agv::board::kLoraDio0);
  static agv::LoraTransport transport(profile, lora, radio);
  g_radio = &transport;

  static agv::LoraGatewayApp gateway(profile, g_clock, transport, g_link_port);
  g_gateway = &gateway;
  gateway.begin();

  if (radio.version() != 0x12) {
    // On le dit, et on continue : le heartbeat doit partir quoi qu'il arrive,
    // sinon l'AGV reste en repli pour une panne de radio.
    ESP_LOGE(TAG, "RFM95W muet (RegVersion 0x%02X, attendu 0x12)", radio.version());
  }

  // La liaison série démarre AVANT tout le reste : le heartbeat vers l'ATmega
  // ne doit dépendre d'aucune radio.
  xTaskCreatePinnedToCore(link_task, "link", 6144, nullptr, 6, nullptr, 1);
  xTaskCreatePinnedToCore(maintenance_task, "maint", 8192, nullptr, 3, nullptr, 0);

  for (;;) {
    const agv::LinkHealth h = transport.health();
    if (h.duty_used_permille > 800) {
      ESP_LOGW(TAG, "budget de rapport cyclique a %u/1000 : emissions bientot refusees",
               static_cast<unsigned>(h.duty_used_permille));
    }
    if (h.up && h.rssi_dbm < -115) {
      ESP_LOGW(TAG, "niveau LoRa faible : %d dBm", static_cast<int>(h.rssi_dbm));
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
