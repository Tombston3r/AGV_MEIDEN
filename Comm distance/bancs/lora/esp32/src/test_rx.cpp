// Essai de RÉCEPTION LoRa sur ESP32 — carte AGV ou poste fixe.
//
// Écoute en continu, décode les trames applicatives, mesure RSSI et SNR, et
// acquitte. En face, faire tourner l'environnement `tx`, ou
// `../unipi/test_tx.py`.
//
//   pio run -e rx -t upload -t monitor
//
// L'accusé consomme lui aussi du budget de rapport cyclique : un récepteur qui
// acquitte est un émetteur. C'est pour cela que `DutyCycleBudget` est présent
// des deux côtés.
#include <cstdio>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config/lora_config.h"
#include "platform/esp32/esp_ports.h"
#include "platform/esp32/sx1276_radio.h"
#include "proto/frame.h"
#include "oled.h"
#include "tbeam_power.h"
#include "test_config.h"
#include "transport/duty_cycle.h"

namespace {

constexpr uint32_t kBilanTousLesMs = 30000;
constexpr size_t kMaxVues = 64;

agv::esp32::EspSpi g_spi;
agv::esp32::EspGpio g_gpio;
agv::esp32::EspClock g_clock;

// Fenêtre d'idempotence : une même (node_id, seq) doit être RÉ-ACQUITTÉE sans
// être ré-exécutée. Sans elle, un accusé perdu déclenche une course en double.
struct Vue {
  uint16_t node_id;
  uint8_t seq;
};
Vue g_vues[kMaxVues];
size_t g_nb_vues = 0;

bool deja_vue(uint16_t node_id, uint8_t seq) {
  for (size_t i = 0; i < g_nb_vues; ++i) {
    if (g_vues[i].node_id == node_id && g_vues[i].seq == seq) return true;
  }
  if (g_nb_vues < kMaxVues) g_vues[g_nb_vues++] = {node_id, seq};
  return false;
}

}  // namespace

extern "C" void app_main() {
  const agv::LoraConfig cfg;

  // Sur une carte à gestionnaire d'alimentation (T-Beam), la radio est hors
  // tension au démarrage. L'interroger avant de l'alimenter fait lire
  // RegVersion = 0x00 et cherche un défaut de câblage qui n'existe pas.
  if (test::kAlimentationGeree) {
    const tbeam::Etat etat = tbeam::alimenter_radio();
    printf("alimentation : %s\n", tbeam::message(etat));
    if (etat != tbeam::Etat::Ok) return;
    tbeam::couper_gps();          // inutile ici, et coûteux sur batterie
    oled::begin();
    oled::page("Banc LoRa - ecoute", "", "en attente...", "");
  }

  if (!g_spi.begin(test::kSpiHost, test::kPinSck, test::kPinMosi, test::kPinMiso,
                   test::kPinNss, test::kSpiFreqHz)) {
    printf("[ECHEC] bus SPI non initialise\n");
    return;
  }

  agv::esp32::Sx1276Radio radio(g_spi, g_gpio, g_clock, test::kPinReset, test::kPinDio0);
  if (!radio.begin(cfg)) {
    printf("[ECHEC] SX1276 muet (RegVersion 0x%02X, attendu 0x12).\n", radio.version());
    printf("        Verifier cablage SPI, alimentation 3,3 V et NSS.\n");
    if (test::kAlimentationGeree) {
      printf("        Sur T-Beam : verifier la revision, le brochage change.\n");
    }
    return;
  }

  printf("%s — SX1276 detecte (RegVersion 0x%02X)\n", test::kNomCarte, radio.version());
  printf("%.1f MHz  SF%u  BW%u  CR4/%u  sync 0x%02X\n",
         cfg.frequency_hz / 1e6, cfg.spreading_factor, cfg.bandwidth_hz / 1000,
         cfg.coding_rate, cfg.sync_word);
  printf("ecoute...\n\n");
  radio.listen();

  const uint32_t attendu_us =
      agv::lora_airtime_us(agv::kFrameBaseSize, cfg.spreading_factor,
                           cfg.bandwidth_hz, cfg.coding_rate);
  agv::DutyCycleBudget duty(cfg.duty_cycle_permille, cfg.duty_window_ms);

  uint32_t valides = 0, rejetees = 0, doublons = 0, non_acquittees = 0;
  int32_t rssi_min = 0, rssi_max = -200, rssi_somme = 0;
  uint32_t prochain_bilan = g_clock.now_ms() + kBilanTousLesMs;

  for (;;) {
    uint8_t buf[agv::kFrameMaxSize];
    size_t recu = 0;
    int16_t rssi = 0;
    int8_t snr = 0;

    if (radio.receive(buf, sizeof(buf), recu, rssi, snr)) {
      agv::Frame trame;
      const agv::FrameError err = agv::decode_frame(buf, recu, trame);

      if (err != agv::FrameError::Ok) {
        ++rejetees;
        printf("  REJETEE : %s (%u octets)\n", agv::frame_error_str(err),
               static_cast<unsigned>(recu));
      } else {
        ++valides;
        if (rssi_max < -199) rssi_min = rssi;
        if (rssi < rssi_min) rssi_min = rssi;
        if (rssi > rssi_max) rssi_max = rssi;
        rssi_somme += rssi;

        // L'écran est ce qui rend le relevé de portée praticable : on lit le
        // niveau reçu en marchant, sans ordinateur au bout d'un câble.
        if (oled::present()) {
          char l1[24], l2[24], l3[24], l4[24];
          snprintf(l1, sizeof(l1), "RSSI  %5d dBm", rssi);
          snprintf(l2, sizeof(l2), "SNR   %+5d dB", snr);
          snprintf(l3, sizeof(l3), "recues %u  rej %u", valides, rejetees);
          snprintf(l4, sizeof(l4), "%s", rssi < -115 ? "!! MARGE FAIBLE" : "marge correcte");
          oled::page("Banc LoRa - ecoute", "", l1, l2, l3, l4);
        }

        const bool doublon = deja_vue(trame.node_id, trame.seq);
        if (doublon) ++doublons;
        printf("  %-9s node=0x%04X seq=%3u station=%4u vitesse=%u  %d dBm  SNR %+d dB%s\n",
               agv::frame_type_str(trame.type), trame.node_id, trame.seq,
               trame.station, trame.speed, rssi, snr, doublon ? "  [DOUBLON]" : "");

        const bool a_acquitter = trame.type == agv::FrameType::CmdGoto ||
                                 trame.type == agv::FrameType::CmdStop ||
                                 trame.type == agv::FrameType::Ping;
        if (a_acquitter) {
          // Le doublon est ré-acquitté sans être ré-exécuté : c'est la règle
          // d'idempotence, et c'est justement ce que cet essai doit montrer.
          if (!duty.can_transmit(attendu_us, g_clock.now_ms())) {
            ++non_acquittees;
            printf("      accuse NON EMIS — budget de rapport cyclique epuise\n");
          } else {
            agv::Frame ack;
            ack.type = agv::FrameType::Ack;
            ack.node_id = test::kNodeIdRx;
            ack.seq = trame.seq;
            ack.station = trame.station;

            uint8_t brut[agv::kFrameMaxSize];
            const size_t len = agv::encode_frame(ack, brut, sizeof(brut));
            if (radio.transmit(brut, len)) {
              while (radio.tx_busy()) vTaskDelay(pdMS_TO_TICKS(1));
              duty.record(attendu_us, g_clock.now_ms());
            }
            radio.listen();
          }
        }
      }
    }

    if (g_clock.now_ms() >= prochain_bilan) {
      prochain_bilan = g_clock.now_ms() + kBilanTousLesMs;
      printf("\n--- Bilan intermediaire ---\n");
      printf("valides %u  rejetees %u  doublons %u  accuses refuses %u\n",
             valides, rejetees, doublons, non_acquittees);
      if (valides > 0) {
        const int32_t moy = rssi_somme / static_cast<int32_t>(valides);
        printf("RSSI  min %d / moy %d / max %d dBm\n", rssi_min, moy, rssi_max);
        if (rssi_min < -115) {
          printf("ATTENTION : RSSI sous -115 dBm, marge insuffisante.\n");
          printf("            Un chariot charge entre les antennes coupera le lien.\n");
        }
      } else {
        printf("aucune trame — verifier frequence, SF et mot de synchronisation\n");
      }
      printf("\n");
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}
