#include "transport/at_engine.h"

#include <cstring>

namespace agv {

bool AtEngine::command(const char* cmd, uint32_t timeout_ms) {
  if (busy_) return false;
  const size_t len = std::strlen(cmd);
  uart_.write(reinterpret_cast<const uint8_t*>(cmd), len);
  const uint8_t crlf[2] = {'\r', '\n'};
  uart_.write(crlf, 2);
  busy_ = true;
  result_ = AtResult::Pending;
  response_[0] = '\0';
  cmd_timeout_ms_ = (timeout_ms != 0) ? timeout_ms : timeout_ms_;
  sent_at_ms_ = uart_.now_ms();
  return true;
}

bool AtEngine::raw(const uint8_t* data, size_t len) {
  return uart_.write(data, len) == len;
}

void AtEngine::handle_line(const char* line) {
  if (line[0] == '\0') return;
  last_rx_ms_ = uart_.now_ms();

  if (busy_) {
    if (std::strcmp(line, "OK") == 0) {
      busy_ = false;
      result_ = AtResult::Ok;
      return;
    }
    if (std::strcmp(line, "ERROR") == 0 || std::strncmp(line, "+CME ERROR", 10) == 0 ||
        std::strncmp(line, "+CMS ERROR", 10) == 0) {
      busy_ = false;
      ++errors_;
      std::strncpy(response_, line, kAtLineMax - 1);
      result_ = AtResult::Error;
      return;
    }
    // Ligne intermédiaire : conservée comme réponse utile. Un URC peut aussi
    // tomber ici : on le transmet également au gestionnaire, sinon un +CMTI
    // arrivé pendant une commande serait perdu, et le SMS jamais lu.
    if (line[0] == '+' && urc_ != nullptr && std::strchr(line, ':') != nullptr) {
      urc_->on_urc(line);
    }
    std::strncpy(response_, line, kAtLineMax - 1);
    return;
  }

  if (urc_ != nullptr) urc_->on_urc(line);
}

AtResult AtEngine::tick() {
  uint8_t buf[64];
  size_t n = uart_.read(buf, sizeof(buf));
  while (n > 0) {
    for (size_t i = 0; i < n; ++i) {
      const char c = static_cast<char>(buf[i]);
      if (c == '\r') continue;
      if (c == '\n') {
        line_[line_len_] = '\0';
        handle_line(line_);
        line_len_ = 0;
        continue;
      }
      if (line_len_ < kAtLineMax - 1) line_[line_len_++] = c;
      // Invite d'envoi de SMS : « > » n'est pas terminé par un saut de ligne.
      if (line_len_ == 1 && c == '>') {
        line_[1] = '\0';
        handle_line(line_);
        line_len_ = 0;
      }
    }
    n = uart_.read(buf, sizeof(buf));
  }

  if (busy_ && (uart_.now_ms() - sent_at_ms_) >= cmd_timeout_ms_) {
    busy_ = false;
    ++timeouts_;
    result_ = AtResult::Timeout;
  }
  return result_;
}

void AtEngine::reset() {
  busy_ = false;
  line_len_ = 0;
  response_[0] = '\0';
  result_ = AtResult::Ok;
}

}  // namespace agv
