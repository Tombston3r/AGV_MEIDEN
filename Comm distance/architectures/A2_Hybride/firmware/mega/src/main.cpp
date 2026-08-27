// Firmware ATmega2560 de la carte AIO AGV Control V6.0 : RÉÉCRIT.
//
// Le firmware d'origine n'est pas disponible (ni sources, ni lecture de flash
// garantie : planification 0.5). Celui-ci est écrit à partir du comportement
// documenté du bus MEIDEN, pas d'une rétro-ingénierie du binaire.
//
// Ce microcontrôleur porte la mission (planification §2.1 à §2.5) :
//   - séquenceur trois phases du bus X/Y,
//   - file de 5 courses,
//   - décodage de la position 10 bits et de la vitesse 4 bits,
//   - repli de sécurité sur perte du heartbeat de l'ESP32.
//
// Il ne connaît ni la radio, ni les boutons. C'est délibéré : le jour où la
// liaison LoRa tombe, l'AGV reste piloté par un microcontrôleur qui n'en
// dépend pas et décide seul de l'arrêt sûr.
//
// ⚠ CE N'EST PAS UN ORGANE DE SÉCURITÉ (brief §3.1). L'arrêt d'urgence, les
// bumpers et le scrutateur laser restent dans une chaîne indépendante conforme
// à l'ISO 3691-4.
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <util/atomic.h>

#include "app/course_queue.h"
#include "app/mega_app.h"
#include "app/persistent_store.h"
#include "app/sequencer.h"
#include "board_ports.h"
#include "bus/avr_port_bus.h"
#include "config/hardware_profile.h"

namespace {

// --- Ports matériels -------------------------------------------------------

class AvrClock final : public agv::IMicroClock {
 public:
  uint64_t now_us() const override {
    // `micros()` déborde toutes les ~70 minutes : on étend à 64 bits, sinon un
    // AGV qui roule depuis plus d'une heure verrait tous ses timeouts sauter.
    const uint32_t now = micros();
    if (now < last_) ++wraps_;
    last_ = now;
    return (static_cast<uint64_t>(wraps_) << 32) | now;
  }
  void delay_us(uint32_t us) override { delayMicroseconds(us); }

 private:
  mutable uint32_t last_ = 0;
  mutable uint32_t wraps_ = 0;
};

class AvrCritical final : public agv::ICriticalSection {
 public:
  void enter() override {
    saved_ = SREG;
    cli();
  }
  void leave() override { SREG = saved_; }

 private:
  uint8_t saved_ = 0;
};

// La liaison vers l'ESP32 est câblée sur D52/D53, qui ne sont pas des broches
// d'UART matériel : voir board_ports.h. Les trois UART du MEGA ont leur RX
// occupé par un signal du bus.
SoftwareSerial g_link(agv::board::kLinkRxPin, agv::board::kLinkTxPin);

class SerialWriter final : public agv::ILinkWriter {
 public:
  void write(const uint8_t* data, size_t len) override { g_link.write(data, len); }
};

// --- Objets applicatifs (allocation statique : pas de tas sur AVR) ---------

AvrClock g_clock;
AvrCritical g_critical;
SerialWriter g_writer;
agv::RamStore g_store;  // la file vit en RAM (planification §2.3)

agv::AvrPortBus* g_bus = nullptr;
agv::CourseQueue* g_queue = nullptr;
agv::Sequencer* g_seq = nullptr;
agv::MegaApp* g_app = nullptr;

// --- Mode découverte du brochage (planification 0.4) ----------------------
//
// Active une sortie X à la fois et annonce laquelle sur la liaison série.
// Permet de relever au multimètre la correspondance signal <-> broche SUB-D,
// automate DÉBRANCHÉ. Sans ce relevé, la table de board_ports.h reste une
// hypothèse, et un mot d'adresse mal câblé envoie l'AGV à la mauvaise station.
bool g_discovery = false;
uint8_t g_discovery_bit = 0;
uint32_t g_discovery_last_ms = 0;
constexpr uint32_t kDiscoveryStepMs = 3000;

void discovery_tick(uint32_t now_ms) {
  if (now_ms - g_discovery_last_ms < kDiscoveryStepMs) return;
  g_discovery_last_ms = now_ms;
  g_bus->drive_single(g_discovery_bit);
  g_link.print(F("# DECOUVERTE bit X="));
  g_link.println(g_discovery_bit);
  Serial.print(F("# DECOUVERTE bit X="));
  Serial.println(g_discovery_bit);
  g_discovery_bit = static_cast<uint8_t>((g_discovery_bit + 1) % 22);
}

}  // namespace

void setup() {
  const agv::HardwareProfile& profile = agv::default_profile();

  // Console de mise au point (USB), jamais dans le chemin de commande.
  Serial.begin(115200);
  g_link.begin(profile.link.baud);
  g_link.listen();

  static agv::AvrBusMap bus_map = agv::board::bus_map();
  static agv::AvrPortBus bus(profile, bus_map, g_critical, g_clock);
  static agv::CourseQueue queue(profile.queue, &g_store);
  static agv::Sequencer seq(profile, bus, queue);
  static agv::MegaApp app(profile, seq, queue, g_writer);

  g_bus = &bus;
  g_queue = &queue;
  g_seq = &seq;
  g_app = &app;

  // begin() met le bus X à zéro AVANT de configurer les broches en sortie.
  if (!app.begin(millis())) {
    Serial.println(F("# ECHEC init sequenceur : maintien en etat sur"));
  }

  Serial.print(F("# ATmega2560 AGV MEIDEN - firmware reecrit, pose en "));
  Serial.print(bus.port_writes_per_pose());
  Serial.println(F(" ecritures de port"));
  Serial.println(F("# 'd' = mode decouverte du brochage SUB-D, 'n' = normal"));
}

void loop() {
  const uint32_t now_ms = millis();

  // Commandes de l'ESP32.
  while (g_link.available() > 0) {
    g_app->feed(static_cast<uint8_t>(g_link.read()), now_ms);
  }

  // Console de mise au point : bascule du mode découverte.
  if (Serial.available() > 0) {
    const int c = Serial.read();
    if (c == 'd') {
      g_discovery = true;
      Serial.println(F("# MODE DECOUVERTE : automate DEBRANCHE obligatoire"));
    } else if (c == 'n') {
      g_discovery = false;
      g_bus->writeX(0);
      Serial.println(F("# mode normal"));
    }
  }

  if (g_discovery) {
    // En découverte, le séquenceur est gelé : on ne pilote rien d'autre.
    discovery_tick(now_ms);
    return;
  }

  // Séquenceur + surveillance du heartbeat.
  g_app->tick(now_ms);
}
