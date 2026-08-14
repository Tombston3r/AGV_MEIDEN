// Passerelle d'alerte SMS bas volume (brief §8.3).
//
// Usage résiduel LÉGITIME du cellulaire : prévenir un technicien de maintenance
// HORS SITE en cas de défaut bloquant. Quelques SMS par mois, ~10 €/an.
//
// CONTRAINTE STRUCTURELLE : ce module est indépendant de la chaîne de commande
// et NE DOIT JAMAIS POUVOIR LA PILOTER. Il n'a donc accès ni au séquenceur, ni
// à la file de courses — seulement à un flux d'octets vers le modem. Cette
// séparation est architecturale, pas une simple convention de nommage.
#pragma once

#include <cstddef>
#include <cstdint>

#include "config/hardware_profile.h"
#include "hal/byte_stream.h"
#include "transport/at_engine.h"

namespace agv {

enum class AlertKind : uint8_t {
  BlockingFault,   // défaut bloquant de l'AGV
  LinkLost,        // liaison perdue durablement
  PlcFault,        // défaut automate
  Recovered,       // retour à la normale
};

class AlertGateway {
 public:
  AlertGateway(const CellularConfig& cfg, IByteStream& uart)
      : cfg_(cfg), at_(uart, cfg.at_timeout_ms), uart_(uart) {}

  // Demande d'envoi. Retourne false si le quota journalier est atteint : un
  // équipement en défaut permanent ne doit pas vider le forfait ni harceler le
  // technicien.
  bool raise(AlertKind kind, const char* detail, uint32_t now_s);
  void tick();

  uint32_t sent_today() const { return sent_today_; }
  uint32_t suppressed() const { return suppressed_; }
  bool busy() const { return pending_ || at_.busy(); }

  static const char* kind_str(AlertKind k);

 private:
  enum class Step : uint8_t { Idle, TextMode, Prompt, Body };

  const CellularConfig& cfg_;
  AtEngine at_;
  IByteStream& uart_;

  Step step_ = Step::Idle;
  bool pending_ = false;
  char message_[128] = {};
  uint32_t day_index_ = 0;
  uint32_t sent_today_ = 0;
  uint32_t suppressed_ = 0;
};

}  // namespace agv
