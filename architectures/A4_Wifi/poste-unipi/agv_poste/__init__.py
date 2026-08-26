"""Poste fixe AGV MEIDEN — architecture Wi-Fi (UniPi E413).

Chaîne : bouton EnOcean PTM 210 -> TCM 515 (ESP3) -> table d'appairage ->
publication MQTT `agv/<id>/cmd` -> ESP32 de la carte V5.0.1.

⚠ Le runtime réellement disponible sur la référence commandée n'est pas tranché
(§12.9, planification 0.1) : voir agv_poste.io_backend.
"""

__version__ = "0.1.0"
