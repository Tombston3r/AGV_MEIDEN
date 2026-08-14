"""Point d'entrée du poste fixe : `python -m agv_poste --config /etc/agv/poste.toml`.

Assemble le récepteur EnOcean (série), le client MQTT et le service.
Le broker Mosquitto et l'interface web sont des services distincts.
"""

from __future__ import annotations

import argparse
import logging
import sys
import time
import tomllib
from pathlib import Path

from .service import PosteService, ServiceConfig

logger = logging.getLogger(__name__)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Poste fixe AGV MEIDEN (architecture Wi-Fi)")
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-8s %(name)s: %(message)s",
    )

    with args.config.open("rb") as handle:
        raw = tomllib.load(handle)

    import paho.mqtt.client as mqtt  # dépendance optionnelle
    import serial  # dépendance optionnelle

    mqtt_cfg = raw.get("mqtt", {})
    agv_id = str(raw.get("agv_id", "1"))

    client = mqtt.Client(client_id=str(mqtt_cfg.get("client_id", "poste-1")))
    if mqtt_cfg.get("tls", True):
        client.tls_set(ca_certs=mqtt_cfg.get("ca_certs"))
    if mqtt_cfg.get("username"):
        client.username_pw_set(mqtt_cfg["username"], mqtt_cfg.get("password", ""))

    def publish(topic: str, payload: str, retain: bool) -> bool:
        info = client.publish(topic, payload, qos=int(mqtt_cfg.get("qos", 1)), retain=retain)
        return info.rc == 0

    service = PosteService(
        ServiceConfig(
            agv_id=agv_id,
            dedup_window_ms=int(raw.get("dedup_window_ms", 100)),
            pairing_timeout_s=int(raw.get("pairing_timeout_s", 60)),
            state_stale_s=float(raw.get("state_stale_s", 5.0)),
            pairings_path=Path(raw.get("pairings_path", "/var/lib/agv/pairings.json")),
        ),
        publish,
    )

    def on_message(_client, _userdata, message) -> None:
        service.on_mqtt_message(message.topic, message.payload.decode(errors="replace"))

    client.on_message = on_message
    client.connect(str(mqtt_cfg.get("host", "127.0.0.1")), int(mqtt_cfg.get("port", 8883)),
                   keepalive=int(mqtt_cfg.get("keepalive_s", 30)))
    client.subscribe([(f"agv/{agv_id}/state", 1), (f"agv/{agv_id}/ack", 1)])
    client.loop_start()

    enocean_cfg = raw.get("enocean", {})
    port = serial.Serial(
        str(enocean_cfg.get("port", "/dev/ttyS0")),
        int(enocean_cfg.get("baud", 57600)),  # ESP3 : 57 600 bauds
        timeout=0.1,
    )
    logger.info("poste fixe démarré (agv_id=%s, %d appairage(s))", agv_id, len(service.pairings))

    try:
        while True:
            data = port.read(64)
            for byte in data:
                service.feed_enocean(byte)
            if not data:
                time.sleep(0.01)
    except KeyboardInterrupt:
        return 0
    finally:
        client.loop_stop()
        client.disconnect()
        port.close()


if __name__ == "__main__":
    sys.exit(main())
