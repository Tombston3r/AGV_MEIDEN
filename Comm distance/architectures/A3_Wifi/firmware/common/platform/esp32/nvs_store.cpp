#include "platform/esp32/nvs_store.h"

namespace agv::esp32 {

bool NvsStore::begin(const char* namespace_name) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // Partition inutilisable : on la réinitialise. La file de courses est
    // perdue, ce qui est le comportement sûr : mieux vaut une file vide qu'une
    // course fantôme.
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) return false;
  return nvs_open(namespace_name, NVS_READWRITE, &handle_) == ESP_OK;
}

size_t NvsStore::read(const char* key, uint8_t* out, size_t capacity) {
  if (handle_ == 0) return 0;
  size_t len = capacity;
  if (nvs_get_blob(handle_, key, out, &len) != ESP_OK) return 0;
  return len;
}

bool NvsStore::write(const char* key, const uint8_t* data, size_t len) {
  if (handle_ == 0) return false;
  return nvs_set_blob(handle_, key, data, len) == ESP_OK;
}

bool NvsStore::erase(const char* key) {
  if (handle_ == 0) return false;
  return nvs_erase_key(handle_, key) == ESP_OK;
}

bool NvsStore::commit() {
  if (handle_ == 0) return false;
  return nvs_commit(handle_) == ESP_OK;
}

}  // namespace agv::esp32
