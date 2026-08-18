# Home Assistant integration

There is nothing to install in Home Assistant — no custom integration, no
HACS repository, no YAML. The panel announces itself over MQTT discovery, and
Home Assistant creates the entities on its own.

## Requirements

- The [MQTT integration](https://www.home-assistant.io/integrations/mqtt/) set
  up and pointed at your broker.
- MQTT discovery enabled with the default `homeassistant` prefix (this is the
  default). If you changed it, set the panel's `discovery_prefix` to match.

## What appears

One device, **Teensy Intercom**, with twelve binary sensors — *Button 1*
through *Button 12*. Each is `on` while its button is held and `off` otherwise,
and all of them go unavailable when the panel loses power or network, via the
retained availability topic and MQTT last will.

Find it under **Settings → Devices & Services → MQTT → Teensy Intercom**.

![HA discovery](screenshots/ha_discovery.png)
![HA entities](screenshots/ha_entities.png)

## Automation blueprint

[`teensy_intercom_button.yaml`](../homeassistant/blueprints/automation/teensy_intercom_button.yaml)
turns a button into an automation without hand-writing triggers.

**Install it:**

1. Copy the file into your Home Assistant config at
   `blueprints/automation/teensy_intercom/teensy_intercom_button.yaml`
   (create the folders if needed).
2. **Developer tools → YAML → Reload blueprints**, or restart Home Assistant.
3. **Settings → Automations & scenes → Blueprints** → *Teensy Intercom button*
   → **Create automation**.

**Use it:** pick the button, choose whether to fire on press or release, and
attach the action. Add one automation per button you care about.

Release is usually the right choice for momentary actions — it fires when the
button is let go, so a long press does not repeat.

## Doing it manually instead

If you would rather write the automation yourself:

```yaml
automation:
  - alias: "Intercom button 1 — announce front door"
    triggers:
      - trigger: state
        entity_id: binary_sensor.teensy_intercom_button_1
        from: "on"
        to: "off"
    actions:
      - action: tts.speak
        target:
          entity_id: tts.piper
        data:
          media_player_entity_id: media_player.kitchen
          message: "Someone is at the front door."
```

Entity IDs are assigned by Home Assistant on first discovery; check
**Developer tools → States** for the exact names on your system.

## Why not a custom integration?

MQTT discovery already gives zero-touch setup. A custom integration would add
install steps (add a HACS repository, install, restart, configure) and would
need ongoing maintenance against Home Assistant's release cadence — in exchange
for nothing this does not already do.
