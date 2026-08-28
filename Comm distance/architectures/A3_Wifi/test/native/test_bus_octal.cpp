// Décodage octal Meiden et encodage des champs bus (brief §4.1, §4.2, §11).
#include "bus/bus_signals.h"
#include "bus/debounce.h"
#include "test_framework.h"

using namespace agv;

TEST(octal_conversion_de_base) {
  CHECK_EQ(octal_to_linear(10), 8u);
  CHECK_EQ(octal_to_linear(27), 23u);
  CHECK_EQ(octal_to_linear(34), 28u);
  CHECK_EQ(linear_to_octal(8), 10u);
  CHECK_EQ(linear_to_octal(24), 30u);
}

TEST(apres_Y27_vient_Y30_pas_Y28) {
  // Le piège de la numérotation octale : la suite des repères n'est pas
  // décimale. Une itération naïve inventerait Y28 et Y29.
  CHECK_EQ(octal_step(23, 0), 23u);
  CHECK_EQ(octal_step(23, 4), 27u);
  CHECK_EQ(octal_step(23, 5), 30u);
  CHECK_EQ(octal_step(23, 9), 34u);
}

TEST(la_plage_Y23_Y34_compte_bien_10_signaux) {
  // 10 bits, 1024 valeurs, et non 12 comme le suggérerait une soustraction
  // décimale (34 - 23 + 1).
  CHECK_EQ(octal_span(23, 34), 10u);
  CHECK_EQ(octal_span(11, 14), 4u);  // vitesse courante Y11…Y14
  CHECK_EQ(octal_span(86, 87), 2u);
}

TEST(encodage_destination_10_bits) {
  const BusLayout& l = kDefaultLayout;
  uint32_t word = encode_station(0, l, 1023);
  CHECK_EQ(decode_field(word, l.x_station_bits, kStationBits), 1023u);
  word = encode_station(word, l, 0);
  CHECK_EQ(decode_field(word, l.x_station_bits, kStationBits), 0u);
  word = encode_station(word, l, 341);  // 0b0101010101
  CHECK_EQ(decode_field(word, l.x_station_bits, kStationBits), 341u);
  // Aucun bit hors des 10 lignes de destination ne doit avoir bougé.
  uint32_t station_mask = 0;
  for (size_t i = 0; i < kStationBits; ++i) station_mask |= 1u << l.x_station_bits[i];
  CHECK_EQ(word & ~station_mask, 0u);
}

TEST(encodage_vitesse_4_bits_independant_de_la_destination) {
  const BusLayout& l = kDefaultLayout;
  uint32_t word = encode_station(0, l, 777);
  word = encode_speed(word, l, 9);
  CHECK_EQ(decode_field(word, l.x_station_bits, kStationBits), 777u);
  CHECK_EQ(decode_field(word, l.x_speed_bits, kSpeedBits), 9u);
}

TEST(debordement_destination_tronque_a_10_bits) {
  const BusLayout& l = kDefaultLayout;
  const uint32_t word = encode_station(0, l, 2000);  // > 1023
  CHECK_EQ(decode_field(word, l.x_station_bits, kStationBits), 2000u & kStationMax);
}

TEST(decodage_position_courante_sur_Y) {
  const BusLayout& l = kDefaultLayout;
  uint32_t y = encode_field(0, l.y_station_bits, kStationBits, 512);
  y = encode_field(y, l.y_speed_bits, kSpeedBits, 3);
  CHECK_EQ(decode_position(y, l), 512u);
  CHECK_EQ(decode_current_speed(y, l), 3u);
}

TEST(antirebond_filtre_les_oscillations) {
  YDebouncer d(2000);  // 2 ms
  d.reset(0, 0);
  CHECK_EQ(d.update(0x01, 1000), 0u);   // front vu, pas encore stable
  CHECK_EQ(d.update(0x00, 1500), 0u);   // rebond
  CHECK_EQ(d.update(0x01, 2000), 0u);   // recompte depuis 2000
  CHECK_EQ(d.update(0x01, 3000), 0u);   // 1 ms écoulée seulement
  CHECK_EQ(d.update(0x01, 4100), 1u);   // 2,1 ms stables -> validé
}

TEST(antirebond_desactive_si_duree_nulle) {
  // §12.1 non relevé : un profil peut légitimement désactiver le filtrage.
  YDebouncer d(0);
  CHECK_EQ(d.update(0x05, 0), 5u);
}
