// Wi-Fi de maintenance de l'AGV (brief §9.4).
//
// Dans cette architecture, l'ESP32 est CLIENT du réseau d'entreprise : il n'a
// plus de point d'accès permanent, contrairement à la carte d'origine qui
// diffusait `agv_atelier` en continu. Le point d'accès ne s'ouvre qu'à la
// demande, sur contact ILS (aimant) ou bouton, et se referme automatiquement
// après `wifi_window_s` (600 s par défaut).
//
// ⚠ Pendant cette fenêtre, l'ESP32 doit basculer en mode APSTA : couper la
// liaison cliente pour servir la page de diagnostic reviendrait à déconnecter
// l'AGV du poste fixe pendant dix minutes.
//
// Objectif : préserver la procédure `/agvdump` du client (§3.3) sans rétablir
// une émission 2,4 GHz permanente.
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
