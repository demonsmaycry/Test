# Test

## ESPHome UART bridge

The `esphome/components/infisolar_pi18` directory contains a UART component
that identifies the inverter protocol and model during the ESPHome startup
sequence. The debug logging mirrors the original firmware output:

```
[I][infisolar.pi18]: [HS] TX ^P005PI (stage=1 attempt=1)
[D][infisolar.pi18]: [HS] RX ascii: ^D00518;
[D][infisolar.pi18]: [HS] RX raw HEX: 5E 44 30 30 35 31 38 3B
[D][infisolar.pi18]: [HS] RX payload HEX: 31 38 3B
[D][infisolar.pi18]: [HS] RX payload ASCII: 18;
[I][infisolar.pi18]: [HS] protocol detected: PI18 (type=D len=5)
[I][infisolar.pi18]: [HS] TX ^P006GMN (stage=2 attempt=1)
[D][infisolar.pi18]: [HS] RX ascii: ^D00512
[D][infisolar.pi18]: [HS] RX raw HEX: 5E 44 30 30 35 31 32
[D][infisolar.pi18]: [HS] RX payload HEX: 31 32
[D][infisolar.pi18]: [HS] RX payload ASCII: 12
[I][infisolar.pi18]: [HS] model detected: 12 (type=D len=5)
```

The payload and HEX dumps follow the same framing used by the
[`InfiniSolarP18`](https://github.com/sustanova/InfiniSolarP18) project, so you
can cross-reference numeric identifiers (for example `12` in the handshake
above) with its lookup tables when you need a friendly inverter model name.

Use the sample configuration in [`esphome/example.yaml`](esphome/example.yaml)
to expose the detected protocol and model as text sensors that Home Assistant
can consume automatically through the native ESPHome API.

## Home Assistant integration

After flashing the ESPHome node, add it to Home Assistant through the ESPHome
integration panel. The two text sensors reported by the firmware appear as
`text_sensor.infisolar_protocol` and `text_sensor.infisolar_model`. For a quick
dashboard view copy the helper templates from
[`homeassistant/README.md`](homeassistant/README.md) into your
`configuration.yaml`.

## PI18 CRC debug helper

The `tools/pi18_crc_debug.py` script helps to validate the CRC reported
by InfiniSolar/PI18 inverters when troubleshooting serial logs like the
following:

```
[E][infisolar:158]: CRC mismatch: got 9A 49 expected 4C 46
```

The device response `^D00512` actually produces the CRC `0x9A49` when
the CRC-16/XMODEM algorithm is applied to the entire ASCII frame (the
leading `^` included). Run the helper with the captured payload to check
what value the inverter is sending:

```bash
python tools/pi18_crc_debug.py '^D00512'
```

If the computed CRC matches what appears on the wire, the mismatch is
caused by the host implementation and not by the inverter. In that case
update the firmware/driver so that it also evaluates the CRC using
CRC-16/XMODEM with an initial value of `0x0000`.


In questo modo il codice prodotto qui può essere pubblicato e condiviso su
GitHub mantenendo il controllo completo delle credenziali e della cronologia.
