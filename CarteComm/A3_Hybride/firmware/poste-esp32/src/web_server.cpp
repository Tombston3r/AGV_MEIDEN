#include "web_server.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <esp_littlefs.h>
#include <esp_log.h>

namespace agv::poste {
namespace {

const char* TAG = "web";
WebServer* g_instance = nullptr;

// Extraction d'un entier d'un corps JSON plat. Volontairement minimal : le
// poste ne reçoit que trois champs numériques, embarquer un analyseur JSON
// complet coûterait plus qu'il ne rapporte.
bool json_int(const char* body, const char* key, long& out) {
  const char* p = std::strstr(body, key);
  if (p == nullptr) return false;
  p = std::strchr(p, ':');
  if (p == nullptr) return false;
  out = std::strtol(p + 1, nullptr, 10);
  return true;
}

bool json_true(const char* body, const char* key) {
  const char* p = std::strstr(body, key);
  if (p == nullptr) return false;
  p = std::strchr(p, ':');
  return p != nullptr && std::strstr(p, "true") != nullptr;
}

size_t read_body(httpd_req_t* req, char* buf, size_t capacity) {
  const size_t len = (req->content_len < capacity - 1) ? req->content_len : capacity - 1;
  int received = httpd_req_recv(req, buf, len);
  if (received <= 0) return 0;
  buf[received] = '\0';
  return static_cast<size_t>(received);
}

}  // namespace

size_t WebServer::render_state_json(char* out, size_t capacity) const {
  const AgvSnapshot& s = app_.snapshot();
  const PosteStats& st = app_.stats();
  const LinkHealth health = app_.transport_health();
  const uint32_t age = app_.telemetry_age_ms();

  const int n = std::snprintf(
      out, capacity,
      "{\"station\":%u,\"speed\":%u,\"moving\":%s,\"in_station\":%s,\"fault\":%s,"
      "\"valid\":%s,\"telemetry_age_ms\":%u,\"rssi_dbm\":%d,\"duty_permille\":%u,"
      "\"tx_refused_duty\":%u,\"commands_sent\":%u,\"commands_refused\":%u,"
      "\"acks\":%u,\"nacks\":%u,\"enocean\":%u,\"unpaired\":%u,"
      "\"pairing_active\":%s,\"operator_feedback\":%s,\"profile\":\"%s\"}",
      s.station, s.speed, s.moving ? "true" : "false", s.in_station ? "true" : "false",
      s.fault ? "true" : "false", s.valid ? "true" : "false",
      (age == UINT32_MAX) ? 0u : age, health.rssi_dbm, health.duty_used_permille,
      health.tx_refused_duty, st.commands_sent, st.commands_refused, st.acks_received,
      st.nacks_received, st.enocean_telegrams, st.enocean_unpaired,
      app_.pairing_active() ? "true" : "false",
      app_.operator_feedback_available() ? "true" : "false", profile_.name);
  return (n < 0) ? 0 : static_cast<size_t>(n);
}

esp_err_t WebServer::on_root(httpd_req_t* req) {
  FILE* f = std::fopen("/littlefs/index.html", "r");
  if (f == nullptr) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "index.html absent de LittleFS");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  char chunk[512];
  size_t n = 0;
  while ((n = std::fread(chunk, 1, sizeof(chunk), f)) > 0) {
    httpd_resp_send_chunk(req, chunk, n);
  }
  std::fclose(f);
  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
}

esp_err_t WebServer::on_dump(httpd_req_t* req) {
  // Format historique : c'est la procédure d'atelier du client (§3.3).
  static char buffer[4096];
  const size_t len = g_instance->app_.render_agvdump(buffer, sizeof(buffer));
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_send(req, buffer, len);
}

esp_err_t WebServer::on_state(httpd_req_t* req) {
  char json[768];
  const size_t len = g_instance->render_state_json(json, sizeof(json));
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, len);
}

esp_err_t WebServer::on_goto(httpd_req_t* req) {
  char body[128];
  if (read_body(req, body, sizeof(body)) == 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "corps vide");
    return ESP_FAIL;
  }
  long station = -1;
  long speed = 0;
  if (!json_int(body, "\"station\"", station) || station < 0 || station > kStationMax) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "station hors bornes");
    return ESP_FAIL;
  }
  json_int(body, "\"speed\"", speed);
  const bool ok = g_instance->app_.request_goto(static_cast<uint16_t>(station),
                                                static_cast<uint8_t>(speed));
  httpd_resp_set_type(req, "application/json");
  // Un refus (budget de rapport cyclique, transport occupé) est remonté tel
  // quel à l'IHM : l'opérateur doit savoir que sa commande n'est PAS partie.
  return httpd_resp_send(req, ok ? "{\"ok\":true}" : "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::on_stop(httpd_req_t* req) {
  char body[128];
  read_body(req, body, sizeof(body));
  const bool ok = g_instance->app_.request_stop(json_true(body, "\"purge\""));
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, ok ? "{\"ok\":true}" : "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::on_pair(httpd_req_t* req) {
  char body[128];
  read_body(req, body, sizeof(body));
  long station = -1;
  long speed = 0;
  if (!json_int(body, "\"station\"", station) || station < 0 || station > kStationMax) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "station hors bornes");
    return ESP_FAIL;
  }
  json_int(body, "\"speed\"", speed);
  g_instance->app_.start_pairing(static_cast<uint16_t>(station), static_cast<uint8_t>(speed));
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, "{\"ok\":true,\"message\":\"appuyez sur le bouton à associer\"}",
                         HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::on_ws(httpd_req_t* req) {
  if (req->method == HTTP_GET) return ESP_OK;  // poignée de main
  httpd_ws_frame_t frame = {};
  frame.type = HTTPD_WS_TYPE_TEXT;
  return httpd_ws_recv_frame(req, &frame, 0);
}

bool WebServer::begin(uint16_t port) {
  g_instance = this;

  esp_vfs_littlefs_conf_t fs = {};
  fs.base_path = "/littlefs";
  fs.partition_label = "littlefs";
  fs.format_if_mount_failed = false;
  if (esp_vfs_littlefs_register(&fs) != ESP_OK) {
    ESP_LOGW(TAG, "LittleFS non monté : la page de supervision sera absente");
  }

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = port;
  cfg.max_uri_handlers = 12;
  if (httpd_start(&server_, &cfg) != ESP_OK) return false;

  const httpd_uri_t routes[] = {
      {"/", HTTP_GET, on_root, nullptr, false, false, nullptr},
      {"/agvdump", HTTP_GET, on_dump, nullptr, false, false, nullptr},
      {"/api/state", HTTP_GET, on_state, nullptr, false, false, nullptr},
      {"/api/goto", HTTP_POST, on_goto, nullptr, false, false, nullptr},
      {"/api/stop", HTTP_POST, on_stop, nullptr, false, false, nullptr},
      {"/api/pair", HTTP_POST, on_pair, nullptr, false, false, nullptr},
      {"/ws", HTTP_GET, on_ws, nullptr, true, false, nullptr},
  };
  for (const auto& r : routes) httpd_register_uri_handler(server_, &r);
  return true;
}

void WebServer::stop() {
  if (server_ != nullptr) {
    httpd_stop(server_);
    server_ = nullptr;
  }
}

void WebServer::broadcast_state() {
  if (server_ == nullptr) return;
  char json[768];
  const size_t len = render_state_json(json, sizeof(json));

  // Diffusion à tous les descripteurs WebSocket ouverts.
  size_t clients = CONFIG_LWIP_MAX_LISTENING_TCP;
  int fds[CONFIG_LWIP_MAX_LISTENING_TCP];
  if (httpd_get_client_list(server_, &clients, fds) != ESP_OK) return;
  for (size_t i = 0; i < clients; ++i) {
    if (httpd_ws_get_fd_info(server_, fds[i]) != HTTPD_WS_CLIENT_WEBSOCKET) continue;
    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = reinterpret_cast<uint8_t*>(json);
    frame.len = len;
    httpd_ws_send_frame_async(server_, fds[i], &frame);
  }
}

}  // namespace agv::poste
