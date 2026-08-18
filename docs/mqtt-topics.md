# MQTT topics

`<base>` is the `base_topic` setting (default `intercom`), and `<prefix>` is
`discovery_prefix` (default `homeassistant`). `<device>` is
`teensy-intercom-<device_id>`.

## Published by the panel

| Topic | Payload | Retained | Notes |
|-------|---------|----------|-------|
| `<base>/availability` | `online` / `offline` | yes | `offline` is also the MQTT last will, so an unplugged board goes unavailable on its own |
| `<base>/button/1`…`N` | `ON` / `OFF` | no | One topic per button |
| `<prefix>/binary_sensor/<device>_<n>/config` | discovery JSON | yes | Re-announced every 5 minutes |

With defaults and a device ID of `0abbcc`:

```
intercom/availability                                     online
intercom/button/1                                         ON
homeassistant/binary_sensor/teensy-intercom-0abbcc_1/config  {...}
```

## Button behaviour

The panel reports **one active button at a time**. When several are held, the
lowest-numbered one wins; pressing a lower-numbered button while another is
held publishes `OFF` for the old one and `ON` for the new one. Presses are
debounced for 20 ms.

## Discovery payload

Each button is announced as a binary sensor tied to a single device, so all
twelve group together in Home Assistant:

```json
{
  "name": "Button 1",
  "uniq_id": "teensy-intercom-0abbcc_btn_1",
  "stat_t": "intercom/button/1",
  "pl_on": "ON",
  "pl_off": "OFF",
  "avty_t": "intercom/availability",
  "pl_avail": "online",
  "pl_not_avail": "offline",
  "dev": {
    "ids": ["teensy-intercom-0abbcc"],
    "name": "Teensy Intercom",
    "mf": "PJRC",
    "mdl": "Teensy 4.1",
    "sw": "2.0.0"
  }
}
```

These payloads are around 330 bytes, which is larger than PubSubClient's
256-byte default buffer — the firmware raises it with `setBufferSize()` at
startup. If discovery ever stops working after a library change, check
`status`: it reports whether the discovery publish was rejected.

## Watching the traffic

```bash
mosquitto_sub -h 192.168.1.10 -u mqtt-user -P secret -v -t 'intercom/#'
mosquitto_sub -h 192.168.1.10 -u mqtt-user -P secret -v -t 'homeassistant/binary_sensor/teensy-intercom-#'
```
