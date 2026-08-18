# Quick start

Two ways to get a board running. **Path A needs no toolchain at all** — no
Arduino IDE, no libraries, no editing source files.

---

## Path A — flash the prebuilt firmware (recommended)

### 1. Download the firmware

Grab `teensy-intercom.hex` from the
[latest release](../../releases/latest).

### 2. Flash it

1. Install the [Teensy Loader](https://www.pjrc.com/teensy/loader.html) for your OS.
2. Plug the Teensy 4.1 into USB.
3. Open `teensy-intercom.hex` in Teensy Loader.
4. Press the small button on the Teensy. It flashes and reboots.

The LED now blinks rapidly and continuously: *no broker configured yet*.

### 3. Tell it about your broker

Pick whichever is easier — both do the same thing.

**Over USB serial** (works even with no network):

Open any serial monitor at **115200 baud** (Arduino IDE Serial Monitor, `screen`,
PuTTY, `pio device monitor`) and type:

```
wizard
```

Answer the prompts — broker address, port, username, password, DHCP or static.
Press Enter to keep a shown value. The wizard saves automatically at the end.
Then:

```
reboot
```

**Over the web page** (once the board has an IP from DHCP):

Type `status` in the serial console to see the board's IP, or find it in your
router's DHCP list — it registers the hostname `teensy-intercom-<id>`. Browse to
`http://<that-ip>/`, fill in the form, hit **Save**, then **Reboot**.

### 4. Check Home Assistant

Within a few seconds the panel appears under **Settings → Devices & Services →
MQTT** as a device with 12 binary sensors. Nothing to add to
`configuration.yaml`.

The LED settles into a repeating **3 blinks** = connected and publishing.

---

## Path B — build from source

Use this if you changed the pin map, the button count, or the firmware itself.

### With PlatformIO (reproducible; this is what CI uses)

```bash
pip install platformio
pio run                # build
pio run -t upload      # build and flash a connected Teensy
pio device monitor     # open the config console
```

Library versions are pinned in `platformio.ini`, so nothing to install by hand.

### With the Arduino IDE (2.3.6 or newer)

1. **Boards Manager** → install *Teensy by PJRC* → select **Teensy 4.1**
2. **Library Manager** → install:
   - **QNEthernet** by Shawn Silverman
   - **PubSubClient** by Nick O'Leary
3. Open `firmware/TeensyIntercom/TeensyIntercom.ino`
4. **Verify** → **Upload**

There is nothing to edit before uploading — configuration happens at runtime.

> **Note:** the firmware calls `mqtt.setBufferSize()` at startup because Home
> Assistant discovery payloads are larger than PubSubClient's 256-byte default
> buffer. You do **not** need to edit `PubSubClient.h` as some older guides
> suggest.

---

## What the LED is telling you

| Pattern | Meaning |
|---------|---------|
| Fast continuous flicker | No broker configured — run `wizard` |
| 1 blink, pause, repeat | No Ethernet link (check the cable) |
| 2 blinks, pause, repeat | Link is up, but not connected to the broker |
| 3 blinks, pause, repeat | Connected and publishing — all good |

If you get 2 blinks forever, type `status` on the serial console. It prints the
MQTT client state, which usually names the problem (bad credentials, wrong
address, unreachable broker).

---

## Upgrading from firmware 1.x

1.x kept broker settings in a hand-edited `secrets.h`. That file is no longer
needed — settings live in EEPROM now.

If `firmware/TeensyIntercom/secrets.h` is still present when you build from
source, its values seed the defaults on first boot, including
`DEVICE_UNIQ_SUFFIX`, so your existing Home Assistant entities keep working.

If you flash the prebuilt `.hex` instead, the board derives a new device ID from
its MAC address, which creates **new** entities. To keep the old ones, set the
device ID back to your old suffix:

```
set device_id ABC123
save
reboot
```

See [configuration.md](configuration.md) for the full settings reference.
