// Wi-Fi de maintenance de l'AGV (brief §9.4).
//
// Le Wi-Fi est DÉSACTIVÉ PAR DÉFAUT : c'est tout l'intérêt du remplacement de
// la V5.0.1 : ne plus polluer le 2,4 GHz saturé du site en permanence.
// Il s'ouvre sur contact ILS (aimant) ou bouton, et se referme automatiquement
// après `wifi_window_s` (600 s par défaut). Pendant la fenêtre, l'ESP32 sert le
// point d'accès et la page /agvdump au format historique.
#pragma once

#ifndef ESP_PLATFORM
#error "maintenance_ap ne se compile que pour la cible ESP32."
#endif

#include <cstddef>
#include <cstdint>

#include "app/clock.h"
#include "config/hardware_profile.h"

namespace agv::maintenance {

// Fournit le contenu de /agvdump. Signature volontairement minimale : la page
// est produite par l'application, pas par le serveur.
using DumpSource = size_t (*)(char* out, size_t capacity);

class AccessPoint {
 public:
  AccessPoint(const HardwareProfile& profile, IClock& clock)
      : profile_(profile), clock_(clock) {}

  void set_dump_source(DumpSource src) { dump_ = src; }

  // Demande d'ouverture (ILS, bouton, commande de maintenance).
  void request_open();
  // Fermeture immédiate.
  void close();
  // À appeler périodiquement : referme la fenêtre à expiration.
  void tick();

  bool open() const { return open_; }
  uint32_t remaining_s() const;

 private:
  bool start_wifi();
  void stop_wifi();

  const HardwareProfile& profile_;
  IClock& clock_;
  DumpSource dump_ = nullptr;
  bool open_ = false;
  uint32_t opened_at_ms_ = 0;
  void* server_ = nullptr;
};

}  // namespace agv::maintenance
