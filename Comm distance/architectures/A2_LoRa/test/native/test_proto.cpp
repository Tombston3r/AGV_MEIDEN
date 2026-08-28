// Tests de la couche protocole : CRC, trame, AES, idempotence (brief §11).
#include <cstring>

#include "proto/aes128.h"
#include "proto/crc16.h"
#include "proto/frame.h"
#include "proto/replay_window.h"
#include "proto/secure_channel.h"
#include "test_framework.h"

using namespace agv;

TEST(crc16_ccitt_vecteur_connu) {
  // Vecteur de référence CRC-16/CCITT-FALSE : "123456789" -> 0x29B1.
  const char* data = "123456789";
  CHECK_EQ(crc16_ccitt(reinterpret_cast<const uint8_t*>(data), 9), 0x29B1u);
}

TEST(crc8_enocean_vecteur_connu) {
  // CRC8 poly 0x07 sur "123456789" -> 0xF4.
  const char* data = "123456789";
  CHECK_EQ(crc8_enocean(reinterpret_cast<const uint8_t*>(data), 9), 0xF4u);
}

TEST(frame_aller_retour_sans_horodatage) {
  Frame f;
  f.ver = 1;
  f.type = FrameType::CmdGoto;
  f.node_id = 0x1234;
  f.seq = 42;
  f.station = 1023;  // valeur maximale sur 10 bits
  f.speed = 15;      // valeur maximale sur 4 bits
  f.flags = flag::kPriority;

  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  CHECK_EQ(len, kFrameBaseSize);

  Frame out;
  CHECK(decode_frame(buf, len, out) == FrameError::Ok);
  CHECK_EQ(out.node_id, 0x1234u);
  CHECK_EQ(out.seq, 42u);
  CHECK_EQ(out.station, 1023u);
  CHECK_EQ(out.speed, 15u);
  CHECK_EQ(out.flags, flag::kPriority);
  CHECK(out.type == FrameType::CmdGoto);
}

TEST(vecteur_partage_avec_le_poste_python) {
  // MÊME vecteur que poste-unipi/tests/test_protocol.py. Si les deux
  // implémentations divergent d'un seul octet, l'AGV cesse d'obéir au poste :
  // silencieusement. Ce test fige l'octet-à-octet des deux côtés.
  Frame f;
  f.ver = 1;
  f.type = FrameType::CmdGoto;
  f.node_id = 0x1234;
  f.seq = 42;
  f.station = 1023;
  f.speed = 15;
  f.flags = flag::kPriority;

  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  REQUIRE(len == 9u);
  const uint8_t expected[9] = {0x10, 0x12, 0x34, 0x2A, 0xFF, 0xFC, 0x01, 0x04, 0x92};
  CHECK(std::memcmp(buf, expected, sizeof(expected)) == 0);
}

TEST(frame_aller_retour_avec_horodatage) {
  Frame f;
  f.type = FrameType::CmdStop;
  f.node_id = 7;
  f.seq = 200;
  f.station = 5;
  f.flags = flag::kTimestamped;
  f.ts_s = 0xDEADBEEF;

  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  CHECK_EQ(len, kFrameMaxSize);

  Frame out;
  CHECK(decode_frame(buf, len, out) == FrameError::Ok);
  CHECK_EQ(out.ts_s, 0xDEADBEEFu);
  CHECK(out.timestamped());
}

TEST(frame_crc_faux_rejete) {
  Frame f;
  f.type = FrameType::Ping;
  f.node_id = 1;
  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  buf[2] ^= 0x01;  // un bit retourné en vol
  Frame out;
  CHECK(decode_frame(buf, len, out) == FrameError::BadCrc);
}

TEST(frame_longueur_invalide_rejetee) {
  Frame f;
  f.type = FrameType::Ping;
  f.flags = flag::kTimestamped;  // annonce 13 octets
  f.ts_s = 1234;
  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  Frame out;
  // Trame tronquée : la longueur annoncée par les flags ne correspond plus.
  CHECK(decode_frame(buf, len - 1, out) == FrameError::BadLength);
  CHECK(decode_frame(buf, 3, out) == FrameError::TooShort);
}

TEST(frame_version_inattendue_rejetee) {
  Frame f;
  f.ver = 2;
  f.type = FrameType::Ping;
  uint8_t buf[kFrameMaxSize];
  const size_t len = encode_frame(f, buf, sizeof(buf));
  Frame out;
  CHECK(decode_frame(buf, len, out, 1) == FrameError::BadVersion);
  CHECK(decode_frame(buf, len, out, 2) == FrameError::Ok);
}

TEST(aes128_vecteur_fips197) {
  // FIPS-197 annexe B.
  const uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                           0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  const uint8_t plain[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  const uint8_t expected[16] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                                0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
  Aes128 aes;
  aes.set_key(key);
  uint8_t out[16];
  aes.encrypt_block(plain, out);
  CHECK(std::memcmp(out, expected, 16) == 0);
}

TEST(secure_channel_aller_retour) {
  const uint8_t key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  SecureChannel tx, rx;
  tx.set_key(key);
  rx.set_key(key);

  Frame f;
  f.type = FrameType::CmdGoto;
  f.node_id = 0x0042;
  f.seq = 9;
  f.station = 300;
  f.speed = 4;

  uint8_t packet[kSecurePacketMax];
  const size_t len = tx.seal(f, packet, sizeof(packet));
  CHECK(len > kFrameBaseSize);

  // Le contenu ne doit pas circuler en clair.
  uint8_t plain[kFrameMaxSize];
  const size_t plain_len = encode_frame(f, plain, sizeof(plain));
  CHECK(std::memcmp(packet + kSecureHeaderSize, plain, plain_len) != 0);

  Frame out;
  CHECK(rx.open(packet, len, out));
  CHECK_EQ(out.station, 300u);
  CHECK_EQ(out.seq, 9u);
}

TEST(secure_channel_nonce_incremente) {
  const uint8_t key[16] = {};
  SecureChannel tx;
  tx.set_key(key);
  Frame f;
  f.node_id = 1;
  uint8_t a[kSecurePacketMax], b[kSecurePacketMax];
  const size_t la = tx.seal(f, a, sizeof(a));
  const size_t lb = tx.seal(f, b, sizeof(b));
  CHECK_EQ(la, lb);
  // Deux trames identiques ne doivent jamais produire le même chiffré.
  CHECK(std::memcmp(a, b, la) != 0);
  CHECK_EQ(tx.nonce_counter(), 2u);
}

TEST(secure_channel_cmac_detecte_falsification) {
  const uint8_t key[16] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6};
  SecureChannel tx, rx;
  tx.set_key(key);
  rx.set_key(key);
  tx.enable_cmac(true);
  rx.enable_cmac(true);

  Frame f;
  f.type = FrameType::CmdGoto;
  f.node_id = 3;
  f.station = 12;
  uint8_t packet[kSecurePacketMax];
  const size_t len = tx.seal(f, packet, sizeof(packet));

  Frame out;
  CHECK(rx.open(packet, len, out));
  packet[kSecureHeaderSize + 4] ^= 0x02;  // bit retourné dans le chiffré
  CHECK(!rx.open(packet, len, out));
}

TEST(idempotence_reacquitte_sans_reexecuter) {
  ReplayWindow w(16, true);
  CHECK(w.classify(1, 10, false, 0, 0, 0) == FrameVerdict::Accept);
  w.remember(1, 10);
  // Même (node_id, seq) : doublon, on ré-acquitte sans relancer la course.
  CHECK(w.classify(1, 10, false, 0, 0, 0) == FrameVerdict::Duplicate);
  // Autre nœud, même seq : commande distincte.
  CHECK(w.classify(2, 10, false, 0, 0, 0) == FrameVerdict::Accept);
}

TEST(fenetre_antirejeu_limitee_a_16) {
  ReplayWindow w(16, true);
  for (uint8_t i = 0; i < 16; ++i) w.remember(1, i);
  CHECK_EQ(w.size(), 16u);
  CHECK(w.classify(1, 0, false, 0, 0, 0) == FrameVerdict::Duplicate);
  w.remember(1, 16);  // la plus ancienne sort de la fenêtre
  CHECK_EQ(w.size(), 16u);
}

TEST(transport_non_ordonne_rejette_trame_ancienne) {
  // Cas SMS : un STOP peut arriver avant le GOTO qu'il annule (§8.1).
  ReplayWindow w(16, /*ordered_transport=*/false);
  CHECK(w.classify(1, 50, false, 0, 0, 0) == FrameVerdict::Accept);
  w.remember(1, 50);
  CHECK(w.classify(1, 49, false, 0, 0, 0) == FrameVerdict::OutOfOrder);
  CHECK(w.classify(1, 51, false, 0, 0, 0) == FrameVerdict::Accept);
}

TEST(transport_ordonne_tolere_le_desordre) {
  ReplayWindow w(16, /*ordered_transport=*/true);
  w.remember(1, 50);
  CHECK(w.classify(1, 49, false, 0, 0, 0) == FrameVerdict::Accept);
}

TEST(seq_roulant_gere_le_bouclage_255_vers_0) {
  ReplayWindow w(16, false);
  w.remember(1, 254);
  CHECK(w.classify(1, 255, false, 0, 0, 0) == FrameVerdict::Accept);
  w.remember(1, 255);
  CHECK(w.classify(1, 0, false, 0, 0, 0) == FrameVerdict::Accept);
  w.remember(1, 0);
  CHECK(w.classify(1, 253, false, 0, 0, 0) == FrameVerdict::OutOfOrder);
}

TEST(commande_perimee_refusee) {
  // §8.1 : une commande vieille de 3 minutes sortie du SMSC ne doit jamais
  // faire bouger l'AGV. max_command_age_s = 15 s par défaut.
  ReplayWindow w(16, false);
  CHECK(w.classify(1, 1, true, /*ts=*/1000, /*now=*/1010, /*max_age=*/15) == FrameVerdict::Accept);
  CHECK(w.classify(1, 1, true, /*ts=*/1000, /*now=*/1180, /*max_age=*/15) == FrameVerdict::Expired);
  // max_age = 0 désactive le contrôle (transport à latence bornée).
  CHECK(w.classify(1, 1, true, 1000, 1180, 0) == FrameVerdict::Accept);
}

TEST(doublon_prioritaire_sur_peremption) {
  // Un ré-envoi tardif d'une commande DÉJÀ exécutée doit être ré-acquitté,
  // pas traité comme périmé : sinon l'émetteur retente indéfiniment.
  ReplayWindow w(16, false);
  w.remember(1, 5);
  CHECK(w.classify(1, 5, true, 1000, 9999, 15) == FrameVerdict::Duplicate);
}
