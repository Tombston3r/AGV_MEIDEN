#include "app/alert_gateway.h"

#include <cstdio>
#include <cstring>

namespace agv {

const char* AlertGateway::kind_str(AlertKind k) {
  switch (k) {
    case AlertKind::BlockingFault: return "DEFAUT BLOQUANT";
    case AlertKind::LinkLost: return "LIAISON PERDUE";
    case AlertKind::PlcFault: return "DEFAUT AUTOMATE";
    case AlertKind::Recovered: return "RETOUR NORMAL";
  }
  return "?";
}

bool AlertGateway::raise(AlertKind kind, const char* detail, uint32_t now_s) {
  if (pending_) return false;

  // Quota journalier glissant sur la journée civile approchée (86 400 s).
  const uint32_t day = now_s / 86400u;
  if (day != day_index_) {
    day_index_ = day;
    sent_today_ = 0;
  }
  if (sent_today_ >= cfg_.alerts_per_day_max) {
    ++suppressed_;
    return false;
  }

  std::snprintf(message_, sizeof(message_), "AGV %s: %s", kind_str(kind),
                (detail != nullptr) ? detail : "");
  pending_ = true;
  step_ = Step::TextMode;
  at_.command("AT+CMGF=1");
  return true;
}

void AlertGateway::tick() {
  const AtResult res = at_.tick();
  if (!pending_) return;

  switch (step_) {
    case Step::Idle:
      break;

    case Step::TextMode:
      if (at_.busy()) break;
      if (res != AtResult::Ok) {
        pending_ = false;  // modem indisponible : l'alerte est abandonnée, la
        break;             // chaîne de commande n'en dépend pas.
      }
      {
        char cmd[48];
        std::snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", cfg_.alert_msisdn);
        at_.command(cmd, 30000);
      }
      step_ = Step::Prompt;
      break;

    case Step::Prompt:
      if (std::strcmp(at_.last_response(), ">") == 0) {
        at_.raw(reinterpret_cast<const uint8_t*>(message_), std::strlen(message_));
        const uint8_t ctrl_z = 0x1A;
        at_.raw(&ctrl_z, 1);
        step_ = Step::Body;
      } else if (!at_.busy()) {
        pending_ = false;
        step_ = Step::Idle;
      }
      break;

    case Step::Body:
      if (at_.busy()) break;
      if (res == AtResult::Ok) ++sent_today_;
      pending_ = false;
      step_ = Step::Idle;
      break;
  }
}

}  // namespace agv
