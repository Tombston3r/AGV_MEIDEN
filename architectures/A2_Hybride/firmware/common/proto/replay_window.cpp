#include "proto/replay_window.h"

namespace agv {

FrameVerdict ReplayWindow::classify(uint16_t node_id, uint8_t seq, bool has_ts, uint32_t ts_s,
                                    uint32_t now_s, uint32_t max_age_s) const {
  // 1. Doublon : priorité absolue, y compris sur une trame périmée — un
  //    ré-envoi tardif d'une commande déjà exécutée doit être ré-acquitté,
  //    jamais réexécuté.
  for (size_t i = 0; i < count_; ++i) {
    if (entries_[i].node_id == node_id && entries_[i].seq == seq) {
      return FrameVerdict::Duplicate;
    }
  }

  // 2. Fraîcheur (§8.1) : une commande sortie du SMSC trois minutes plus tard
  //    ne doit jamais faire bouger l'AGV.
  if (has_ts && max_age_s != 0) {
    // `ts_s > now_s` : horloge de l'émetteur en avance, toléré (age = 0).
    const uint32_t age_s = (now_s > ts_s) ? (now_s - ts_s) : 0u;
    if (age_s > max_age_s) return FrameVerdict::Expired;
  }

  // 3. Désordre : seulement si le transport ne garantit pas l'ordre.
  if (!ordered_) {
    for (size_t i = 0; i < node_count_; ++i) {
      if (nodes_[i].used && nodes_[i].node_id == node_id) {
        if (!seq_after(seq, nodes_[i].last_seq)) return FrameVerdict::OutOfOrder;
        break;
      }
    }
  }
  return FrameVerdict::Accept;
}

void ReplayWindow::remember(uint16_t node_id, uint8_t seq) {
  entries_[next_] = Entry{node_id, seq};
  next_ = (next_ + 1) % window_;
  if (count_ < window_) ++count_;

  for (size_t i = 0; i < node_count_; ++i) {
    if (nodes_[i].used && nodes_[i].node_id == node_id) {
      if (seq_after(seq, nodes_[i].last_seq)) nodes_[i].last_seq = seq;
      return;
    }
  }
  if (node_count_ < kReplayNodesMax) {
    nodes_[node_count_++] = NodeState{node_id, seq, true};
  } else {
    // Plus de place : on recycle la plus ancienne entrée. Un site à plus de 16
    // nœuds impose de relever kReplayNodesMax, pas d'accepter la perte d'état.
    nodes_[0] = NodeState{node_id, seq, true};
  }
}

void ReplayWindow::reset() {
  count_ = 0;
  next_ = 0;
  node_count_ = 0;
}

}  // namespace agv
