#include "proto/json.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace agv::json {
namespace {

// Recherche `"key"` suivi de `:` et retourne le début de la valeur.
const char* find_value(const char* json, const char* key) {
  if (json == nullptr || key == nullptr) return nullptr;
  char needle[48];
  const int n = std::snprintf(needle, sizeof(needle), "\"%s\"", key);
  if (n <= 0 || static_cast<size_t>(n) >= sizeof(needle)) return nullptr;

  const char* p = std::strstr(json, needle);
  if (p == nullptr) return nullptr;
  p = std::strchr(p + n, ':');
  if (p == nullptr) return nullptr;
  ++p;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
  return p;
}

}  // namespace

void Writer::raw(const char* text) {
  const size_t n = std::strlen(text);
  if (overflow_ || len_ + n + 1 >= capacity_) {
    overflow_ = true;
    return;
  }
  std::memcpy(out_ + len_, text, n);
  len_ += n;
  out_[len_] = '\0';
}

void Writer::separator() {
  if (first_) {
    first_ = false;
  } else {
    raw(",");
  }
}

void Writer::begin() {
  len_ = 0;
  first_ = true;
  overflow_ = false;
  if (capacity_ > 1) {
    out_[0] = '{';
    out_[1] = '\0';
    len_ = 1;
  } else {
    overflow_ = true;
  }
}

void Writer::field(const char* key, int32_t value) {
  separator();
  char buf[48];
  std::snprintf(buf, sizeof(buf), "\"%s\":%ld", key, static_cast<long>(value));
  raw(buf);
}

void Writer::field(const char* key, uint32_t value) {
  separator();
  char buf[48];
  std::snprintf(buf, sizeof(buf), "\"%s\":%lu", key, static_cast<unsigned long>(value));
  raw(buf);
}

void Writer::field(const char* key, bool value) {
  separator();
  char buf[48];
  std::snprintf(buf, sizeof(buf), "\"%s\":%s", key, value ? "true" : "false");
  raw(buf);
}

void Writer::field(const char* key, const char* value) {
  separator();
  char buf[96];
  std::snprintf(buf, sizeof(buf), "\"%s\":\"%s\"", key, (value != nullptr) ? value : "");
  raw(buf);
}

size_t Writer::end() {
  raw("}");
  return overflow_ ? 0u : len_;
}

bool get_int(const char* json, const char* key, int32_t& out) {
  const char* p = find_value(json, key);
  if (p == nullptr) return false;
  if (*p != '-' && (*p < '0' || *p > '9')) return false;
  char* endptr = nullptr;
  const long value = std::strtol(p, &endptr, 10);
  if (endptr == p) return false;
  out = static_cast<int32_t>(value);
  return true;
}

bool get_bool(const char* json, const char* key, bool& out) {
  const char* p = find_value(json, key);
  if (p == nullptr) return false;
  if (std::strncmp(p, "true", 4) == 0) {
    out = true;
    return true;
  }
  if (std::strncmp(p, "false", 5) == 0) {
    out = false;
    return true;
  }
  return false;
}

bool get_string(const char* json, const char* key, char* out, size_t capacity) {
  const char* p = find_value(json, key);
  if (p == nullptr || *p != '"' || capacity == 0) return false;
  ++p;
  size_t i = 0;
  while (*p != '\0' && *p != '"' && i + 1 < capacity) out[i++] = *p++;
  out[i] = '\0';
  return *p == '"';
}

}  // namespace agv::json
