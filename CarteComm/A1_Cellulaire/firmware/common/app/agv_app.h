// Application embarquée sur l'AGV : colle entre transport, file et séquenceur.
//
// Responsabilités :
//  - traiter les trames reçues (idempotence, anti-rejeu, fraîcheur) ;
//  - acquitter SANS RÉ-EXÉCUTER les doublons (brief §5.1) ;
//  - empiler les courses et persister la file en NVS ;
//  - chien de garde de liaison : absence de trame valide pendant
//    `link_watchdog_s` -> arrêt sûr au point d'arrêt suivant + LED FAULT ;
//  - émettre la télémétrie.
#pragma once

#include <cstdint>

#include "app/agvdump.h"
#include "app/clock.h"
#include "app/course_queue.h"
#include "app/sequencer.h"
#include "proto/replay_window.h"
#include "transport/itransport.h"

namespace agv {

// Sorties de signalisation locales (LED FAULT, LED liaison).
class IIndicators {
 public:
  virtual ~IIndicators() = default;
  virtual void set_fault(bool on) = 0;
  virtual void set_link(bool on) = 0;
};

struct AppStats {
  uint32_t cmd_accepted = 0;
  uint32_t cmd_duplicate = 0;
  uint32_t cmd_out_of_order = 0;
  uint32_t cmd_expired = 0;
  uint32_t cmd_rejected_full = 0;
  uint32_t link_losses = 0;
  uint32_t telemetry_sent = 0;
};

class AgvApp {
 public:
  AgvApp(const HardwareProfile& profile, IClock& clock, ITransport& transport, Sequencer& seq,
         CourseQueue& queue, IIndicators* indicators = nullptr)
      : profile_(profile),
        clock_(clock),
        transport_(transport),
        seq_(seq),
        queue_(queue),
        indicators_(indicators),
        replay_(profile.protocol.replay_window, transport.ordered()) {}

  bool begin();
  void tick();

  // Période d'émission de la télémétrie. 0 = désactivée.
  void set_telemetry_period_ms(uint32_t ms) { telemetry_period_ms_ = ms; }

  const AppStats& stats() const { return stats_; }
  bool link_up() const { return link_up_; }
  uint32_t last_valid_frame_ms() const { return last_valid_frame_ms_; }
  size_t restored_courses() const { return restored_courses_; }
  size_t dropped_courses() const { return dropped_courses_; }

  // Rendu `agvdump` (format historique, brief §3.3 et §9.3).
  size_t render_agvdump(char* out, size_t capacity) const;

 private:
  void handle_frame(const Frame& f);
  void send_ack(const Frame& request, bool positive);
  void send_telemetry();

  const HardwareProfile& profile_;
  IClock& clock_;
  ITransport& transport_;
  Sequencer& seq_;
  CourseQueue& queue_;
  IIndicators* indicators_;
  ReplayWindow replay_;
  AppStats stats_{};

  bool link_up_ = false;
  uint32_t last_valid_frame_ms_ = 0;
  uint32_t last_telemetry_ms_ = 0;
  uint32_t telemetry_period_ms_ = 2000;
  uint8_t tx_seq_ = 0;
  size_t restored_courses_ = 0;
  size_t dropped_courses_ = 0;
};

}  // namespace agv
