// Profil de timings de l'automate simulé (brief §10).
//
// Permet de rejouer aussi bien un automate rapide qu'un automate lent et de
// vérifier que le séquenceur tient dans les deux cas. Les valeurs réelles
// relèvent du §12.5 et ne sont PAS connues : le simulateur sert justement à
// borner le comportement avant de disposer du relevé.
#pragma once

#include <cstdint>
#include <string>

namespace agv::sim {

struct TimingProfile {
  // Délai entre le front montant de X93 et l'accusé Y22.
  uint32_t y22_delay_us = 5000;
  // Délai entre le front montant de X82 et Y05 (moving flag).
  uint32_t y05_delay_us = 50000;
  // Temps de parcours simulé entre deux stations consécutives.
  uint32_t travel_per_station_us = 200000;
  // Délai entre l'arrêt du mouvement et Y10 (in station flag).
  uint32_t y10_delay_us = 30000;
  // Rebond appliqué aux fronts Y (le séquenceur doit le filtrer).
  uint32_t y_bounce_us = 0;
  // Temps de stabilisation minimal exigé par l'automate avant le strobe :
  // si le firmware strobe plus tôt, l'écriture est refusée (Y22 jamais posé).
  uint32_t required_setup_us = 100;

  // --- Injection de défauts (tests de dégradation, §11) ---
  uint32_t drop_every_nth_y22 = 0;  // 0 = jamais ; 2 = un accusé sur deux perdu
  uint32_t drop_every_nth_y05 = 0;
  bool force_fault_y03 = false;     // défaut automate permanent
  bool force_no_destination = false;  // Y21 : pas de destination programmée

  // Charge un profil YAML plat (« clé: valeur », commentaires « # »).
  // Retourne false si une clé est inconnue : un profil de test silencieusement
  // ignoré donnerait un vert trompeur.
  bool load(const std::string& yaml_text, std::string* error = nullptr);

  static TimingProfile fast();
  static TimingProfile slow();
};

}  // namespace agv::sim
