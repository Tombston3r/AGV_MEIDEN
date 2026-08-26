// Nœud bouton sur pile — architecture 1 (brief §6.1).
//
// Framework Arduino toléré ICI SEULEMENT (brief §2).
//
// Comportement :
//   - sommeil profond permanent (< 2 µA), réveil sur front GPIO ;
//   - émission CMD_GOTO, attente d'ACK jusqu'à 400 ms, 3 tentatives ;
//   - LED verte fixe 2 s = ACK reçu ; LED rouge clignotante = échec.
//
// Ce retour visuel est ABSENT de la solution EnOcean pure (§7) : c'est un
// argument à conserver dans la comparaison des architectures.
//
// Ajouter un bouton = flasher un node_id et une station. Aucune modification
// côté AGV.
#include <Arduino.h>
#include <SPI.h>
#include <esp_sleep.h>

#include "config/hardware_profile.h"
#include "config/lora_config.h"
#include "proto/secure_channel.h"

namespace {

// --- Identité du nœud : SEULES lignes à changer pour ajouter un bouton -----
constexpr uint16_t kNodeId = 0x0101;
constexpr uint16_t kStation = 2;
constexpr uint8_t kSpeed = 4;

constexpr gpio_num_t kButtonPin = GPIO_NUM_33;  // vers la masse, pull-up interne
constexpr int kLedGreen = 25;
constexpr int kLedRed = 26;
constexpr int kLoraCs = 5;
constexpr int kLoraReset = 14;
constexpr int kLoraDio0 = 27;

// Le compteur de séquence survit au sommeil profond : sans cela, chaque appui
// repartirait à zéro et l'AGV traiterait la commande comme un doublon.
RTC_DATA_ATTR uint8_t g_seq = 0;
RTC_DATA_ATTR uint32_t g_nonce = 0;

void led(int pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

void sleep_forever() {
  led(kLedGreen, false);
  led(kLedRed, false);
  // Réveil sur front descendant du bouton. Consommation visée < 2 µA : ne
  // laisser AUCUNE périphérie active.
  esp_sleep_enable_ext0_wakeup(kButtonPin, 0);
  esp_deep_sleep_start();
}

void blink_failure() {
  for (int i = 0; i < 6; ++i) {
    led(kLedRed, i % 2 == 0);
    delay(150);
  }
}

}  // namespace

// Les fonctions radio bas niveau sont volontairement laissées à l'implémentation
// SX1276 partagée ; ce fichier ne décrit que la logique du nœud.
extern bool radio_begin(const agv::LoraConfig& cfg, int cs, int reset, int dio0);
extern bool radio_send(const uint8_t* data, size_t len);
extern bool radio_wait_ack(uint8_t expected_seq, uint32_t timeout_ms, agv::SecureChannel& channel);

void setup() {
  pinMode(kLedGreen, OUTPUT);
  pinMode(kLedRed, OUTPUT);
  pinMode(kButtonPin, INPUT_PULLUP);

  const agv::HardwareProfile& profile = agv::default_profile();
  const agv::LoraConfig lora;  // valeurs par défaut, ou lora_config_from_profile()

  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) {
    // Premier démarrage (pile insérée) : on ne commande rien, on dort.
    sleep_forever();
  }

  if (!radio_begin(lora, kLoraCs, kLoraReset, kLoraDio0)) {
    blink_failure();
    sleep_forever();
  }

  static agv::SecureChannel channel;
  // Clé AES gravée au flash du bouton (partagée avec l'AGV).
  static const uint8_t kKey[agv::kAesKeySize] = {0};  // PROVISOIRE : à provisionner au flash
  channel.set_key(kKey);
  channel.set_nonce_counter(g_nonce);

  agv::Frame f;
  f.ver = profile.protocol.version;
  f.type = agv::FrameType::CmdGoto;
  f.node_id = kNodeId;
  f.seq = ++g_seq;
  f.station = kStation;
  f.speed = kSpeed;

  bool acked = false;
  for (uint32_t attempt = 0; attempt < lora.max_tries && !acked; ++attempt) {
    if (attempt > 0) f.flags |= agv::flag::kRetry;
    uint8_t packet[agv::kSecurePacketMax];
    const size_t len = channel.seal(f, packet, sizeof(packet));
    if (len == 0) break;
    if (!radio_send(packet, len)) break;
    // Même seq à chaque tentative : c'est l'idempotence côté AGV qui évite la
    // course en double si l'ACK est perdu (§5.1).
    acked = radio_wait_ack(f.seq, lora.ack_timeout_ms, channel);
  }
  g_nonce = channel.nonce_counter();

  if (acked) {
    led(kLedGreen, true);
    delay(2000);  // LED verte fixe 2 s
  } else {
    blink_failure();
  }
  sleep_forever();
}

void loop() {
  // Jamais atteint : le nœud dort en permanence entre deux appuis.
}
