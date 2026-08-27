#include "maintenance_ap.h"

#include <cstring>

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

namespace agv::maintenance {
namespace {

const char* TAG = "maint";
DumpSource g_dump = nullptr;

esp_err_t handle_agvdump(httpd_req_t* req) {
  static char buffer[4096];
  size_t len = 0;
  if (g_dump != nullptr) len = g_dump(buffer, sizeof(buffer));
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_send(req, buffer, len);
}

esp_err_t handle_root(httpd_req_t* req) {
  static const char* kPage =
      "<!doctype html><meta charset=utf-8><title>AGV maintenance</title>"
      "<h1>AGV : maintenance</h1>"
      "<p>Fenêtre de maintenance ouverte. Le Wi-Fi se refermera automatiquement.</p>"
      "<p><a href=\"/agvdump\">/agvdump</a></p>";
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, kPage, HTTPD_RESP_USE_STRLEN);
}

}  // namespace

bool AccessPoint::start_wifi() {
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  if (esp_wifi_init(&init) != ESP_OK) return false;

  wifi_config_t cfg = {};
  std::strncpy(reinterpret_cast<char*>(cfg.ap.ssid), profile_.maintenance.wifi_ssid,
               sizeof(cfg.ap.ssid) - 1);
  cfg.ap.ssid_len = static_cast<uint8_t>(std::strlen(profile_.maintenance.wifi_ssid));
  cfg.ap.max_connection = 2;
  cfg.ap.authmode = WIFI_AUTH_OPEN;  // réseau de maintenance, portée réduite

  if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) return false;
  if (esp_wifi_set_config(WIFI_IF_AP, &cfg) != ESP_OK) return false;
  if (esp_wifi_start() != ESP_OK) return false;

  httpd_config_t http = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &http) != ESP_OK) return false;

  g_dump = dump_;
  httpd_uri_t dump_uri = {"/agvdump", HTTP_GET, handle_agvdump, nullptr};
  httpd_uri_t root_uri = {"/", HTTP_GET, handle_root, nullptr};
  httpd_register_uri_handler(server, &dump_uri);
  httpd_register_uri_handler(server, &root_uri);
  server_ = server;
  return true;
}

void AccessPoint::stop_wifi() {
  if (server_ != nullptr) {
    httpd_stop(static_cast<httpd_handle_t>(server_));
    server_ = nullptr;
  }
  esp_wifi_stop();
  esp_wifi_deinit();
}

void AccessPoint::request_open() {
  if (open_) {
    opened_at_ms_ = clock_.now_ms();  // re-arme la fenêtre
    return;
  }
  if (!start_wifi()) {
    ESP_LOGE(TAG, "ouverture du point d'accès refusée");
    return;
  }
  open_ = true;
  opened_at_ms_ = clock_.now_ms();
  ESP_LOGW(TAG, "Wi-Fi de maintenance OUVERT pour %u s", profile_.maintenance.wifi_window_s);
}

void AccessPoint::close() {
  if (!open_) return;
  stop_wifi();
  open_ = false;
  ESP_LOGI(TAG, "Wi-Fi de maintenance refermé");
}

uint32_t AccessPoint::remaining_s() const {
  if (!open_) return 0;
  const uint32_t elapsed_s = (clock_.now_ms() - opened_at_ms_) / 1000u;
  const uint32_t window = profile_.maintenance.wifi_window_s;
  return (elapsed_s >= window) ? 0u : (window - elapsed_s);
}

void AccessPoint::tick() {
  // Extinction automatique : la fenêtre ne dépend d'aucune action opérateur,
  // sinon le Wi-Fi finit par rester ouvert en permanence.
  if (open_ && remaining_s() == 0) close();
}

}  // namespace agv::maintenance
