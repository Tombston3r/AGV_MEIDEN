"""Point d'entrée du service : `python -m agv_poste --config /etc/agv/poste.toml`."""

from __future__ import annotations

import argparse
import logging
import sys
import tomllib
from pathlib import Path

from .io_backend import UnsupportedBackend, make_backend
from .service import ButtonBinding, PosteService, ServiceConfig
from .transport import LoopbackTransport, MqttTransport, SmsTransport, Transport


def build_transport(section: dict[str, object]) -> Transport:
    kind = str(section.get("kind", ""))
    if kind == "mqtt":
        return MqttTransport(**{k: v for k, v in section.items() if k != "kind"})  # type: ignore[arg-type]
    if kind == "sms":
        import serial  # dépendance optionnelle

        port = serial.Serial(str(section["port"]), int(section.get("baud", 115200)), timeout=0.2)
        return SmsTransport(port, str(section["peer_msisdn"]))
    if kind == "loopback":
        return LoopbackTransport()
    raise UnsupportedBackend(f"transport inconnu : {kind!r}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Service poste fixe AGV (UniPi E413)")
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-8s %(name)s: %(message)s",
    )

    with args.config.open("rb") as handle:
        raw = tomllib.load(handle)

    service_cfg = ServiceConfig(
        node_id=int(raw.get("node_id", 2)),
        ack_timeout_s=float(raw.get("ack_timeout_s", 30.0)),
        max_command_age_s=int(raw.get("max_command_age_s", 15)),
        ordered_transport=bool(raw.get("ordered_transport", False)),
        buttons=[
            ButtonBinding(str(b["channel"]), int(b["station"]), int(b.get("speed", 4)))
            for b in raw.get("buttons", [])
        ],
    )
    io_section = dict(raw.get("io", {}))
    inputs = make_backend(str(io_section.pop("kind", "")), **io_section)
    transport = build_transport(dict(raw.get("transport", {})))

    PosteService(service_cfg, inputs, transport).run_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
