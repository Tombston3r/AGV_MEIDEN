// Stockage NVS (flash ESP32) : implémentation de IPersistentStore.
//
// Porte la file de courses (§4.5), la table d'appairage EnOcean (§7), la clé
// AES et le compteur de nonce. Une NVS illisible ne doit jamais empêcher le
// démarrage : l'appelant retombe alors sur un RamStore, file vide.
#pragma once

#ifndef ESP_PLATFORM
#error "platform/esp32 ne se compile que pour la cible ESP32."
#endif

#include <nvs.h>
#include <nvs_flash.h>

#include "app/persistent_store.h"

namespace agv::esp32 {

class NvsStore final : public IPersistentStore {
 public:
  bool begin(const char* namespace_name = "agv");
  size_t read(const char* key, uint8_t* out, size_t capacity) override;
  bool write(const char* key, const uint8_t* data, size_t len) override;
  bool erase(const char* key) override;
  bool commit() override;
  bool ok() const { return handle_ != 0; }

 private:
  nvs_handle_t handle_ = 0;
};

}  // namespace agv::esp32
