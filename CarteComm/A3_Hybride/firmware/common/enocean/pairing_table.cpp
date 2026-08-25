#include "enocean/pairing_table.h"

#include "proto/crc16.h"

namespace agv {
namespace {
constexpr size_t kRecordSize = 8;  // id4 + station2 + speed1 + rocker1
}

bool PairingTable::lookup(uint32_t enocean_id, uint8_t rocker, Pairing& out) const {
  for (const auto& e : entries_) {
    if (e.used && e.enocean_id == enocean_id && e.rocker == rocker) {
      out = e;
      return true;
    }
  }
  return false;
}

bool PairingTable::set(uint32_t enocean_id, uint8_t rocker, uint16_t station, uint8_t speed) {
  for (auto& e : entries_) {
    if (e.used && e.enocean_id == enocean_id && e.rocker == rocker) {
      e.station = station;
      e.speed = speed;
      return true;
    }
  }
  for (auto& e : entries_) {
    if (!e.used) {
      e = Pairing{enocean_id, station, speed, rocker, true};
      return true;
    }
  }
  return false;  // table pleine : un bouton de plus exige de relever kMaxPairings
}

bool PairingTable::remove(uint32_t enocean_id, uint8_t rocker) {
  for (auto& e : entries_) {
    if (e.used && e.enocean_id == enocean_id && e.rocker == rocker) {
      e.used = false;
      return true;
    }
  }
  return false;
}

size_t PairingTable::size() const {
  size_t n = 0;
  for (const auto& e : entries_) {
    if (e.used) ++n;
  }
  return n;
}

bool PairingTable::save() {
  if (store_ == nullptr) return false;
  uint8_t blob[2 + kMaxPairings * kRecordSize + 2];
  size_t i = 0;
  blob[i++] = kBlobVersion;
  const size_t count = size();
  blob[i++] = static_cast<uint8_t>(count);
  for (const auto& e : entries_) {
    if (!e.used) continue;
    blob[i++] = static_cast<uint8_t>(e.enocean_id >> 24);
    blob[i++] = static_cast<uint8_t>(e.enocean_id >> 16);
    blob[i++] = static_cast<uint8_t>(e.enocean_id >> 8);
    blob[i++] = static_cast<uint8_t>(e.enocean_id & 0xFFu);
    blob[i++] = static_cast<uint8_t>(e.station >> 8);
    blob[i++] = static_cast<uint8_t>(e.station & 0xFFu);
    blob[i++] = e.speed;
    blob[i++] = e.rocker;
  }
  const uint16_t crc = crc16_ccitt(blob, i);
  blob[i++] = static_cast<uint8_t>(crc >> 8);
  blob[i++] = static_cast<uint8_t>(crc & 0xFFu);
  return store_->write(kKey, blob, i) && store_->commit();
}

size_t PairingTable::load() {
  for (auto& e : entries_) e.used = false;
  if (store_ == nullptr) return 0;

  uint8_t blob[2 + kMaxPairings * kRecordSize + 2];
  const size_t len = store_->read(kKey, blob, sizeof(blob));
  if (len < 4) return 0;
  const uint16_t crc_recv = static_cast<uint16_t>((blob[len - 2] << 8) | blob[len - 1]);
  if (crc16_ccitt(blob, len - 2) != crc_recv) return 0;
  if (blob[0] != kBlobVersion) return 0;

  const size_t count = blob[1];
  if (len != 2 + count * kRecordSize + 2 || count > kMaxPairings) return 0;

  size_t i = 2;
  for (size_t c = 0; c < count; ++c) {
    const uint32_t id = (static_cast<uint32_t>(blob[i]) << 24) |
                        (static_cast<uint32_t>(blob[i + 1]) << 16) |
                        (static_cast<uint32_t>(blob[i + 2]) << 8) |
                        static_cast<uint32_t>(blob[i + 3]);
    const uint16_t station = static_cast<uint16_t>((blob[i + 4] << 8) | blob[i + 5]);
    set(id, blob[i + 7], station, blob[i + 6]);
    i += kRecordSize;
  }
  return size();
}

void PairingTable::start_pairing(uint16_t station, uint8_t speed, uint32_t now_s,
                                 uint32_t timeout_s) {
  pairing_active_ = true;
  pairing_station_ = station;
  pairing_speed_ = speed;
  pairing_started_s_ = now_s;
  pairing_timeout_s_ = timeout_s;
}

bool PairingTable::complete_pairing(uint32_t enocean_id, uint8_t rocker, uint32_t now_s) {
  if (!pairing_active(now_s)) return false;
  const bool ok = set(enocean_id, rocker, pairing_station_, pairing_speed_);
  pairing_active_ = false;
  if (ok) save();
  return ok;
}

}  // namespace agv
