// Pose du bus par les ports de l'ATmega2560, sur le CÂBLAGE RÉEL.
//
// Le banc reproduit la table de firmware/mega/src/board_ports.h avec de simples
// octets à la place des registres AVR. C'est ce qui permet de vérifier en natif
// les propriétés qui comptent — masquage des ports mixtes, regroupement des
// écritures — sans avoir la carte sous la main.
#include <vector>

#include "bus/avr_port_bus.h"
#include "test_framework.h"

using namespace agv;

namespace {

// Index des ports, identiques à ceux de board_ports.h.
enum : uint8_t { kA = 0, kB, kC, kD, kE, kF, kG, kH, kJ, kK, kL, kPortCount };

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

 private:
  uint64_t us_ = 0;
};

struct AvrBench {
  HardwareProfile profile = default_profile();
  FakePort port[kPortCount];
  CountingCritical critical;
  StepClock clock;
  AvrBusMap map{};

  AvrBench() {
    map.port_count = kPortCount;
    for (uint8_t p = 0; p < kPortCount; ++p) {
      map.ports[p] = {&port[p].out, &port[p].dir, &port[p].in};
    }
    // Câblage réel — voir firmware/mega/src/board_ports.h.
    map.x[0]  = {kL, 6};  map.x[1]  = {kA, 3};  map.x[2]  = {kA, 0};  map.x[3]  = {kB, 2};
    map.x[4]  = {kC, 7};  map.x[5]  = {kL, 1};  map.x[6]  = {kC, 6};  map.x[7]  = {kL, 3};
    map.x[8]  = {kA, 4};  map.x[9]  = {kL, 2};  map.x[10] = {kC, 4};  map.x[11] = {kB, 3};
    map.x[12] = {kG, 2};  map.x[13] = {kL, 0};  map.x[14] = {kG, 0};  map.x[15] = {kL, 5};
    map.x[16] = {kC, 2};  map.x[17] = {kL, 4};  map.x[18] = {kA, 6};  map.x[19] = {kC, 0};
    map.x[20] = {kA, 7};  map.x[21] = {kA, 2};

    map.y[0]  = {kB, 5};  map.y[1]  = {kJ, 0};  map.y[2]  = {kH, 6};  map.y[3]  = {kH, 0};
    map.y[4]  = {kH, 4};  map.y[5]  = {kD, 2};  map.y[6]  = {kE, 3};  map.y[7]  = {kD, 0};
    map.y[8]  = {kE, 5};  map.y[9]  = {kA, 1};  map.y[10] = {kE, 4};  map.y[11] = {kF, 1};
    map.y[12] = {kG, 5};  map.y[13] = {kF, 3};  map.y[14] = {kH, 3};  map.y[15] = {kF, 5};
    map.y[16] = {kH, 5};  map.y[17] = {kF, 7};  map.y[18] = {kB, 4};  map.y[19] = {kK, 1};
    map.y[20] = {kB, 6};
  }

  AvrPortBus make() { return AvrPortBus(profile, map, critical, clock); }

  // Relit le niveau ÉLECTRIQUE réellement présent sur les 22 sorties.
  //
  // En sortie poussée, il est dans PORTx. En collecteur ouvert, la broche ne
  // sort jamais de niveau haut : DDR à 1 tire à la masse (niveau 0), DDR à 0
  // relâche et c'est le tirage 6 V de l'automate qui fait le niveau 1.
  uint32_t posed_word() const {
    uint32_t w = 0;
    for (size_t i = 0; i < 22; ++i) {
      const BitLocation& loc = map.x[i];
      if (loc.port >= map.port_count) continue;  // ligne non câblée
      const bool level = profile.bus.x_open_drain
                             ? (((port[loc.port].dir >> loc.bit) & 1u) == 0u)
                             : (((port[loc.port].out >> loc.bit) & 1u) != 0u);
      if (level) w |= (1u << i);
    }
    return w;
  }
  void set_y(size_t index, bool level) {
    const BitLocation& loc = map.y[index];
    const uint8_t mask = static_cast<uint8_t>(1u << loc.bit);
    port[loc.port].in = level ? static_cast<uint8_t>(port[loc.port].in | mask)
                              : static_cast<uint8_t>(port[loc.port].in & ~mask);
  }
};

}  // namespace

TEST(avr_le_cablage_reel_occupe_onze_ports) {
  // Constat structurant : aucun champ du bus n'est aligné sur un port, donc
  // aucun `PORTx = valeur` ne peut poser une adresse complète.
  AvrBench b;
  bool used[kPortCount] = {};
  for (size_t i = 0; i < 22; ++i) used[b.map.x[i].port] = true;
  for (size_t i = 0; i < 21; ++i) used[b.map.y[i].port] = true;
  int count = 0;
  for (bool u : used) {
    if (u) ++count;
  }
  CHECK_EQ(count, 11);
}

TEST(avr_les_ports_mixtes_ne_voient_pas_leur_direction_ecrasee) {
  // LE piège de ce câblage. PORTA porte six sorties X et l'entrée Y21 (D23) ;
  // PORTB deux sorties et trois entrées ; PORTG deux sorties et Y24.
  // Un `DDRA = 0xFF` mettrait D23 en sortie face à l'automate : conflit franc.
  AvrBench b;
  b.profile.bus.x_open_drain = false;  // étage poussé : DDR fixe pour les X
  AvrPortBus bus = b.make();
  CHECK(bus.begin());

  // PORTA : bits 0,2,3,4,6,7 en sortie ; bit 1 (Y21) reste en ENTRÉE.
  CHECK_EQ(b.port[kA].dir, 0xDDu);
  CHECK_EQ(b.port[kA].dir & (1u << 1), 0u);
  // PORTB : bits 2,3 en sortie ; bits 4,5,6 (Y32/Y03/Y34) en entrée.
  CHECK_EQ(b.port[kB].dir, 0x0Cu);
  // PORTG : bits 0,2 en sortie ; bit 5 (Y24) en entrée.
  CHECK_EQ(b.port[kG].dir, 0x05u);
  // Ports d'entrée pure : aucune broche en sortie.
  CHECK_EQ(b.port[kH].dir, 0x00u);
  CHECK_EQ(b.port[kF].dir, 0x00u);
  CHECK_EQ(b.port[kJ].dir, 0x00u);
}

TEST(avr_bits_etrangers_d_un_port_mixte_sont_preserves) {
  // D13 (PORTB bit 7) et les bits libres n'appartiennent pas au bus : les
  // écraser serait un effet de bord invisible sur une carte partagée.
  AvrBench b;
  b.profile.bus.x_open_drain = false;
  AvrPortBus bus = b.make();
  bus.begin();
  b.port[kB].out |= 0x80u;  // usage étranger au bus
  b.port[kB].dir |= 0x80u;

  bus.writeX(0x3FFFFFu);
  CHECK_EQ(b.port[kB].out & 0x80u, 0x80u);
  CHECK_EQ(b.port[kB].dir & 0x80u, 0x80u);
}

TEST(avr_bus_a_zero_au_demarrage) {
  // §3.1 : état de repos posé AVANT le passage en sortie.
  AvrBench b;
  AvrPortBus bus = b.make();
  CHECK(bus.begin());
  CHECK_EQ(b.posed_word(), 0u);
  CHECK_EQ(bus.lastX(), 0u);
}

TEST(avr_pose_complete_en_une_seule_section_critique) {
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  const uint32_t before = b.critical.enters;

  CHECK(bus.writeX(0x2AAAAAu));
  CHECK_EQ(b.critical.enters - before, 1u);
  CHECK_EQ(b.critical.depth, 0);
  CHECK_EQ(b.critical.max_depth, 1);  // jamais imbriquée
  CHECK_EQ(b.posed_word(), 0x2AAAAAu);
}

TEST(avr_cinq_ecritures_de_port_suffisent_pour_les_22_sorties) {
  // Les sorties occupent PORTA, PORTB, PORTC, PORTG et PORTL. Cinq écritures à
  // 16 MHz ≈ 0,3 µs : trois ordres de grandeur sous le t_setup attendu, et sans
  // commune mesure avec les ~150 µs d'un expandeur I²C.
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  CHECK_EQ(bus.port_writes_per_pose(), 5u);
}

TEST(avr_adresse_10_bits_correctement_eclatee_sur_quatre_ports) {
  // Station 682 = 0b1010101010 : un bit sur deux, réparti sur PORTA/C/G/L.
  // C'est le test qui attrape une erreur de recopie dans la table de câblage.
  AvrBench b;
  b.profile.bus.x_open_drain = false;  // étage à MOSFET : la broche pilote une grille
  AvrPortBus bus = b.make();
  bus.begin();

  const BusLayout& layout = kDefaultLayout;
  const uint32_t word = encode_station(0, layout, 682);
  bus.writeX(word);

  CHECK_EQ(decode_field(b.posed_word(), layout.x_station_bits, kStationBits), 682u);

  // Vérification côté registres, en sortie poussée — le mode du matériel réel,
  // où la broche attaque une grille de MOSFET :
  // XA7 (bit 21) = PORTA bit 2 doit être à 1.
  CHECK(((b.port[kA].out >> 2) & 1u) == 1u);
  // X96 (bit 12) = PORTG bit 2 doit être à 0 (682 est pair).
  CHECK_EQ(b.port[kG].out & (1u << 2), 0u);
}

TEST(avr_polarite_inverse_pose_l_etat_de_repos_a_un) {
  // §12.3 : en logique NPN le repos est électriquement haut. Poser 0 au
  // démarrage activerait les 22 sorties.
  AvrBench b;
  b.profile.bus.x_open_drain = false;
  b.profile.bus.x_active_high = false;
  AvrPortBus bus = b.make();
  CHECK(bus.begin());
  CHECK_EQ(b.posed_word(), (1u << 22) - 1u);
  // Et seulement les bits du bus : PORTA bit 1 (entrée Y21) reste à 0.
  CHECK_EQ(b.port[kA].out & (1u << 1), 0u);
}

TEST(avr_lecture_reassemble_les_21_entrees_eparses) {
  AvrBench b;
  b.profile.bus.y_debounce_us = 0;
  AvrPortBus bus = b.make();
  bus.begin();

  // Position 341 = 0b0101010101 sur Y23…Y34, plus Y05 (moving).
  const BusLayout& layout = kDefaultLayout;
  const uint32_t expected = encode_field(1u << 1, layout.y_station_bits, kStationBits, 341);
  for (size_t i = 0; i < 21; ++i) b.set_y(i, (expected >> i) & 1u);

  const uint32_t read = bus.readY();
  CHECK_EQ(read, expected);
  CHECK_EQ(decode_position(read, layout), 341u);
}

TEST(avr_le_profil_est_en_sortie_poussee_car_l_etage_est_a_mosfet) {
  // Le projet KiCad montre 23 IRF520 avec résistances de grille : le collecteur
  // ouvert est fait par le MATÉRIEL. La broche du microcontrôleur attaque une
  // grille et doit donc être poussée — la laisser flotter mettrait le MOSFET
  // dans un état indéterminé, le pire cas sur un étage de puissance.
  CHECK(!default_profile().bus.x_open_drain);
}

// --- Mode collecteur ouvert côté microcontrôleur ---------------------------
//
// Conservé et testé : il reste le mode sûr pour toute carte SANS étage de
// sortie, où la broche attaquerait directement la ligne de l'automate.

TEST(avr_collecteur_ouvert_ne_sort_jamais_de_niveau_haut) {
  // C'est la protection : une sortie poussée contre un tirage côté automate
  // ferait remonter du courant dans la diode de protection de la broche.
  // En collecteur ouvert, les registres de données des X restent à 0 quoi
  // qu'il arrive — la broche tire à la masse ou se met en haute impédance.
  AvrBench b;
  b.profile.bus.x_open_drain = true;
  AvrPortBus bus = b.make();
  bus.begin();

  bus.writeX(0x3FFFFFu);
  for (size_t i = 0; i < 22; ++i) {
    const BitLocation& loc = b.map.x[i];
    CHECK_EQ(b.port[loc.port].out & (1u << loc.bit), 0u);
  }
  bus.writeX(0u);
  for (size_t i = 0; i < 22; ++i) {
    const BitLocation& loc = b.map.x[i];
    CHECK_EQ(b.port[loc.port].out & (1u << loc.bit), 0u);
  }
}

TEST(avr_collecteur_ouvert_tire_a_la_masse_ou_relache) {
  AvrBench b;
  b.profile.bus.x_open_drain = true;
  AvrPortBus bus = b.make();
  bus.begin();

  // Niveau électrique 0 sur X93 -> la broche tire à la masse (DDR à 1).
  bus.writeX(0u);
  const BitLocation& strobe = b.map.x[x::X93];
  CHECK((b.port[strobe.port].dir >> strobe.bit) & 1u);

  // Niveau électrique 1 -> haute impédance, c'est le tirage côté automate qui
  // fait le niveau haut (DDR à 0).
  bus.writeX(1u << x::X93);
  CHECK_EQ(b.port[strobe.port].dir & (1u << strobe.bit), 0u);
  CHECK_EQ(b.posed_word(), 1u << x::X93);
}

TEST(avr_collecteur_ouvert_ne_met_jamais_une_entree_en_sortie) {
  // Le mode collecteur ouvert manipule DDR en permanence : c'est justement le
  // registre qu'il ne faut pas écraser sur les ports mixtes.
  AvrBench b;
  b.profile.bus.x_open_drain = true;
  AvrPortBus bus = b.make();
  bus.begin();

  for (uint32_t word : {0u, 0x3FFFFFu, 0x2AAAAAu, 0x155555u}) {
    bus.writeX(word);
    for (size_t i = 0; i < 21; ++i) {
      const BitLocation& loc = b.map.y[i];
      CHECK_EQ(b.port[loc.port].dir & (1u << loc.bit), 0u);
    }
  }
}

TEST(avr_pull_ups_des_entrees_suivent_le_profil) {
  // §12.1 : sorties automate à collecteur ouvert -> pull-up indispensable ;
  // sorties poussées -> pull-up nuisible. Ce n'est pas devinable.
  AvrBench sans;
  AvrPortBus bus_sans = sans.make();
  bus_sans.begin();
  CHECK_EQ(sans.port[kH].out, 0x00u);

  AvrBench avec;
  avec.profile.bus.y_pullups = true;
  AvrPortBus bus_avec = avec.make();
  bus_avec.begin();
  // PORTH porte Y10, Y11, Y12, Y26, Y30 : bits 0,3,4,5,6.
  CHECK_EQ(avec.port[kH].out, 0x79u);
  // Le pull-up ne doit jamais toucher une broche de sortie. En collecteur
  // ouvert les données des X sont à 0, donc PORTA ne porte QUE le pull-up Y21.
  CHECK_EQ(avec.port[kA].out & (1u << 1), (1u << 1));
  CHECK_EQ(avec.port[kA].out & ~(1u << 1), 0u);
}

TEST(avr_mode_decouverte_active_une_seule_ligne) {
  // Sert au contrôle du brochage au multimètre, automate débranché : si deux
  // lignes bougeaient ensemble, le relevé serait inexploitable.
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();

  for (uint8_t bit = 0; bit < 22; ++bit) {
    CHECK(bus.drive_single(bit));
    CHECK_EQ(b.posed_word(), 1u << bit);
  }
  CHECK(!bus.drive_single(22));
}

TEST(avr_impulsion_monte_puis_redescend) {
  AvrBench b;
  AvrPortBus bus = b.make();
  bus.begin();
  CHECK(bus.pulse(x::X93, 500));
  CHECK_EQ(b.posed_word() & (1u << x::X93), 0u);
  CHECK(bus.stats().x_writes >= 2u);
}

TEST(avr_une_ligne_non_cablee_est_ignoree_sans_planter) {
  // D27 est câblé jusqu'au SUB-D mais non connecté côté AGV ; d'autres lignes
  // pourraient l'être un jour. Une entrée `kUnwired` ne doit ni écrire ailleurs
  // ni faire échouer la pose.
  AvrBench b;
  b.map.x[11] = {kUnwired, 0};  // X95 déclaré non câblé
  AvrPortBus bus = b.make();
  CHECK(bus.begin());
  CHECK(bus.writeX(0x3FFFFFu));
  CHECK_EQ(b.posed_word() & (1u << 11), 0u);   // X95 reste inactif
  CHECK_EQ(b.posed_word() | (1u << 11), 0x3FFFFFu);  // tout le reste est posé
  CHECK(!bus.drive_single(11));
}
