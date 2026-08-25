// Horloges injectées : monotone (ms) et murale (s).
//
// La distinction est essentielle : les timeouts utilisent la monotone (jamais
// perturbée par un recalage), la fraîcheur des commandes et la validité des
// courses persistées utilisent la murale. Sans horloge murale synchronisée,
// `now_s()` retourne 0 et les contrôles associés se déclarent inopérants
// plutôt que de trancher sur une base fausse.
#pragma once

#include <cstdint>

namespace agv {

class IClock {
 public:
  virtual ~IClock() = default;
  virtual uint32_t now_ms() const = 0;
  virtual uint32_t now_s() const = 0;  // 0 si l'horloge murale n'est pas sûre
};

// Horloge de test, avançable à la main.
class FakeClock final : public IClock {
 public:
  uint32_t now_ms() const override { return ms_; }
  uint32_t now_s() const override { return wall_s_; }
  void advance_ms(uint32_t delta) {
    ms_ += delta;
    if (wall_s_ != 0) wall_s_ += delta / 1000u;
  }
  void set_wall_s(uint32_t s) { wall_s_ = s; }

 private:
  uint32_t ms_ = 0;
  uint32_t wall_s_ = 0;
};

}  // namespace agv
