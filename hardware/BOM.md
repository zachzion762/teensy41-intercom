# Bill of Materials (BOM)

Exact parts I used to build the Teensy 4.1 Intercom button panel.

| Item | Qty | Link | Notes |
|------|-----|------|-------|
| **Teensy 4.1 + Ethernet Kit** | 1 | [Amazon](https://www.amazon.com/dp/B09NXYWYK7?ref=ppx_yo2ov_dt_b_fed_asin_title) | Main MCU with native Ethernet (PJRC kit) |
| **RJ45 PoE Splitter (12V out)** | 1 | [Amazon](https://www.amazon.com/dp/B07W6KM59K?ref=ppx_yo2ov_dt_b_fed_asin_title) | Powers the Teensy over Ethernet (12 V) |
| **Screw Terminal Breakout (Teensy 4.1)** | 1 | [Amazon](https://www.amazon.com/dp/B08RSCFBNF?ref=ppx_yo2ov_dt_b_fed_asin_title) | Easier button wiring / strain relief |
| **Momentary pushbuttons (SPST, NO)** | 12 | Local/Amazon | Any panel‑mount momentary button |
| **Hookup wire** | — | Local | 22–24 AWG stranded recommended |
| **3D printed bracket** | 1 | [hardware/Teensy_4.1_Ethernet_Mount.stl](hardware/Teensy_4.1_Ethernet_Mount.stl) | Mount for Teensy + Ethernet kit |
| **Mounting panel / wood block** | 1 | Local | See photos |

> Notes:
> - Buttons are wired **GPIO → button → GND** (Teensy pins use INPUT_PULLUP, press = LOW).
> - The PoE splitter provides 12 V; regulate as needed if you power anything else.
