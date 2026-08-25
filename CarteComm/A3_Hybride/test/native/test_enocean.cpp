// Décodeur ESP3, déduplication PTM 210 et table d'appairage (brief §7, §11).
#include <vector>

#include "app/persistent_store.h"
#include "enocean/esp3.h"
#include "enocean/pairing_table.h"
#include "proto/crc16.h"
#include "test_framework.h"

using namespace agv;

namespace {

// Construit une trame ESP3 RADIO_ERP1 portant un télégramme RPS de PTM 210.
std::vector<uint8_t> make_rps_frame(uint32_t sender_id, uint8_t data, int8_t rssi = -60) {
  std::vector<uint8_t> d = {kRorgRps,
                            data,
                            static_cast<uint8_t>(sender_id >> 24),
                            static_cast<uint8_t>(sender_id >> 16),
                            static_cast<uint8_t>(sender_id >> 8),
                            static_cast<uint8_t>(sender_id & 0xFFu),
                            0x30};
  std::vector<uint8_t> opt = {0x03, 0xFF, 0xFF, 0xFF, 0xFF,
                              static_cast<uint8_t>(-rssi), 0x00};

  std::vector<uint8_t> header = {static_cast<uint8_t>(d.size() >> 8),
                                 static_cast<uint8_t>(d.size() & 0xFFu),
                                 static_cast<uint8_t>(opt.size()), kEsp3TypeRadioErp1};

  std::vector<uint8_t> frame = {kEsp3Sync};
  frame.insert(frame.end(), header.begin(), header.end());
  frame.push_back(crc8_enocean(header.data(), header.size()));
  frame.insert(frame.end(), d.begin(), d.end());
  frame.insert(frame.end(), opt.begin(), opt.end());
  std::vector<uint8_t> body = d;
  body.insert(body.end(), opt.begin(), opt.end());
  frame.push_back(crc8_enocean(body.data(), body.size()));
  return frame;
}

}  // namespace

TEST(esp3_decode_un_telegramme_rps_complet) {
  Esp3Decoder dec;
  Esp3Packet pkt;
  bool got = false;
  for (uint8_t b : make_rps_frame(0x0189ABCD, 0x30)) {
    if (dec.feed(b, pkt)) got = true;
  }
  CHECK(got);
  CHECK_EQ(pkt.packet_type, kEsp3TypeRadioErp1);
  CHECK_EQ(pkt.rssi_dbm, -60);

  RpsTelegram rps;
  CHECK(Esp3Decoder::parse_rps(pkt, rps));
  CHECK_EQ(rps.sender_id, 0x0189ABCDu);
  CHECK(rps.pressed);  // bit 4 (energy bow) armé
  CHECK_EQ(rps.rssi_dbm, -60);
}

TEST(esp3_crc_header_faux_est_rejete_et_resynchronise) {
  Esp3Decoder dec;
  Esp3Packet pkt;
  auto frame = make_rps_frame(0x11223344, 0x30);
  frame[5] ^= 0xFF;  // CRC8 header corrompu
  for (uint8_t b : frame) dec.feed(b, pkt);
  CHECK_EQ(dec.crc_header_errors(), 1u);
  CHECK_EQ(dec.packets_ok(), 0u);

  // Le décodeur doit se remettre en phase sur la trame suivante.
  bool got = false;
  for (uint8_t b : make_rps_frame(0x11223344, 0x30)) {
    if (dec.feed(b, pkt)) got = true;
  }
  CHECK(got);
}

TEST(esp3_crc_data_faux_est_rejete) {
  Esp3Decoder dec;
  Esp3Packet pkt;
  auto frame = make_rps_frame(0x55667788, 0x30);
  frame.back() ^= 0x5A;
  bool got = false;
  for (uint8_t b : frame) {
    if (dec.feed(b, pkt)) got = true;
  }
  CHECK(!got);
  CHECK_EQ(dec.crc_data_errors(), 1u);
}

TEST(esp3_octets_parasites_avant_le_sync_sont_ignores) {
  Esp3Decoder dec;
  Esp3Packet pkt;
  bool got = false;
  for (uint8_t b : {0x00, 0xAA, 0x12, 0xFF}) dec.feed(b, pkt);
  for (uint8_t b : make_rps_frame(0x01020304, 0x30)) {
    if (dec.feed(b, pkt)) got = true;
  }
  CHECK(got);
}

TEST(ptm210_trois_sous_telegrammes_ne_font_qu_un_appui) {
  // Sans déduplication, un appui déclencherait TROIS courses (§7).
  EnoceanDeduplicator dedup(100);
  CHECK(dedup.accept(0x1234, 0x30, 1000));
  CHECK(!dedup.accept(0x1234, 0x30, 1020));
  CHECK(!dedup.accept(0x1234, 0x30, 1045));
  CHECK_EQ(dedup.duplicates(), 2u);
  // Nouvel appui hors fenêtre : accepté.
  CHECK(dedup.accept(0x1234, 0x30, 1200));
}

TEST(deduplication_distingue_deux_boutons_simultanes) {
  EnoceanDeduplicator dedup(100);
  CHECK(dedup.accept(0x1111, 0x30, 500));
  CHECK(dedup.accept(0x2222, 0x30, 505));
}

TEST(appairage_associe_un_identifiant_usine_a_une_station) {
  RamStore store;
  PairingTable table(&store);
  Pairing p;
  CHECK(!table.lookup(0xAABBCCDD, 0, p));

  // Mode appairage : « appuyez sur le bouton à associer ».
  table.start_pairing(7, 4, /*now_s=*/1000, /*timeout_s=*/60);
  CHECK(table.pairing_active(1000));
  CHECK(table.complete_pairing(0xAABBCCDD, 0, 1010));
  CHECK(!table.pairing_active(1010));

  CHECK(table.lookup(0xAABBCCDD, 0, p));
  CHECK_EQ(p.station, 7u);
  CHECK_EQ(p.speed, 4u);
}

TEST(appairage_expire_apres_le_delai_configure) {
  RamStore store;
  PairingTable table(&store);
  table.start_pairing(3, 2, 1000, 60);
  CHECK(!table.pairing_active(1061));
  CHECK(!table.complete_pairing(0x1, 0, 1061));
  Pairing p;
  CHECK(!table.lookup(0x1, 0, p));
}

TEST(table_d_appairage_persiste_et_se_recharge) {
  RamStore store;
  {
    PairingTable table(&store);
    table.set(0x01020304, 0, 11, 5);
    table.set(0x05060708, 1, 22, 6);
    CHECK(table.save());
  }
  PairingTable reloaded(&store);
  CHECK_EQ(reloaded.load(), 2u);
  Pairing p;
  CHECK(reloaded.lookup(0x05060708, 1, p));
  CHECK_EQ(p.station, 22u);
  CHECK_EQ(p.rocker, 1u);
}

TEST(table_d_appairage_corrompue_ne_charge_rien) {
  RamStore store;
  {
    PairingTable table(&store);
    table.set(0x01020304, 0, 11, 5);
    table.save();
  }
  uint8_t blob[128];
  const size_t len = store.read("enocean_map", blob, sizeof(blob));
  blob[3] ^= 0xFF;  // corruption en flash
  store.write("enocean_map", blob, len);

  PairingTable reloaded(&store);
  CHECK_EQ(reloaded.load(), 0u);  // silence plutôt qu'un appairage fantaisiste
}

TEST(deux_bascules_du_meme_bouton_visent_deux_stations) {
  RamStore store;
  PairingTable table(&store);
  table.set(0x0A0B0C0D, 0, 4, 3);
  table.set(0x0A0B0C0D, 1, 9, 3);
  Pairing p;
  CHECK(table.lookup(0x0A0B0C0D, 0, p));
  CHECK_EQ(p.station, 4u);
  CHECK(table.lookup(0x0A0B0C0D, 1, p));
  CHECK_EQ(p.station, 9u);
}
