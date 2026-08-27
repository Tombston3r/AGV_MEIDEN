// Chaîne complète côté poste fixe : EnOcean -> appairage -> trame de commande.
#include <deque>
#include <string>
#include <vector>

#include "app/poste_app.h"
#include "proto/crc16.h"
#include "test_framework.h"

using namespace agv;

namespace {

class CaptureTransport final : public ITransport {
 public:
  bool begin() override { return true; }
  bool send(const Frame& f) override {
    if (refuse) return false;
    sent.push_back(f);
    return true;
  }
  bool poll(Frame& out) override {
    if (inbox.empty()) return false;
    out = inbox.front();
    inbox.pop_front();
    return true;
  }
  LinkHealth health() const override { return {}; }
  const char* name() const override { return "capture"; }

  std::vector<Frame> sent;
  std::deque<Frame> inbox;
  bool refuse = false;
};

// data 0x10 : energy bow armé (appui), bascule 0. Le champ « rocker » vit dans
// les bits 7..5 : un 0x30 désignerait la bascule 1, pas la 0.
std::vector<uint8_t> rps_frame(uint32_t sender_id, uint8_t data) {
  std::vector<uint8_t> d = {kRorgRps,
                            data,
                            static_cast<uint8_t>(sender_id >> 24),
                            static_cast<uint8_t>(sender_id >> 16),
                            static_cast<uint8_t>(sender_id >> 8),
                            static_cast<uint8_t>(sender_id & 0xFFu),
                            0x30};
  std::vector<uint8_t> opt = {0x03, 0xFF, 0xFF, 0xFF, 0xFF, 60, 0x00};
  std::vector<uint8_t> header = {0, static_cast<uint8_t>(d.size()),
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

struct PosteBench {
  HardwareProfile profile = default_profile();
  FakeClock clock;
  RamStore store;
  CaptureTransport transport;
  PairingTable pairings{&store};
  PosteApp app{profile, clock, transport, pairings};

  PosteBench() {
    clock.set_wall_s(1'700'000'000u);
    app.begin();
  }
  void press(uint32_t id, uint8_t data = 0x10) {
    for (uint8_t b : rps_frame(id, data)) app.feed_enocean(b);
  }
};

}  // namespace

TEST(poste_appui_enocean_appaire_declenche_une_commande_goto) {
  PosteBench b;
  b.pairings.set(0x0189ABCD, 0, 7, 5);
  b.press(0x0189ABCD);

  REQUIRE(b.transport.sent.size() == 1u);
  CHECK(b.transport.sent[0].type == FrameType::CmdGoto);
  CHECK_EQ(b.transport.sent[0].station, 7u);
  CHECK_EQ(b.transport.sent[0].speed, 5u);
  CHECK_EQ(b.app.stats().commands_sent, 1u);
}

TEST(poste_les_trois_sous_telegrammes_ne_donnent_qu_une_commande) {
  PosteBench b;
  b.pairings.set(0x11223344, 0, 3, 4);
  b.press(0x11223344);
  b.press(0x11223344);  // sous-télégramme 2
  b.press(0x11223344);  // sous-télégramme 3

  CHECK_EQ(b.transport.sent.size(), 1u);
  CHECK_EQ(b.app.stats().enocean_duplicates, 2u);
}

TEST(poste_deuxieme_appui_hors_fenetre_est_une_nouvelle_commande) {
  PosteBench b;
  b.pairings.set(0x11223344, 0, 3, 4);
  b.press(0x11223344);
  b.clock.advance_ms(b.profile.enocean.dedup_window_ms + 50);
  b.press(0x11223344);
  CHECK_EQ(b.transport.sent.size(), 2u);
}

TEST(poste_bouton_non_appaire_ne_declenche_rien) {
  // On ne devine jamais une station : un bouton inconnu est compté, pas exécuté.
  PosteBench b;
  b.press(0xDEADBEEF);
  CHECK_EQ(b.transport.sent.size(), 0u);
  CHECK_EQ(b.app.stats().enocean_unpaired, 1u);
}

TEST(poste_mode_appairage_associe_le_bouton_puis_le_rend_operationnel) {
  PosteBench b;
  b.app.start_pairing(12, 6);
  CHECK(b.app.pairing_active());
  b.press(0x0A0B0C0D);  // « appuyez sur le bouton à associer »
  CHECK(!b.app.pairing_active());
  CHECK_EQ(b.app.stats().pairings_done, 1u);
  CHECK_EQ(b.transport.sent.size(), 0u);  // l'appui d'appairage ne roule pas

  b.clock.advance_ms(500);
  b.press(0x0A0B0C0D);
  REQUIRE(b.transport.sent.size() == 1u);
  CHECK_EQ(b.transport.sent[0].station, 12u);
}

TEST(poste_refus_du_transport_est_compte_et_non_masque) {
  PosteBench b;
  b.pairings.set(0x1, 0, 4, 3);
  b.transport.refuse = true;  // budget de rapport cyclique épuisé, par exemple
  b.press(0x1);
  CHECK_EQ(b.app.stats().commands_refused, 1u);
  CHECK_EQ(b.app.stats().commands_sent, 0u);
}

TEST(poste_telemetrie_alimente_l_instantane_de_supervision) {
  PosteBench b;
  Frame t;
  t.ver = 1;
  t.type = FrameType::Telemetry;
  t.node_id = 1;
  t.station = 17;
  t.speed = 6;
  t.flags = flag::kStatusMoving;
  b.transport.inbox.push_back(t);
  b.app.tick();

  CHECK(b.app.snapshot().valid);
  CHECK_EQ(b.app.snapshot().station, 17u);
  CHECK(b.app.snapshot().moving);
  CHECK(!b.app.snapshot().in_station);
  CHECK_EQ(b.app.telemetry_age_ms(), 0u);

  b.clock.advance_ms(3000);
  CHECK_EQ(b.app.telemetry_age_ms(), 3000u);
}

TEST(poste_sans_telemetrie_la_fraicheur_est_infinie) {
  PosteBench b;
  CHECK(!b.app.snapshot().valid);
  CHECK_EQ(b.app.telemetry_age_ms(), UINT32_MAX);
}

TEST(poste_nack_est_distingue_de_l_ack) {
  PosteBench b;
  Frame ack;
  ack.type = FrameType::Ack;
  b.transport.inbox.push_back(ack);
  Frame nack;
  nack.type = FrameType::Ack;
  nack.flags = flag::kNack;
  b.transport.inbox.push_back(nack);
  b.app.tick();
  CHECK_EQ(b.app.stats().acks_received, 1u);
  CHECK_EQ(b.app.stats().nacks_received, 1u);
}

TEST(poste_avec_TCM515_aucun_retour_operateur_n_est_promis) {
  // §12.8 : tant que rx_only est vrai, l'IHM ne doit rien annoncer côté bouton.
  PosteBench b;
  CHECK(!b.app.operator_feedback_available());
}

TEST(poste_les_deux_bascules_d_un_meme_bouton_sont_distinguees) {
  // Bit 7..5 = bascule actionnée : deux stations par bouton double.
  PosteBench b;
  b.pairings.set(0x77, 0, 4, 3);   // bascule A
  b.pairings.set(0x77, 1, 9, 3);   // bascule B
  b.press(0x77, 0x10);             // bascule 0
  b.clock.advance_ms(500);
  b.press(0x77, 0x30);             // bascule 1
  REQUIRE(b.transport.sent.size() == 2u);
  CHECK_EQ(b.transport.sent[0].station, 4u);
  CHECK_EQ(b.transport.sent[1].station, 9u);
}

TEST(poste_dump_annonce_sa_source_et_reprend_les_noms_de_champs) {
  PosteBench b;
  char buf[2048];
  const size_t len = b.app.render_agvdump(buf, sizeof(buf));
  REQUIRE(len > 0u);
  const std::string dump(buf, len);
  // La source doit être explicite : ce dump vient de la télémétrie, pas du bus.
  CHECK(dump.find("source: poste fixe") != std::string::npos);
  for (const char* field : {"[AGV STATE]", "current_station", "[LINK]", "tx_refused_duty",
                            "enocean_unpaired", "commands_refused"}) {
    CHECK(dump.find(field) != std::string::npos);
  }
}

TEST(poste_commande_porte_un_numero_de_sequence_croissant) {
  PosteBench b;
  b.pairings.set(0x1, 0, 4, 3);
  b.press(0x1);
  b.clock.advance_ms(500);
  b.press(0x1);
  REQUIRE(b.transport.sent.size() == 2u);
  CHECK_EQ(static_cast<uint8_t>(b.transport.sent[1].seq - b.transport.sent[0].seq), 1u);
}
