#include "transport/mqtt_lte_transport.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace agv {
namespace {
constexpr const char* kPayloadPrefix = "AGV:";
}

void MqttLteTransport::enter(State s) {
  state_ = s;
  state_since_ms_ = uart_.now_ms();
  step_ = 0;
}

bool MqttLteTransport::begin() {
  power_.pulse_pwrkey(profile_.cellular.pwrkey_on_ms);
  enter(State::PowerOn);
  return true;
}

const char* MqttLteTransport::topic_for(FrameType t) const {
  switch (t) {
    case FrameType::Ack: return "ack";
    case FrameType::Telemetry: return "telemetry";
    case FrameType::CmdGoto:
    case FrameType::CmdStop: return "cmd";
    default: return "status";
  }
}

void MqttLteTransport::on_urc(const char* line) {
  // +SMSUB: "agv/1/cmd","AGV:0102..."
  if (std::strncmp(line, "+SMSUB:", 7) == 0) {
    const char* payload = std::strstr(line, kPayloadPrefix);
    if (payload == nullptr) return;
    payload += std::strlen(kPayloadPrefix);
    uint8_t packet[kSecurePacketMax];
    size_t n = 0;
    for (size_t i = 0; payload[i] != '\0' && payload[i + 1] != '\0'; i += 2) {
      const auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = nib(payload[i]);
      const int lo = nib(payload[i + 1]);
      if (hi < 0 || lo < 0 || n >= sizeof(packet)) break;
      packet[n++] = static_cast<uint8_t>((hi << 4) | lo);
    }
    Frame f;
    if (n > 0 && channel_.open(packet, n, f, profile_.protocol.version)) {
      ++health_.rx_ok;
      health_.last_rx_ms = uart_.now_ms();
      if (rx_count_ < kRxQueue) rx_queue_[rx_count_++] = f;
    } else {
      ++health_.rx_bad_crc;
    }
    return;
  }
  if (std::strncmp(line, "+CSQ:", 5) == 0) {
    const int raw = std::atoi(line + 5);
    health_.rssi_dbm = (raw == 99) ? 0 : static_cast<int16_t>(-113 + 2 * raw);
    return;
  }
  if (std::strncmp(line, "+SMSTATE:", 9) == 0) {
    health_.up = std::atoi(line + 9) != 0;
    return;
  }
}

void MqttLteTransport::advance_step() {
  char cmd[160];
  switch (state_) {
    case State::Attach: {
      static const char* kSteps[] = {"AT", "ATE0", "AT+CMEE=1", "AT+CPIN?", "AT+CGATT?"};
      if (step_ < sizeof(kSteps) / sizeof(kSteps[0])) {
        at_.command(kSteps[step_]);
      } else if (step_ == sizeof(kSteps) / sizeof(kSteps[0])) {
        std::snprintf(cmd, sizeof(cmd), "AT+CNCFG=0,1,\"%s\"", profile_.cellular.apn);
        at_.command(cmd);
      } else if (step_ == sizeof(kSteps) / sizeof(kSteps[0]) + 1) {
        at_.command("AT+CNACT=0,1", 20000);
      } else {
        enter(State::Configure);
        advance_step();
      }
      break;
    }
    case State::Configure: {
      switch (step_) {
        case 0:
          std::snprintf(cmd, sizeof(cmd), "AT+SMCONF=\"URL\",\"%s\",%u",
                        profile_.cellular.mqtt_host,
                        static_cast<unsigned>(profile_.cellular.mqtt_port));
          at_.command(cmd);
          break;
        case 1:
          std::snprintf(cmd, sizeof(cmd), "AT+SMCONF=\"CLIENTID\",\"%s\"",
                        profile_.cellular.mqtt_client_id);
          at_.command(cmd);
          break;
        case 2:
          std::snprintf(cmd, sizeof(cmd), "AT+SMCONF=\"KEEPTIME\",%u",
                        static_cast<unsigned>(profile_.cellular.mqtt_keepalive_s));
          at_.command(cmd);
          break;
        case 3:
          // Last Will and Testament : détection immédiate de perte de l'AGV
          // côté broker (§8.2). Sans lui, la perte n'apparaît qu'au timeout.
          std::snprintf(cmd, sizeof(cmd), "AT+SMCONF=\"TOPIC\",\"agv/%s/status\"",
                        profile_.cellular.mqtt_client_id);
          at_.command(cmd);
          break;
        case 4:
          at_.command("AT+SMCONF=\"MESSAGE\",\"OFFLINE\"");
          break;
        case 5:
          at_.command("AT+SMCONF=\"RETAIN\",1");
          break;
        case 6:
          at_.command("AT+SMCONF=\"QOS\",1");
          break;
        default:
          enter(State::Connecting);
          at_.command("AT+SMCONN", 30000);
          break;
      }
      break;
    }
    default:
      break;
  }
}

void MqttLteTransport::tick() {
  power_.kick_hardware_watchdog();
  const AtResult res = at_.tick();
  const uint32_t now = uart_.now_ms();
  char cmd[160];

  if (state_ != State::PowerOff && state_ != State::PowerOn && state_ != State::Recovering &&
      at_.last_rx_ms() != 0 &&
      (now - at_.last_rx_ms()) > profile_.cellular.modem_mute_timeout_ms) {
    health_.up = false;
    power_.pulse_pwrkey(profile_.cellular.pwrkey_off_ms);
    at_.reset();
    enter(State::Recovering);
    return;
  }

  switch (state_) {
    case State::PowerOff:
      break;

    case State::PowerOn:
      if ((now - state_since_ms_) >= profile_.cellular.pwrkey_on_ms) {
        enter(State::Attach);
        advance_step();
      }
      break;

    case State::Attach:
    case State::Configure:
      if (at_.busy()) break;
      if (res == AtResult::Ok) {
        ++step_;
        advance_step();
      } else {
        power_.pulse_pwrkey(profile_.cellular.pwrkey_off_ms);
        enter(State::Recovering);
      }
      break;

    case State::Connecting:
      if (at_.busy()) break;
      if (res == AtResult::Ok) {
        std::snprintf(cmd, sizeof(cmd), "AT+SMSUB=\"agv/%s/cmd\",%u",
                      profile_.cellular.mqtt_client_id,
                      static_cast<unsigned>(profile_.cellular.mqtt_qos));
        at_.command(cmd);
        enter(State::Subscribing);
      } else {
        enter(State::Recovering);
      }
      break;

    case State::Subscribing:
      if (at_.busy()) break;
      if (res == AtResult::Ok) {
        health_.up = true;
        // Statut « en ligne » retenu : contrepartie du LWT.
        std::snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"agv/%s/status\",6,1,1",
                      profile_.cellular.mqtt_client_id);
        at_.command(cmd);
        enter(State::Online);
      } else {
        enter(State::Recovering);
      }
      break;

    case State::Online:
      if (at_.busy()) break;
      if (std::strcmp(at_.last_response(), ">") == 0) {
        at_.raw(reinterpret_cast<const uint8_t*>("ONLINE"), 6);
        break;
      }
      if (tx_pending_) {
        char hex[kSecurePacketMax * 2 + 8];
        std::memcpy(hex, kPayloadPrefix, std::strlen(kPayloadPrefix));
        static const char* digits = "0123456789ABCDEF";
        size_t o = std::strlen(kPayloadPrefix);
        for (size_t i = 0; i < tx_len_; ++i) {
          hex[o++] = digits[(tx_buf_[i] >> 4) & 0x0Fu];
          hex[o++] = digits[tx_buf_[i] & 0x0Fu];
        }
        hex[o] = '\0';
        std::snprintf(topic_buf_, sizeof(topic_buf_), "agv/%s/%s",
                      profile_.cellular.mqtt_client_id, topic_for(tx_type_));
        std::snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"%s\",%u,%u,0", topic_buf_,
                      static_cast<unsigned>(o), static_cast<unsigned>(profile_.cellular.mqtt_qos));
        at_.command(cmd);
        at_.raw(reinterpret_cast<const uint8_t*>(hex), o);
        enter(State::Publishing);
      }
      break;

    case State::Publishing:
      if (at_.busy()) break;
      tx_pending_ = false;
      if (res == AtResult::Ok) {
        ++health_.tx_ok;
      } else {
        ++health_.tx_failed;
        health_.up = false;
        enter(State::Recovering);
        break;
      }
      enter(State::Online);
      break;

    case State::Recovering:
      if ((now - state_since_ms_) >= profile_.cellular.pwrkey_off_ms) {
        power_.pulse_pwrkey(profile_.cellular.pwrkey_on_ms);
        enter(State::PowerOn);
      }
      break;
  }
}

bool MqttLteTransport::send(const Frame& f) {
  if (tx_pending_) return false;
  Frame stamped = f;
  if (stamped.ts_s != 0) stamped.flags |= flag::kTimestamped;
  tx_len_ = channel_.seal(stamped, tx_buf_, sizeof(tx_buf_));
  if (tx_len_ == 0) return false;
  tx_type_ = f.type;
  tx_pending_ = true;
  return true;
}

bool MqttLteTransport::poll(Frame& out) {
  if (rx_count_ == 0) return false;
  out = rx_queue_[0];
  for (size_t i = 1; i < rx_count_; ++i) rx_queue_[i - 1] = rx_queue_[i];
  --rx_count_;
  return true;
}

}  // namespace agv
