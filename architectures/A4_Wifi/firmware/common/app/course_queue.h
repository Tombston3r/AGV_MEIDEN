// File de courses — LE point fonctionnel central du projet (brief §1 et §4.5).
//
// L'AGV ne connaît qu'UNE SEULE destination à la fois et l'oublie au
// redémarrage. C'est la carte qui porte la mémoire de mission : jusqu'à
// 5 courses en attente.
//
// Amélioration par rapport à la V5.0.1 : persistance en NVS avec restauration
// au boot et politique de validité (une course plus vieille que
// `course_validity_min` est écartée).
//
// Noms de champs compatibles `agvdump` : nb_courses_programmed,
// programmed_courses[5].
#pragma once

#include <cstddef>
#include <cstdint>

#include "app/persistent_store.h"
#include "config/hardware_profile.h"

namespace agv {

constexpr size_t kMaxCourses = 5;

struct Course {
  uint16_t station = 0;
  uint8_t speed = 0;
  uint16_t node_id = 0;   // demandeur, pour la traçabilité et l'ACK
  uint8_t seq = 0;
  uint32_t enqueued_at_s = 0;
  uint8_t flags = 0;
};

class CourseQueue {
 public:
  CourseQueue(const QueueConfig& cfg, IPersistentStore* store)
      : cfg_(cfg), store_(store) {}

  // Empile en queue ; en tête si `flag::kPriority`. False si la file est pleine.
  bool push(const Course& c);
  bool push_front(const Course& c);
  // Dépile la course la plus ancienne. False si vide.
  bool pop(Course& out);
  bool peek(Course& out) const;
  void clear();

  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ >= capacity(); }
  size_t capacity() const {
    return cfg_.max_courses < kMaxCourses ? cfg_.max_courses : kMaxCourses;
  }
  const Course& at(size_t index) const { return courses_[index]; }

  // Persistance (§4.5). `now_s` = horloge murale ; sans horloge valide,
  // passer 0 : la politique de validité est alors inopérante et la file est
  // restaurée telle quelle — comportement à documenter à l'exploitation.
  bool save(uint32_t now_s);
  // Restaure et écarte les courses périmées. Retourne le nombre de courses
  // restaurées ; `dropped` reçoit le nombre de courses écartées.
  size_t restore(uint32_t now_s, size_t* dropped = nullptr);

 private:
  static constexpr uint8_t kBlobVersion = 1;
  static constexpr const char* kKey = "courses";

  const QueueConfig& cfg_;
  IPersistentStore* store_;
  Course courses_[kMaxCourses] = {};
  size_t count_ = 0;
};

}  // namespace agv
