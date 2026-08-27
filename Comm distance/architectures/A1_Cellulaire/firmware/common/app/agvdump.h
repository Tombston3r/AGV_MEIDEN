// Rendu de la page /agvdump (brief §3.3 et §9.3).
//
// COMPATIBILITÉ : la procédure de diagnostic `agvdump` est utilisée en
// production par le client. Les NOMS de champs et de compteurs sont repris
// littéralement du firmware d'origine. La mise en page exacte de la V5.0.1
// n'est PAS relevée — PROVISOIRE §12.6 : à recaler sur un dump réel avant
// mise en service, sinon les procédures d'atelier deviennent caduques.
#pragma once

#include <cstddef>

#include "app/course_queue.h"
#include "app/sequencer.h"
#include "transport/itransport.h"

namespace agv {

struct AgvDumpInput {
  const SeqCounters* counters = nullptr;
  const CourseQueue* queue = nullptr;
  const Sequencer* sequencer = nullptr;
  const LinkHealth* link = nullptr;
  const char* transport_name = "none";
  const char* profile_name = "?";
  uint32_t uptime_s = 0;
  uint16_t battery_mv = 0;  // ADC AGV, traverse la barrière d'isolation
  bool link_up = false;
};

// Écrit le dump texte. Retourne le nombre d'octets écrits (hors terminateur).
size_t render_agvdump(const AgvDumpInput& in, char* out, size_t capacity);

}  // namespace agv
