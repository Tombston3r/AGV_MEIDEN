#include "timing_profile.h"

#include <cstdlib>
#include <sstream>

namespace agv::sim {
namespace {

std::string trim(const std::string& s) {
  const size_t b = s.find_first_not_of(" \t\r");
  if (b == std::string::npos) return {};
  const size_t e = s.find_last_not_of(" \t\r");
  return s.substr(b, e - b + 1);
}

bool to_bool(const std::string& v) { return v == "true" || v == "1" || v == "yes"; }

}  // namespace

bool TimingProfile::load(const std::string& yaml_text, std::string* error) {
  std::istringstream in(yaml_text);
  std::string line;
  int lineno = 0;
  while (std::getline(in, line)) {
    ++lineno;
    const size_t hash = line.find('#');
    if (hash != std::string::npos) line = line.substr(0, hash);
    line = trim(line);
    if (line.empty()) continue;

    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
      if (error) *error = "ligne " + std::to_string(lineno) + " : « clé: valeur » attendu";
      return false;
    }
    const std::string key = trim(line.substr(0, colon));
    const std::string value = trim(line.substr(colon + 1));
    const auto num = [&value]() { return static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 10)); };

    if (key == "y22_delay_us") y22_delay_us = num();
    else if (key == "y05_delay_us") y05_delay_us = num();
    else if (key == "travel_per_station_us") travel_per_station_us = num();
    else if (key == "y10_delay_us") y10_delay_us = num();
    else if (key == "y_bounce_us") y_bounce_us = num();
    else if (key == "required_setup_us") required_setup_us = num();
    else if (key == "drop_every_nth_y22") drop_every_nth_y22 = num();
    else if (key == "drop_every_nth_y05") drop_every_nth_y05 = num();
    else if (key == "force_fault_y03") force_fault_y03 = to_bool(value);
    else if (key == "force_no_destination") force_no_destination = to_bool(value);
    else {
      if (error) *error = "clé inconnue à la ligne " + std::to_string(lineno) + " : " + key;
      return false;
    }
  }
  return true;
}

TimingProfile TimingProfile::fast() {
  TimingProfile p;
  p.y22_delay_us = 1000;
  p.y05_delay_us = 5000;
  p.travel_per_station_us = 50000;
  p.y10_delay_us = 5000;
  return p;
}

TimingProfile TimingProfile::slow() {
  TimingProfile p;
  // Volontairement AU-DELÀ des timeouts par défaut (300 ms / 1,5 s) : c'est le
  // cas qui prouve que les timeouts du §12.5 doivent être relevés, pas devinés.
  p.y22_delay_us = 400000;
  p.y05_delay_us = 2500000;
  p.travel_per_station_us = 2000000;
  p.y10_delay_us = 400000;
  p.y_bounce_us = 1500;
  return p;
}

}  // namespace agv::sim
