// Anti-rebond des 21 entrées Y, partagé par toutes les implémentations de
// IBusDriver (matérielles et simulée).
//
// La durée dépend de l'amplitude réelle des lignes Y : 6 V rail LM7806 ou 24 V,
// non relevée (§12.1). Elle vient donc du profil, jamais d'une constante.
#pragma once

#include <cstdint>

namespace agv {

class YDebouncer {
 public:
  explicit YDebouncer(uint32_t debounce_us) : debounce_us_(debounce_us) {}

  // `raw` : mot brut lu sur les entrées ; retourne le mot stabilisé.
  uint32_t update(uint32_t raw, uint64_t now_us) {
    if (debounce_us_ == 0) {
      stable_ = raw;
      candidate_ = raw;
      return stable_;
    }
    if (raw != candidate_) {
      candidate_ = raw;
      candidate_since_us_ = now_us;
      return stable_;
    }
    if (candidate_ != stable_ && (now_us - candidate_since_us_) >= debounce_us_) {
      stable_ = candidate_;
    }
    return stable_;
  }

  uint32_t stable() const { return stable_; }
  void reset(uint32_t value, uint64_t now_us) {
    stable_ = value;
    candidate_ = value;
    candidate_since_us_ = now_us;
  }
  void set_debounce_us(uint32_t v) { debounce_us_ = v; }
  uint32_t debounce_us() const { return debounce_us_; }

 private:
  uint32_t debounce_us_;
  uint32_t stable_ = 0;
  uint32_t candidate_ = 0;
  uint64_t candidate_since_us_ = 0;
};

}  // namespace agv
