"""Pilote RFM95W / SX1276 pour Linux — registres, via spidev + gpiod.

Équivalent Python de `firmware/common/platform/esp32/sx1276_radio.cpp`. Les
registres et les séquences viennent de la même fiche technique (SX1276/77/78/79
rév. 7), et les paramètres par défaut de `firmware/common/config/lora_config.h`.

⚠️ Le module est HALF-DUPLEX. Pendant une émission, la réception est
impossible : c'est une propriété du composant, pas une limite de ce pilote.
"""

import time
from dataclasses import dataclass

FXOSC = 32_000_000.0
FSTEP = FXOSC / (1 << 19)

REG_FIFO = 0x00
REG_OP_MODE = 0x01
REG_FRF_MSB = 0x06
REG_PA_CONFIG = 0x09
REG_LNA = 0x0C
REG_FIFO_ADDR_PTR = 0x0D
REG_FIFO_TX_BASE = 0x0E
REG_FIFO_RX_BASE = 0x0F
REG_FIFO_RX_CURRENT = 0x10
REG_IRQ_FLAGS = 0x12
REG_RX_NB_BYTES = 0x13
REG_PKT_SNR = 0x19
REG_PKT_RSSI = 0x1A
REG_MODEM_CONFIG_1 = 0x1D
REG_MODEM_CONFIG_2 = 0x1E
REG_PREAMBLE_MSB = 0x20
REG_PAYLOAD_LENGTH = 0x22
REG_MODEM_CONFIG_3 = 0x26
REG_SYNC_WORD = 0x39
REG_DIO_MAPPING_1 = 0x40
REG_VERSION = 0x42
REG_PA_DAC = 0x4D

MODE_LONG_RANGE = 0x80
MODE_SLEEP = 0x00
MODE_STDBY = 0x01
MODE_TX = 0x03
MODE_RX_CONTINUOUS = 0x05

IRQ_TX_DONE = 0x08
IRQ_RX_DONE = 0x40
IRQ_CRC_ERROR = 0x20

BW_TABLE = {7800: 0, 10400: 1, 15600: 2, 20800: 3, 31250: 4,
            41700: 5, 62500: 6, 125000: 7, 250000: 8, 500000: 9}


@dataclass
class LoraConfig:
    """Mêmes valeurs par défaut que `LoraConfig` côté C++."""
    frequency_hz: int = 868_100_000
    spreading_factor: int = 9
    bandwidth_hz: int = 125_000
    coding_rate: int = 5          # 4/5
    sync_word: int = 0x12         # privé — DOIT différer de 0x34 (LoRaWAN)
    tx_power_dbm: int = 14
    preamble_symbols: int = 8


def airtime_ms(payload_len: int, cfg: LoraConfig) -> float:
    """Temps d'antenne d'une trame, en millisecondes.

    Formule de la fiche technique §4.1.1.6. Elle sert à vérifier le budget de
    rapport cyclique de 1 %/heure imposé par l'ERC 70-03 — obligation
    réglementaire, pas un réglage.
    """
    sf, bw = cfg.spreading_factor, cfg.bandwidth_hz
    t_sym = (1 << sf) / bw * 1000.0
    low_rate = 1 if (sf >= 11 and bw == 125_000) else 0
    num = 8 * payload_len - 4 * sf + 28 + 16          # en-tête explicite, CRC actif
    den = 4 * (sf - 2 * low_rate)
    n_payload = max(0, -(-num // den)) * cfg.coding_rate + 8
    return (cfg.preamble_symbols + 4.25) * t_sym + n_payload * t_sym


class RadioUnavailable(RuntimeError):
    """spidev/gpiod absents, ou le module ne répond pas."""


class Sx1276:
    def __init__(self, cfg=None, spi_bus=0, spi_cs=0, reset_pin=None,
                 dio0_pin=None, gpiochip="gpiochip0"):
        self.cfg = cfg or LoraConfig()
        self._spi_bus, self._spi_cs = spi_bus, spi_cs
        self._reset_pin, self._dio0_pin = reset_pin, dio0_pin
        self._gpiochip = gpiochip
        self._spi = None
        self._lines = None

    # --- Accès bas niveau --------------------------------------------------
    def _xfer(self, addr, value=0x00):
        return self._spi.xfer2([addr, value])[1]

    def read(self, reg):
        return self._xfer(reg & 0x7F)

    def write(self, reg, value):
        self._xfer(reg | 0x80, value & 0xFF)

    def _mode(self, mode):
        self.write(REG_OP_MODE, MODE_LONG_RANGE | mode)

    # --- Cycle de vie ------------------------------------------------------
    def begin(self):
        try:
            import spidev
        except ImportError as exc:  # pragma: no cover - dépend du matériel
            raise RadioUnavailable(
                "spidev absent. Sur Debian : apt install python3-spidev, et "
                "activer le bus SPI (le Gate G100 n'en expose pas — voir "
                "../README.md)."
            ) from exc

        self._spi = spidev.SpiDev()
        self._spi.open(self._spi_bus, self._spi_cs)
        self._spi.max_speed_hz = 5_000_000
        self._spi.mode = 0

        self._reset()

        version = self.read(REG_VERSION)
        if version != 0x12:
            raise RadioUnavailable(
                f"RegVersion = 0x{version:02X}, attendu 0x12. Câblage SPI, "
                "alimentation 3,3 V ou broche NSS à vérifier."
            )

        self._mode(MODE_SLEEP)          # le passage en LoRa EXIGE le mode veille
        time.sleep(0.01)

        frf = int(self.cfg.frequency_hz / FSTEP)
        for i, shift in enumerate((16, 8, 0)):
            self.write(REG_FRF_MSB + i, (frf >> shift) & 0xFF)

        self.write(REG_FIFO_TX_BASE, 0x00)
        self.write(REG_FIFO_RX_BASE, 0x00)
        self.write(REG_LNA, self.read(REG_LNA) | 0x03)   # gain max + boost

        bw = BW_TABLE[self.cfg.bandwidth_hz]
        self.write(REG_MODEM_CONFIG_1, (bw << 4) | ((self.cfg.coding_rate - 4) << 1))
        self.write(REG_MODEM_CONFIG_2, (self.cfg.spreading_factor << 4) | 0x04)  # CRC actif
        self.write(REG_MODEM_CONFIG_3, 0x04)             # AGC automatique
        self.write(REG_PREAMBLE_MSB, 0x00)
        self.write(REG_PREAMBLE_MSB + 1, self.cfg.preamble_symbols)
        self.write(REG_SYNC_WORD, self.cfg.sync_word)

        # PA_BOOST : c'est la sortie câblée sur un RFM95W, RFO n'est pas reliée.
        self.write(REG_PA_DAC, 0x84)
        self.write(REG_PA_CONFIG, 0x80 | max(0, min(15, self.cfg.tx_power_dbm - 2)))

        self._mode(MODE_STDBY)
        return version

    def _reset(self):
        if self._reset_pin is None:
            return
        try:
            import gpiod
        except ImportError:                # pragma: no cover
            return                          # le module démarre sans, moins fiable
        chip = gpiod.Chip(self._gpiochip)
        line = chip.get_line(self._reset_pin)
        line.request(consumer="sx1276", type=gpiod.LINE_REQ_DIR_OUT)
        line.set_value(0)
        time.sleep(0.001)
        line.set_value(1)
        time.sleep(0.01)
        line.release()

    def close(self):
        if self._spi is not None:
            self._spi.close()
            self._spi = None

    # --- Émission ----------------------------------------------------------
    def transmit(self, data: bytes, timeout_s=5.0) -> float:
        """Émet et attend TxDone. Rend la durée réellement mesurée, en ms."""
        self._mode(MODE_STDBY)
        self.write(REG_FIFO_ADDR_PTR, 0x00)
        for byte in data:
            self.write(REG_FIFO, byte)
        self.write(REG_PAYLOAD_LENGTH, len(data))

        debut = time.monotonic()
        self._mode(MODE_TX)
        while time.monotonic() - debut < timeout_s:
            if self.read(REG_IRQ_FLAGS) & IRQ_TX_DONE:
                self.write(REG_IRQ_FLAGS, IRQ_TX_DONE)
                return (time.monotonic() - debut) * 1000.0
            time.sleep(0.001)
        raise TimeoutError("TxDone jamais levé — module bloqué ou mal alimenté")

    # --- Réception ---------------------------------------------------------
    def listen(self):
        self.write(REG_DIO_MAPPING_1, 0x00)   # DIO0 = RxDone
        self._mode(MODE_RX_CONTINUOUS)

    def receive(self):
        """Rend `(payload, rssi_dbm, snr_db)`, ou None si rien n'est arrivé."""
        flags = self.read(REG_IRQ_FLAGS)
        if not flags & IRQ_RX_DONE:
            return None
        self.write(REG_IRQ_FLAGS, flags)

        if flags & IRQ_CRC_ERROR:
            # CRC de la couche LoRa, distinct du CRC applicatif de la trame.
            return b"", None, None

        length = self.read(REG_RX_NB_BYTES)
        self.write(REG_FIFO_ADDR_PTR, self.read(REG_FIFO_RX_CURRENT))
        payload = bytes(self.read(REG_FIFO) for _ in range(length))

        snr_brut = self.read(REG_PKT_SNR)
        snr = (snr_brut - 256 if snr_brut > 127 else snr_brut) / 4.0
        rssi = self.read(REG_PKT_RSSI) - 157      # bande haute (868 MHz)
        if snr < 0:
            rssi += snr                            # correction sous le plancher
        return payload, int(rssi), snr
