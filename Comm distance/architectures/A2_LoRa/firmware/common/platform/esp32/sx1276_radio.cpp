#include "platform/esp32/sx1276_radio.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace agv::esp32 {
namespace {

constexpr uint8_t kRegFifo = 0x00;
constexpr uint8_t kRegOpMode = 0x01;
constexpr uint8_t kRegFrfMsb = 0x06;
constexpr uint8_t kRegPaConfig = 0x09;
constexpr uint8_t kRegFifoAddrPtr = 0x0D;
constexpr uint8_t kRegFifoTxBase = 0x0E;
constexpr uint8_t kRegFifoRxBase = 0x0F;
constexpr uint8_t kRegFifoRxCurrent = 0x10;
constexpr uint8_t kRegIrqFlags = 0x12;
constexpr uint8_t kRegRxNbBytes = 0x13;
constexpr uint8_t kRegPktSnrValue = 0x19;
constexpr uint8_t kRegPktRssiValue = 0x1A;
constexpr uint8_t kRegModemConfig1 = 0x1D;
constexpr uint8_t kRegModemConfig2 = 0x1E;
constexpr uint8_t kRegPreambleMsb = 0x20;
constexpr uint8_t kRegPayloadLength = 0x22;
constexpr uint8_t kRegModemConfig3 = 0x26;
constexpr uint8_t kRegSyncWord = 0x39;
constexpr uint8_t kRegDioMapping1 = 0x40;
constexpr uint8_t kRegVersion = 0x42;
constexpr uint8_t kRegPaDac = 0x4D;

constexpr uint8_t kModeSleep = 0x00;
constexpr uint8_t kModeStandby = 0x01;
constexpr uint8_t kModeTx = 0x03;
constexpr uint8_t kModeRxContinuous = 0x05;
constexpr uint8_t kModeLongRange = 0x80;

constexpr uint8_t kIrqTxDone = 0x08;
constexpr uint8_t kIrqRxDone = 0x40;
constexpr uint8_t kIrqPayloadCrcError = 0x20;

}  // namespace

uint8_t Sx1276Radio::read_reg(uint8_t reg) const {
  const uint8_t tx[2] = {static_cast<uint8_t>(reg & 0x7Fu), 0x00};
  uint8_t rx[2] = {};
  spi_.transfer(tx, rx, 2);
  return rx[1];
}

void Sx1276Radio::write_reg(uint8_t reg, uint8_t value) {
  const uint8_t tx[2] = {static_cast<uint8_t>(reg | 0x80u), value};
  spi_.transfer(tx, nullptr, 2);
}

void Sx1276Radio::set_mode(uint8_t mode) { write_reg(kRegOpMode, kModeLongRange | mode); }

bool Sx1276Radio::begin(const LoraConfig& cfg) {
  gpio_.configure_output(reset_pin_);
  gpio_.configure_input(dio0_pin_, false);

  gpio_.set(reset_pin_, false);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_.set(reset_pin_, true);
  vTaskDelay(pdMS_TO_TICKS(10));

  version_ = read_reg(kRegVersion);
  if (version_ != 0x12) return false;  // 0x12 = SX1276 ; sinon câblage SPI

  set_mode(kModeSleep);  // le passage en mode LoRa exige le mode sommeil
  vTaskDelay(pdMS_TO_TICKS(10));

  // Fréquence : Frf = f_rf / F_step, F_step = 32 MHz / 2^19.
  const uint64_t frf = (static_cast<uint64_t>(cfg.frequency_hz) << 19) / 32000000ull;
  write_reg(kRegFrfMsb, static_cast<uint8_t>(frf >> 16));
  write_reg(kRegFrfMsb + 1, static_cast<uint8_t>(frf >> 8));
  write_reg(kRegFrfMsb + 2, static_cast<uint8_t>(frf));

  write_reg(kRegFifoTxBase, 0x00);
  write_reg(kRegFifoRxBase, 0x00);

  // Bande passante : 125 kHz = 0x07, 250 kHz = 0x08, 500 kHz = 0x09.
  uint8_t bw_code = 0x07;
  if (cfg.bandwidth_hz >= 500000) bw_code = 0x09;
  else if (cfg.bandwidth_hz >= 250000) bw_code = 0x08;
  const uint8_t cr_code = static_cast<uint8_t>((cfg.coding_rate - 4) & 0x07u);
  write_reg(kRegModemConfig1, static_cast<uint8_t>((bw_code << 4) | (cr_code << 1)));
  // CRC matériel activé ; le CRC-16 applicatif reste en plus (défense en
  // profondeur : le CRC radio ne couvre pas la traversée du firmware).
  write_reg(kRegModemConfig2, static_cast<uint8_t>((cfg.spreading_factor << 4) | 0x04u));
  // LowDataRateOptimize obligatoire si Tsym > 16 ms (SF11/SF12 à 125 kHz).
  const bool ldro = (cfg.spreading_factor >= 11 && cfg.bandwidth_hz <= 125000);
  write_reg(kRegModemConfig3, ldro ? 0x0C : 0x04);  // AGC automatique

  write_reg(kRegPreambleMsb, 0x00);
  write_reg(kRegPreambleMsb + 1, 0x08);  // 8 symboles

  // Sync word privé : DOIT différer de 0x34, réservé LoRaWAN (§6).
  write_reg(kRegSyncWord, cfg.sync_word);

  // PA_BOOST, puissance en dBm (2..17).
  const uint8_t power = static_cast<uint8_t>((cfg.tx_power_dbm < 2) ? 2
                                             : (cfg.tx_power_dbm > 17) ? 17
                                                                       : cfg.tx_power_dbm);
  write_reg(kRegPaDac, 0x84);
  write_reg(kRegPaConfig, static_cast<uint8_t>(0x80u | (power - 2)));

  set_mode(kModeStandby);
  return true;
}

bool Sx1276Radio::transmit(const uint8_t* data, size_t len) {
  if (transmitting_ || len == 0 || len > 255) return false;
  set_mode(kModeStandby);
  write_reg(kRegIrqFlags, 0xFF);
  write_reg(kRegFifoAddrPtr, 0x00);
  write_reg(kRegPayloadLength, static_cast<uint8_t>(len));
  for (size_t i = 0; i < len; ++i) write_reg(kRegFifo, data[i]);
  write_reg(kRegDioMapping1, 0x40);  // DIO0 = TxDone
  set_mode(kModeTx);
  transmitting_ = true;
  return true;
}

bool Sx1276Radio::tx_busy() const {
  if (!transmitting_) return false;
  if ((read_reg(kRegIrqFlags) & kIrqTxDone) != 0) {
    const_cast<Sx1276Radio*>(this)->write_reg(kRegIrqFlags, kIrqTxDone);
    transmitting_ = false;
    return false;
  }
  return true;
}

void Sx1276Radio::listen() {
  write_reg(kRegDioMapping1, 0x00);  // DIO0 = RxDone
  set_mode(kModeRxContinuous);
}

bool Sx1276Radio::receive(uint8_t* buf, size_t capacity, size_t& len, int16_t& rssi_dbm,
                          int8_t& snr_db) {
  const uint8_t irq = read_reg(kRegIrqFlags);
  if ((irq & kIrqRxDone) == 0) return false;
  write_reg(kRegIrqFlags, kIrqRxDone | kIrqPayloadCrcError);
  if ((irq & kIrqPayloadCrcError) != 0) return false;  // CRC radio invalide

  const uint8_t n = read_reg(kRegRxNbBytes);
  if (n == 0 || n > capacity) return false;
  write_reg(kRegFifoAddrPtr, read_reg(kRegFifoRxCurrent));
  for (uint8_t i = 0; i < n; ++i) buf[i] = read_reg(kRegFifo);
  len = n;

  snr_db = static_cast<int8_t>(static_cast<int8_t>(read_reg(kRegPktSnrValue)) / 4);
  // -157 dBm d'offset en bande haute (868 MHz), corrigé du SNR quand il est
  // négatif : sinon le RSSI affiché est optimiste sur les trames faibles.
  const int raw_rssi = static_cast<int>(read_reg(kRegPktRssiValue)) - 157;
  rssi_dbm = static_cast<int16_t>((snr_db < 0) ? raw_rssi + snr_db : raw_rssi);
  return true;
}

}  // namespace agv::esp32
