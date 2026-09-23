# 4. Troubleshooting

## The screen is grey

- **"NO LOGGER DATA":** the screen cannot find any log data. The logger may not be running, or it has not written a row yet.
- **"NO LIVE DATA":** the newest reading is more than 8 seconds old. The logger has stopped or lost the adapter.

Checks, in order:

1. Is the ignition on and the adapter firmly in the OBD port, with its light on?
2. Give it a minute. The logger retries the adapter every 5 seconds by itself.
3. Restart the Pi (or the ESP32-S3 by cycling power).
4. Still nothing: see [pi-setup.md](../pi-setup.md) for the technical checks.

With the engine off, some readings may be blank. That is normal.

## The adapter only talks to one thing at a time

The adapter accepts **one connection at a time** (the repo says this for Bluetooth LE adapters, and for the OBDLink MX+ on the Pi). If a phone app, another logger or a second device is connected, the logger will look dead.

- Close phone apps such as Car Scanner, and turn off Bluetooth on the phone if unsure.
- Do not run the Pi and the ESP32-S3 on the same adapter together.

## The times or dates look wrong

Neither the Pi nor the ESP32-S3 has a real-time clock.

- **Pi:** the clock can be wrong in a car with no network, so times in the logs may be off.
- **ESP32-S3:** the clock is set once per boot, from the internet (after joining Wi-Fi) or from the first phone that opens its web page. Until then there are no clock times, and Telegram messages are not sent. Join a hotspot, or open the web page on your phone.
- The clock uses UK time only.

## No Telegram messages

- Is your phone hotspot on **before** the ignition, and 2.4 GHz?
- Was the bot started in Telegram, and are the token and chat set correctly? Ask whoever set it up.
- Remember the ESP32-S3 has to have its clock set first.

## Something looks wrong on the dashboard

One known bug: the dashboard's `/api/schema` page fails, which can affect a small hint in the web page. Logging is not affected.
