// Stockage persistant abstrait (NVS sur ESP32, RAM en test).
//
// Isolé derrière une interface pour que la file de courses et la table
// d'appairage EnOcean soient testables en natif, sans flash.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

class IPersistentStore {
 public:
  virtual ~IPersistentStore() = default;
  // Retourne la taille lue, 0 si absent ou tampon trop petit.
  virtual size_t read(const char* key, uint8_t* out, size_t capacity) = 0;
  virtual bool write(const char* key, const uint8_t* data, size_t len) = 0;
  virtual bool erase(const char* key) = 0;
  virtual bool commit() = 0;
};

// Implémentation mémoire : tests natifs et mode dégradé si la NVS est illisible.
class RamStore final : public IPersistentStore {
 public:
  static constexpr size_t kMaxEntries = 8;
  static constexpr size_t kMaxValue = 256;

  size_t read(const char* key, uint8_t* out, size_t capacity) override;
  bool write(const char* key, const uint8_t* data, size_t len) override;
  bool erase(const char* key) override;
  bool commit() override {
    ++commits;
    return true;
  }

  uint32_t writes = 0;
  uint32_t commits = 0;

 private:
  struct Entry {
    char key[24] = {};
    uint8_t value[kMaxValue] = {};
    size_t len = 0;
    bool used = false;
  };
  Entry* find(const char* key);

  Entry entries_[kMaxEntries];
};

}  // namespace agv
