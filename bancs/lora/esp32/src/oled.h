// Écran OLED SSD1306 128×64 de la LILYGO T-Beam.
//
// Il n'est pas décoratif : c'est lui qui rend le RELEVÉ DE PORTÉE praticable.
// Sans écran, il faut promener un ordinateur au bout d'un câble USB le long du
// parcours ; avec, la carte tient dans une main et affiche le niveau reçu.
//
// Pilote minimal et volontairement pauvre : du texte, rien d'autre. Il n'y a
// pas de graphisme à faire ici, et une bibliothèque tierce ajouterait une
// dépendance pour afficher six lignes.
//
// Sur une carte sans écran, tout se compile en fonctions vides.
#pragma once

#include <cstdint>

namespace oled {

constexpr int kColonnes = 21;   // 128 px / 6 px par caractère
constexpr int kLignes = 8;      // 64 px / 8 px par ligne

// Suppose le bus I²C DÉJÀ démarré — c'est `tbeam::alimenter_radio()` qui s'en
// charge, l'AXP192 et l'écran partageant le même bus.
bool begin();
bool present();

void effacer();
void texte(int colonne, int ligne, const char* s);
void ligne_horizontale(int ligne);          // séparateur, en bas de la ligne
void afficher();                            // pousse le tampon vers l'écran

// Raccourci courant : efface, écrit les lignes fournies, affiche.
void page(const char* l0, const char* l1, const char* l2, const char* l3,
          const char* l4 = nullptr, const char* l5 = nullptr);

}  // namespace oled
