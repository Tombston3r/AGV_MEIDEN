// Firmware embarqué sur l'AGV — cœur du projet.
//
// Répartition FreeRTOS imposée par le §4.4 :
//   cœur 1 : tâche BUS. Séquenceur trois phases et pose du bus MEIDEN. La
//            section adresse+vitesse+strobe s'exécute en section critique.
//   cœur 0 : tâche LIAISON. Pile radio/modem, télémétrie, serveur de
//            maintenance. Rien de ce qui vit ici ne doit pouvoir retarder la
//            pose du bus.
//
// ⚠ CE FIRMWARE N'EST PAS UN ORGANE DE SÉCURITÉ (brief §3.1). L'arrêt
// d'urgence, les bumpers et le scrutateur laser restent dans une chaîne
// indépendante conforme à l'ISO 3691-4. Aucune ligne de ce fichier ne doit
// prétendre assurer une fonction de sécurité.
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "app/agv_app.h"
#include "app/course_queue.h"
#include "app/sequencer.h"
#include "board_pins.h"
#include "bus/mcp23017_bus.h"
#include "bus/mega_uart_bus.h"
#include "bus/shift_bus.h"
#include "config/hardware_profile.h"
#include "maintenance_ap.h"
#include "platform/esp32/esp_ports.h"
#include "platform/esp32/nvs_store.h"
#include "platform/esp32/sx1276_radio.h"
#include "transport/lora_transport.h"

namespace {

const char* TAG = "agv";

agv::esp32::EspClock g_clock;
agv::esp32::EspI2c g_i2c;
agv::esp32::EspSpi g_spi;
agv::esp32::EspGpio g_gpio;
agv::esp32::NvsStore g_nvs;
agv::RamStore g_fallback_store;

// Signalisation locale : LED FAULT et LED liaison.
class BoardIndicators final : public agv::IIndicators {
 public:
  void begin() {
    g_gpio.configure_output(agv::board::kLedFault);
    g_gpio.configure_output(agv::board::kLedLink);
  }
  void set_fault(bool on) override { g_gpio.set(agv::board::kLedFault, on); }
  void set_link(bool on) override { g_gpio.set(agv::board::kLedLink, on); }
};

BoardIndicators g_indicators;

// Sélection de la variante d'interface bus (§12.10) : NON TRANCHÉE. Le choix
// vient du profil, pas d'un #ifdef enfoui dans le code métier.
agv::IBusDriver* make_bus_driver(const agv::HardwareProfile& profile) {
  static agv::esp32::EspUart mega_uart;
  switch (profile.bus.variant) {
    case agv::DriverVariant::Mcp23017: {
      static agv::Mcp23017Bus driver(profile, g_i2c, g_clock);
      return &driver;
    }
    case agv::DriverVariant::Shift595: {
      static agv::ShiftPins pins{agv::board::kShiftRclk, agv::board::kShiftPl,
                                 agv::board::kShiftOe};
      static agv::ShiftBus driver(profile, g_spi, g_gpio, g_clock, pins);
      return &driver;
    }
    case agv::DriverVariant::MegaUart: {
      mega_uart.begin(UART_NUM_2, agv::board::kMegaTx, agv::board::kMegaRx, 500000);
      static agv::MegaUartBus driver(profile, mega_uart, g_clock);
      return &driver;
    }
    case agv::DriverVariant::Sim:
    default:
      // Le profil « sim » n'a pas de sens sur cible : c'est une erreur de
      // configuration, signalée plutôt que contournée silencieusement.
      ESP_LOGE(TAG, "profil 'sim' flashé sur cible réelle — vérifier profiles/*.yaml");
      return nullptr;
  }
}

struct Runtime {
  const agv::HardwareProfile* profile = nullptr;
  agv::IBusDriver* bus = nullptr;
  agv::CourseQueue* queue = nullptr;
  agv::Sequencer* sequencer = nullptr;
  agv::ITransport* transport = nullptr;
  agv::AgvApp* app = nullptr;
  SemaphoreHandle_t lock = nullptr;
};

Runtime g_rt;

// --- Tâche BUS (cœur 1) -----------------------------------------------------
void bus_task(void*) {
  const TickType_t period = pdMS_TO_TICKS(2);  // 500 Hz : très au-dessus des
                                               // constantes de temps du bus
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    xSemaphoreTake(g_rt.lock, portMAX_DELAY);
    g_rt.sequencer->tick();
    xSemaphoreGive(g_rt.lock);
    vTaskDelayUntil(&last, period);
  }
}

// --- Tâche LIAISON (cœur 0) -------------------------------------------------
void link_task(void*) {
  for (;;) {
    xSemaphoreTake(g_rt.lock, portMAX_DELAY);
    g_rt.app->tick();
    xSemaphoreGive(g_rt.lock);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// --- Tâche MAINTENANCE (cœur 0) ---------------------------------------------
// Wi-Fi désactivé par défaut, ouvert 10 min sur contact ILS (§9.4).
void maintenance_task(void*) {
  agv::maintenance::AccessPoint ap(*g_rt.profile, g_clock);
  ap.set_dump_source([](char* out, size_t cap) { return g_rt.app->render_agvdump(out, cap); });
  g_gpio.configure_input(agv::board::kMaintenanceReed, true);
  for (;;) {
    // Contact ILS actif bas (aimant présent).
    if (!g_gpio.get(agv::board::kMaintenanceReed)) ap.request_open();
    ap.tick();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

}  // namespace

extern "C" void app_main() {
  const agv::HardwareProfile& profile = agv::default_profile();
  g_rt.profile = &profile;
  ESP_LOGI(TAG, "profil=%s variante_bus=%d", profile.name, static_cast<int>(profile.bus.variant));

  g_indicators.begin();
  g_indicators.set_fault(true);  // FAULT allumée tant que l'init n'est pas finie

  agv::IPersistentStore* store = &g_fallback_store;
  if (g_nvs.begin()) {
    static agv::esp32::NvsStore& nvs = g_nvs;
    store = &nvs;
  } else {
    ESP_LOGE(TAG, "NVS indisponible : file de courses non persistée");
  }

  g_i2c.begin(I2C_NUM_0, agv::board::kI2cSda, agv::board::kI2cScl, CFG_BUS_I2C_FREQ_HZ);
  g_spi.begin(SPI2_HOST, agv::board::kSpiSclk, agv::board::kSpiMosi, agv::board::kSpiMiso,
              agv::board::kLoraCs, CFG_BUS_SPI_FREQ_HZ);

  static agv::CourseQueue queue(profile.queue, store);
  g_rt.queue = &queue;

  g_rt.bus = make_bus_driver(profile);
  if (g_rt.bus == nullptr) {
    ESP_LOGE(TAG, "aucun driver de bus : arrêt en état sûr");
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
  }

  static agv::Sequencer sequencer(profile, *g_rt.bus, queue);
  g_rt.sequencer = &sequencer;

  static agv::esp32::Sx1276Radio radio(g_spi, g_gpio, g_clock, agv::board::kLoraReset,
                                       agv::board::kLoraDio0);
  static agv::LoraTransport lora(profile, radio);
  g_rt.transport = &lora;

  // Clé AES partagée, lue en NVS. Sans clé, le canal reste en clair : c'est
  // signalé bruyamment, jamais silencieux.
  uint8_t key[agv::kAesKeySize] = {};
  if (store->read("aes_key", key, sizeof(key)) == sizeof(key)) {
    lora.channel().set_key(key);
  } else {
    ESP_LOGW(TAG, "clé AES absente en NVS — liaison NON chiffrée");
    lora.channel().set_enabled(false);
  }
  uint8_t nonce_blob[4] = {};
  if (store->read("aes_nonce", nonce_blob, sizeof(nonce_blob)) == sizeof(nonce_blob)) {
    lora.channel().set_nonce_counter((static_cast<uint32_t>(nonce_blob[0]) << 24) |
                                     (static_cast<uint32_t>(nonce_blob[1]) << 16) |
                                     (static_cast<uint32_t>(nonce_blob[2]) << 8) |
                                     nonce_blob[3]);
  }

  static agv::AgvApp app(profile, g_clock, lora, sequencer, queue, &g_indicators);
  g_rt.app = &app;

  g_rt.lock = xSemaphoreCreateMutex();
  if (!app.begin()) {
    ESP_LOGE(TAG, "initialisation refusée : maintien en état sûr");
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGI(TAG, "file restaurée : %u course(s), %u écartée(s)",
           static_cast<unsigned>(app.restored_courses()),
           static_cast<unsigned>(app.dropped_courses()));
  g_indicators.set_fault(false);

  // Épinglage : bus sur le cœur 1, pile radio sur le cœur 0 (§4.4).
  xTaskCreatePinnedToCore(bus_task, "bus", 4096, nullptr, configMAX_PRIORITIES - 2, nullptr, 1);
  xTaskCreatePinnedToCore(link_task, "link", 8192, nullptr, 5, nullptr, 0);
  xTaskCreatePinnedToCore(maintenance_task, "maint", 8192, nullptr, 3, nullptr, 0);
}
