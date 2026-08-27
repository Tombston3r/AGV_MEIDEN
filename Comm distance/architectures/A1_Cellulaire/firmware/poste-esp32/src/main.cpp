// Poste fixe ESP32 : architecture SMS + EnOcean (brief §9.1).
//
// Chaîne : bouton PTM 210 (sans pile) -> TCM 515 sur UART1 (ESP3) -> traduction
// en trame applicative -> modem cellulaire -> AGV.
//
// Ethernet FILAIRE via W5500 pour la supervision : choix délibéré, AUCUNE
// émission 2,4 GHz permanente. Le Wi-Fi n'est jamais démarré sur ce firmware.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_eth.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <mdns.h>

#include "app/alert_gateway.h"
#include "app/poste_app.h"
#include "config/hardware_profile.h"
#include "enocean/pairing_table.h"
#include "platform/esp32/esp_ports.h"
#include "platform/esp32/nvs_store.h"
#include "transport/mqtt_lte_transport.h"
#include "transport/sms_transport.h"
#include "web_server.h"

namespace {

const char* TAG = "poste";

// Brochage du poste : PROVISOIRE §12.2, dépend de la carte retenue.
constexpr gpio_num_t kEnoceanRx = GPIO_NUM_16;  // TCM 515 -> ESP32
constexpr gpio_num_t kEnoceanTx = GPIO_NUM_17;  // inutilisé avec un TCM 515
constexpr gpio_num_t kSpiSclk = GPIO_NUM_18;   // W5500 (Ethernet filaire)
constexpr gpio_num_t kSpiMosi = GPIO_NUM_23;
constexpr gpio_num_t kSpiMiso = GPIO_NUM_19;
constexpr gpio_num_t kEthCs = GPIO_NUM_5;
constexpr gpio_num_t kModemTx = GPIO_NUM_4;    // modem cellulaire sur UART2
constexpr gpio_num_t kModemRx = GPIO_NUM_2;
constexpr gpio_num_t kModemPwrkey = GPIO_NUM_13;
constexpr gpio_num_t kWatchdogDone = GPIO_NUM_15;  // TPL5010
constexpr gpio_num_t kPairButton = GPIO_NUM_0;
constexpr uint16_t kPairDefaultStation = 1;

agv::esp32::EspClock g_clock;
agv::esp32::EspSpi g_spi;
agv::esp32::EspGpio g_gpio;
agv::esp32::EspUart g_enocean_uart;
agv::esp32::EspUart g_modem_uart;
agv::esp32::EspModemPower g_modem_power(kModemPwrkey, kWatchdogDone);
agv::esp32::NvsStore g_nvs;
agv::RamStore g_fallback;

agv::PosteApp* g_app = nullptr;
agv::poste::WebServer* g_web = nullptr;

// Ethernet filaire W5500 (SPI). Alternative WT32-ETH01 : remplacer ce bloc par
// l'initialisation esp_eth_mac_new_esp32 : le reste du firmware est identique.
bool start_ethernet(const agv::HardwareProfile&) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t* netif = esp_netif_new(&netif_cfg);

  eth_w5500_config_t w5500_cfg = ETH_W5500_DEFAULT_CONFIG(SPI2_HOST, nullptr);
  eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
  phy_cfg.autonego_timeout_ms = 0;

  esp_eth_mac_t* mac = esp_eth_mac_new_w5500(&w5500_cfg, &mac_cfg);
  esp_eth_phy_t* phy = esp_eth_phy_new_w5500(&phy_cfg);
  esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t handle = nullptr;
  if (esp_eth_driver_install(&eth_cfg, &handle) != ESP_OK) return false;
  if (esp_netif_attach(netif, esp_eth_new_netif_glue(handle)) != ESP_OK) return false;
  return esp_eth_start(handle) == ESP_OK;
}

// --- Tâche EnOcean : lecture UART1 et injection octet par octet -------------
void enocean_task(void*) {
  uint8_t buf[64];
  for (;;) {
    const size_t n = g_enocean_uart.read(buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) {
      if (g_app->feed_enocean(buf[i])) {
        ESP_LOGI(TAG, "appui EnOcean traité");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// --- Tâche LIAISON : pile modem + diffusion WebSocket -----------------------
void link_task(void*) {
  uint32_t last_station = 0xFFFF;
  bool last_moving = false;
  for (;;) {
    g_app->tick();
    // Diffusion WebSocket sur CHANGEMENT, pas en boucle : le WebSocket sert
    // justement à supprimer le polling.
    const auto& s = g_app->snapshot();
    if (s.station != last_station || s.moving != last_moving) {
      last_station = s.station;
      last_moving = s.moving;
      g_web->broadcast_state();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// --- Tâche IHM locale : bouton d'appairage ---------------------------------
void pairing_button_task(void*) {
  g_gpio.configure_input(kPairButton, true);
  bool previous = true;
  for (;;) {
    const bool level = g_gpio.get(kPairButton);
    if (previous && !level) {
      // Appui long non requis : le mode appairage expire tout seul.
      g_app->start_pairing(kPairDefaultStation, 4);
      ESP_LOGW(TAG, "mode appairage ouvert : appuyez sur le bouton à associer");
    }
    previous = level;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

}  // namespace

extern "C" void app_main() {
  const agv::HardwareProfile& profile = agv::default_profile();
  ESP_LOGI(TAG, "poste fixe, profil=%s", profile.name);

  agv::IPersistentStore* store = &g_fallback;
  if (g_nvs.begin("poste")) store = &g_nvs;

  g_enocean_uart.begin(UART_NUM_1, kEnoceanTx, kEnoceanRx, 57600);  // ESP3 : 57 600 bauds
  g_spi.begin(SPI2_HOST, kSpiSclk, kSpiMosi, kSpiMiso, kEthCs, CFG_BUS_SPI_FREQ_HZ);

  g_modem_power.begin();
  g_modem_uart.begin(UART_NUM_2, kModemTx, kModemRx, 115200);
#ifdef TRANSPORT_SMS
  static agv::SmsTransport cellular(profile, g_modem_uart, g_modem_power);
  ESP_LOGW(TAG, "transport SMS : latence NON bornée, ordre NON garanti (Archi_2 §3.2)");
#else
  static agv::MqttLteTransport cellular(profile, g_modem_uart, g_modem_power);
#endif

  uint8_t key[agv::kAesKeySize] = {};
  if (store->read("aes_key", key, sizeof(key)) == sizeof(key)) {
    cellular.channel().set_key(key);
  } else {
    ESP_LOGW(TAG, "clé AES absente : liaison NON chiffrée");
    cellular.channel().set_enabled(false);
  }

  static agv::PairingTable pairings(store);
  static agv::PosteApp app(profile, g_clock, cellular, pairings);
  g_app = &app;

  static agv::poste::WebServer web(app, profile);
  g_web = &web;

  if (!app.begin()) {
    ESP_LOGE(TAG, "initialisation du transport refusée");
  }
  if (!start_ethernet(profile)) {
    ESP_LOGE(TAG, "Ethernet indisponible : supervision inaccessible, commande inchangée");
  }
  mdns_init();
  mdns_hostname_set("agv");  // http://agv.local
  mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
  web.begin();

  if (profile.enocean.rx_only) {
    // §12.8 : à signaler à l'exploitant, l'IHM ne doit rien promettre côté
    // bouton EnOcean tant qu'un TCM 310 n'est pas retenu.
    ESP_LOGW(TAG, "TCM 515 (Rx seul) : aucun retour d'accusé possible vers le bouton");
  }

  xTaskCreatePinnedToCore(enocean_task, "enocean", 4096, nullptr, 6, nullptr, 1);
  xTaskCreatePinnedToCore(link_task, "link", 8192, nullptr, 5, nullptr, 0);
  xTaskCreatePinnedToCore(pairing_button_task, "pair", 3072, nullptr, 3, nullptr, 0);
}
