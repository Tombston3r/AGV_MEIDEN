// Mise sous tension de la radio sur LILYGO T-Beam.
//
// C'EST L'ÉTAPE QUI MANQUE À LA PLUPART DES PREMIERS ESSAIS. Sur une T-Beam
// v1.0 et suivantes, le SX1276 et le GPS ne sont pas alimentés directement :
// ils passent par un gestionnaire d'alimentation AXP192, sur le bus I²C à
// l'adresse 0x34. Tant que la sortie correspondante n'est pas activée, le
// composant radio est hors tension — `RegVersion` lit alors 0x00 ou 0xFF, et
// l'on cherche un défaut de câblage SPI qui n'existe pas.
//
//   LDO2 -> radio SX1276      (celle qui nous intéresse)
//   LDO3 -> GPS
//   DCDC1 -> périphériques 3,3 V (dont l'écran OLED)
//
// ⚠ La révision v1.2 embarque un AXP2101, dont les registres DIFFÈRENT. Ce
// code cible l'AXP192 des v1.0 et v1.1. Sur une v1.2, `verifier()` le dira
// plutôt que d'écrire n'importe où.
#pragma once

#include <cstdint>

namespace tbeam {

// Résultat de la mise sous tension, pour un message d'erreur utile.
enum class Etat : uint8_t {
  Ok,
  BusIndisponible,     // l'I²C n'a pas démarré
  PmuAbsent,           // rien ne répond à l'adresse 0x34
  PmuInattendu,        // quelque chose répond, mais ce n'est pas un AXP192
};

const char* message(Etat e);

// Initialise l'I²C et met la radio sous tension. À appeler AVANT toute
// tentative de dialogue SPI avec le SX1276.
Etat alimenter_radio();

// Coupe le GPS et son antenne active. Ils ne servent à rien pour un essai de
// portée radio, et consomment plusieurs dizaines de milliampères sur batterie.
void couper_gps();

// Tension et courant de la batterie, pour savoir si un relevé de portée qui
// s'arrête est une panne de liaison ou une batterie vide.
float tension_batterie_v();

}  // namespace tbeam
