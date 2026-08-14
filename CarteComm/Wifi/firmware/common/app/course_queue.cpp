#include "app/course_queue.h"

#include "proto/crc16.h"

namespace agv {
namespace {

constexpr size_t kRecordSize = 12;  // station2 + speed1 + node2 + seq1 + ts4 + flags1 + pad1

void put_u16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v & 0xFFu);
}
uint16_t get_u16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

void put_u32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v >> 24);
  p[1] = static_cast<uint8_t>(v >> 16);
  p[2] = static_cast<uint8_t>(v >> 8);
  p[3] = static_cast<uint8_t>(v & 0xFFu);
}
uint32_t get_u32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

}  // namespace

bool CourseQueue::push(const Course& c) {
  if (full()) return false;
  courses_[count_++] = c;
  return true;
}

bool CourseQueue::push_front(const Course& c) {
  if (full()) return false;
  for (size_t i = count_; i > 0; --i) courses_[i] = courses_[i - 1];
  courses_[0] = c;
  ++count_;
  return true;
}

bool CourseQueue::pop(Course& out) {
  if (count_ == 0) return false;
  out = courses_[0];
  for (size_t i = 1; i < count_; ++i) courses_[i - 1] = courses_[i];
  --count_;
  return true;
}

bool CourseQueue::peek(Course& out) const {
  if (count_ == 0) return false;
  out = courses_[0];
  return true;
}

void CourseQueue::clear() { count_ = 0; }

bool CourseQueue::save(uint32_t now_s) {
  if (!cfg_.persist_to_nvs || store_ == nullptr) return false;
  uint8_t blob[2 + kMaxCourses * kRecordSize + 4 + 2];
  size_t i = 0;
  blob[i++] = kBlobVersion;
  blob[i++] = static_cast<uint8_t>(count_);
  for (size_t c = 0; c < count_; ++c) {
    const Course& course = courses_[c];
    put_u16(&blob[i], course.station); i += 2;
    blob[i++] = course.speed;
    put_u16(&blob[i], course.node_id); i += 2;
    blob[i++] = course.seq;
    put_u32(&blob[i], course.enqueued_at_s); i += 4;
    blob[i++] = course.flags;
    blob[i++] = 0;  // réservé, garde l'enregistrement aligné
  }
  put_u32(&blob[i], now_s); i += 4;  // horodatage de sauvegarde
  const uint16_t crc = crc16_ccitt(blob, i);
  put_u16(&blob[i], crc); i += 2;
  return store_->write(kKey, blob, i) && store_->commit();
}

size_t CourseQueue::restore(uint32_t now_s, size_t* dropped) {
  if (dropped != nullptr) *dropped = 0;
  count_ = 0;
  if (!cfg_.persist_to_nvs || store_ == nullptr) return 0;

  uint8_t blob[2 + kMaxCourses * kRecordSize + 4 + 2];
  const size_t len = store_->read(kKey, blob, sizeof(blob));
  if (len < 8) return 0;

  const uint16_t crc_calc = crc16_ccitt(blob, len - 2);
  if (crc_calc != get_u16(&blob[len - 2])) return 0;  // blob corrompu : file vide
  if (blob[0] != kBlobVersion) return 0;

  const size_t stored = blob[1];
  if (stored > kMaxCourses) return 0;
  if (len != 2 + stored * kRecordSize + 6) return 0;

  const uint32_t validity_s = cfg_.course_validity_min * 60u;
  size_t i = 2;
  size_t dropped_count = 0;
  for (size_t c = 0; c < stored; ++c) {
    Course course;
    course.station = get_u16(&blob[i]); i += 2;
    course.speed = blob[i++];
    course.node_id = get_u16(&blob[i]); i += 2;
    course.seq = blob[i++];
    course.enqueued_at_s = get_u32(&blob[i]); i += 4;
    course.flags = blob[i++];
    ++i;  // octet réservé

    // Politique de validité : une course trop vieille est écartée. Sans horloge
    // murale valide (now_s == 0), on ne peut rien décider — on restaure tout.
    const bool clock_ok = now_s != 0 && course.enqueued_at_s != 0;
    const bool expired = clock_ok && validity_s != 0 && now_s > course.enqueued_at_s &&
                         (now_s - course.enqueued_at_s) > validity_s;
    if (expired) {
      ++dropped_count;
      continue;
    }
    if (!full()) courses_[count_++] = course;
  }
  if (dropped != nullptr) *dropped = dropped_count;
  return count_;
}

}  // namespace agv
