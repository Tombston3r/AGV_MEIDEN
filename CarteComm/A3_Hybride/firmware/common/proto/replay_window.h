// Idempotence et anti-rejeu (brief §5.1 et §8.1).
//
// Deux protections distinctes, à ne pas confondre :
//
//  1. IDEMPOTENCE — si la même (node_id, seq) revient, on RÉ-ACQUITTE SANS
//     RÉ-EXÉCUTER. Sans ça, un ACK perdu déclenche une course en double.
//  2. ANTI-DÉSORDRE — sur transport non ordonné (SMS), une trame plus ancienne
//     que la dernière traitée pour ce nœud est REJETÉE. C'est la seule
//     protection contre un STOP arrivant avant le GOTO qu'il annule.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

enum class FrameVerdict : uint8_t {
  Accept,      // nouvelle commande, à exécuter
  Duplicate,   // déjà vue : ré-acquitter sans ré-exécuter
  OutOfOrder,  // plus ancienne que la dernière traitée : rejeter
  Expired,     // horodatage trop vieux (max_command_age_s)
};

constexpr size_t kReplayWindowMax = 32;
constexpr size_t kReplayNodesMax = 16;

class ReplayWindow {
 public:
  // `window` : nombre de (node_id, seq) mémorisés (16 par défaut, §5.1).
  // `ordered_transport` : true si le transport garantit l'ordre (MQTT sur TCP)
  //   et false s'il ne le garantit pas (SMS).
  //   En transport non ordonné, le verdict OutOfOrder est activé.
  ReplayWindow(size_t window, bool ordered_transport)
      : window_(window > kReplayWindowMax ? kReplayWindowMax : window),
        ordered_(ordered_transport) {}

  // Classe une trame reçue. `now_s` et `max_age_s` ne servent que si la trame
  // porte un horodatage ; `max_age_s == 0` désactive le contrôle de fraîcheur.
  FrameVerdict classify(uint16_t node_id, uint8_t seq, bool has_ts, uint32_t ts_s,
                        uint32_t now_s, uint32_t max_age_s) const;

  // Enregistre une trame acceptée. À n'appeler qu'après exécution effective.
  void remember(uint16_t node_id, uint8_t seq);

  void reset();
  size_t size() const { return count_; }

 private:
  struct Entry {
    uint16_t node_id;
    uint8_t seq;
  };
  struct NodeState {
    uint16_t node_id;
    uint8_t last_seq;
    bool used;
  };

  // Compare deux seq 8 bits roulants : true si `a` est postérieur à `b`.
  static bool seq_after(uint8_t a, uint8_t b) {
    return static_cast<int8_t>(static_cast<uint8_t>(a - b)) > 0;
  }

  Entry entries_[kReplayWindowMax] = {};
  size_t count_ = 0;
  size_t next_ = 0;  // index d'écriture circulaire
  NodeState nodes_[kReplayNodesMax] = {};
  size_t node_count_ = 0;
  size_t window_;
  bool ordered_;
};

}  // namespace agv
