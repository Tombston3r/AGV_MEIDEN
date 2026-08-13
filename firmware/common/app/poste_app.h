// Logique du poste fixe (architectures A1 et A3), indépendante de la plateforme.
//
// Chaîne A3 : appui PTM 210 -> TCM 515 (ESP3) -> déduplication -> table
// d'appairage -> trame CMD_GOTO -> LoRa -> AGV.
//
// Tout ce qui est ici est testable en natif ; le serveur web, l'Ethernet et les
// UART vivent dans firmware/poste-esp32.
#pragma once

#include <cstdint>

#include "app/clock.h"
#include "bus/bus_signals.h"  // kStationMax / kSpeedMax : mêmes bornes que le bus
#include "config/hardware_profile.h"
#include "enocean/esp3.h"
#include "enocean/pairing_table.h"
#include "transport/itransport.h"

namespace agv {

struct AgvSnapshot {
  uint16_t station = 0;
  uint8_t speed = 0;
  bool moving = false;
  bool in_station = false;
  bool fault = false;
  uint32_t last_update_ms = 0;
  bool valid = false;
};

struct PosteStats {
  uint32_t enocean_telegrams = 0;
  uint32_t enocean_duplicates = 0;
  uint32_t enocean_unpaired = 0;   // bouton non appairé : appui ignoré
  uint32_t commands_sent = 0;
  uint32_t commands_refused = 0;   // transport occupé ou budget épuisé
  uint32_t acks_received = 0;
  uint32_t nacks_received = 0;
  uint32_t pairings_done = 0;
};

class PosteApp {
 public:
  PosteApp(const HardwareProfile& profile, IClock& clock, ITransport& transport,
           PairingTable& pairings)
      : profile_(profile),
        clock_(clock),
        transport_(transport),
        pairings_(pairings),
        dedup_(profile.enocean.dedup_window_ms) {}

  bool begin();
  void tick();

  // Injecte un octet reçu du TCM 515. Retourne true si un appui a été traité.
  bool feed_enocean(uint8_t byte);

  // Appel direct (bouton filaire du poste, IHM web, poste UniPi).
  bool request_goto(uint16_t station, uint8_t speed, uint8_t flags = 0);
  bool request_stop(bool purge_queue);

  // Mode appairage : « appuyez sur le bouton à associer ».
  void start_pairing(uint16_t station, uint8_t speed);
  bool pairing_active() const { return pairings_.pairing_active(clock_.now_s()); }

  const AgvSnapshot& snapshot() const { return snapshot_; }
  const PosteStats& stats() const { return stats_; }
  LinkHealth transport_health() const { return transport_.health(); }

  // Vue `agvdump` du poste : mêmes noms de champs qu'à bord (§3.3), mais
  // alimentés par la TÉLÉMÉTRIE reçue. Ce n'est pas le dump de l'AGV : la
  // source est indiquée dans l'en-tête pour qu'aucun atelier ne confonde les
  // deux. Le dump de référence reste celui servi par l'AGV en §9.4.
  size_t render_agvdump(char* out, size_t capacity) const;
  // Fraîcheur de liaison : âge de la dernière télémétrie, en ms.
  uint32_t telemetry_age_ms() const;
  // Le retour d'accusé vers le bouton EnOcean est-il possible ? (§12.8)
  bool operator_feedback_available() const { return !profile_.enocean.rx_only; }

 private:
  bool send_command(FrameType type, uint16_t station, uint8_t speed, uint8_t flags);

  const HardwareProfile& profile_;
  IClock& clock_;
  ITransport& transport_;
  PairingTable& pairings_;
  Esp3Decoder decoder_;
  EnoceanDeduplicator dedup_;
  AgvSnapshot snapshot_{};
  PosteStats stats_{};
  uint8_t tx_seq_ = 0;
};

}  // namespace agv
