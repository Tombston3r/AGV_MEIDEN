#include "app/sequencer.h"

namespace agv {
namespace {
constexpr uint32_t kXMask = (1u << 22) - 1u;
// Type de donnée écrit sur X94 : 0 = numéro de station, 1 = comptage de
// marqueurs. Seul le mode « station » est utilisé par la file de courses.
constexpr bool kDataTypeStation = false;
}  // namespace

bool Sequencer::begin() {
  // §3.1 : le bus X est physiquement à zéro à la mise sous tension ; le
  // firmware confirme cet état avant toute autre action.
  x_word_ = 0;
  if (!bus_.begin()) return false;
  if (!write_bus(0)) return false;
  state_since_us_ = bus_.now_us();
  transition(SeqState::Idle);
  return true;
}

uint32_t Sequencer::x_logical() const { return x_word_; }

void Sequencer::set_x(uint8_t bit, bool value) { x_word_ = with_bit(x_word_, bit, value); }

bool Sequencer::write_bus(uint32_t word) {
  x_word_ = word & kXMask;
  // Polarité §12.3 : la logique applicative est toujours en actif haut ; la
  // conversion PNP/NPN n'a lieu qu'ici, au contact du matériel.
  const uint32_t electrical = profile_.bus.x_active_high ? x_word_ : (~x_word_ & kXMask);
  if (!bus_.writeX(electrical)) {
    enter_fault(FaultCause::BusWriteError);
    return false;
  }
  return true;
}

void Sequencer::sample_inputs() {
  const uint32_t raw = bus_.readY();
  y_word_ = profile_.bus.y_active_high ? raw : (~raw & ((1u << 21) - 1u));
  moving_ = bit_set(y_word_, y::Y05);
  in_station_ = bit_set(y_word_, y::Y10);
  plc_fault_ = bit_set(y_word_, y::Y03);
  no_destination_ = bit_set(y_word_, y::Y21);
  switch_echo_ = bit_set(y_word_, y::Y15);
  direction_echo_ = bit_set(y_word_, y::Y20);
  counters_.current_station = decode_position(y_word_, layout_);
  counters_.current_speed = decode_current_speed(y_word_, layout_);
  counters_.nb_courses_programmed = static_cast<uint8_t>(queue_.size() + (has_active_ ? 1u : 0u));
}

void Sequencer::transition(SeqState next) {
  if (next == state_) return;
  const SeqState from = state_;
  state_ = next;
  state_since_us_ = bus_.now_us();
  ++counters_.transitions;
  if (observer_ != nullptr) observer_->on_transition(from, next, state_since_us_);
}

void Sequencer::enter_fault(FaultCause cause) {
  fault_cause_ = cause;
  ++counters_.faults;
  // État sûr : toutes les sorties au repos. La chaîne de sécurité (arrêt
  // d'urgence, bumpers, scrutateur) reste indépendante : voir brief §3.1.
  x_word_ = 0;
  const uint32_t electrical = profile_.bus.x_active_high ? 0u : kXMask;
  bus_.writeX(electrical);
  if (observer_ != nullptr) observer_->on_fault(cause);
  transition(SeqState::Fault);
}

void Sequencer::request_safe_stop(FaultCause cause) {
  safe_stop_requested_ = true;
  if (cause != FaultCause::None && fault_cause_ == FaultCause::None) {
    fault_cause_ = cause;
  }
  // En transit, on laisse l'AGV rejoindre le point d'arrêt suivant plutôt que
  // de couper la commande en pleine allée : jamais d'état indéterminé.
  if (state_ == SeqState::Idle) transition(SeqState::SafeStop);
}

void Sequencer::clear_fault() {
  safe_stop_requested_ = false;
  fault_cause_ = FaultCause::None;
  has_active_ = false;
  x_word_ = 0;
  write_bus(0);
  transition(SeqState::Idle);
}

bool Sequencer::start_next_course() {
  if (!queue_.pop(active_)) return false;
  has_active_ = true;
  counters_.write_tries = 0;
  counters_.write_op_return = OpReturn::None;
  counters_.start_tries = 0;
  counters_.start_op_return = OpReturn::None;

  // Phase ÉCRITURE, étapes 1 à 4.
  uint32_t word = 0;
  word = with_bit(word, x::X94, kDataTypeStation);      // 1. type de donnée
  word = encode_station(word, layout_, active_.station);  // 2. destination
  word = encode_speed(word, layout_, active_.speed);      // 3. vitesse
  word = with_bit(word, x::X92, true);                    // 4. data input switch
  if (!write_bus(word)) return false;
  data_set_us_ = bus_.now_us();
  if (observer_ != nullptr) observer_->on_course_started(active_);
  transition(SeqState::WriteSetup);
  return true;
}

void Sequencer::tick() {
  sample_inputs();

  // Défaut automate : prioritaire sur toute la séquence, quel que soit l'état.
  if (plc_fault_ && state_ != SeqState::Fault) {
    enter_fault(FaultCause::PlcFault);
    return;
  }

  switch (state_) {
    case SeqState::Boot:
      // begin() n'a pas été appelé : on refuse de piloter le bus.
      break;

    case SeqState::Idle:
      if (safe_stop_requested_) {
        transition(SeqState::SafeStop);
      } else if (!queue_.empty()) {
        start_next_course();
      }
      break;

    case SeqState::WriteSetup:
      // 5. Monter X93 seulement après t_setup (§12.4) : le strobe posé trop tôt
      //    est la première cause d'écriture perdue.
      if ((bus_.now_us() - data_set_us_) >= profile_.bus.t_setup_us) {
        set_x(x::X93, true);
        if (!write_bus(x_word_)) break;
        ++counters_.write_tries;
        transition(SeqState::WriteStrobe);
      }
      break;

    case SeqState::WriteStrobe:
      // 6. Attendre Y22 (instruction reading complete).
      if (bit_set(y_word_, y::Y22)) {
        counters_.write_op_return = OpReturn::Ok;
        transition(SeqState::WriteRelease);
      } else if (elapsed_us() >= static_cast<uint64_t>(profile_.timeouts.y22_write_ack_ms) * 1000u) {
        ++counters_.y22_timeouts;
        set_x(x::X93, false);
        write_bus(x_word_);
        if (counters_.write_tries >= profile_.timeouts.write_max_tries) {
          counters_.write_op_return = OpReturn::Timeout;
          enter_fault(FaultCause::WriteTimeout);
        } else {
          // On recommence à l'étape 1 : données re-posées, nouveau t_setup.
          data_set_us_ = bus_.now_us();
          transition(SeqState::WriteSetup);
        }
      }
      break;

    case SeqState::WriteRelease:
      // 7. Redescendre X93 puis X92, en respectant t_hold entre les deux.
      if (bit_set(x_word_, x::X93)) {
        set_x(x::X93, false);
        write_bus(x_word_);
        state_since_us_ = bus_.now_us();
      } else if (elapsed_us() >= profile_.bus.t_hold_us) {
        set_x(x::X92, false);
        if (!write_bus(x_word_)) break;
        // 8. Phase DÉMARRAGE : monter X82.
        set_x(x::X82, true);
        if (!write_bus(x_word_)) break;
        ++counters_.start_tries;
        transition(SeqState::StartPulse);
      }
      break;

    case SeqState::StartPulse:
      // 9. Attendre Y05 (moving flag).
      if (moving_) {
        counters_.start_op_return = OpReturn::Ok;
        transition(SeqState::StartRelease);
      } else if (elapsed_us() >= static_cast<uint64_t>(profile_.timeouts.y05_start_ack_ms) * 1000u) {
        ++counters_.y05_timeouts;
        set_x(x::X82, false);
        write_bus(x_word_);
        if (counters_.start_tries >= profile_.timeouts.start_max_tries) {
          counters_.start_op_return = OpReturn::Timeout;
          enter_fault(FaultCause::StartTimeout);
        } else {
          // Nouvelle tentative de front montant après t_hold.
          set_x(x::X82, true);
          write_bus(x_word_);
          ++counters_.start_tries;
          state_since_us_ = bus_.now_us();
        }
      }
      break;

    case SeqState::StartRelease:
      // 10. Redescendre X82 après t_hold.
      if (elapsed_us() >= profile_.bus.t_hold_us) {
        set_x(x::X82, false);
        if (!write_bus(x_word_)) break;
        transition(SeqState::Transit);
      }
      break;

    case SeqState::Transit:
      // 11-12. Position sur Y23…Y34 (déjà décodée par sample_inputs), et
      //        surveillance des drapeaux automate.
      if (in_station_ && !moving_) {
        // 13. Arrivée. Testée AVANT Y21 : à l'arrivée l'automate lève aussi
        //     « pas de destination programmée », puisqu'il vient de consommer
        //     la sienne. Inverser l'ordre transformerait chaque arrivée en
        //     défaut.
        transition(SeqState::Arrived);
      } else if (no_destination_ && !moving_) {
        // L'automate déclare n'avoir aucune destination programmée alors qu'on
        // vient de lui en écrire une : l'écriture a été perdue.
        enter_fault(FaultCause::NoDestination);
      } else if (elapsed_us() >= static_cast<uint64_t>(profile_.timeouts.y10_arrival_ms) * 1000u) {
        ++counters_.y10_timeouts;
        enter_fault(FaultCause::ArrivalTimeout);
      }
      break;

    case SeqState::Arrived:
      // 14. X83 (standby stop) : confirmé par l'extinction du moving flag.
      set_x(x::X83, true);
      if (!write_bus(x_word_)) break;
      ++counters_.stop_tries;
      state_since_us_ = bus_.now_us();
      transition(SeqState::StopPulse);
      break;

    case SeqState::StopPulse:
      if (elapsed_us() >= profile_.bus.t_strobe_us) {
        set_x(x::X83, false);
        if (!write_bus(x_word_)) break;
        counters_.stop_op_return = moving_ ? OpReturn::Timeout : OpReturn::Ok;
        if (moving_) {
          // L'AGV bouge encore : nouvelle impulsion X83 (comptée dans l'état
          // Arrived), jusqu'à stop_max_tries.
          if (counters_.stop_tries >= profile_.timeouts.stop_max_tries) {
            enter_fault(FaultCause::StopTimeout);
            break;
          }
          transition(SeqState::Arrived);
          break;
        }
        // 15. Course terminée : on dépile la suivante.
        ++counters_.courses_completed;
        counters_.stop_tries = 0;
        if (observer_ != nullptr) observer_->on_course_done(active_, true);
        has_active_ = false;
        if (safe_stop_requested_) {
          transition(SeqState::SafeStop);
        } else if (!queue_.empty()) {
          start_next_course();
        } else {
          transition(SeqState::Idle);
        }
      }
      break;

    case SeqState::SafeStop:
      // Arrêt sûr atteint : plus aucune course lancée tant que l'opérateur n'a
      // pas acquitté. Les sorties restent au repos.
      if (x_word_ != 0) write_bus(0);
      break;

    case SeqState::Fault:
      // Sorties déjà au repos, LED FAULT pilotée par l'application.
      break;
  }
}

const char* Sequencer::state_str(SeqState s) {
  switch (s) {
    case SeqState::Boot: return "BOOT";
    case SeqState::Idle: return "IDLE";
    case SeqState::WriteSetup: return "WRITE_SETUP";
    case SeqState::WriteStrobe: return "WRITE_STROBE";
    case SeqState::WriteRelease: return "WRITE_RELEASE";
    case SeqState::StartPulse: return "START_PULSE";
    case SeqState::StartRelease: return "START_RELEASE";
    case SeqState::Transit: return "TRANSIT";
    case SeqState::Arrived: return "ARRIVED";
    case SeqState::StopPulse: return "STOP_PULSE";
    case SeqState::SafeStop: return "SAFE_STOP";
    case SeqState::Fault: return "FAULT";
  }
  return "?";
}

const char* Sequencer::fault_str(FaultCause c) {
  switch (c) {
    case FaultCause::None: return "NONE";
    case FaultCause::WriteTimeout: return "WRITE_TIMEOUT";
    case FaultCause::StartTimeout: return "START_TIMEOUT";
    case FaultCause::StopTimeout: return "STOP_TIMEOUT";
    case FaultCause::ArrivalTimeout: return "ARRIVAL_TIMEOUT";
    case FaultCause::PlcFault: return "PLC_FAULT";
    case FaultCause::NoDestination: return "NO_DESTINATION";
    case FaultCause::BusWriteError: return "BUS_WRITE_ERROR";
    case FaultCause::LinkLost: return "LINK_LOST";
  }
  return "?";
}

const char* Sequencer::op_return_str(OpReturn r) {
  switch (r) {
    case OpReturn::None: return "NONE";
    case OpReturn::Ok: return "OK";
    case OpReturn::Timeout: return "TIMEOUT";
    case OpReturn::Aborted: return "ABORTED";
  }
  return "?";
}

}  // namespace agv
