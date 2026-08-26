// Séquenceur trois phases du bus MEIDEN (brief §4.3).
//
// Reproduit à l'identique la séquence du firmware d'origine :
//   ÉCRITURE  : X94, adresse 10 bits, vitesse 4 bits, X92, strobe X93, Y22
//   DÉMARRAGE : X82, attente Y05
//   TRANSIT   : lecture continue Y23…Y34, surveillance Y03/Y21/Y15/Y20
//   ARRIVÉE   : Y10, éventuel X83, dépilement de la course suivante
//
// Machine à états explicite : aucun `delay()`, aucun blocage, chaque timeout
// instrumenté. Les compteurs portent LITTÉRALEMENT les noms de la section
// `AGV STATE` de `agvdump` (brief §3.3) : write_tries, write_op_return,
// start_tries, start_op_return, stop_tries, stop_op_return.
#pragma once

#include <cstdint>

#include "app/course_queue.h"
#include "bus/bus_signals.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"

namespace agv {

enum class SeqState : uint8_t {
  Boot,          // bus X à zéro, attente du premier tick
  Idle,          // file vide, rien à faire
  WriteSetup,    // données posées, attente de t_setup avant strobe
  WriteStrobe,   // X93 haut, attente Y22
  WriteRelease,  // retombée X93 puis X92
  StartPulse,    // X82 haut, attente Y05
  StartRelease,  // retombée X82
  Transit,       // en route, décodage de la position
  Arrived,       // Y10 vu
  StopPulse,     // X83 haut
  SafeStop,      // arrêt sûr demandé : plus aucune course n'est lancée
  Fault,         // défaut applicatif, sorties au repos, LED FAULT
};

// Résultat final d'une opération. ATTENTION : la valeur numérique exposée par
// `agvdump` d'origine n'est pas relevée — PROVISOIRE §12.6, à recaler sur une
// sortie réelle de la V5.0.1 avant mise en production.
enum class OpReturn : uint8_t {
  None = 0,
  Ok = 1,
  Timeout = 2,
  Aborted = 3,
};

// Cause de défaut, remontée telle quelle dans agvdump et la supervision.
enum class FaultCause : uint8_t {
  None = 0,
  WriteTimeout,     // Y22 jamais reçu après write_max_tries
  StartTimeout,     // Y05 jamais reçu après start_max_tries
  StopTimeout,      // arrêt non confirmé
  ArrivalTimeout,   // Y10 jamais reçu
  PlcFault,         // Y03
  NoDestination,    // Y21 alors qu'une destination vient d'être écrite
  BusWriteError,    // le driver n'a pas pu poser le bus
  LinkLost,         // chien de garde de liaison (§3.1)
};

struct SeqCounters {
  // --- noms littéralement compatibles agvdump ---
  uint32_t write_tries = 0;
  OpReturn write_op_return = OpReturn::None;
  uint32_t start_tries = 0;
  OpReturn start_op_return = OpReturn::None;
  uint32_t stop_tries = 0;
  OpReturn stop_op_return = OpReturn::None;
  uint16_t current_station = 0;
  uint8_t current_speed = 0;
  uint8_t nb_courses_programmed = 0;
  // --- instrumentation additionnelle ---
  uint32_t courses_completed = 0;
  uint32_t faults = 0;
  uint32_t transitions = 0;
  uint32_t y22_timeouts = 0;
  uint32_t y05_timeouts = 0;
  uint32_t y10_timeouts = 0;
};

// Observateur des transitions : journalisation structurée sans coupler le
// séquenceur à une implémentation de log.
class ISequencerObserver {
 public:
  virtual ~ISequencerObserver() = default;
  virtual void on_transition(SeqState from, SeqState to, uint64_t now_us) = 0;
  virtual void on_course_started(const Course& c) = 0;
  virtual void on_course_done(const Course& c, bool ok) = 0;
  virtual void on_fault(FaultCause cause) = 0;
};

class Sequencer {
 public:
  Sequencer(const HardwareProfile& profile, IBusDriver& bus, CourseQueue& queue)
      : profile_(profile), bus_(bus), queue_(queue), layout_(kDefaultLayout) {}

  void set_observer(ISequencerObserver* obs) { observer_ = obs; }
  void set_layout(const BusLayout& layout) { layout_ = layout; }

  bool begin();
  // À appeler en boucle depuis la tâche épinglée sur le cœur 1.
  void tick();

  // Demande d'arrêt sûr : la course en cours va jusqu'au point d'arrêt suivant,
  // aucune course supplémentaire n'est lancée (brief §3.1).
  void request_safe_stop(FaultCause cause = FaultCause::None);
  // Sortie d'état sûr / acquittement de défaut, sur action opérateur.
  void clear_fault();

  SeqState state() const { return state_; }
  FaultCause fault_cause() const { return fault_cause_; }
  const SeqCounters& counters() const { return counters_; }
  const Course& active_course() const { return active_; }
  bool has_active_course() const { return has_active_; }
  bool moving() const { return moving_; }
  bool in_station() const { return in_station_; }
  bool plc_fault() const { return plc_fault_; }
  bool no_destination_flag() const { return no_destination_; }
  bool switch_echo() const { return switch_echo_; }
  bool direction_echo() const { return direction_echo_; }
  uint32_t last_y_word() const { return y_word_; }

  static const char* state_str(SeqState s);
  static const char* fault_str(FaultCause c);
  static const char* op_return_str(OpReturn r);

 private:
  void transition(SeqState next);
  void sample_inputs();
  bool write_bus(uint32_t word);
  void enter_fault(FaultCause cause);
  bool start_next_course();
  void set_x(uint8_t bit, bool value);
  uint32_t x_logical() const;
  uint64_t elapsed_us() const { return bus_.now_us() - state_since_us_; }

  const HardwareProfile& profile_;
  IBusDriver& bus_;
  CourseQueue& queue_;
  BusLayout layout_;
  ISequencerObserver* observer_ = nullptr;

  SeqState state_ = SeqState::Boot;
  FaultCause fault_cause_ = FaultCause::None;
  SeqCounters counters_{};

  Course active_{};
  bool has_active_ = false;
  bool safe_stop_requested_ = false;

  uint32_t x_word_ = 0;   // mot logique (polarité appliquée à l'écriture)
  uint32_t y_word_ = 0;   // mot logique lu
  uint64_t state_since_us_ = 0;
  uint64_t data_set_us_ = 0;

  // Drapeaux issus du bus Y.
  bool moving_ = false;
  bool in_station_ = false;
  bool plc_fault_ = false;
  bool no_destination_ = false;
  bool switch_echo_ = false;
  bool direction_echo_ = false;
};

}  // namespace agv
