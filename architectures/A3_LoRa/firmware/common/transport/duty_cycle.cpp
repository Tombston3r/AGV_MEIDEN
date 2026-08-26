#include "transport/duty_cycle.h"

namespace agv {

uint32_t lora_airtime_us(size_t payload_bytes, uint8_t sf, uint32_t bandwidth_hz, uint8_t cr,
                         uint8_t preamble_symbols, bool explicit_header, bool crc_on,
                         bool low_data_rate_optimize) {
  if (sf < 6 || sf > 12 || bandwidth_hz == 0) return 0;
  // Durée d'un symbole : Ts = 2^SF / BW.
  const double ts_s = static_cast<double>(1u << sf) / static_cast<double>(bandwidth_hz);

  const int de = low_data_rate_optimize ? 1 : 0;
  const int ih = explicit_header ? 0 : 1;
  const int crc = crc_on ? 1 : 0;
  const int cr_n = (cr >= 5 && cr <= 8) ? (cr - 4) : 1;

  // Nombre de symboles utiles (Semtech AN1200.13).
  const double numerator = 8.0 * static_cast<double>(payload_bytes) - 4.0 * sf + 28.0 +
                           16.0 * crc - 20.0 * ih;
  const double denominator = 4.0 * (sf - 2.0 * de);
  double n_payload = 0.0;
  if (numerator > 0.0) {
    // ceil(numerator/denominator) * (cr+4)
    const double ratio = numerator / denominator;
    double ceil_ratio = static_cast<double>(static_cast<long long>(ratio));
    if (ceil_ratio < ratio) ceil_ratio += 1.0;
    n_payload = ceil_ratio * static_cast<double>(cr_n + 4);
  }
  n_payload += 8.0;

  const double t_preamble = (static_cast<double>(preamble_symbols) + 4.25) * ts_s;
  const double t_payload = n_payload * ts_s;
  return static_cast<uint32_t>((t_preamble + t_payload) * 1e6);
}

void DutyCycleBudget::prune(uint32_t now_ms) const {
  size_t keep = 0;
  for (size_t i = 0; i < count_; ++i) {
    // Soustraction non signée : gère le débordement du compteur de millisecondes.
    if (static_cast<uint32_t>(now_ms - events_[i].at_ms) < window_ms_) {
      events_[keep++] = events_[i];
    }
  }
  count_ = keep;
}

uint64_t DutyCycleBudget::used_us(uint32_t now_ms) const {
  prune(now_ms);
  uint64_t total = 0;
  for (size_t i = 0; i < count_; ++i) total += events_[i].airtime_us;
  return total;
}

bool DutyCycleBudget::can_transmit(uint32_t airtime_us, uint32_t now_ms) const {
  if (permille_ == 0) return false;  // budget nul : bande interdite
  return (used_us(now_ms) + airtime_us) <= budget_us();
}

void DutyCycleBudget::record(uint32_t airtime_us, uint32_t now_ms) {
  prune(now_ms);
  if (count_ >= kMaxEvents) {
    // Tampon plein : on fusionne dans l'événement le plus ancien plutôt que de
    // perdre la consommation. Sous-estimer le budget serait une infraction.
    events_[0].airtime_us += airtime_us;
    return;
  }
  events_[count_++] = Event{now_ms, airtime_us};
}

uint32_t DutyCycleBudget::used_permille_of_budget(uint32_t now_ms) const {
  const uint64_t budget = budget_us();
  if (budget == 0) return 1000;
  return static_cast<uint32_t>((used_us(now_ms) * 1000ull) / budget);
}

uint32_t DutyCycleBudget::wait_ms(uint32_t airtime_us, uint32_t now_ms) const {
  if (can_transmit(airtime_us, now_ms)) return 0;
  prune(now_ms);
  // On libère les émissions les plus anciennes jusqu'à retomber sous le budget.
  uint64_t used = used_us(now_ms);
  const uint64_t budget = budget_us();
  for (size_t i = 0; i < count_; ++i) {
    used -= events_[i].airtime_us;
    if (used + airtime_us <= budget) {
      const uint32_t age = now_ms - events_[i].at_ms;
      return (age >= window_ms_) ? 0u : (window_ms_ - age);
    }
  }
  return window_ms_;
}

void DutyCycleBudget::reset() {
  count_ = 0;
  refusals_ = 0;
}

}  // namespace agv
