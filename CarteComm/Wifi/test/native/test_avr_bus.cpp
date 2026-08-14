// Pose du bus par les ports de l'ATmega2560 (carte V5.0.1).
//
// Les registres AVR sont remplacés par de simples octets : c'est ce qui permet
// de vérifier en natif la propriété qui compte — la simultanéité de la pose des
// 22 lignes — sans avoir la carte sous la main.
#include "bus/avr_port_bus.h"
#include "test_framework.h"

using namespace agv;

namespace {

// Registres factices : un octet par PORTx / DDRx / PINx.
struct FakePort {
  uint8_t out = 0;
  uint8_t dir = 0;
  uint8_t in = 0;
};

class CountingCritical final : public ICriticalSection {
 public:
  void enter() override {
    ++depth;
    if (depth > max_depth) max_depth = depth;
    ++enters;
  }
  void leave() override { --depth; }

  int depth = 0;
  int max_depth = 0;
  uint32_t enters = 0;
};

class StepClock final : public IMicroClock {
 public:
  uint64_t now_us() const override { return us_; }
  void delay_us(uint32_t us) override { us_ += us; }
  void advance(uint32_t us) { us_ += us; }

 private:
  uint64_t us_ = 0;
};

struct AvrBench {
  HardwareProfile profile = default_profile();
  FakePort x0, x1, x2, y0, y1, y2;
  CountingCritical critical;
  StepClock clock;
  AvrBusPorts ports{};

  AvrBench() {
    ports.port_x[0] = {&x0.out, &x0.dir, &x0.in};
    ports.port_x[1] = {&x1.out, &x1.dir, &x1.in};
    ports.port_x[2] = {&x2.out, &x2.dir, &x2.in};
    ports.port_y[0] = {&y0.out, &y0.dir, &y0.in};
    ports.port_y[1] = {&y1.out, &y1.dir, &y1.in};
    ports.port_y[2] = {&y2.out, &y2.dir, &y2.in};
  }
  AvrPortBus make() { return AvrPortBus(profile, ports, critical, clock); }
  uint32_t word() const {
    return static_cast<uint32_t>(x0.out) | (static_cast<uint32_t>(x1.out) << 8) |
           (static_cast<uint32_t>(x2.out & 0x3Fu) << 16);
  }
};

}  // namespace

TEST(avr_bus_a_zero_avant_de_passer_les_broches_en_sortie) {
  // §3.1 : configurer les broches en sortie AVANT d'avoir posé l'état de repos
  // produirait une impulsion parasite sur les 22 lignes au démarrage, que
  // l'automate pourrait interpréter comme une commande.
  AvrBench b;
  AvrPortBus bus = b.make();
  CHECK(bus.begin());
  CHECK_EQ(b.word(), 0u);
  CHECK_EQ(b.x0.dir, 0xFFu);
  CHECK_EQ(b.x1.dir, 0xFFu);
  CHECK_EQ(b.x2.dir, 0x3Fu);  // 22 bits : 8 + 8 + 6
  // Les entrées Y restent en entrée.
  CHECK_EQ(b.y0.dir, 0x00u);
  CHECK_EQ(b.y1.dir, 0x00u);
  CHECK_EQ(b.y2.dir, 0x00u);
}

TEST(avr_les_22_lignes_sont_posees_dans_une_seule_section_critique) {
  // C'est LA propriété que la carte d'origine avait gratuitement : trois
  // écritures de port sans interruption entre elles. Une interruption au
  // milieu casserait la simultanéité et le strobe X93 pourrait tomber sur un
  // mot d'adresse incomplet.
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  const uint32_t before = b.critical.enters;

  CHECK(bus.writeX(0x2AAAAAu));
  CHECK_EQ(b.critical.enters - before, 1u);
  CHECK_EQ(b.critical.depth, 0);      // section refermée
  CHECK_EQ(b.critical.max_depth, 1);  // jamais imbriquée
  CHECK_EQ(b.word(), 0x2AAAAAu);
}

TEST(avr_les_bits_hauts_du_troisieme_port_ne_sont_pas_ecrases) {
  // PORTL ne porte que 6 des 22 lignes : les 2 bits restants peuvent servir à
  // autre chose sur la carte. Les écraser serait un effet de bord invisible.
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  b.x2.out |= 0xC0u;  // deux bits appartenant à une autre fonction

  bus.writeX(0x3FFFFFu);
  CHECK_EQ(b.x2.out & 0xC0u, 0xC0u);
  CHECK_EQ(b.x2.out & 0x3Fu, 0x3Fu);
}

TEST(avr_polarite_inverse_pose_l_etat_de_repos_a_un) {
  // §12.3 : en logique NPN, le repos est électriquement haut. Poser 0 au
  // démarrage activerait les 22 sorties.
  AvrBench b;
  b.profile.bus.x_active_high = false;
  AvrPortBus bus = b.make();
  CHECK(bus.begin());
  CHECK_EQ(b.word(), (1u << 22) - 1u);
}

TEST(avr_lecture_des_21_entrees) {
  AvrBench b;
  b.profile.bus.y_debounce_us = 0;  // filtrage neutralisé pour ce test
  AvrPortBus bus = b.make();
  bus.begin();
  b.y0.in = 0x03;
  b.y1.in = 0xAA;
  b.y2.in = 0x1F;
  CHECK_EQ(bus.readY(), 0x1FAA03u);
}

TEST(avr_lecture_ignore_les_bits_hors_des_21_entrees) {
  AvrBench b;
  b.profile.bus.y_debounce_us = 0;
  AvrPortBus bus = b.make();
  bus.begin();
  b.y2.in = 0xFF;  // seuls les 5 bits de poids faible sont des signaux Y
  CHECK_EQ(bus.readY() >> 16, 0x1Fu);
}

TEST(avr_mode_decouverte_active_une_seule_ligne) {
  // Sert au relevé de brochage SUB-D au multimètre, automate débranché : si
  // deux lignes bougeaient ensemble, le relevé serait inexploitable.
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();

  for (uint8_t bit = 0; bit < 22; ++bit) {
    CHECK(bus.drive_single(bit));
    const uint32_t w = b.word();
    CHECK_EQ(w, 1u << bit);
  }
  CHECK(!bus.drive_single(22));  // hors des 22 sorties
}

TEST(avr_impulsion_monte_puis_redescend) {
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  CHECK(bus.pulse(x::X93, 500));
  CHECK_EQ(b.word() & (1u << x::X93), 0u);  // retombée effective
  CHECK(bus.stats().x_writes >= 2u);
}

TEST(avr_duree_de_pose_est_instrumentee) {
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  bus.writeX(0x1234u);
  // Sans horloge qui avance pendant l'écriture, la durée mesurée est nulle :
  // ce qui compte est que le compteur existe et soit renseigné pour le banc.
  // begin() écrit les registres directement, sans passer par writeX().
  CHECK_EQ(bus.stats().x_writes, 1u);
  CHECK(bus.stats().max_write_us == bus.stats().last_write_us);
}
