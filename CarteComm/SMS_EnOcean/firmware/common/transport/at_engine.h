// Machine à états AT — pile de dialogue modem (brief §8.1).
//
// Explicitement une machine à états : une pile AT écrite en séquentiel bloquant
// est impossible à sortir d'un modem muet, et c'est le mode de panne le plus
// fréquent sur site.
//
// Gère : découpage ligne à ligne, URC non sollicités (+CMTI, +CREG…), timeout
// par commande, détection de modem muet.
#pragma once

#include <cstddef>
#include <cstdint>

#include "hal/byte_stream.h"

namespace agv {

constexpr size_t kAtLineMax = 128;

enum class AtResult : uint8_t {
  Pending,
  Ok,
  Error,
  Timeout,
};

// Callback d'URC : ligne non sollicitée reçue hors réponse de commande.
class IAtUrcHandler {
 public:
  virtual ~IAtUrcHandler() = default;
  virtual void on_urc(const char* line) = 0;
};

class AtEngine {
 public:
  AtEngine(IByteStream& uart, uint32_t default_timeout_ms)
      : uart_(uart), timeout_ms_(default_timeout_ms) {}

  void set_urc_handler(IAtUrcHandler* h) { urc_ = h; }

  // Envoie une commande. False si une commande est déjà en cours.
  bool command(const char* cmd, uint32_t timeout_ms = 0);
  // Envoie des données brutes (corps d'un SMS suivi de Ctrl-Z, par exemple).
  bool raw(const uint8_t* data, size_t len);

  // À appeler en boucle. Retourne l'état de la commande courante.
  AtResult tick();

  bool busy() const { return busy_; }
  // Dernière ligne « utile » reçue (avant OK/ERROR).
  const char* last_response() const { return response_; }
  // Date de la dernière activité du modem : sert à détecter un modem muet.
  uint32_t last_rx_ms() const { return last_rx_ms_; }
  uint32_t timeouts() const { return timeouts_; }
  uint32_t errors() const { return errors_; }

  void reset();

 private:
  void handle_line(const char* line);

  IByteStream& uart_;
  IAtUrcHandler* urc_ = nullptr;
  uint32_t timeout_ms_;
  uint32_t cmd_timeout_ms_ = 0;
  uint32_t sent_at_ms_ = 0;
  uint32_t last_rx_ms_ = 0;
  uint32_t timeouts_ = 0;
  uint32_t errors_ = 0;

  char line_[kAtLineMax] = {};
  size_t line_len_ = 0;
  char response_[kAtLineMax] = {};

  bool busy_ = false;
  AtResult result_ = AtResult::Ok;
};

}  // namespace agv
