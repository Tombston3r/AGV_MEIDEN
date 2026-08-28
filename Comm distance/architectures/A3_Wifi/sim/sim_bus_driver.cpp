#include "sim_bus_driver.h"

namespace agv::sim {

bool SimBusDriver::begin() {
  // Brief §3.1 : à la mise sous tension le bus X est physiquement à zéro avant
  // que le firmware ne prenne la main. Le simulateur impose la même hypothèse.
  x_word_ = 0;
  // Toutes les entrées au repos LOGIQUE : en logique inverse (§12.3) cela
  // correspond à un mot électrique à 1, pas à 0. Laisser 0 ferait apparaître
  // un défaut Y03 fantôme dès le boot.
  y_raw_word_ = profile_.bus.y_active_high ? 0u : ((1u << 21) - 1u);
  has_destination_ = false;
  moving_ = false;
  set_y(agv::y::Y21, true);  // pas de destination programmée
  set_y(agv::y::Y10, true);  // à l'arrêt, en station
  debouncer_.set_debounce_us(profile_.bus.y_debounce_us);
  debouncer_.reset(y_raw_word_, now_us_);
  return true;
}

bool SimBusDriver::x(uint8_t bit) const {
  const bool level = agv::bit_set(x_word_, bit);
  return profile_.bus.x_active_high ? level : !level;
}

bool SimBusDriver::y_raw(uint8_t bit) const { return agv::bit_set(y_raw_word_, bit); }

void SimBusDriver::set_y(uint8_t bit, bool value) {
  // La polarité Y (§12.3) est appliquée à la lecture par le driver réel ; ici
  // le mot brut représente déjà le niveau électrique.
  const bool level = profile_.bus.y_active_high ? value : !value;
  y_raw_word_ = agv::with_bit(y_raw_word_, bit, level);
}

void SimBusDriver::bounce(uint8_t bit) {
  if (timings_.y_bounce_us == 0) return;
  bounce_mask_ |= (1u << bit);
  bounce_until_us_ = now_us_ + timings_.y_bounce_us;
}

bool SimBusDriver::writeX(uint32_t word) {
  const uint32_t previous = x_word_;
  x_word_ = word;
  ++stats_.x_writes;
  stats_.last_write_us = 0;  // pose immédiate en simulation
  on_x_changed(previous, word);
  return true;
}

void SimBusDriver::on_x_changed(uint32_t previous, uint32_t current) {
  const auto rose = [&](uint8_t bit) {
    const bool before = profile_.bus.x_active_high ? agv::bit_set(previous, bit)
                                                   : !agv::bit_set(previous, bit);
    const bool after = profile_.bus.x_active_high ? agv::bit_set(current, bit)
                                                  : !agv::bit_set(current, bit);
    return !before && after;
  };
  const auto fell = [&](uint8_t bit) {
    const bool before = profile_.bus.x_active_high ? agv::bit_set(previous, bit)
                                                   : !agv::bit_set(previous, bit);
    const bool after = profile_.bus.x_active_high ? agv::bit_set(current, bit)
                                                  : !agv::bit_set(current, bit);
    return before && !after;
  };

  // Toute modification des données (adresse, vitesse, X92, X94) relance le
  // compte de stabilisation : l'automate exige `required_setup_us` avant X93.
  uint32_t data_mask = 0;
  for (size_t i = 0; i < agv::kStationBits; ++i) data_mask |= 1u << layout_.x_station_bits[i];
  for (size_t i = 0; i < agv::kSpeedBits; ++i) data_mask |= 1u << layout_.x_speed_bits[i];
  data_mask |= (1u << agv::x::X92) | (1u << agv::x::X94);
  if ((previous & data_mask) != (current & data_mask)) {
    last_data_change_us_ = now_us_;
  }

  if (rose(agv::x::X93)) {
    ++strobe_count_;
    const bool setup_ok = (now_us_ - last_data_change_us_) >= timings_.required_setup_us;
    const bool switch_ok = x(agv::x::X92);
    const bool drop = timings_.drop_every_nth_y22 != 0 &&
                      (strobe_count_ % timings_.drop_every_nth_y22) == 0;
    if (!setup_ok) ++setup_violations_;
    if (setup_ok && switch_ok && !drop) {
      // Polarité §12.3 : en logique inverse, un 0 électrique vaut un 1 logique.
      const uint32_t logical = profile_.bus.x_active_high ? x_word_ : ~x_word_;
      destination_ = static_cast<uint16_t>(
          agv::decode_field(logical, layout_.x_station_bits, agv::kStationBits));
      speed_ = static_cast<uint8_t>(
          agv::decode_field(logical, layout_.x_speed_bits, agv::kSpeedBits));
      y22_due_us_ = now_us_ + timings_.y22_delay_us;
    }
  }
  if (fell(agv::x::X93)) {
    // L'automate relâche « instruction reading complete » à la retombée.
    set_y(agv::y::Y22, false);
    y22_due_us_ = kNever;
  }

  if (rose(agv::x::X82)) {
    ++start_count_;
    const bool drop = timings_.drop_every_nth_y05 != 0 &&
                      (start_count_ % timings_.drop_every_nth_y05) == 0;
    if (has_destination_ && !drop && !timings_.force_fault_y03) {
      y05_due_us_ = now_us_ + timings_.y05_delay_us;
    }
  }
  if (rose(agv::x::X83)) {
    ++stop_count_;
    // Arrêt demandé : l'AGV termine sa course jusqu'au point d'arrêt suivant.
    if (moving_) {
      destination_ = static_cast<uint16_t>(position_ + (destination_ > position_ ? 1 : 0));
    }
  }

  // Échos d'aiguillage et de sens (Y15 / Y20), recopie immédiate.
  set_y(agv::y::Y15, x(agv::x::X84));
  set_y(agv::y::Y20, x(agv::x::X85));
}

void SimBusDriver::step_events() {
  if (y22_due_us_ != kNever && now_us_ >= y22_due_us_) {
    y22_due_us_ = kNever;
    has_destination_ = true;
    set_y(agv::y::Y22, true);
    set_y(agv::y::Y21, false);
    bounce(agv::y::Y22);
  }
  if (y05_due_us_ != kNever && now_us_ >= y05_due_us_) {
    y05_due_us_ = kNever;
    if (destination_ != position_) {
      moving_ = true;
      set_y(agv::y::Y05, true);
      set_y(agv::y::Y10, false);
      bounce(agv::y::Y05);
      next_step_due_us_ = now_us_ + timings_.travel_per_station_us;
    } else {
      // Déjà à destination : arrivée immédiate.
      y10_due_us_ = now_us_ + timings_.y10_delay_us;
    }
  }
  if (moving_ && next_step_due_us_ != kNever && now_us_ >= next_step_due_us_) {
    if (position_ < destination_) ++position_;
    else if (position_ > destination_) --position_;
    if (position_ == destination_) {
      moving_ = false;
      next_step_due_us_ = kNever;
      set_y(agv::y::Y05, false);
      y10_due_us_ = now_us_ + timings_.y10_delay_us;
    } else {
      next_step_due_us_ = now_us_ + timings_.travel_per_station_us;
    }
  }
  if (y10_due_us_ != kNever && now_us_ >= y10_due_us_) {
    y10_due_us_ = kNever;
    set_y(agv::y::Y10, true);
    bounce(agv::y::Y10);
    // La destination atteinte est consommée : l'AGV ne connaît plus qu'une
    // seule destination à la fois (brief §1) : d'où la file portée par la carte.
    has_destination_ = false;
    set_y(agv::y::Y21, true);
  }

  set_y(agv::y::Y03, timings_.force_fault_y03);
  if (timings_.force_no_destination) set_y(agv::y::Y21, true);

  if (bounce_until_us_ != kNever && now_us_ >= bounce_until_us_) {
    bounce_until_us_ = kNever;
    bounce_mask_ = 0;
  }

  // Vitesse courante recopiée sur Y11…Y14 quand l'AGV roule.
  // set_y applique la polarité §12.3 bit à bit : ne pas court-circuiter par un
  // encode_field direct sur le mot électrique.
  const uint8_t reported_speed = moving_ ? speed_ : 0;
  for (size_t i = 0; i < agv::kSpeedBits; ++i) {
    set_y(layout_.y_speed_bits[i], ((reported_speed >> i) & 1u) != 0u);
  }
  for (size_t i = 0; i < agv::kStationBits; ++i) {
    set_y(layout_.y_station_bits[i], ((position_ >> i) & 1u) != 0u);
  }
}

void SimBusDriver::advance(uint64_t delta_us) {
  // Pas fixe court : suffisant pour les tests, et surtout déterministe.
  constexpr uint64_t kSlice = 50;
  uint64_t remaining = delta_us;
  while (remaining > 0) {
    const uint64_t slice = remaining < kSlice ? remaining : kSlice;
    now_us_ += slice;
    remaining -= slice;
    step_events();
  }
}

uint32_t SimBusDriver::readY() {
  ++stats_.y_reads;
  step_events();
  uint32_t raw = y_raw_word_;
  // Pendant la fenêtre de rebond, les bits concernés oscillent.
  if (bounce_until_us_ != kNever && ((now_us_ / 100) & 1u)) {
    raw ^= bounce_mask_;
  }
  // On rend le mot ÉLECTRIQUE : la conversion en logique applicative appartient
  // au séquenceur (comme pour un driver matériel), sinon la polarité §12.3
  // serait appliquée deux fois.
  return debouncer_.update(raw, now_us_);
}

bool SimBusDriver::pulse(uint8_t x_bit, uint32_t duration_us) {
  uint32_t word = x_word_;
  writeX(agv::with_bit(word, x_bit, profile_.bus.x_active_high));
  advance(duration_us);
  writeX(agv::with_bit(x_word_, x_bit, !profile_.bus.x_active_high));
  return true;
}

}  // namespace agv::sim
