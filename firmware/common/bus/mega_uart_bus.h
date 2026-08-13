// Variante C du §4.4 : ATmega2560 conservé comme organe de pose du bus.
//
// Le MEGA garde sa propriété gratuite (`PORTA = x`, < 1 µs, simultané) et
// l'ESP32 lui parle par un protocole inter-MCU UART, à définir — c'est fait
// ici, et documenté dans docs/protocole_mega.md :
//
//   ESP32 -> MEGA : A5 | cmd | len | payload[len] | crc16(cmd..payload)
//   MEGA  -> ESP32: 5A | cmd | len | payload[len] | crc16
//
//   cmd 0x01 SET_X  payload 3 octets (22 bits utiles)   -> réponse ACK vide
//   cmd 0x02 GET_Y  payload vide                        -> réponse 3 octets
//   cmd 0x03 PULSE  payload bit(1) + duree_us(2, BE)    -> réponse ACK vide
//   cmd 0x04 PING   payload vide                        -> réponse version(1)
//
// Le MEGA applique lui-même t_setup et la polarité ? NON : il reste un simple
// organe de pose. Toute la logique — donc tous les paramètres du §12 — demeure
// dans l'ESP32, sinon deux firmwares porteraient la même vérité.
#pragma once

#include <cstdint>

#include "bus/debounce.h"
#include "bus/ibus_driver.h"
#include "config/hardware_profile.h"
#include "hal/bus_ports.h"
#include "hal/byte_stream.h"

namespace agv {

constexpr uint8_t kMegaSofRequest = 0xA5;
constexpr uint8_t kMegaSofReply = 0x5A;
constexpr uint8_t kMegaCmdSetX = 0x01;
constexpr uint8_t kMegaCmdGetY = 0x02;
constexpr uint8_t kMegaCmdPulse = 0x03;
constexpr uint8_t kMegaCmdPing = 0x04;

class MegaUartBus final : public IBusDriver {
 public:
  MegaUartBus(const HardwareProfile& profile, IByteStream& uart, IMicroClock& clock,
              uint32_t reply_timeout_us = 20000)
      : profile_(profile), uart_(uart), clock_(clock), reply_timeout_us_(reply_timeout_us),
        debouncer_(profile.bus.y_debounce_us) {}

  bool begin() override;
  bool writeX(uint32_t word) override;
  uint32_t lastX() const override { return last_x_; }
  uint32_t readY() override;
  bool pulse(uint8_t x_bit, uint32_t duration_us) override;
  const char* name() const override { return "mega_uart"; }
  const BusStats& stats() const override { return stats_; }
  uint64_t now_us() const override { return clock_.now_us(); }

  uint32_t desync_count() const { return desyncs_; }
  uint8_t peer_version() const { return peer_version_; }

  // Exposés pour les tests : encodage d'une requête et lecture d'une réponse.
  static size_t encode_request(uint8_t cmd, const uint8_t* payload, uint8_t len, uint8_t* out,
                               size_t capacity);

 private:
  bool exchange(uint8_t cmd, const uint8_t* payload, uint8_t len, uint8_t* reply,
                uint8_t& reply_len);

  const HardwareProfile& profile_;
  IByteStream& uart_;
  IMicroClock& clock_;
  uint32_t reply_timeout_us_;
  YDebouncer debouncer_;
  BusStats stats_{};
  uint32_t last_x_ = 0;
  uint32_t desyncs_ = 0;
  uint8_t peer_version_ = 0;
};

}  // namespace agv
