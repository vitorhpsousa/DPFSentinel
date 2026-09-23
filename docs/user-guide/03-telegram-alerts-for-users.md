# 3. Telegram alerts

Only the **ESP32-S3 version** sends Telegram messages. The Pi version does not. Setting it up is covered in [telegram-alerts.md](../telegram-alerts.md); this page explains what you will receive.

## Messages

| Message starts with | Meaning |
|---|---|
| **DPF regeneration started** | The regen has been flagged for about 3 seconds. Shows soot (g), miles since the last regen, exhaust temperature and, if known, filter pressure. |
| **DPF regen still running** | Sent about every 30 seconds during a regen, with time so far. |
| **DPF regeneration finished** | Shows duration, peak exhaust temperature and soot before and after. |
| **DPF regeneration was interrupted** | A regen was open and data then resumed after a gap of over 2 minutes (engine stopped or signal lost). It is sent when data comes back, **not when it stops**. |
| **TEST alert** | Sent when you test the setup. |

Other messages, if switched on in the settings: a note when the board joins a Wi-Fi network (name and address), a periodic status ping (every 5 minutes by default while the engine is running and warm), and a daily report at 17:30 with session logs.

## Limits to know about

- Messages are sent only when the board is on Wi-Fi **and** its clock is set. In the car that usually means a phone hotspot.
- The board only tries to join Wi-Fi at start-up and then every 10 minutes while the engine is **not running**. Turn your hotspot on **before** the ignition. It will not join one that appears mid-drive.
- Up to 4 messages wait in memory. If full, the oldest is dropped, so a "started" message can be lost in a long offline regen. Waiting messages are lost if power is cut.
- The regen flag may be wrong or late. A missing message does not prove nothing happened.
