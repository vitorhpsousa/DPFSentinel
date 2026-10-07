# 1. What it does

This device listens to your car's engine computer and shows how full the diesel particulate filter (DPF) is. It only reads numbers the car already reports. It changes nothing on the car.

> **Not a diagnostic tool.** It cannot tell you the filter is blocked or that the car is safe to drive. Follow your handbook, warning lights and a garage. See the [disclaimer](../disclaimer.md).

## Soot and regeneration

- **Soot** is the black residue diesel engines make. The DPF traps it so it does not come out of the exhaust.
- **Regeneration ("regen")** is when the car burns that soot off by making the exhaust very hot. It only works if the engine stays warm and running for long enough. Many short trips do not give it the chance, so soot builds up.

The soot figure on the screen is in **grams**. It is the engine computer's own **estimate**, not a measurement of the filter.

## The colours

On the 3.5" screen the background changes with the soot figure:

| Colour | Soot | Meaning |
|---|---|---|
| Green | below 14 g | Low |
| Amber | 14 g to under 17 g | Getting high |
| Red | 17 g and above | High. It shades towards dark red as soot rises, fully dark red at 28 g |

These limits are the owner's own choices, **not** manufacturer limits. On the author's car the soot estimate reached 17.6 g after two short rides.

## "Do not switch off"

When the car reports a regen in progress, the screen turns near-black and shows **ACTIVE REGENERATION, DO NOT SWITCH OFF UNTIL IT FINISHES**, with the soot figure and the exhaust temperature.

It means: interrupting a regen is undesirable, so keep the engine running if it is safe to do so. This is general advice. The flag behind it was worked out from one reference log, so it may be late or wrong. If in doubt, follow your car's handbook.

## Other screens

- **Grey, "NO LIVE DATA" or "NO LOGGER DATA":** no fresh readings. See [Troubleshooting](04-troubleshooting.md).
- Otherwise a grid of extra readings shows below the soot figure (RPM, speed, coolant, pressure, temperatures, distance since last regen, and so on).

## Versions

- **Raspberry Pi version:** screen and a web page on your network. This is a separate project, not included in this repository and not yet public.
- **ESP32-S3 version (this repository):** logs to an SD card, has a web page, and sends [Telegram messages](03-telegram-alerts-for-users.md). The Pi version has no Telegram support. There are three ESP32-S3 builds documented in the main [README](../../README.md#supported-targets); day-to-day use is the same across all of them.
