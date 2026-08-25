// Essai d'ÉMISSION LoRa sur ESP32 — carte AGV ou poste fixe.
//
// Émet des trames applicatives réelles et attend l'accusé du pair. En face,
// faire tourner l'environnement `rx`, ou `../unipi/test_rx.py`.
//
//   pio run -e tx -t upload -t monitor
//
// Le budget de rapport cyclique est appliqué par `DutyCycle`, le composant du
// projet : le script refuse d'émettre au-delà de 1 % sur une heure glissante.
// C'est une obligation réglementaire (EN 300 220 / ERC 70-03). Un essai
// d'endurance est précisément le moment où on la franchit sans le voir.
#include <cstdio>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config/lora_config.h"
#include "platform/esp32/esp_ports.h"
#include "platform/esp32/sx1276_radio.h"
#include "proto/frame.h"
#include "test_config.h"
#include "transport/duty_cycle.h"

namespace {

constexpr int kNbTrames = 20;
constexpr uint32_t kIntervalleMs = 2000;
constexpr uint32_t kAckTimeoutMs = 400;
constexpr uint16_t kStation = 42;

agv::esp32::EspSpi g_spi;
agv::esp32::EspGpio g_gpio;
agv::esp32::EspClock g_clock;

}  // namespace

extern "C" void app_main() {
  const agv::LoraConfig cfg;  // valeurs par défaut du projet : 868,1 MHz SF9

  if (!g_spi.begin(test::kSpiHost, test::kPinSck, test::kPinMosi, test::kPinMiso,
                   test::kPinNss, test::kSpiFreqHz)) {
    printf("[ECHEC] bus SPI non initialise\n");
    return;
  }

  agv::esp32::Sx1276Radio radio(g_spi, g_gpio, g_clock, test::kPinReset, test::kPinDio0);
  if (!radio.begin(cfg)) {
    printf("[ECHEC] RFM95W muet (RegVersion 0x%02X, attendu 0x12).\n", radio.version());
    printf("        Verifier cablage SPI, alimentation 3,3 V et NSS.\n");
    return;
  }

  const uint32_t attendu_us =
      agv::lora_airtime_us(agv::kFrameBaseSize, cfg.spreading_factor,
                           cfg.bandwidth_hz, cfg.coding_rate);
  const uint32_t attendu_ms = attendu_us / 1000;
  printf("RFM95W detecte (RegVersion 0x%02X)\n", radio.version());
  printf("%.1f MHz  SF%u  BW%u  CR4/%u  sync 0x%02X  %d dBm\n",
         cfg.frequency_hz / 1e6, cfg.spreading_factor, cfg.bandwidth_hz / 1000,
         cfg.coding_rate, cfg.sync_word, cfg.tx_power_dbm);
  printf("temps d'antenne : %u ms/trame -> %u emissions/heure au maximum legal\n\n",
         attendu_ms, 36000u / (attendu_ms ? attendu_ms : 1));

  agv::DutyCycleBudget duty(cfg.duty_cycle_permille, cfg.duty_window_ms);
  int envoyees = 0, acquittees = 0, refusees = 0;

  for (int i = 0; i < kNbTrames; ++i) {
    const uint32_t now = g_clock.now_ms();

    if (!duty.can_transmit(attendu_us, now)) {
      ++refusees;
      printf("[%3d] REFUSE — budget de rapport cyclique epuise\n", i);
      vTaskDelay(pdMS_TO_TICKS(kIntervalleMs));
      continue;
    }

    agv::Frame trame;
    trame.type = agv::FrameType::CmdGoto;
    trame.node_id = test::kNodeIdTx;
    trame.seq = static_cast<uint8_t>(i);
    trame.station = kStation;
    trame.speed = 2;

    uint8_t brut[agv::kFrameMaxSize];
    const size_t len = agv::encode_frame(trame, brut, sizeof(brut));

    const uint32_t debut = g_clock.now_ms();
    if (!radio.transmit(brut, len)) {
      printf("[%3d] emission refusee par le pilote\n", i);
      vTaskDelay(pdMS_TO_TICKS(kIntervalleMs));
      continue;
    }
    while (radio.tx_busy()) vTaskDelay(pdMS_TO_TICKS(1));
    const uint32_t mesure = g_clock.now_ms() - debut;

    duty.record(attendu_us, g_clock.now_ms());
    ++envoyees;

    // Fenêtre d'écoute d'accusé — le module est HALF-DUPLEX, l'écoute ne peut
    // commencer qu'une fois l'émission terminée.
    radio.listen();
    bool acquittee = false;
    const uint32_t limite = g_clock.now_ms() + kAckTimeoutMs;
    while (g_clock.now_ms() < limite && !acquittee) {
      uint8_t buf[agv::kFrameMaxSize];
      size_t recu = 0;
      int16_t rssi = 0;
      int8_t snr = 0;
      if (radio.receive(buf, sizeof(buf), recu, rssi, snr)) {
        agv::Frame ack;
        if (agv::decode_frame(buf, recu, ack) == agv::FrameError::Ok &&
            ack.type == agv::FrameType::Ack && ack.seq == trame.seq) {
          acquittee = true;
          ++acquittees;
          printf("[%3d] emise seq=%3u %u o %4u ms   -> ACK  %d dBm  SNR %+d dB\n",
                 i, trame.seq, static_cast<unsigned>(len), mesure, rssi, snr);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (!acquittee) {
      printf("[%3d] emise seq=%3u %u o %4u ms   -> PAS D'ACCUSE\n",
             i, trame.seq, static_cast<unsigned>(len), mesure);
    }

    vTaskDelay(pdMS_TO_TICKS(kIntervalleMs));
  }

  printf("\n--- Bilan ---\n");
  printf("emises     : %d\n", envoyees);
  printf("refusees   : %d  (budget de rapport cyclique)\n", refusees);
  printf("acquittees : %d  (%d %%)\n", acquittees,
         envoyees ? (100 * acquittees / envoyees) : 0);
}
