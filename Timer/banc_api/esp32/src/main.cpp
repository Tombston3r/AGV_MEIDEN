// Banc d'essai de l'API planning, SUR LA CARTE (ESP32 de l'AGV Control V5.0.1).
//
// Le banc local tourne sur 127.0.0.1 : il valide le moteur et les routes, pas
// le fait de tenir un réseau, une pile IP embarquée et plusieurs clients avec
// 320 Ko de RAM. C'est ce que ce point d'entrée ajoute, et rien d'autre : le
// serveur lui-même est le MÊME fichier `banc_api/serveur.cpp`, pas une copie.
//
// Ce que ce banc NE fait PAS, et qu'il ne faut pas lui prêter :
//   - il ne commande pas l'AGV : aucune trame n'est émise vers la MEGA. Une
//     mission due est journalisée et servie par /api/missions, point.
//   - il n'a pas d'heure : ni RTC, ni NTP sur un point d'accès sans internet.
//     Le moteur démarre GELÉ et n'exécute rien tant qu'un opérateur n'a pas
//     posé l'heure par POST /api/sim/heure (voir Banc::heure_fiable).
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <lwip/inet.h>
#include <nvs_flash.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

// Défini par banc_api/serveur.cpp : corps commun hôte/ESP32.
int banc_api_executer(int port, const std::string& dossier_web, uint32_t adresse_bind);

namespace {

constexpr char kSsid[] = "agv-atelier";
// WPA2 plutôt qu'ouvert : ce banc expose des routes qui posent l'heure et
// déclenchent des appels. Un banc n'est pas une raison de laisser n'importe
// quel terminal de l'atelier écrire dedans. À changer avant tout usage réel.
constexpr char kMotDePasse[] = "agv-atelier-2026";
constexpr int kPort = 80;
constexpr char kTag[] = "banc-api";

void demarrer_point_acces() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init));

  wifi_config_t cfg{};
  std::memcpy(cfg.ap.ssid, kSsid, sizeof(kSsid));
  cfg.ap.ssid_len = sizeof(kSsid) - 1;
  std::memcpy(cfg.ap.password, kMotDePasse, sizeof(kMotDePasse));
  cfg.ap.channel = 1;
  cfg.ap.max_connection = 4;
  cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(kTag, "point d'acces \"%s\" ouvert, banc sur http://192.168.4.1/", kSsid);
}

// Le serveur vit dans sa propre tâche : la pile de 3,5 Ko d'app_main ne suffit
// pas aux std::string du parseur de requêtes.
void tache_serveur(void*) {
  banc_api_executer(kPort, "", INADDR_ANY);
  vTaskDelete(nullptr);
}

}  // namespace

extern "C" void app_main() {
  esp_err_t nvs = nvs_flash_init();
  if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs);

  // Même fuseau que la cible (spec §2.4) : transitions été/hiver comprises.
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  demarrer_point_acces();
  xTaskCreate(tache_serveur, "banc_api", 16384, nullptr, 5, nullptr);
}
