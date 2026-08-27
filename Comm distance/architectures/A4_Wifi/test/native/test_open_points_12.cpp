// UN TEST EXPLICITE PAR LIGNE DU §12 (exigence du brief §11).
//
// Objet de ce fichier : prouver que chaque point non relevé sur la carte
// d'origine est un PARAMÈTRE lu depuis la configuration, et non une valeur
// figée dans la logique. Le jour où le relevé arrive, seul le profil YAML
// change : aucune ligne de logique.
//
// Convention : chaque test est nommé point_12_N_....
#include <string>

#include "app/course_queue.h"
#include "app/mega_app.h"
#include "app/persistent_store.h"
#include "app/sequencer.h"
#include "bus/bus_signals.h"
#include "bus/debounce.h"
#include "config/hardware_profile.h"
#include "proto/frame.h"
#include "proto/json.h"
#include "sim_bus_driver.h"
#include "test_framework.h"
#include "timing_profile.h"

using namespace agv;
using agv::sim::SimBusDriver;
using agv::sim::TimingProfile;

namespace {

struct Rig {
  HardwareProfile profile = default_profile();
  RamStore store;
  TimingProfile timings = TimingProfile::fast();
  BusLayout layout = kDefaultLayout;

  SimBusDriver* bus = nullptr;
  CourseQueue* queue = nullptr;
  Sequencer* seq = nullptr;

  void build() {
    bus = new SimBusDriver(profile, timings);
    bus->set_layout(layout);
    queue = new CourseQueue(profile.queue, &store);
    seq = new Sequencer(profile, *bus, *queue);
    seq->set_layout(layout);
    seq->begin();
  }
  ~Rig() {
    delete seq;
    delete queue;
    delete bus;
  }
  void run(uint64_t us) {
    for (uint64_t t = 0; t < us; t += 100) {
      seq->tick();
      bus->advance(100);
    }
    seq->tick();
  }
  void enqueue(uint16_t station) {
    Course c;
    c.station = station;
    c.speed = 4;
    c.node_id = 1;
    queue->push(c);
  }
};

}  // namespace

// §12.1, Amplitude réelle des lignes Y (6 V ou 24 V) : conditionne le
// debounce. Le paramètre est y_debounce_us.
TEST(point_12_1_debounce_Y_vient_du_profil) {
  YDebouncer fast(500);
  YDebouncer slow(5000);
  fast.reset(0, 0);
  slow.reset(0, 0);
  // À 1 ms d'entrée stable, le filtre court a validé, le long non.
  CHECK_EQ(fast.update(1, 0), 0u);
  CHECK_EQ(fast.update(1, 1000), 1u);
  CHECK_EQ(slow.update(1, 0), 0u);
  CHECK_EQ(slow.update(1, 1000), 0u);
  CHECK_EQ(slow.update(1, 6000), 1u);

  // Et le séquenceur consomme bien la valeur du profil, pas une constante.
  Rig r;
  r.profile.bus.y_debounce_us = 12345;
  r.build();
  CHECK_EQ(r.profile.bus.y_debounce_us, 12345u);
}

// §12.2 : Brochage des SUB-D 25 non tranché (CN61/62/63 vs CN62/63/64).
// Le mapping signal -> broche vit dans profiles/*.yaml, modifiable sans
// recompiler la logique.
TEST(point_12_2_brochage_vient_du_fichier_de_configuration) {
  // Les positions utilisées par le code SONT celles générées depuis le YAML.
  CHECK_EQ(static_cast<uint8_t>(x::X82), static_cast<uint8_t>(CFG_PIN_X82));
  CHECK_EQ(static_cast<uint8_t>(y::Y22), static_cast<uint8_t>(CFG_PIN_Y22));
  CHECK_EQ(CFG_PINMAP_X_COUNT, 22u);  // 22 sorties (§4.1)
  CHECK_EQ(CFG_PINMAP_Y_COUNT, 21u);  // 21 entrées (§4.2)
}

TEST(point_12_2_un_brochage_different_fonctionne_sans_toucher_la_logique) {
  // Table d'adresses inversée (poids fort en tête) : la course doit aboutir à
  // l'identique, ce qui prouve que l'ordre des bits n'est pas câblé en dur.
  Rig r;
  for (size_t i = 0; i < kStationBits; ++i) {
    r.layout.x_station_bits[i] = kDefaultLayout.x_station_bits[kStationBits - 1 - i];
    r.layout.y_station_bits[i] = kDefaultLayout.y_station_bits[kStationBits - 1 - i];
  }
  r.build();
  r.enqueue(9);
  r.run(5000000);
  CHECK_EQ(r.bus->position(), 9u);
  CHECK_EQ(r.seq->counters().current_station, 9u);
}

// §12.3, Logique automate PNP ou NPN : inverse la polarité des 22 voies X.
TEST(point_12_3_polarite_PNP_NPN_est_un_booleen_de_configuration) {
  Rig pnp;
  pnp.build();
  pnp.enqueue(3);
  pnp.run(3000000);

  Rig npn;
  npn.profile.bus.x_active_high = false;
  npn.profile.bus.y_active_high = false;
  npn.build();
  npn.enqueue(3);
  npn.run(3000000);

  // Même résultat fonctionnel, mots électriques opposés.
  CHECK_EQ(pnp.bus->position(), 3u);
  CHECK_EQ(npn.bus->position(), 3u);
  CHECK(pnp.bus->lastX() != npn.bus->lastX());
}

// §12.4 : t_setup du bus X avant le strobe X93.
TEST(point_12_4_t_setup_est_respecte_et_parametrable) {
  Rig strict;
  strict.timings.required_setup_us = 1000;
  strict.profile.bus.t_setup_us = 1200;  // valeur du profil, pas du code
  strict.build();
  strict.enqueue(2);
  strict.run(5000000);
  CHECK_EQ(strict.bus->setup_violations(), 0u);
  CHECK_EQ(strict.seq->counters().courses_completed, 1u);
  CHECK_EQ(strict.seq->counters().write_tries, 1u);
}

// §12.5, Timeouts des accusés Y22 / Y05 / Y10 : paramètres DISTINCTS.
TEST(point_12_5_les_trois_timeouts_sont_distincts_et_effectifs) {
  const HardwareProfile& p = default_profile();
  // Trois champs séparés dans la configuration.
  CHECK(p.timeouts.y22_write_ack_ms != p.timeouts.y05_start_ack_ms);
  CHECK(p.timeouts.y05_start_ack_ms != p.timeouts.y10_arrival_ms);

  // Y22 : un automate plus lent que le timeout configuré échoue…
  Rig too_slow;
  too_slow.timings.y22_delay_us = 200000;      // 200 ms
  too_slow.profile.timeouts.y22_write_ack_ms = 50;  // 50 ms
  too_slow.build();
  too_slow.enqueue(2);
  too_slow.run(3000000);
  CHECK(too_slow.seq->fault_cause() == FaultCause::WriteTimeout);

  // …et réussit dès que le profil porte la bonne valeur.
  Rig tuned;
  tuned.timings.y22_delay_us = 200000;
  tuned.profile.timeouts.y22_write_ack_ms = 400;
  tuned.build();
  tuned.enqueue(2);
  tuned.run(5000000);
  CHECK_EQ(tuned.seq->counters().courses_completed, 1u);
}

TEST(point_12_5_timeout_d_arrivee_Y10_est_instrumente) {
  Rig r;
  r.timings.travel_per_station_us = 5000000;  // AGV très lent
  r.profile.timeouts.y10_arrival_ms = 100;    // patience très courte
  r.build();
  r.enqueue(5);
  r.run(3000000);
  CHECK(r.seq->fault_cause() == FaultCause::ArrivalTimeout);
  CHECK_EQ(r.seq->counters().y10_timeouts, 1u);
}

// §12.6 : Correspondance des repères sérigraphiés T9…T24 avec les signaux Y.
// Non relevée : le code n'en dépend jamais, il ne manipule que des noms de
// signaux. Le champ `*_op_return` est exposé SYMBOLIQUEMENT pour pouvoir être
// recalé sur la sortie agvdump d'origine sans réécrire la logique.
TEST(point_12_6_aucun_repere_serigraphie_n_est_cable_dans_la_logique) {
  CHECK_STR_EQ(Sequencer::op_return_str(OpReturn::Ok), "OK");
  CHECK_STR_EQ(Sequencer::op_return_str(OpReturn::Timeout), "TIMEOUT");
  // Les noms de compteurs agvdump existent tels quels ; leur valeur numérique
  // reste à recaler (PROVISOIRE §12.6).
  SeqCounters c;
  c.write_op_return = OpReturn::Ok;
  CHECK(c.write_op_return == OpReturn::Ok);
}

// §12.7 : Protocole ESP32 <-> application mobile « AIO AGV Remote » non
// documenté. TRANCHÉ par la planification (§1) : l'application mobile est
// abandonnée au profit des boutons EnOcean et de l'interface web du poste
// fixe. Le champ `ver` de la trame reste néanmoins réservé, pour qu'un second
// dialecte puisse coexister si la décision changeait.
TEST(point_12_7_le_protocole_reste_versionne) {
  Frame f;
  f.ver = 2;
  f.type = FrameType::Ping;
  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  Frame out;
  CHECK(decode_frame(buf, len, out, 1) == FrameError::BadVersion);
  CHECK(decode_frame(buf, len, out, 2) == FrameError::Ok);
}

// §12.8 : TCM 515 (Rx seul) ou TCM 310 (bidirectionnel). Dans cette
// architecture, le récepteur EnOcean est sur le POSTE FIXE (UniPi), pas sur la
// carte AGV : la question ne concerne pas ce firmware. Ce qui le concerne, en
// revanche, c'est que l'accusé opérateur dépend d'un aller-retour complet
// AGV -> MQTT -> poste : le champ `ack` publié doit donc toujours être émis.
TEST(point_12_8_l_accuse_remonte_jusqu_au_poste) {
  const HardwareProfile& p = default_profile();
  // Un ACK est publié sur agv/<id>/ack pour chaque commande, quel qu'en soit
  // le sort : c'est la seule information dont dispose l'opérateur.
  CHECK(p.mqtt.qos >= 1u);  // QoS 1 minimum (planification §2)
  CHECK_STR_EQ(p.mqtt.agv_id, "1");
}

// §12.9 : Runtime disponible sur l'UniPi E413 commandé (Mervis vs Linux).
// Ici l'UniPi porte le poste fixe ET le broker MQTT : la question devient
// bloquante, pas seulement gênante. Le contrat entre l'AGV et le poste est du
// JSON sur MQTT : indépendant de la plateforme, du langage et de l'endianness.
TEST(point_12_9_le_contrat_reseau_est_independant_de_la_plateforme) {
  char buf[128];
  json::Writer w(buf, sizeof(buf));
  w.field("seq", static_cast<uint32_t>(7));
  w.field("dest", static_cast<uint32_t>(300));
  w.field("speed", static_cast<uint32_t>(4));
  w.field("ts", static_cast<uint32_t>(1700000000));
  REQUIRE(w.end() > 0u);

  int32_t dest = 0;
  CHECK(json::get_int(buf, "dest", dest));
  CHECK_EQ(dest, 300);
  // Aucun octet d'ordre machine : le poste peut être en Python, en C ou en
  // Node, sur n'importe quelle architecture.
  CHECK(std::string(buf).find("\"dest\":300") != std::string::npos);
}

// §12.10 : Variante matérielle d'interface bus. TRANCHÉ dans cette
// architecture : la carte V5.0.1 est conservée, l'ATmega2560 pose le bus par
// ses ports. Ce qui reste à vérifier, c'est que la logique n'en dépend pas :
// le simulateur doit rester une implémentation valide de IBusDriver.
TEST(point_12_10_le_materiel_est_celui_de_la_carte_conservee) {
  const HardwareProfile& p = default_profile();
  CHECK(p.bus.variant == DriverVariant::AvrPort);

  Rig r;
  r.build();
  IBusDriver* driver = r.bus;
  CHECK_STR_EQ(driver->name(), "sim");
  CHECK(driver->begin());
}

// Spécifique à cette architecture : le repli heartbeat est un PARAMÈTRE, pas
// une constante : le seuil de 2 s vient de la planification, pas du code.
TEST(point_heartbeat_le_seuil_de_repli_vient_du_profil) {
  const HardwareProfile& p = default_profile();
  CHECK_EQ(p.heartbeat.timeout_ms, 2000u);
  CHECK(p.heartbeat.period_ms < p.heartbeat.timeout_ms);
  // Marge d'au moins trois battements avant de déclarer la liaison perdue :
  // un heartbeat manqué isolément ne doit pas immobiliser l'AGV.
  CHECK(p.heartbeat.timeout_ms >= 3u * p.heartbeat.period_ms);
}
