// Application de l'ATmega2560 (planification §2.1 à §2.5).
//
// C'est le microcontrôleur qui porte la mission : séquenceur trois phases,
// file de 5 courses, décodage de position. Il ne connaît ni le Wi-Fi, ni MQTT,
// ni le poste fixe, seulement une liaison série et un heartbeat.
//
// REPLI DE SÉCURITÉ NON NÉGOCIABLE (planification §2) :
// si le heartbeat de l'ESP32 disparaît plus de `heartbeat.timeout_ms`, l'ATmega
//   1. laisse la course en cours aller jusqu'au point d'arrêt SUIVANT,
//   2. refuse toute nouvelle course,
//   3. le signale dans son état (`kSafeStop`),
// jusqu'au retour du heartbeat. Ce repli ne dépend d'aucun réseau : c'est
// précisément ce qui le rend utile le jour où l'infrastructure tombe.
//
// ⚠ Ce n'est PAS un organe de sécurité (brief §3.1). L'arrêt d'urgence, les
// bumpers et le scrutateur laser restent dans une chaîne indépendante
// conforme à l'ISO 3691-4.
#pragma once

#include <cstdint>

#include "app/course_queue.h"
#include "app/sequencer.h"
#include "bus/bus_signals.h"  // kStationMax / kSpeedMax
#include "config/hardware_profile.h"
#include "link/link_protocol.h"

namespace agv {

struct MegaStats {
  uint32_t goto_accepted = 0;
  uint32_t goto_duplicate = 0;   // ré-acquittée sans être ré-exécutée
  uint32_t goto_refused_full = 0;
  uint32_t goto_refused_safe_stop = 0;
  uint32_t stops = 0;
  uint32_t heartbeat_losses = 0;
  uint32_t link_crc_errors = 0;
  uint32_t frames_in = 0;
  uint32_t frames_out = 0;
};

// Sortie série vers l'ESP32. Abstraite pour que toute l'application soit
// testable en natif, sans AVR.
class ILinkWriter {
 public:
  virtual ~ILinkWriter() = default;
  virtual void write(const uint8_t* data, size_t len) = 0;
};

class MegaApp {
 public:
  MegaApp(const HardwareProfile& profile, Sequencer& seq, CourseQueue& queue, ILinkWriter& out)
      : profile_(profile), seq_(seq), queue_(queue), out_(out), parser_(link::kSofToMega) {}

  bool begin(uint32_t now_ms);

  // Injecte un octet reçu de l'ESP32.
  void feed(uint8_t byte, uint32_t now_ms);

  // À appeler en boucle : séquenceur + surveillance du heartbeat.
  void tick(uint32_t now_ms);

  bool safe_stop_active() const { return safe_stop_; }
  bool heartbeat_ok() const { return heartbeat_ok_; }
  uint32_t last_heartbeat_ms() const { return last_heartbeat_ms_; }
  const MegaStats& stats() const { return stats_; }
  link::LinkState snapshot() const;

 private:
  void handle(link::Cmd cmd, const uint8_t* payload, uint8_t len, uint32_t now_ms);
  void send_ack(uint8_t seq, link::CmdResult result);
  void send_state();
  bool already_seen(uint8_t seq) const;
  void remember(uint8_t seq);

  const HardwareProfile& profile_;
  Sequencer& seq_;
  CourseQueue& queue_;
  ILinkWriter& out_;
  link::Parser parser_;
  MegaStats stats_{};

  uint32_t last_heartbeat_ms_ = 0;
  bool heartbeat_ok_ = false;
  bool safe_stop_ = false;
  bool started_ = false;

  // Idempotence : les 8 dernières séquences vues (brief §5.1). Volontairement
  // court : la mémoire de l'ATmega est comptée, et l'ESP32 filtre déjà.
  static constexpr uint8_t kSeenMax = 8;
  uint8_t seen_[kSeenMax] = {};
  uint8_t seen_count_ = 0;
  uint8_t seen_next_ = 0;
  uint8_t last_seq_ = 0;
};

}  // namespace agv
