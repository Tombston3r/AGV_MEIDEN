// Fidélité du dump d'atelier.
//
// `/agvdump` est utilisé EN PRODUCTION par le client (brief §3.3) : ses noms
// de champs et de compteurs sont repris du firmware d'origine, et les
// procédures d'atelier les lisent. Le format ne se modernise pas, il se
// conserve.
//
// Le rendu est donc une COPIE OCTET POUR OCTET de celui de l'architecture A4.
// Ce fichier vérifie deux choses :
//   1. que la copie n'a pas dérivé de son original ;
//   2. que le format rendu contient bien les champs attendus, aux bons noms.
//
// Sans le premier contrôle, une copie est une divergence en sursis.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "atelier/app/agvdump.h"

namespace {

int g_tests = 0;
int g_echecs = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::printf("    ECHEC %s:%d : %s\n", __FILE__, __LINE__, #cond); \
      ++g_echecs;                                                       \
    }                                                                   \
  } while (0)

#define RUN(f)                        \
  do {                                \
    std::printf("[ RUN  ] %s\n", #f); \
    ++g_tests;                        \
    f();                              \
  } while (0)

std::string lire(const std::string& chemin) {
  std::ifstream f(chemin, std::ios::binary);
  if (!f) return {};
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

// Le chemin de l'original, relatif à la racine du dépôt.
const char* kRacineA4 = "../Comm distance/architectures/A4_Wifi/firmware/common/";

void la_copie_n_a_pas_derive_de_l_original() {
  struct Paire {
    const char* copie;
    const char* origine;
  };
  const Paire paires[] = {
      {"atelier/agvdump.cpp", "app/agvdump.cpp"},
      {"atelier/app/agvdump.h", "app/agvdump.h"},
      {"atelier/link/link_protocol.h", "link/link_protocol.h"},
  };
  for (const Paire& p : paires) {
    const std::string copie = lire(p.copie);
    const std::string origine = lire(std::string(kRacineA4) + p.origine);
    if (origine.empty()) {
      // Dossier A4 absent (export du seul chantier Timer) : on ne peut pas
      // comparer, mais on le DIT plutôt que de faire passer le test en vert.
      std::printf("    (original introuvable : %s%s, comparaison sautée)\n",
                  kRacineA4, p.origine);
      continue;
    }
    CHECK(!copie.empty());
    if (copie != origine) {
      std::printf("    DIVERGENCE : %s ne correspond plus a %s%s\n", p.copie,
                  kRacineA4, p.origine);
      ++g_echecs;
    }
  }
}

void le_dump_porte_les_noms_de_champs_du_client() {
  agv::link::LinkState etat;
  etat.station = 17;
  etat.speed = 3;
  etat.queue_len = 2;
  etat.write_tries = 4;
  etat.last_seq = 9;
  etat.flags = agv::link::state_flag::kMoving | agv::link::state_flag::kHeartbeatOk;

  agv::AgvDumpInput in;
  in.state = &etat;
  in.profile_name = "banc";
  in.uptime_s = 3600;
  in.link_up = true;
  in.heartbeats_sent = 42;

  char buf[2048];
  const size_t n = agv::render_agvdump(in, buf, sizeof(buf));
  CHECK(n > 0);
  const std::string dump(buf, n);

  // Ces noms sont un CONTRAT avec les procédures d'atelier : les renommer
  // rendrait les outils du client caducs.
  for (const char* champ : {"AIO AGV CONTROL - DUMP", "profile=", "uptime_s=",
                            "[AGV STATE]", "state=", "fault=", "current_station=",
                            "current_speed=", "moving=", "in_station=", "plc_fault=",
                            "no_destination=", "safe_stop=", "heartbeat_ok=",
                            "write_tries=", "start_tries=", "stop_tries=", "last_seq=",
                            "battery_mv=", "[QUEUE]", "nb_courses_programmed=",
                            "[LINK]", "mcu_link_up=", "link_timeouts=",
                            "heartbeats_sent=", "wifi_up=", "ssid=", "rssi_dbm=",
                            "mqtt_up=", "cmd_expired=", "cmd_duplicate="}) {
      if (dump.find(champ) == std::string::npos) {
        std::printf("    CHAMP ABSENT : %s\n", champ);
        ++g_echecs;
      }
  }
  CHECK(dump.find("current_station=17") != std::string::npos);
  CHECK(dump.find("nb_courses_programmed=2") != std::string::npos);
  CHECK(dump.find("moving=1") != std::string::npos);
  CHECK(dump.find("heartbeat_ok=1") != std::string::npos);
  CHECK(dump.find("heartbeats_sent=42") != std::string::npos);
}

void le_dump_est_du_texte_brut_sans_balise() {
  agv::AgvDumpInput in;
  char buf[2048];
  const size_t n = agv::render_agvdump(in, buf, sizeof(buf));
  const std::string dump(buf, n);
  // Le jour où quelqu'un « modernisera » le dump en HTML, ce test tombera.
  CHECK(dump.find('<') == std::string::npos);
  CHECK(dump.find("state=BOOT") != std::string::npos);
  CHECK(dump.find("fault=NONE") != std::string::npos);
}

}  // namespace

int main() {
  RUN(la_copie_n_a_pas_derive_de_l_original);
  RUN(le_dump_porte_les_noms_de_champs_du_client);
  RUN(le_dump_est_du_texte_brut_sans_balise);
  std::printf("\n%d tests, %d echec(s)\n", g_tests, g_echecs);
  return g_echecs == 0 ? 0 : 1;
}
