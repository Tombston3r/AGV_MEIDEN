#include "app/persistent_store.h"

#include <cstring>

namespace agv {

RamStore::Entry* RamStore::find(const char* key) {
  for (auto& e : entries_) {
    if (e.used && std::strncmp(e.key, key, sizeof(e.key) - 1) == 0) return &e;
  }
  return nullptr;
}

size_t RamStore::read(const char* key, uint8_t* out, size_t capacity) {
  Entry* e = find(key);
  if (e == nullptr || e->len > capacity) return 0;
  std::memcpy(out, e->value, e->len);
  return e->len;
}

bool RamStore::write(const char* key, const uint8_t* data, size_t len) {
  if (len > kMaxValue) return false;
  Entry* e = find(key);
  if (e == nullptr) {
    for (auto& candidate : entries_) {
      if (!candidate.used) {
        e = &candidate;
        break;
      }
    }
    if (e == nullptr) return false;
    std::strncpy(e->key, key, sizeof(e->key) - 1);
    e->used = true;
  }
  std::memcpy(e->value, data, len);
  e->len = len;
  ++writes;
  return true;
}

bool RamStore::erase(const char* key) {
  Entry* e = find(key);
  if (e == nullptr) return false;
  e->used = false;
  e->len = 0;
  return true;
}

}  // namespace agv
