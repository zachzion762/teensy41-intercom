# Configuration reference

All settings live in the Teensy's EEPROM. Nothing is compiled into the
firmware, which is why one prebuilt image works on any network.

Changes take effect after `save` plus a reboot. MQTT settings reconnect on the
next attempt; network settings (DHCP, static IP, MAC) need the reboot.

---

## Settings

| Key | Default | Notes |
|-----|---------|-------|
| `host` | *(empty)* | Broker IP or hostname, e.g. `192.168.1.10` or `homeassistant.local` |
| `port` | `1883` | 1–65535 |
| `user` | *(empty)* | Blank connects without authentication |
| `pass` | *(empty)* | Blank connects without authentication |
| `device_id` | *(from MAC)* | Suffix for Home Assistant `unique_id`. **Changing it creates new entities.** |
| `device_name` | `Teensy Intercom` | Device name shown in Home Assistant |
| `base_topic` | `intercom` | Root of the state topics |
| `discovery_prefix` | `homeassistant` | Must match your HA MQTT discovery prefix |
| `dhcp` | `on` | `on`/`off`. When `off`, set `ip`/`mask`/`gw`/`dns` |
| `ip` | `0.0.0.0` | Static address (only used when `dhcp` is `off`) |
| `mask` | `0.0.0.0` | Subnet mask |
| `gw` | `0.0.0.0` | Gateway |
| `dns` | `0.0.0.0` | DNS server — needed if `host` is a hostname |
| `mac` | *(auto)* | Override the MAC, e.g. `02:12:34:56:78:9a`. Empty restores the built-in one |
| `web` | `on` | Turn the configuration web page off entirely |

Booleans accept `on`/`off`, `true`/`false`, `yes`/`no`, `1`/`0`.

---

## Serial console

115200 baud, over USB. Works before the network does, so it is always the way
back in.

| Command | Effect |
|---------|--------|
| `help` | List commands and setting keys |
| `wizard` | Guided setup, one prompt per setting |
| `show` | Print the current configuration (password masked) |
| `show secrets` | Same, but reveals the stored password |
| `set <key> <value>` | Change one setting |
| `save` | Write the configuration to EEPROM |
| `status` | Link state, IP, MAC, MQTT state, uptime |
| `factory-reset` | Erase stored configuration and reboot |
| `reboot` | Restart the board |

`set` takes the rest of the line as the value, so passwords may contain spaces.
Changes are not persistent until you run `save` — except in the wizard, which
saves for you at the end.

Example:

```
set host 192.168.1.10
set user mqtt-user
set pass my long passphrase
save
reboot
```

---

## Web page

Served on port 80 at the board's IP address once it has one. It exposes the
same settings as the serial console, plus a **Reboot** button.

Leaving the password box empty keeps the stored password; tick *clear the
stored password* to remove it.

### Security

The page is **plain HTTP with no authentication**, intended for a trusted home
LAN — the same trust level as an unauthenticated MQTT broker on the same
network. Anyone who can reach the board's IP can read the broker address and
username (the password is never sent back to the browser) and change settings.

If that is not acceptable on your network, turn it off:

```
set web off
save
reboot
```

Serial then becomes the only configuration path, which is exactly what
`factory-reset` is for if you get locked out.

---

## Changing the button count or pins

Button pins are still compile-time, since they are a property of how you wired
the panel rather than of your network. Edit `kButtonPins` in
`firmware/TeensyIntercom/src/BoardConfig.h`:

```cpp
static const uint8_t kButtonPins[] = {1, 3, 5, 7, 9, 10, 12, 24, 26, 28, 30, 32};
```

`kButtonCount` follows the array automatically, and discovery, state topics,
and the scanner all derive from it. Rebuild and upload.

If you shrink the panel, the retired buttons keep their retained discovery
configs in the broker and will linger in Home Assistant as unavailable
entities. Delete the stale `homeassistant/binary_sensor/<device>_<n>/config`
topics on the broker to clear them.

---

## Storage details

The configuration is a single struct at EEPROM address 0, guarded by a magic
number, a version, and a CRC32. A failed check falls back to defaults rather
than loading garbage, so a partial write or a firmware version bump cannot
brick the board — it just comes up unconfigured.

`EEPROM.put()` only rewrites bytes that actually changed, so repeated saves do
not meaningfully wear the emulated flash.
