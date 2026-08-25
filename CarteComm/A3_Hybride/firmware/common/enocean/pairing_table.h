// Table d'appairage EnOcean : identifiant émetteur -> station (brief §7).
//
// Le PTM 210 envoie un identifiant 32 bits gravé en usine, PAS un numéro de
// station. Sans table d'appairage persistée, un bouton neuf est inutilisable et
// un bouton remplacé casse l'installation.
//
// Le mode appairage est déclenché depuis l'IHM web ou un bouton du poste :
// « appuyez sur le bouton à associer », puis le premier identifiant inconnu vu
// est associé à la station saisie.
#pragma once

#include <cstddef>
#include <cstdint>

#include "app/persistent_store.h"

namespace agv {

constexpr size_t kMaxPairings = 32;

struct Pairing {
  uint32_t enocean_id = 0;
  uint16_t station = 0;
  uint8_t speed = 0;
  uint8_t rocker = 0;  // permet 2 stations par bouton double (A et B)
  bool used = false;
};

class PairingTable {
 public:
  explicit PairingTable(IPersistentStore* store) : store_(store) {}

  // Recherche. False si l'identifiant n'est pas appairé.
  bool lookup(uint32_t enocean_id, uint8_t rocker, Pairing& out) const;
  // Ajoute ou remplace une association.
  bool set(uint32_t enocean_id, uint8_t rocker, uint16_t station, uint8_t speed);
  bool remove(uint32_t enocean_id, uint8_t rocker);
  size_t size() const;
  const Pairing& at(size_t i) const { return entries_[i]; }
  static constexpr size_t capacity() { return kMaxPairings; }

  bool save();
  size_t load();

  // --- Mode appairage ---
  // `station` / `speed` : ce qui sera associé au prochain bouton pressé.
  void start_pairing(uint16_t station, uint8_t speed, uint32_t now_s, uint32_t timeout_s);
  void cancel_pairing() { pairing_active_ = false; }
  bool pairing_active(uint32_t now_s) const {
    return pairing_active_ && (now_s - pairing_started_s_) < pairing_timeout_s_;
  }
  // À appeler sur chaque appui reçu pendant le mode appairage.
  bool complete_pairing(uint32_t enocean_id, uint8_t rocker, uint32_t now_s);
  uint16_t pairing_station() const { return pairing_station_; }

 private:
  static constexpr const char* kKey = "enocean_map";
  static constexpr uint8_t kBlobVersion = 1;

  IPersistentStore* store_;
  Pairing entries_[kMaxPairings] = {};

  bool pairing_active_ = false;
  uint16_t pairing_station_ = 0;
  uint8_t pairing_speed_ = 0;
  uint32_t pairing_started_s_ = 0;
  uint32_t pairing_timeout_s_ = 60;
};

}  // namespace agv
