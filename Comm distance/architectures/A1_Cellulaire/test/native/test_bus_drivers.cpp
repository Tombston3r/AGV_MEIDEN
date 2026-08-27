// Tests des trois variantes matérielles d'interface bus (brief §4.4, §12.10).
//
// Elles sont interchangeables derrière IBusDriver ; ces tests vérifient les
// propriétés qui les distinguent : c'est ce qui doit rester vrai quel que soit
// le choix final du client.
#include <map>
#include <utility>
#include <vector>

#include "bus/mcp23017_bus.h"
#include "bus/mega_uart_bus.h"
#include "bus/shift_bus.h"
#include "fakes.h"
#include "proto/crc16.h"
#include "test_framework.h"

using namespace agv;

namespace {

class FakeClockUs final : public IMicroClock {
 public:
  uint64_t now_us() const override { return us_; }
  void delay_us(uint32_t us) override { us_ += us; }
  void advance(uint32_t us) { us_ += us; }

 private:
  uint64_t us_ = 0;
};

struct I2cOp {
  uint8_t addr;
  uint8_t reg;
  std::vector<uint8_t> data;
};

class FakeI2c final : public II2cBus {
 public:
  bool write_reg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) override {
    if (fail) return false;
    writes.push_back({addr, reg, std::vector<uint8_t>(data, data + len)});
    return true;
  }
  bool read_reg(uint8_t addr, uint8_t reg, uint8_t* out, size_t len) override {
    (void)addr;
    (void)reg;
    for (size_t i = 0; i < len; ++i) out[i] = read_value[i % read_value.size()];
    return !fail;
  }
  std::vector<I2cOp> writes;
  std::vector<uint8_t> read_value{0x00, 0x00};
  bool fail = false;
};

class FakeSpi final : public ISpiBus {
 public:
  bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) override {
    if (tx != nullptr) transfers.push_back(std::vector<uint8_t>(tx, tx + len));
    if (rx != nullptr) {
      for (size_t i = 0; i < len; ++i) rx[i] = (i < rx_value.size()) ? rx_value[i] : 0;
    }
    return !fail;
  }
  std::vector<std::vector<uint8_t>> transfers;
  std::vector<uint8_t> rx_value{0, 0, 0};
  bool fail = false;
};

class FakeGpio final : public IGpio {
 public:
  void set(uint8_t pin, bool level) override {
    levels[pin] = level;
    events.push_back({pin, level});
  }
  bool get(uint8_t pin) const override {
    const auto it = levels.find(pin);
    return it != levels.end() && it->second;
  }
  std::map<uint8_t, bool> levels;
  std::vector<std::pair<uint8_t, bool>> events;
};

}  // namespace

// --- MCP23017 ---------------------------------------------------------------

TEST(mcp23017_ecrit_GPIOA_et_GPIOB_en_une_seule_transaction) {
  // §4.4 : c'est ce qui ramène le décalage A/B au minimum. Deux transactions
  // séparées doubleraient le décalage entre moitiés du mot d'adresse.
  HardwareProfile profile = default_profile();
  FakeI2c i2c;
  FakeClockUs clock;
  Mcp23017Bus bus(profile, i2c, clock);
  bus.begin();
  i2c.writes.clear();

  CHECK(bus.writeX(0x2AAAAAu));
  // Un composant pour les bits 0..15, un pour 16..21 : deux écritures, chacune
  // de DEUX octets à partir de GPIOA.
  CHECK_EQ(i2c.writes.size(), 2u);
  for (const auto& op : i2c.writes) {
    CHECK_EQ(op.reg, kMcpGpioA);
    CHECK_EQ(op.data.size(), 2u);
  }
  CHECK_EQ(bus.lastX(), 0x2AAAAAu);
}

TEST(mcp23017_met_le_bus_a_zero_au_demarrage) {
  HardwareProfile profile = default_profile();
  FakeI2c i2c;
  FakeClockUs clock;
  Mcp23017Bus bus(profile, i2c, clock);
  CHECK(bus.begin());
  CHECK_EQ(bus.lastX(), 0u);
  bool zeroed = false;
  for (const auto& op : i2c.writes) {
    if (op.reg == kMcpOlatA && op.data[0] == 0 && op.data[1] == 0) zeroed = true;
  }
  CHECK(zeroed);
}

TEST(mcp23017_nack_i2c_est_compte_et_remonte) {
  // Une écriture I²C perdue ne doit jamais passer inaperçue : c'est un défaut
  // de pose du bus, donc un état sûr côté séquenceur.
  HardwareProfile profile = default_profile();
  FakeI2c i2c;
  FakeClockUs clock;
  Mcp23017Bus bus(profile, i2c, clock);
  bus.begin();
  i2c.fail = true;
  CHECK(!bus.writeX(0x1234u));
  CHECK_EQ(bus.stats().write_errors, 1u);
}

TEST(mcp23017_decalage_AB_reste_inferieur_au_t_setup) {
  // Garde-fou : si le t_setup retenu (§12.4) devenait inférieur au décalage
  // GPIOA/GPIOB, le strobe pourrait tomber avant la pose complète du mot.
  HardwareProfile profile = default_profile();
  FakeI2c i2c;
  FakeClockUs clock;
  Mcp23017Bus bus(profile, i2c, clock);
  CHECK(bus.ab_skew_us() < profile.bus.t_setup_us);
}

// --- 74HC595 / 74HC165 ------------------------------------------------------

TEST(shift595_pose_les_22_lignes_avec_un_seul_front_de_latch) {
  // La simultanéité vient du RCLK commun : un seul front par pose, jamais un
  // latch entre deux octets décalés.
  HardwareProfile profile = default_profile();
  FakeSpi spi;
  FakeGpio gpio;
  FakeClockUs clock;
  ShiftPins pins;
  ShiftBus bus(profile, spi, gpio, clock, pins);
  bus.begin();
  const uint32_t latches_before = bus.latch_count();
  spi.transfers.clear();

  CHECK(bus.writeX(0x155555u));
  CHECK_EQ(spi.transfers.size(), 1u);       // un seul décalage de 3 octets
  CHECK_EQ(spi.transfers[0].size(), 3u);
  CHECK_EQ(bus.latch_count(), latches_before + 1u);  // un seul front RCLK
}

TEST(shift595_valide_les_sorties_apres_avoir_mis_le_bus_a_zero) {
  // OE doit rester inactif tant que le registre n'est pas à zéro (§3.1).
  HardwareProfile profile = default_profile();
  FakeSpi spi;
  FakeGpio gpio;
  FakeClockUs clock;
  ShiftPins pins;
  ShiftBus bus(profile, spi, gpio, clock, pins);
  CHECK(bus.begin());

  int oe_low_index = -1;
  int first_latch_index = -1;
  for (size_t i = 0; i < gpio.events.size(); ++i) {
    if (gpio.events[i].first == pins.oe && !gpio.events[i].second && oe_low_index < 0) {
      oe_low_index = static_cast<int>(i);
    }
    if (gpio.events[i].first == pins.rclk && gpio.events[i].second && first_latch_index < 0) {
      first_latch_index = static_cast<int>(i);
    }
  }
  CHECK(first_latch_index >= 0);
  CHECK(oe_low_index > first_latch_index);  // sorties validées APRÈS le zéro
  CHECK_EQ(bus.lastX(), 0u);
}

TEST(shift595_lecture_Y_capture_puis_decale) {
  HardwareProfile profile = default_profile();
  profile.bus.y_debounce_us = 0;  // filtrage neutralisé pour ce test
  FakeSpi spi;
  FakeGpio gpio;
  FakeClockUs clock;
  ShiftBus bus(profile, spi, gpio, clock);
  bus.begin();
  spi.rx_value = {0x01, 0x02, 0x03};
  const uint32_t y = bus.readY();
  CHECK_EQ(y, 0x010203u & ((1u << 21) - 1u));
}

// --- ATmega2560 conservé ----------------------------------------------------

namespace {

// MEGA factice : répond aux requêtes selon le protocole documenté.
class FakeMega {
 public:
  explicit FakeMega(agv::test::FakeUart& uart) : uart_(uart) {}

  void pump() {
    std::string& in = uart_.written;
    while (in.size() >= 5) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(in.data());
      if (p[0] != kMegaSofRequest) {
        in.erase(0, 1);
        continue;
      }
      const uint8_t cmd = p[1];
      const uint8_t len = p[2];
      const size_t total = static_cast<size_t>(len) + 5u;
      if (in.size() < total) return;
      if (cmd == kMegaCmdSetX) {
        x = (static_cast<uint32_t>(p[3]) << 16) | (static_cast<uint32_t>(p[4]) << 8) | p[5];
        reply(cmd, nullptr, 0);
      } else if (cmd == kMegaCmdGetY) {
        const uint8_t payload[3] = {static_cast<uint8_t>((y >> 16) & 0xFFu),
                                    static_cast<uint8_t>((y >> 8) & 0xFFu),
                                    static_cast<uint8_t>(y & 0xFFu)};
        reply(cmd, payload, 3);
      } else if (cmd == kMegaCmdPing) {
        const uint8_t version = 3;
        reply(cmd, &version, 1);
      } else if (cmd == kMegaCmdPulse) {
        ++pulses;
        reply(cmd, nullptr, 0);
      }
      in.erase(0, total);
    }
  }

  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t pulses = 0;
  bool mute = false;

 private:
  void reply(uint8_t cmd, const uint8_t* payload, uint8_t len) {
    if (mute) return;
    uint8_t frame[16];
    size_t i = 0;
    frame[i++] = kMegaSofReply;
    frame[i++] = cmd;
    frame[i++] = len;
    for (uint8_t k = 0; k < len; ++k) frame[i++] = payload[k];
    const uint16_t crc = crc16_ccitt(frame + 1, i - 1);
    frame[i++] = static_cast<uint8_t>(crc >> 8);
    frame[i++] = static_cast<uint8_t>(crc & 0xFFu);
    uart_.inject(std::string(reinterpret_cast<const char*>(frame), i));
  }

  agv::test::FakeUart& uart_;
};

// Horloge qui pompe le MEGA factice à chaque attente : simule le temps réel de
// la liaison UART sans threads.
class PumpingClock final : public IMicroClock {
 public:
  PumpingClock(FakeMega& mega, agv::FakeClock& clock) : mega_(mega), clock_(clock) {}
  uint64_t now_us() const override { return static_cast<uint64_t>(clock_.now_ms()) * 1000ull; }
  void delay_us(uint32_t us) override {
    mega_.pump();
    clock_.advance_ms(us / 1000u + 1u);
  }

 private:
  FakeMega& mega_;
  agv::FakeClock& clock_;
};

}  // namespace

TEST(mega_uart_dialogue_complet_set_x_get_y) {
  HardwareProfile profile = default_profile();
  profile.bus.y_debounce_us = 0;
  FakeClock ms_clock;
  agv::test::FakeUart uart(ms_clock);
  FakeMega mega(uart);
  PumpingClock clock(mega, ms_clock);
  MegaUartBus bus(profile, uart, clock);

  CHECK(bus.begin());
  CHECK_EQ(bus.peer_version(), 3u);
  CHECK_EQ(mega.x, 0u);  // bus à zéro au démarrage (§3.1)

  CHECK(bus.writeX(0x0ABCDEu));
  CHECK_EQ(mega.x, 0x0ABCDEu);

  mega.y = 0x0155AAu;
  CHECK_EQ(bus.readY(), 0x0155AAu);
  CHECK_EQ(bus.desync_count(), 0u);
}

TEST(mega_uart_muet_est_detecte_sans_bloquer) {
  HardwareProfile profile = default_profile();
  FakeClock ms_clock;
  agv::test::FakeUart uart(ms_clock);
  FakeMega mega(uart);
  PumpingClock clock(mega, ms_clock);
  MegaUartBus bus(profile, uart, clock);
  bus.begin();

  mega.mute = true;
  CHECK(!bus.writeX(0x01u));  // échec borné, pas de blocage
  CHECK(bus.desync_count() > 0u);
  CHECK_EQ(bus.stats().write_errors, 1u);
}

TEST(mega_uart_trame_encodee_porte_un_crc16) {
  uint8_t payload[3] = {0x00, 0x12, 0x34};
  uint8_t frame[16];
  const size_t n = MegaUartBus::encode_request(kMegaCmdSetX, payload, 3, frame, sizeof(frame));
  CHECK_EQ(n, 8u);
  CHECK_EQ(frame[0], kMegaSofRequest);
  const uint16_t crc = crc16_ccitt(frame + 1, n - 3);
  CHECK_EQ(static_cast<uint16_t>((frame[n - 2] << 8) | frame[n - 1]), crc);
}
