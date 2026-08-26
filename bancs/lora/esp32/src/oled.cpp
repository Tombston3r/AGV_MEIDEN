#include "oled.h"

#include <cstring>

#include "oled_font.h"
#include "test_config.h"

#if defined(CARTE_TBEAM)
#include <driver/i2c.h>
#endif

namespace oled {
namespace {

#if defined(CARTE_TBEAM)

constexpr i2c_port_t kPort = I2C_NUM_0;
constexpr uint8_t kAdresse = 0x3C;
constexpr int kLargeur = 128;
constexpr int kPages = 8;

uint8_t g_tampon[kLargeur * kPages];
bool g_present = false;

bool commande(const uint8_t* cmds, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const uint8_t trame[2] = {0x00, cmds[i]};   // 0x00 : octet de contrôle
    if (i2c_master_write_to_device(kPort, kAdresse, trame, 2,
                                   pdMS_TO_TICKS(50)) != ESP_OK) {
      return false;
    }
  }
  return true;
}

#endif  // CARTE_TBEAM

}  // namespace

#if defined(CARTE_TBEAM)

bool begin() {
  // Séquence d'initialisation SSD1306 (fiche technique §10.1), réduite au
  // nécessaire : 128×64, adressage horizontal, contraste moyen, écran allumé.
  static const uint8_t kInit[] = {
      0xAE,              // écran éteint pendant la configuration
      0xD5, 0x80,        // horloge
      0xA8, 0x3F,        // multiplexage : 64 lignes
      0xD3, 0x00,        // pas de décalage vertical
      0x40,              // début de ligne 0
      0x8D, 0x14,        // pompe de charge interne — sans elle, écran noir
      0x20, 0x00,        // adressage horizontal
      0xA1, 0xC8,        // orientation : origine en haut à gauche
      0xDA, 0x12,        // brochage COM
      0x81, 0x9F,        // contraste
      0xD9, 0xF1, 0xDB, 0x40,
      0xA4,              // affiche la RAM, pas tout allumé
      0xA6,              // vidéo normale
      0xAF,              // écran allumé
  };
  g_present = commande(kInit, sizeof(kInit));
  if (g_present) {
    effacer();
    afficher();
  }
  return g_present;
}

bool present() { return g_present; }

void effacer() { std::memset(g_tampon, 0, sizeof(g_tampon)); }

void texte(int colonne, int ligne, const char* s) {
  if (s == nullptr || ligne < 0 || ligne >= kPages) return;
  int x = colonne * (kLargeurGlyphe + 1);
  for (; *s && x + kLargeurGlyphe <= kLargeur; ++s) {
    const char c = (*s < kPremierCar || *s > kDernierCar) ? '?' : *s;
    const uint8_t* glyphe = kPolice[c - kPremierCar];
    for (int i = 0; i < kLargeurGlyphe; ++i) {
      g_tampon[ligne * kLargeur + x + i] = glyphe[i];
    }
    x += kLargeurGlyphe + 1;
  }
}

void ligne_horizontale(int ligne) {
  if (ligne < 0 || ligne >= kPages) return;
  for (int x = 0; x < kLargeur; ++x) g_tampon[ligne * kLargeur + x] |= 0x80;
}

void afficher() {
  if (!g_present) return;
  static const uint8_t kFenetre[] = {0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
  if (!commande(kFenetre, sizeof(kFenetre))) return;
  // Le tampon part en une fois, précédé de l'octet de contrôle 0x40.
  uint8_t trame[1 + sizeof(g_tampon)];
  trame[0] = 0x40;
  std::memcpy(trame + 1, g_tampon, sizeof(g_tampon));
  i2c_master_write_to_device(kPort, kAdresse, trame, sizeof(trame),
                             pdMS_TO_TICKS(200));
}

#else   // carte sans écran

bool begin() { return false; }
bool present() { return false; }
void effacer() {}
void texte(int, int, const char*) {}
void ligne_horizontale(int) {}
void afficher() {}

#endif

void page(const char* l0, const char* l1, const char* l2, const char* l3,
          const char* l4, const char* l5) {
  if (!present()) return;
  effacer();
  const char* lignes[] = {l0, l1, l2, l3, l4, l5};
  int y = 0;
  for (const char* l : lignes) {
    if (l != nullptr) texte(0, y, l);
    ++y;
  }
  afficher();
}

}  // namespace oled
