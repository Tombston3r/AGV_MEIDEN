// Paramètres radio LoRa — propres à ce dossier d'architecture.
//
// Ils ne vivent PAS dans `HardwareProfile` : le cœur métier est partagé entre
// architectures, et un dossier cellulaire n'a aucune raison de transporter un
// facteur d'étalement. `LoraTransport` reçoit donc sa configuration à part.
//
// Les valeurs par défaut viennent du profil YAML, via le fragment
// `profiles/lora_fragment.yaml` à réintégrer dans `profiles/*.yaml` lorsque ce
// dossier est complété (voir ../README.md).
#pragma once

#include <cstdint>

namespace agv {

struct LoraConfig {
  uint32_t frequency_hz = 868100000;
  uint8_t spreading_factor = 9;
  uint32_t bandwidth_hz = 125000;
  uint8_t coding_rate = 5;      // 4/5
  uint8_t sync_word = 0x12;     // privé — DOIT différer de 0x34 (LoRaWAN)
  int8_t tx_power_dbm = 14;
  uint32_t ack_timeout_ms = 400;
  uint32_t max_tries = 3;
  // EN 300 220 / ERC 70-03 : 1 % sur 1 h glissante. Obligation réglementaire.
  uint32_t duty_cycle_permille = 10;
  uint32_t duty_window_ms = 3600000;
};

// Configuration compilée depuis le profil, si le fragment YAML a été réintégré.
#ifdef CFG_LORA_FREQUENCY_HZ
inline LoraConfig lora_config_from_profile() {
  LoraConfig cfg;
  cfg.frequency_hz = CFG_LORA_FREQUENCY_HZ;
  cfg.spreading_factor = static_cast<uint8_t>(CFG_LORA_SPREADING_FACTOR);
  cfg.bandwidth_hz = CFG_LORA_BANDWIDTH_HZ;
  cfg.coding_rate = static_cast<uint8_t>(CFG_LORA_CODING_RATE);
  cfg.sync_word = static_cast<uint8_t>(CFG_LORA_SYNC_WORD);
  cfg.tx_power_dbm = static_cast<int8_t>(CFG_LORA_TX_POWER_DBM);
  cfg.ack_timeout_ms = CFG_LORA_ACK_TIMEOUT_MS;
  cfg.max_tries = CFG_LORA_MAX_TRIES;
  cfg.duty_cycle_permille = CFG_LORA_DUTY_CYCLE_PERMILLE;
  cfg.duty_window_ms = CFG_LORA_DUTY_WINDOW_MS;
  return cfg;
}
#endif

}  // namespace agv
