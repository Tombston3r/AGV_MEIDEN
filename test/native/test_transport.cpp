// Tests des transports : budget de rapport cyclique, half-duplex LoRa,
// pile AT SMS et dégradations (brief §11).
#include "fakes.h"
#include "test_framework.h"
#include "transport/duty_cycle.h"
#include "transport/lora_transport.h"
#include "transport/sms_transport.h"

using namespace agv;
using agv::test::FakeModemPower;
using agv::test::FakeRadio;
using agv::test::FakeUart;

// --- Rapport cyclique -------------------------------------------------------

TEST(temps_d_antenne_lora_sf9_ordre_de_grandeur) {
  // SF9, BW 125 kHz, CR 4/5, 13 octets : ~165 ms (Semtech AN1200.13).
  //
  // CONSÉQUENCE À REMONTER AU PROJET : le §6 vise ~200 ms de latence typique.
  // À SF9, l'aller seul coûte déjà 165 ms d'antenne ; avec l'ACK, un
  // aller-retour tourne autour de 330 ms, et trois tentatives dépassent la
  // seconde — au-delà du « pire cas ~800 ms » annoncé. SF7 (~46 ms) tient la
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
  FakeClock clock;
  FakeRadio radio{clock};
  LoraTransport transport{profile, radio};

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
  CHECK_EQ(b.radio.tx_count, b.profile.lora.max_tries);
  CHECK(b.transport.tx_state() == LoraTransport::TxState::Idle);
  CHECK(!b.transport.health().up);
}

TEST(lora_refuse_d_emettre_au_dela_du_budget_legal) {
  LoraBench b;
  // Budget volontairement minuscule : une seule émission passe.
  b.profile.lora.duty_cycle_permille = 10;
  b.profile.lora.duty_window_ms = 1000;  // 1 % de 1 s = 10 ms d'antenne
  LoraBench small;
  small.profile.lora.duty_cycle_permille = 10;
  small.profile.lora.duty_window_ms = 1000;
  LoraTransport transport(small.profile, small.radio);
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

// --- SMS --------------------------------------------------------------------

TEST(sms_declare_transport_non_ordonne_et_controle_de_fraicheur) {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeUart uart(clock);
  FakeModemPower power;
  SmsTransport sms(profile, uart, power);

  // C'est la déclaration qui active le rejet du désordre et de la péremption.
  CHECK(!sms.ordered());
  CHECK_EQ(sms.max_command_age_s(), profile.safety.max_command_age_s);
}

TEST(sms_sequence_pwrkey_et_initialisation_at) {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeUart uart(clock);
  FakeModemPower power;
  SmsTransport sms(profile, uart, power);

  sms.begin();
  CHECK_EQ(power.pulses, 1u);
  CHECK_EQ(power.last_duration_ms, profile.cellular.pwrkey_on_ms);  // 1 000 ms

  // Le modem répond OK à chaque étape.
  for (int i = 0; i < 20; ++i) {
    clock.advance_ms(200);
    sms.tick();
    uart.inject("\r\nOK\r\n");
    sms.tick();
  }
  CHECK(uart.wrote("ATE0"));
  CHECK(uart.wrote("AT+CMGF=1"));
  CHECK(uart.wrote("AT+CNMI=2,1"));
  CHECK(sms.state() == SmsTransport::State::Ready);
}

TEST(sms_urc_cmti_declenche_la_lecture_puis_la_suppression) {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeUart uart(clock);
  FakeModemPower power;
  SmsTransport sms(profile, uart, power);
  sms.begin();
  for (int i = 0; i < 20; ++i) {
    clock.advance_ms(200);
    sms.tick();
    uart.inject("\r\nOK\r\n");
    sms.tick();
  }
  CHECK(sms.state() == SmsTransport::State::Ready);

  // Une commande GOTO horodatée, scellée puis mise en hexadécimal.
  Frame f;
  f.ver = 1;
  f.type = FrameType::CmdGoto;
  f.node_id = 5;
  f.seq = 11;
  f.station = 42;
  f.speed = 3;
  f.ts_s = 1000;
  f.flags = flag::kTimestamped;
  uint8_t packet[kSecurePacketMax];
  const size_t len = sms.channel().seal(f, packet, sizeof(packet));
  char hex[kSecurePacketMax * 2 + 1];
  SmsTransport::to_hex(packet, len, hex, sizeof(hex));

  uart.clear();
  uart.inject("\r\n+CMTI: \"SM\",3\r\n");
  sms.tick();
  CHECK(uart.wrote("AT+CMGR=3"));

  uart.inject(std::string("\r\n+CMGR: \"REC UNREAD\",\"+33600000000\"\r\nAGV:") + hex +
              "\r\n\r\nOK\r\n");
  sms.tick();
  sms.tick();
  CHECK(uart.wrote("AT+CMGD=3"));

  Frame out;
  CHECK(sms.poll(out));
  CHECK_EQ(out.station, 42u);
  CHECK_EQ(out.seq, 11u);
  CHECK_EQ(out.ts_s, 1000u);
}

TEST(sms_refuse_une_commande_sans_horodatage) {
  // Sans horloge murale, impossible d'appliquer max_command_age_s : on refuse
  // plutôt que d'émettre une commande qui ne pourra pas être datée.
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeUart uart(clock);
  FakeModemPower power;
  SmsTransport sms(profile, uart, power);

  Frame f;
  f.type = FrameType::CmdGoto;
  f.node_id = 1;
  f.station = 3;
  f.ts_s = 0;
  CHECK(!sms.send(f));

  f.ts_s = 1700000000;
  CHECK(sms.send(f));
}

TEST(sms_modem_muet_declenche_un_cycle_d_alimentation) {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  FakeUart uart(clock);
  FakeModemPower power;
  SmsTransport sms(profile, uart, power);
  sms.begin();
  for (int i = 0; i < 20; ++i) {
    clock.advance_ms(200);
    sms.tick();
    uart.inject("\r\nOK\r\n");
    sms.tick();
  }
  const uint32_t pulses_before = power.pulses;

  // Plus aucune réponse au-delà de modem_mute_timeout_ms.
  clock.advance_ms(profile.cellular.modem_mute_timeout_ms + 1000);
  sms.tick();

  CHECK(sms.state() == SmsTransport::State::Recovering);
  CHECK(power.pulses > pulses_before);
  CHECK_EQ(power.last_duration_ms, profile.cellular.pwrkey_off_ms);  // 2 500 ms
  CHECK_EQ(sms.recoveries(), 1u);
  // Le chien de garde matériel est entretenu à chaque tick (TPL5010).
  CHECK(power.kicks > 0u);
}

TEST(sms_hex_aller_retour) {
  const uint8_t data[4] = {0x00, 0x7F, 0x80, 0xFF};
  char hex[16];
  CHECK_EQ(SmsTransport::to_hex(data, 4, hex, sizeof(hex)), 8u);
  CHECK_STR_EQ(hex, "007F80FF");
  uint8_t back[4];
  CHECK_EQ(SmsTransport::from_hex(hex, back, sizeof(back)), 4u);
  CHECK_EQ(back[3], 0xFFu);
}
