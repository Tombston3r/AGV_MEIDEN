#include "tbeam_power.h"

#include <driver/i2c.h>

#include "test_config.h"

namespace tbeam {
namespace {

#if defined(CARTE_TBEAM)

constexpr i2c_port_t kPort = I2C_NUM_0;
constexpr uint8_t kAdresseAxp = 0x34;

// Registres AXP192 (fiche technique X-Powers, rév. 1.1).
constexpr uint8_t kRegPuissanceSorties = 0x12;  // active/coupe LDO2, LDO3, DCDC
constexpr uint8_t kRegTensionLdo23 = 0x28;      // LDO2 sur les 4 bits hauts
constexpr uint8_t kRegIdentifiant = 0x03;
constexpr uint8_t kRegTensionBatterieH = 0x78;
constexpr uint8_t kRegModeAdc1 = 0x82;

// Le bit de chaque sortie dans le registre 0x12.
constexpr uint8_t kBitLdo3 = 1 << 3;   // GPS
constexpr uint8_t kBitLdo2 = 1 << 2;   // radio

bool ecrire(uint8_t reg, uint8_t valeur) {
  const uint8_t trame[2] = {reg, valeur};
  return i2c_master_write_to_device(kPort, kAdresseAxp, trame, sizeof(trame),
                                    pdMS_TO_TICKS(50)) == ESP_OK;
}

bool lire(uint8_t reg, uint8_t* out, size_t n = 1) {
  return i2c_master_write_read_device(kPort, kAdresseAxp, &reg, 1, out, n,
                                      pdMS_TO_TICKS(50)) == ESP_OK;
}

bool demarrer_i2c() {
  i2c_config_t cfg = {};
  cfg.mode = I2C_MODE_MASTER;
  cfg.sda_io_num = test::kPinSda;
  cfg.scl_io_num = test::kPinScl;
  cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.master.clk_speed = 400000;
  if (i2c_param_config(kPort, &cfg) != ESP_OK) return false;
  return i2c_driver_install(kPort, I2C_MODE_MASTER, 0, 0, 0) == ESP_OK;
}

#endif  // CARTE_TBEAM

}  // namespace

const char* message(Etat e) {
  switch (e) {
    case Etat::Ok: return "AXP192 initialise, radio sous tension";
    case Etat::BusIndisponible: return "bus I2C indisponible";
    case Etat::PmuAbsent:
      return "aucun AXP192 a l'adresse 0x34 : revision T-Beam v0.7 (sans PMU), "
             "ou v1.2 (AXP2101, registres differents)";
    case Etat::PmuInattendu:
      return "reponse inattendue du PMU : verifier la revision de la carte";
  }
  return "?";
}

#if defined(CARTE_TBEAM)

Etat alimenter_radio() {
  if (!demarrer_i2c()) return Etat::BusIndisponible;

  uint8_t id = 0;
  if (!lire(kRegIdentifiant, &id)) return Etat::PmuAbsent;

  // LDO2 à 3,3 V : les 4 bits hauts du registre 0x28, pas de 100 mV depuis
  // 1,8 V — soit (3300 - 1800) / 100 = 15, la valeur maximale.
  uint8_t tensions = 0;
  lire(kRegTensionLdo23, &tensions);
  if (!ecrire(kRegTensionLdo23, static_cast<uint8_t>((tensions & 0x0F) | (15 << 4)))) {
    return Etat::PmuInattendu;
  }

  uint8_t sorties = 0;
  if (!lire(kRegPuissanceSorties, &sorties)) return Etat::PmuInattendu;
  if (!ecrire(kRegPuissanceSorties, static_cast<uint8_t>(sorties | kBitLdo2))) {
    return Etat::PmuInattendu;
  }

  // Mesure de la tension batterie : utile pour distinguer une liaison perdue
  // d'une batterie vide pendant un relevé de portée.
  uint8_t adc = 0;
  lire(kRegModeAdc1, &adc);
  ecrire(kRegModeAdc1, static_cast<uint8_t>(adc | 0xC0));

  // Le SX1276 demande quelques millisecondes après sa mise sous tension avant
  // de répondre sur le SPI. Interroger trop tôt fait lire 0x00.
  vTaskDelay(pdMS_TO_TICKS(20));
  return Etat::Ok;
}

void couper_gps() {
  uint8_t sorties = 0;
  if (lire(kRegPuissanceSorties, &sorties)) {
    ecrire(kRegPuissanceSorties, static_cast<uint8_t>(sorties & ~kBitLdo3));
  }
}

float tension_batterie_v() {
  uint8_t brut[2] = {0, 0};
  if (!lire(kRegTensionBatterieH, brut, 2)) return 0.0f;
  // 12 bits répartis 8 + 4, pas de 1,1 mV.
  const uint16_t valeur = static_cast<uint16_t>((brut[0] << 4) | (brut[1] & 0x0F));
  return valeur * 1.1f / 1000.0f;
}

#else  // carte sans gestionnaire d'alimentation

Etat alimenter_radio() { return Etat::Ok; }
void couper_gps() {}
float tension_batterie_v() { return 0.0f; }

#endif

}  // namespace tbeam
