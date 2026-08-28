// Tests du transport LoRa et du budget de rapport cyclique (brief §6, §11).
//
// Dossier d'architecture LoRa : ces tests ne compilent qu'une fois ce dossier
// complété par le cœur commun (voir ../README.md). Ils n'ont besoin, en plus
// des sources de ce dossier, que de `app/clock.h` et du cadre de test.
//
// La radio factice vit ici et non dans `fakes.h` : elle est spécifique à cette
// architecture.
#include <cstring>
#include <deque>
#include <vector>

#include "app/clock.h"
#include "test_framework.h"
#include "transport/duty_cycle.h"
#include "transport/lora_transport.h"

using namespace agv;

namespace {

// Radio LoRa factice : temps d'émission simulé, canal partagé entre deux
// instances pour rejouer un échange poste <-> AGV.
class FakeRadio final : public agv::ILoraRadio {
 public:
  explicit FakeRadio(agv::FakeClock& clock) : clock_(clock) {}

  bool begin(const agv::LoraConfig& cfg) override {
    cfg_ = cfg;
    started = true;
    return true;
  }

  bool transmit(const uint8_t* data, size_t len) override {
    if (tx_busy_) return false;
    last_tx.assign(data, data + len);
    ++tx_count;
    tx_busy_ = true;
    tx_end_ms_ = clock_.now_ms() + tx_duration_ms;
    listening = false;
    if (peer != nullptr && !drop_next) peer->deliver(last_tx);
    drop_next = false;
    return true;
  }

  bool tx_busy() const override {
    if (tx_busy_ && clock_.now_ms() >= tx_end_ms_) tx_busy_ = false;
    return tx_busy_;
  }

  void listen() override { listening = true; }

  bool receive(uint8_t* buf, size_t capacity, size_t& len, int16_t& rssi, int8_t& snr) override {
    if (inbox.empty()) return false;
    const auto& front = inbox.front();
    if (front.size() > capacity) {
      inbox.pop_front();
      return false;
    }
    std::memcpy(buf, front.data(), front.size());
    len = front.size();
    rssi = rssi_dbm;
    snr = snr_db;
    inbox.pop_front();
    return true;
  }

  uint32_t now_ms() const override { return clock_.now_ms(); }

  void deliver(const std::vector<uint8_t>& packet) { inbox.push_back(packet); }

  FakeRadio* peer = nullptr;
  bool started = false;
  bool listening = false;
  bool drop_next = false;
  uint32_t tx_count = 0;
  uint32_t tx_duration_ms = 60;
  int16_t rssi_dbm = -95;
  int8_t snr_db = 7;
  std::vector<uint8_t> last_tx;
  std::deque<std::vector<uint8_t>> inbox;

 private:
  agv::FakeClock& clock_;
  agv::LoraConfig cfg_{};
  mutable bool tx_busy_ = false;
  uint32_t tx_end_ms_ = 0;
};

}  // namespace

// --- Rapport cyclique -------------------------------------------------------

TEST(temps_d_antenne_lora_sf9_ordre_de_grandeur) {
  // SF9, BW 125 kHz, CR 4/5, 13 octets : ~165 ms (Semtech AN1200.13).
  //
  // CONSÉQUENCE À REMONTER AU PROJET : le §6 vise ~200 ms de latence typique.
  // À SF9, l'aller seul coûte déjà 165 ms d'antenne ; avec l'ACK, un
  // aller-retour tourne autour de 330 ms, et trois tentatives dépassent la
  // seconde, au-delà du « pire cas ~800 ms » annoncé. SF7 (~46 ms) tient la
  // cible ; c'est un arbitrage portée/latence à trancher avec le client.
  const uint32_t us = lora_airtime_us(13, 9, 125000, 5);
  CHECK(us > 140000u);
  CHECK(us < 200000u);
  CHECK(lora_airtime_us(13, 7, 125000, 5) < 60000u);
  // Un SF plus élevé coûte plus cher en temps d'antenne.
  CHECK(lora_airtime_us(13, 12, 125000, 5) > us);
}

TEST(budget_1_pourcent_refuse_au_dela) {
  // EN 300 220 / ERC 70-03 : 1 % sur 1 h = 36 s de temps d'antenne.
  DutyCycleBudget duty(10, 3600000);
  CHECK_EQ(duty.budget_us(), 36000000ull);

  uint32_t now = 0;
  const uint32_t airtime = 1000000;  // 1 s par émission, volontairement gros
  for (int i = 0; i < 36; ++i) {
    CHECK(duty.can_transmit(airtime, now));
    duty.record(airtime, now);
    now += 1000;
  }
  // Budget épuisé : l'émission suivante DOIT être refusée.
  CHECK(!duty.can_transmit(airtime, now));
  CHECK(duty.wait_ms(airtime, now) > 0u);
}

TEST(budget_se_libere_apres_la_fenetre_glissante) {
  DutyCycleBudget duty(10, 3600000);
  duty.record(36000000, 0);
  CHECK(!duty.can_transmit(1000, 1000));
  // Une heure plus tard, la fenêtre a glissé.
  CHECK(duty.can_transmit(1000, 3600001));
  CHECK_EQ(duty.used_us(3600001), 0ull);
}

TEST(budget_nul_interdit_toute_emission) {
  DutyCycleBudget duty(0, 3600000);
  CHECK(!duty.can_transmit(1, 0));
}

// --- LoRa -------------------------------------------------------------------

namespace {

struct LoraBench {
  HardwareProfile profile = default_profile();
  LoraConfig lora;
  FakeClock clock;
  FakeRadio radio{clock};
  LoraTransport transport{profile, lora, radio};

  void run(uint32_t duration_ms, uint32_t step_ms = 10) {
    for (uint32_t t = 0; t < duration_ms; t += step_ms) {
      transport.tick();
      clock.advance_ms(step_ms);
    }
    transport.tick();
  }
};

Frame make_goto(uint8_t seq, uint16_t station) {
  Frame f;
  f.ver = 1;
  f.type = FrameType::CmdGoto;
  f.node_id = 42;
  f.seq = seq;
  f.station = station;
  f.speed = 4;
  return f;
}

}  // namespace

TEST(lora_emission_puis_fenetre_d_ecoute_d_ack) {
  LoraBench b;
  b.transport.begin();
  CHECK(b.radio.started);
  CHECK(b.radio.listening);  // écoute de télémétrie par défaut

  CHECK(b.transport.send(make_goto(1, 12)));
  b.run(10);
  CHECK_EQ(b.radio.tx_count, 1u);
  CHECK(!b.radio.listening);  // half-duplex : pas d'écoute pendant l'émission

  b.run(200);
  CHECK(b.transport.tx_state() == LoraTransport::TxState::AwaitAck);
  CHECK(b.radio.listening);  // fenêtre d'ACK ouverte après l'émission
}

TEST(lora_ack_recu_termine_la_transaction) {
  LoraBench b;
  b.transport.begin();
  b.transport.send(make_goto(7, 3));
  b.run(150);

  // Le pair renvoie un ACK sur la même séquence.
  Frame ack;
  ack.ver = 1;
  ack.type = FrameType::Ack;
  ack.node_id = 1;
  ack.seq = 7;
  uint8_t buf[kSecurePacketMax];
  const size_t len = b.transport.channel().seal(ack, buf, sizeof(buf));
  b.radio.deliver(std::vector<uint8_t>(buf, buf + len));

  b.run(50);
  CHECK(b.transport.tx_state() == LoraTransport::TxState::Idle);
  CHECK_EQ(b.transport.retries(), 1u);
  CHECK(b.transport.health().last_ack_ms > 0u);
  // L'ACK est consommé par le transport : le métier ne le voit pas.
  Frame out;
  CHECK(!b.transport.poll(out));
}

TEST(lora_sans_ack_retransmet_puis_declare_la_liaison_perdue) {
  LoraBench b;
  b.transport.begin();
  b.transport.send(make_goto(9, 3));
  b.run(3000);

  // 3 tentatives par défaut (§6), pire cas ~800 ms.
  CHECK_EQ(b.radio.tx_count, b.lora.max_tries);
  CHECK(b.transport.tx_state() == LoraTransport::TxState::Idle);
  CHECK(!b.transport.health().up);
}

TEST(lora_refuse_d_emettre_au_dela_du_budget_legal) {
  LoraBench b;
  // Budget volontairement minuscule : une seule émission passe.
  (void)b;
  LoraBench small;
  small.lora.duty_cycle_permille = 10;
  small.lora.duty_window_ms = 1000;  // 1 % de 1 s = 10 ms d'antenne
  LoraTransport transport(small.profile, small.lora, small.radio);
  transport.begin();

  transport.send(make_goto(1, 5));
  for (int i = 0; i < 50; ++i) {
    transport.tick();
    small.clock.advance_ms(10);
  }
  // Le temps d'antenne SF9 (~60 ms) dépasse le budget : rien n'est émis.
  CHECK_EQ(small.radio.tx_count, 0u);
  CHECK(transport.duty_blocked());
  CHECK(transport.health().tx_refused_duty > 0u);
}

TEST(lora_trame_corrompue_comptee_et_ignoree) {
  LoraBench b;
  b.transport.begin();
  Frame f = make_goto(3, 8);
  uint8_t buf[kSecurePacketMax];
  const size_t len = b.transport.channel().seal(f, buf, sizeof(buf));
  buf[len - 1] ^= 0xFF;  // corruption
  b.radio.deliver(std::vector<uint8_t>(buf, buf + len));
  b.run(20);

  Frame out;
  CHECK(!b.transport.poll(out));
  CHECK(b.transport.health().rx_bad_crc > 0u);
}

TEST(lora_declare_l_ordre_garanti_et_pas_de_controle_de_fraicheur) {
  LoraBench b;
  CHECK(b.transport.ordered());
  CHECK_EQ(b.transport.max_command_age_s(), 0u);
}

