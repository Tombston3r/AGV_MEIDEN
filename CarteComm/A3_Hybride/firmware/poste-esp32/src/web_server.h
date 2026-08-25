// Serveur de supervision du poste fixe (brief §9.1 et §9.3).
//
// ÉCART ASSUMÉ À SIGNALER : le brief cite `ESPAsyncWebServer`, qui impose le
// cœur Arduino. On utilise ici `esp_http_server` d'ESP-IDF, qui gère nativement
// les WebSockets depuis l'IDF 4.2. Motif : garder UN seul framework (ESP-IDF,
// comme demandé au §2) et ne pas faire entrer le cœur Arduino dans un firmware
// qui partage son cœur 0 avec la pile radio. Le basculement inverse ne touche
// que ce fichier.
//
// Routes :
//   GET  /            page de supervision (LittleFS)
//   GET  /agvdump     format historique, compatible atelier (§3.3)
//   GET  /api/state   instantané JSON
//   POST /api/goto    { "station": n, "speed": n }
//   POST /api/stop    { "purge": bool }
//   POST /api/pair    { "station": n, "speed": n } -> ouvre le mode appairage
//   WS   /ws          diffusion temps réel de l'état
#pragma once

#ifndef ESP_PLATFORM
#error "web_server ne se compile que pour la cible ESP32."
#endif

#include <esp_http_server.h>

#include "app/poste_app.h"

namespace agv::poste {

class WebServer {
 public:
  WebServer(PosteApp& app, const HardwareProfile& profile) : app_(app), profile_(profile) {}

  bool begin(uint16_t port = 80);
  void stop();

  // Diffuse l'état courant à tous les clients WebSocket. À appeler quand la
  // télémétrie change, pas en boucle : le WebSocket sert à éviter le polling.
  void broadcast_state();

  // Rendu JSON de l'état, partagé entre /api/state et le WebSocket.
  size_t render_state_json(char* out, size_t capacity) const;

 private:
  static esp_err_t on_root(httpd_req_t* req);
  static esp_err_t on_dump(httpd_req_t* req);
  static esp_err_t on_state(httpd_req_t* req);
  static esp_err_t on_goto(httpd_req_t* req);
  static esp_err_t on_stop(httpd_req_t* req);
  static esp_err_t on_pair(httpd_req_t* req);
  static esp_err_t on_ws(httpd_req_t* req);

  PosteApp& app_;
  const HardwareProfile& profile_;
  httpd_handle_t server_ = nullptr;
};

}  // namespace agv::poste
