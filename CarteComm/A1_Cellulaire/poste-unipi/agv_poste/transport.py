"""Transports du poste UniPi : SMS (variante A) et MQTT/LTE-M (variante B).

Le brief est explicite : « Si le cellulaire est imposé par le client, c'est la
variante B qu'il faut coder, jamais le SMS. » Les deux sont fournis pour
l'analyse comparative, mais `SmsTransport` journalise un avertissement au
démarrage — un transport à latence non bornée pilotant un engin mobile ne doit
jamais être mis en service par inadvertance.
"""

from __future__ import annotations

import logging
import re
import time
from typing import Protocol

logger = logging.getLogger(__name__)

PAYLOAD_PREFIX = "AGV:"


class Transport(Protocol):
    def send(self, payload: bytes) -> bool: ...
    def receive(self) -> list[bytes]: ...
    def close(self) -> None: ...


class SmsTransport:
    """SMS via modem AT (SIM7600E-H ou modem intégré de l'UniPi LTE).

    ⚠ Ni latence bornée, ni ordre de remise, ni garantie de remise, ni
    protection contre les doublons. Réservé à l'analyse comparative et aux
    alertes bas volume (§8.3).
    """

    CMTI = re.compile(r"\+CMTI:\s*\"[^\"]*\",\s*(\d+)")

    def __init__(self, serial_port: object, peer_msisdn: str) -> None:
        self._serial = serial_port
        self._peer = peer_msisdn
        self._buffer = ""
        logger.warning(
            "transport SMS actif : latence NON bornée et ordre NON garanti. "
            "Déconseillé comme liaison principale (voir Archi_2 §8)."
        )
        for command in ("AT", "ATE0", "AT+CMGF=1", "AT+CNMI=2,1,0,0,0"):
            self._command(command)

    def _command(self, command: str, wait_s: float = 1.0) -> str:
        self._serial.write((command + "\r\n").encode())  # type: ignore[attr-defined]
        time.sleep(wait_s)
        data = self._serial.read(4096)  # type: ignore[attr-defined]
        return data.decode(errors="replace") if isinstance(data, bytes) else str(data)

    def send(self, payload: bytes) -> bool:
        body = PAYLOAD_PREFIX + payload.hex().upper()
        self._serial.write(f'AT+CMGS="{self._peer}"\r'.encode())  # type: ignore[attr-defined]
        time.sleep(0.5)
        self._serial.write(body.encode() + b"\x1a")  # type: ignore[attr-defined]
        time.sleep(3.0)
        response = self._serial.read(4096)  # type: ignore[attr-defined]
        text = response.decode(errors="replace") if isinstance(response, bytes) else str(response)
        return "OK" in text

    def receive(self) -> list[bytes]:
        raw = self._serial.read(4096)  # type: ignore[attr-defined]
        if raw:
            self._buffer += raw.decode(errors="replace") if isinstance(raw, bytes) else str(raw)

        payloads: list[bytes] = []
        for match in self.CMTI.finditer(self._buffer):
            index = int(match.group(1))
            text = self._command(f"AT+CMGR={index}", wait_s=1.5)
            payloads.extend(self._extract(text))
            self._command(f"AT+CMGD={index}", wait_s=0.5)
        self._buffer = ""

        payloads.extend(self._extract(self._buffer))
        return payloads

    @staticmethod
    def _extract(text: str) -> list[bytes]:
        out: list[bytes] = []
        for line in text.splitlines():
            position = line.find(PAYLOAD_PREFIX)
            if position < 0:
                continue
            hex_part = line[position + len(PAYLOAD_PREFIX):].strip()
            try:
                out.append(bytes.fromhex(hex_part))
            except ValueError:
                logger.warning("charge utile SMS illisible : %r", hex_part)
        return out

    def close(self) -> None:
        try:
            self._serial.close()  # type: ignore[attr-defined]
        except Exception:  # noqa: BLE001 - fermeture au mieux
            logger.debug("fermeture du port série ignorée", exc_info=True)


class MqttTransport:
    """LTE-M / NB-IoT + MQTT — la seule variante cellulaire défendable (§8.2).

    QoS 1 minimum, Last Will and Testament sur `status` pour une détection
    immédiate de la perte de l'AGV, TLS si le broker est hors site.
    """

    def __init__(
        self,
        host: str,
        port: int = 8883,
        client_id: str = "poste-1",
        agv_id: str = "1",
        qos: int = 1,
        tls: bool = True,
        ca_certs: str | None = None,
    ) -> None:
        import paho.mqtt.client as mqtt  # dépendance optionnelle

        self._qos = qos
        self._agv_id = agv_id
        self._inbox: list[bytes] = []
        self._client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311)
        # LWT : le broker annonce la perte du poste sans attendre un timeout
        # applicatif.
        self._client.will_set(f"poste/{client_id}/status", "OFFLINE", qos=qos, retain=True)
        if tls:
            self._client.tls_set(ca_certs=ca_certs)
        self._client.on_message = self._on_message
        self._client.connect(host, port, keepalive=60)
        self._client.subscribe([(f"agv/{agv_id}/ack", qos), (f"agv/{agv_id}/telemetry", qos)])
        self._client.publish(f"poste/{client_id}/status", "ONLINE", qos=qos, retain=True)
        self._client.loop_start()

    def _on_message(self, _client: object, _userdata: object, message: object) -> None:
        payload = getattr(message, "payload", b"")
        text = payload.decode(errors="replace") if isinstance(payload, bytes) else str(payload)
        if text.startswith(PAYLOAD_PREFIX):
            try:
                self._inbox.append(bytes.fromhex(text[len(PAYLOAD_PREFIX):]))
            except ValueError:
                logger.warning("charge utile MQTT illisible")

    def send(self, payload: bytes) -> bool:
        info = self._client.publish(
            f"agv/{self._agv_id}/cmd", PAYLOAD_PREFIX + payload.hex().upper(), qos=self._qos
        )
        return info.rc == 0

    def receive(self) -> list[bytes]:
        out, self._inbox = self._inbox, []
        return out

    def close(self) -> None:
        self._client.loop_stop()
        self._client.disconnect()


class LoopbackTransport:
    """Transport de test : ce qui est émis peut être relu par le test."""

    def __init__(self) -> None:
        self.sent: list[bytes] = []
        self.inbox: list[bytes] = []
        self.refuse = False

    def send(self, payload: bytes) -> bool:
        if self.refuse:
            return False
        self.sent.append(payload)
        return True

    def receive(self) -> list[bytes]:
        out, self.inbox = self.inbox, []
        return out

    def close(self) -> None:
        self.sent.clear()
