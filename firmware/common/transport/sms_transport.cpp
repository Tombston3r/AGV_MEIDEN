#include "transport/sms_transport.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace agv {
namespace {

constexpr const char* kPayloadPrefix = "AGV:";

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

}  // namespace

size_t SmsTransport::to_hex(const uint8_t* data, size_t len, char* out, size_t capacity) {
  if (capacity < len * 2 + 1) return 0;
  static const char* digits = "0123456789ABCDEF";
  for (size_t i = 0; i < len; ++i) {
    out[i * 2] = digits[(data[i] >> 4) & 0x0Fu];
    out[i * 2 + 1] = digits[data[i] & 0x0Fu];
  }
  out[len * 2] = '\0';
  return len * 2;
}

size_t SmsTransport::from_hex(const char* text, uint8_t* out, size_t capacity) {
  size_t n = 0;
  for (size_t i = 0; text[i] != '\0' && text[i + 1] != '\0'; i += 2) {
    const int hi = hex_value(text[i]);
    const int lo = hex_value(text[i + 1]);
    if (hi < 0 || lo < 0) break;
    if (n >= capacity) return 0;
    out[n++] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return n;
}

bool SmsTransport::extract_payload(const char* line, uint8_t* out, size_t capacity, size_t& len) {
  const char* start = std::strstr(line, kPayloadPrefix);
  if (start == nullptr) return false;
  start += std::strlen(kPayloadPrefix);
  len = from_hex(start, out, capacity);
  return len > 0;
}

void SmsTransport::enter(State s) {
  state_ = s;
  state_since_ms_ = uart_.now_ms();
}

bool SmsTransport::begin() {
  // Séquence PWRKEY : 1 000 ms pour allumer (§8.1).
  power_.pulse_pwrkey(profile_.cellular.pwrkey_on_ms);
  init_step_ = 0;
  enter(State::PowerOn);
  return true;
}

void SmsTransport::on_urc(const char* line) {
  // +CMTI: "SM",3  -> un SMS est arrivé à l'index 3.
  if (std::strncmp(line, "+CMTI:", 6) == 0) {
    const char* comma = std::strrchr(line, ',');
    if (comma != nullptr && pending_count_ < kPendingMax) {
      pending_idx_[pending_count_++] = static_cast<uint16_t>(std::atoi(comma + 1));
    }
    return;
  }
  if (std::strncmp(line, "+CSQ:", 5) == 0) {
    const int rssi_raw = std::atoi(line + 5);
    // 0…31 -> -113…-51 dBm ; 99 = inconnu.
    health_.rssi_dbm = (rssi_raw == 99) ? 0 : static_cast<int16_t>(-113 + 2 * rssi_raw);
    return;
  }
  if (std::strncmp(line, "+CREG:", 6) == 0 || std::strncmp(line, "+CEREG:", 7) == 0) {
    // « ,1 » ou « ,5 » : attaché (nominal ou roaming).
    const char* comma = std::strrchr(line, ',');
    const int stat = (comma != nullptr) ? std::atoi(comma + 1) : 0;
    health_.up = (stat == 1 || stat == 5);
    return;
  }
}

void SmsTransport::advance_init() {
  // Séquence d'initialisation. Chaque étape est une commande AT distincte :
  // aucune n'est bloquante, et un échec fait retomber en récupération.
  static const char* kSteps[] = {
      "AT",           // le modem répond-il ?
      "ATE0",         // pas d'écho : sinon chaque commande revient en URC
      "AT+CMEE=1",    // erreurs numériques explicites
      "AT+CPIN?",     // SIM présente et déverrouillée ?
      "AT+CMGF=1",    // mode texte
      "AT+CSCS=\"GSM\"",
      "AT+CNMI=2,1,0,0,0",  // notification d'arrivée par +CMTI, pas de dump
      "AT+CREG?",
      "AT+CSQ",
  };
  constexpr uint8_t kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

  if (init_step_ >= kStepCount) {
    enter(State::Ready);
    return;
  }
  at_.command(kSteps[init_step_]);
}

void SmsTransport::tick() {
  power_.kick_hardware_watchdog();
  const AtResult res = at_.tick();
  const uint32_t now = uart_.now_ms();

  // Détection de modem muet : le chien de garde logiciel ne suffit pas, la pile
  // AT peut rester silencieuse indéfiniment (§8.1).
  if (state_ != State::PowerOff && state_ != State::PowerOn && state_ != State::Recovering &&
      at_.last_rx_ms() != 0 &&
      (now - at_.last_rx_ms()) > profile_.cellular.modem_mute_timeout_ms) {
    ++recoveries_;
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
      // Le SIM7600 met plusieurs secondes à répondre après PWRKEY.
      if ((now - state_since_ms_) >= profile_.cellular.pwrkey_on_ms) {
        init_step_ = 0;
        enter(State::Init);
        advance_init();
      }
      break;

    case State::Init:
      if (at_.busy()) break;
      if (res == AtResult::Ok) {
        ++init_step_;
        advance_init();
      } else {
        // Étape refusée (PIN, SIM éjectée, modem pas prêt) : on repart du début
        // plutôt que de poursuivre sur un modem dans un état inconnu.
        ++health_.tx_failed;
        init_step_ = 0;
        power_.pulse_pwrkey(profile_.cellular.pwrkey_off_ms);
        enter(State::Recovering);
      }
      break;

    case State::Ready:
      if (at_.busy()) break;
      if (pending_count_ > 0) {
        reading_idx_ = pending_idx_[0];
        for (size_t i = 1; i < pending_count_; ++i) pending_idx_[i - 1] = pending_idx_[i];
        --pending_count_;
        char cmd[32];
        std::snprintf(cmd, sizeof(cmd), "AT+CMGR=%u", static_cast<unsigned>(reading_idx_));
        at_.command(cmd);
        enter(State::ReadSms);
      } else if (tx_pending_) {
        char cmd[48];
        std::snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", profile_.cellular.peer_msisdn);
        at_.command(cmd);
        enter(State::SendPrompt);
      }
      break;

    case State::ReadSms: {
      if (at_.busy()) break;
      if (res == AtResult::Ok) {
        uint8_t packet[kSecurePacketMax];
        size_t len = 0;
        if (extract_payload(at_.last_response(), packet, sizeof(packet), len)) {
          Frame f;
          if (channel_.open(packet, len, f, profile_.protocol.version)) {
            ++health_.rx_ok;
            health_.last_rx_ms = now;
            if (rx_count_ < kRxQueue) rx_queue_[rx_count_++] = f;
          } else {
            ++health_.rx_bad_crc;
          }
        }
      }
      // Le SMS est supprimé dans tous les cas : la mémoire SIM est minuscule,
      // un message illisible qui y reste bloque toute réception ultérieure.
      char cmd[32];
      std::snprintf(cmd, sizeof(cmd), "AT+CMGD=%u", static_cast<unsigned>(reading_idx_));
      at_.command(cmd);
      enter(State::DeleteSms);
      break;
    }

    case State::DeleteSms:
      if (!at_.busy()) enter(State::Ready);
      break;

    case State::SendPrompt:
      if (std::strcmp(at_.last_response(), ">") == 0) {
        char hex[kSecurePacketMax * 2 + 8];
        std::memcpy(hex, kPayloadPrefix, std::strlen(kPayloadPrefix));
        to_hex(tx_buf_, tx_len_, hex + std::strlen(kPayloadPrefix),
               sizeof(hex) - std::strlen(kPayloadPrefix));
        at_.raw(reinterpret_cast<const uint8_t*>(hex), std::strlen(hex));
        const uint8_t ctrl_z = 0x1A;
        at_.raw(&ctrl_z, 1);
        enter(State::SendBody);
      } else if (!at_.busy()) {
        tx_pending_ = false;
        ++health_.tx_failed;
        enter(State::Ready);
      }
      break;

    case State::SendBody:
      if (!at_.busy()) {
        tx_pending_ = false;
        if (res == AtResult::Ok) {
          ++health_.tx_ok;
        } else {
          ++health_.tx_failed;
        }
        enter(State::Ready);
      }
      break;

    case State::Recovering:
      if ((now - state_since_ms_) >= profile_.cellular.pwrkey_off_ms) {
        power_.pulse_pwrkey(profile_.cellular.pwrkey_on_ms);
        enter(State::PowerOn);
      }
      break;
  }
}

bool SmsTransport::send(const Frame& f) {
  if (tx_pending_) return false;
  Frame stamped = f;
  // Horodatage obligatoire sur ce transport : sans lui, impossible de refuser
  // une commande sortie du SMSC trois minutes plus tard (§8.1). Une commande
  // sans horloge valide est REFUSÉE à l'émission plutôt qu'envoyée en aveugle.
  const bool is_command = (f.type == FrameType::CmdGoto || f.type == FrameType::CmdStop);
  if (is_command && f.ts_s == 0) return false;
  if (stamped.ts_s != 0) stamped.flags |= flag::kTimestamped;
  tx_len_ = channel_.seal(stamped, tx_buf_, sizeof(tx_buf_));
  if (tx_len_ == 0) return false;
  tx_pending_ = true;
  return true;
}

bool SmsTransport::poll(Frame& out) {
  if (rx_count_ == 0) return false;
  out = rx_queue_[0];
  for (size_t i = 1; i < rx_count_; ++i) rx_queue_[i - 1] = rx_queue_[i];
  --rx_count_;
  return true;
}

}  // namespace agv
