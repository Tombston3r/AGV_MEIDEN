// Simulateur logiciel de l'automate MEIDEN (brief §10, niveau 1).
//
// Implémentation de IBusDriver qui rejoue le comportement de l'automate :
// accusés Y22 / Y05 / Y10 avec délais paramétrables, position courante sur
// Y23…Y34, injection de timeouts, de défauts Y03 et de rebonds.
// Permet de développer et tester 90 % du logiciel sans matériel, en temps
// simulé : aucun appel bloquant, aucune horloge réelle.
#pragma once

#include <cstdint>

#include "bus/bus_signals.h"
#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "timing_profile.h"

namespace agv::sim {

class SimBusDriver final : public agv::IBusDriver {
 public:
  SimBusDriver(const agv::HardwareProfile& profile, const TimingProfile& timings)
      : profile_(profile),
        timings_(timings),
        layout_(agv::kDefaultLayout),
        debouncer_(profile.bus.y_debounce_us) {}

  // --- IBusDriver ---------------------------------------------------------
  bool begin() override;
  bool writeX(uint32_t word) override;
  uint32_t lastX() const override { return x_word_; }
  uint32_t readY() override;
  bool pulse(uint8_t x_bit, uint32_t duration_us) override;
  const char* name() const override { return "sim"; }
  const agv::BusStats& stats() const override { return stats_; }
  uint64_t now_us() const override { return now_us_; }

  // --- Pilotage du temps simulé -------------------------------------------
  // Avance l'horloge simulée et fait évoluer l'automate. Aucun `delay()` :
  // c'est le test (ou la boucle du banc) qui décide du pas.
  void advance(uint64_t delta_us);

  // Brochage/ordre des bits (§12.2) : le simulateur suit la même table que le
  // séquenceur, sinon le banc validerait un mapping qui n'est pas celui posé.
  void set_layout(const agv::BusLayout& layout) { layout_ = layout; }

  // --- Observation / injection --------------------------------------------
  uint16_t position() const { return position_; }
  void set_position(uint16_t station) { position_ = station & agv::kStationMax; }
  uint16_t programmed_destination() const { return destination_; }
  bool moving() const { return moving_; }
  TimingProfile& timings() { return timings_; }
  const TimingProfile& timings() const { return timings_; }

  // Compteurs d'observation utiles aux tests d'intégration.
  uint32_t strobe_count() const { return strobe_count_; }
  uint32_t setup_violations() const { return setup_violations_; }
  uint32_t start_count() const { return start_count_; }
  uint32_t stop_count() const { return stop_count_; }

 private:
  static constexpr uint64_t kNever = UINT64_MAX;

  bool x(uint8_t bit) const;              // valeur logique, polarité appliquée
  void set_y(uint8_t bit, bool value);    // pose une entrée dans le mot brut
  bool y_raw(uint8_t bit) const;
  void on_x_changed(uint32_t previous, uint32_t current);
  void step_events();
  void bounce(uint8_t bit);

  const agv::HardwareProfile& profile_;
  TimingProfile timings_;
  agv::BusLayout layout_;
  agv::YDebouncer debouncer_;
  agv::BusStats stats_{};

  uint64_t now_us_ = 0;
  uint32_t x_word_ = 0;
  uint32_t y_raw_word_ = 0;

  // État interne de l'automate simulé.
  uint16_t position_ = 0;
  uint16_t destination_ = 0;
  bool has_destination_ = false;
  uint8_t speed_ = 0;
  bool moving_ = false;

  uint64_t last_data_change_us_ = 0;
  uint64_t y22_due_us_ = kNever;
  uint64_t y05_due_us_ = kNever;
  uint64_t y10_due_us_ = kNever;
  uint64_t next_step_due_us_ = kNever;
  uint64_t bounce_until_us_ = kNever;
  uint32_t bounce_mask_ = 0;

  uint32_t strobe_count_ = 0;
  uint32_t start_count_ = 0;
  uint32_t stop_count_ = 0;
  uint32_t setup_violations_ = 0;
};

}  // namespace agv::sim
