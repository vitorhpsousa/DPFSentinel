# 2. Daily use

## Switching on

Plug the adapter into the car's OBD port and switch the ignition on. The logger starts by itself if it has been set up to (see the setup guides). Give it a short while to connect. Until it does, the screen may be grey.

## Reading the screen

- **Big number:** soot estimate in grams. The background colour follows it.
- **Grid below it:** other readings. "Since regen" is the distance since the last regeneration.
- **Near-black screen:** a regen is in progress.
- **Grey screen:** no fresh data.

Do not study the screen while driving. Glance only when it is safe.

## What to do at each colour

The colours are the owner's thresholds, so treat these as sensible habits, not rules.

| Colour | Suggested action |
|---|---|
| Green | Nothing to do. |
| Amber | Plan a longer run at steady speed so the engine gets fully warm (inferred general diesel advice, not tested by this project). |
| Red | Do the same soon. If a warning light is on, or the car behaves oddly, follow your handbook and see a garage. |
| Near-black, "do not switch off" | Keep the engine running until the screen returns to a colour, if it is safe. |

## Regen best practice

Inferred from general advice and the screen's wording, not from testing in this project:

- Let a regen finish rather than switching off in the middle of it.
- If you need to stop, do so safely. Your safety comes first.
- Short trips are what stop regens completing. An occasional longer drive helps.
- Warning lights and your handbook override this device.

## Switching off

Check the screen first. If it shows the regen banner, finish the regen if you can. The ESP32-S3 version sends an "interrupted" message if a regen was cut short ([see Telegram alerts](03-telegram-alerts-for-users.md)).
