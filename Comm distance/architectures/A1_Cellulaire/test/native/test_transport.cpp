// Tests du transport SMS : pile AT, URC, dégradations (brief §11).
//
// Les tests du transport LoRa et du budget de rapport cyclique vivent dans le
// dossier d'architecture CarteComm/LoRa/ — ils n'ont pas de sens ici.
#include "fakes.h"
#include "test_framework.h"
#include "transport/sms_transport.h"

using namespace agv;
using agv::test::FakeModemPower;
using agv::test::FakeUart;

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
