// Protocole série ESP32 <-> ATmega2560 et JSON MQTT (planification §2.4, §2).
#include <cstring>
#include <string>
#include <vector>

#include "link/link_protocol.h"
#include "proto/crc16.h"
#include "proto/json.h"
#include "test_framework.h"

using namespace agv;
using namespace agv::link;

// --- Trames série ----------------------------------------------------------

TEST(link_trame_goto_aller_retour) {
  uint8_t frame[kFrameMax];
  const size_t n = encode_goto(42, 777, 6, 0x01, frame, sizeof(frame));
  REQUIRE(n == 10u);
  CHECK_EQ(frame[0], kSofToMega);
  CHECK_EQ(frame[1], static_cast<uint8_t>(Cmd::Goto));
  CHECK_EQ(frame[2], 5u);

  Parser parser(kSofToMega);
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (parser.feed(frame[i], cmd, payload, len)) got = true;
  }
  REQUIRE(got);
  CHECK(cmd == Cmd::Goto);
  CHECK_EQ(len, 5u);
  CHECK_EQ(payload[0], 42u);
  CHECK_EQ(static_cast<uint16_t>((payload[1] << 8) | payload[2]), 777u);
  CHECK_EQ(payload[3], 6u);
}

TEST(link_les_deux_sens_ont_des_sof_distincts) {
  // Une trame réfléchie par un câblage douteux ne doit pas passer pour une
  // commande : c'est tout l'intérêt de deux SOF différents.
  uint8_t frame[kFrameMax];
  const size_t n = encode_ack(7, CmdResult::Accepted, frame, sizeof(frame));
  CHECK_EQ(frame[0], kSofToEsp);

  Parser mega_side(kSofToMega);  // le MEGA n'écoute que 0xA5
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (mega_side.feed(frame[i], cmd, payload, len)) got = true;
  }
  CHECK(!got);
}

TEST(link_crc_faux_est_compte_et_ignore) {
  uint8_t frame[kFrameMax];
  const size_t n = encode_goto(1, 5, 4, 0, frame, sizeof(frame));
  frame[4] ^= 0xFF;  // corruption du contenu

  Parser parser(kSofToMega);
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (parser.feed(frame[i], cmd, payload, len)) got = true;
  }
  CHECK(!got);
  CHECK_EQ(parser.crc_errors(), 1u);
}

TEST(link_resynchronisation_apres_octets_parasites) {
  uint8_t frame[kFrameMax];
  const size_t n = encode_goto(3, 9, 2, 0, frame, sizeof(frame));

  Parser parser(kSofToMega);
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  for (uint8_t junk : {0x00, 0x12, 0xFF, 0x5A}) parser.feed(junk, cmd, payload, len);

  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (parser.feed(frame[i], cmd, payload, len)) got = true;
  }
  CHECK(got);
}

TEST(link_longueur_impossible_declenche_une_resynchronisation) {
  Parser parser(kSofToMega);
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  parser.feed(kSofToMega, cmd, payload, len);
  parser.feed(static_cast<uint8_t>(Cmd::Goto), cmd, payload, len);
  parser.feed(200, cmd, payload, len);  // > kPayloadMax
  CHECK_EQ(parser.resyncs(), 1u);

  // Le parseur doit être immédiatement réutilisable.
  uint8_t frame[kFrameMax];
  const size_t n = encode_goto(1, 1, 1, 0, frame, sizeof(frame));
  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (parser.feed(frame[i], cmd, payload, len)) got = true;
  }
  CHECK(got);
}

TEST(link_etat_aller_retour_complet) {
  LinkState s;
  s.station = 512;
  s.speed = 7;
  s.seq_state = 6;
  s.fault = 3;
  s.flags = state_flag::kMoving | state_flag::kHeartbeatOk;
  s.queue_len = 4;
  s.write_tries = 2;
  s.start_tries = 1;
  s.stop_tries = 0;
  s.last_seq = 99;

  uint8_t frame[kFrameMax];
  const size_t n = encode_state(s, frame, sizeof(frame));
  REQUIRE(n > 0u);

  Parser parser(kSofToEsp);
  Cmd cmd;
  uint8_t payload[kPayloadMax];
  uint8_t len = 0;
  bool got = false;
  for (size_t i = 0; i < n; ++i) {
    if (parser.feed(frame[i], cmd, payload, len)) got = true;
  }
  REQUIRE(got);
  CHECK(cmd == Cmd::State);

  LinkState out;
  CHECK(decode_state(payload, len, out));
  CHECK_EQ(out.station, 512u);
  CHECK_EQ(out.speed, 7u);
  CHECK_EQ(out.queue_len, 4u);
  CHECK_EQ(out.last_seq, 99u);
  CHECK((out.flags & state_flag::kMoving) != 0);
  CHECK((out.flags & state_flag::kSafeStop) == 0);
}

TEST(link_charge_utile_trop_grande_est_refusee_a_l_encodage) {
  uint8_t payload[64] = {};
  uint8_t frame[kFrameMax];
  CHECK_EQ(encode(kSofToMega, Cmd::Goto, payload, 60, frame, sizeof(frame)), 0u);
}

// --- JSON ------------------------------------------------------------------

TEST(json_ecriture_et_relecture) {
  char buf[256];
  json::Writer w(buf, sizeof(buf));
  w.field("seq", static_cast<uint32_t>(42));
  w.field("dest", static_cast<uint32_t>(777));
  w.field("speed", static_cast<uint32_t>(6));
  w.field("moving", true);
  w.field("name", "agv-1");
  const size_t n = w.end();
  REQUIRE(n > 0u);

  const std::string s(buf, n);
  CHECK(s.front() == '{');
  CHECK(s.back() == '}');

  int32_t seq = 0, dest = 0;
  CHECK(json::get_int(buf, "seq", seq));
  CHECK(json::get_int(buf, "dest", dest));
  CHECK_EQ(seq, 42);
  CHECK_EQ(dest, 777);

  bool moving = false;
  CHECK(json::get_bool(buf, "moving", moving));
  CHECK(moving);

  char name[16];
  CHECK(json::get_string(buf, "name", name, sizeof(name)));
  CHECK_STR_EQ(name, "agv-1");
}

TEST(json_cle_absente_ne_ment_pas) {
  const char* payload = "{\"seq\":1}";
  int32_t value = 12345;
  CHECK(!json::get_int(payload, "dest", value));
  CHECK_EQ(value, 12345);  // la sortie n'est pas touchée
}

TEST(json_valeur_negative_et_valeur_non_numerique) {
  const char* payload = "{\"a\":-42,\"b\":\"texte\"}";
  int32_t a = 0;
  CHECK(json::get_int(payload, "a", a));
  CHECK_EQ(a, -42);
  int32_t b = 7;
  CHECK(!json::get_int(payload, "b", b));
  CHECK_EQ(b, 7);
}

TEST(json_debordement_retourne_zero_pas_un_json_tronque) {
  // Un JSON tronqué serait accepté par un analyseur permissif puis mal
  // interprété : il vaut mieux ne rien produire.
  char tiny[16];
  json::Writer w(tiny, sizeof(tiny));
  w.field("station_tres_longue_cle", static_cast<uint32_t>(123456));
  CHECK_EQ(w.end(), 0u);
}

TEST(json_prefixe_de_cle_ne_provoque_pas_de_faux_positif) {
  const char* payload = "{\"destination\":9,\"dest\":3}";
  int32_t dest = 0;
  CHECK(json::get_int(payload, "dest", dest));
  CHECK_EQ(dest, 3);
}
