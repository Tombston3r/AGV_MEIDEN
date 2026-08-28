// Rendu de la page /agvdump (brief §3.3 et §9.3).
//
// COMPATIBILITÉ : la procédure de diagnostic `agvdump` est utilisée en
// production par le client. Les NOMS de champs et de compteurs sont repris
// littéralement du firmware d'origine. La mise en page exacte de la V5.0.1
// n'est PAS relevée, PROVISOIRE §12.6 : à recaler sur un dump réel avant
// mise en service, sinon les procédures d'atelier deviennent caduques.
#pragma once

#include <cstddef>

#include "link/link_protocol.h"

namespace agv {

// Sur cette carte, `/agvdump` est servi par l'ESP32 pendant la fenêtre de
// maintenance (planification §2.8) : les compteurs du séquenceur ne sont donc
// pas lus directement, ils arrivent par la liaison série depuis l'ATmega.
struct AgvDumpInput {
  const link::LinkState* state = nullptr;
  const char* profile_name = "?";
  uint32_t uptime_s = 0;
  uint16_t battery_mv = 0;   // ADC AGV, traverse la barrière d'isolation
  bool link_up = false;      // liaison série ESP32 <-> ATmega
  bool wifi_up = false;
  // Nom conservé volontairement : le format `/agvdump` est celui des
  // procédures d'atelier du client (§3.3). En LoRa on y place l'état de
  // la liaison radio : renommer le champ casserait leurs outils.
  bool mqtt_up = false;
  int16_t rssi_dbm = 0;
  const char* ssid = "";
  uint32_t heartbeats_sent = 0;
  uint32_t link_timeouts = 0;
  uint32_t cmd_expired = 0;
  uint32_t cmd_duplicate = 0;
};

// Écrit le dump texte. Retourne le nombre d'octets écrits (hors terminateur).
size_t render_agvdump(const AgvDumpInput& in, char* out, size_t capacity);

}  // namespace agv
