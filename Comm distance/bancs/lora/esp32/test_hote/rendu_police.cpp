// Rendu de la police 5x7 en ASCII, sur poste de développement.
//
// Une table de police écrite à la main ne se relit pas : elle se REGARDE. Ce
// programme la rend lisible, et vérifie que chaque glyphe imprimable est non
// vide : un glyphe oublié donnerait un blanc silencieux à l'écran.
#include <cstdio>
#include <cstring>

#include "../src/oled_font.h"

namespace {

void rendre(const char* texte) {
  for (int ligne = 0; ligne < oled::kHauteurGlyphe; ++ligne) {
    for (const char* p = texte; *p; ++p) {
      const int idx = *p - oled::kPremierCar;
      for (int col = 0; col < oled::kLargeurGlyphe; ++col) {
        const uint8_t colonne = oled::kPolice[idx][col];
        std::putchar((colonne >> ligne) & 1 ? '#' : ' ');
      }
      std::putchar(' ');
    }
    std::putchar('\n');
  }
}

}  // namespace

int main() {
  rendre("RSSI -87 dBm");
  std::puts("");
  rendre("ACK 94% SF9");
  std::puts("");

  int vides = 0;
  for (int c = oled::kPremierCar + 1; c <= oled::kDernierCar; ++c) {
    const uint8_t* g = oled::kPolice[c - oled::kPremierCar];
    if (!g[0] && !g[1] && !g[2] && !g[3] && !g[4]) {
      std::printf("GLYPHE VIDE : '%c' (0x%02X)\n", c, c);
      ++vides;
    }
  }
  std::printf("%d glyphes, %d vide(s) hors espace\n", oled::kNbGlyphes, vides);
  return vides ? 1 : 0;
}
