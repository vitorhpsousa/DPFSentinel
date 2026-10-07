# Easy setup — no technical experience needed

This page is for someone who has **never used a terminal, never installed
developer tools, and has no idea what "flashing firmware" means**, but wants
to build their own DPF monitor for a 2014 Hyundai ix35 1.7 CRDi. Everything
here is point-and-click. If you're comfortable with a command line already,
[manual-setup.md](../manual-setup.md) and [workstation-setup.md](../workstation-setup.md)
are faster.

This guide builds **one board: the CYD 3.5" touchscreen** (`boards/cyd-s3-3p5`
in this repo). It's the right one to start with — no soldering, no Raspberry
Pi, no Linux skills, and it doesn't need your phone once it's set up.

## Two ways to do this

**Option A — have an AI assistant walk you through it.** Open
[Claude](https://claude.ai) or [ChatGPT](https://chatgpt.com) in a browser
tab, and paste the box below. It will ask you questions and give you one
step at a time, in plain language, checking in before anything that matters.

````text
I'm a complete beginner and I want to build a DPF (diesel particulate filter)
monitor for my car from an open-source project, using a ready-made ESP32-S3
touchscreen board called "CYD 3.5-inch". I have no programming or command-line
experience. Walk me through this from scratch, GUI only — no terminal commands
unless there is truly no other way, and if there isn't, explain what the
command does in plain English before I type it.

The steps, in order, are documented at:
https://github.com/vitorhpsousa/DPFGuardian/blob/main/docs/easy-setup/README.md (this page — the shopping list and the
config file walkthrough) and https://github.com/vitorhpsousa/DPFGuardian/blob/main/docs/easy-setup/windows.md (or mac.md /
linux.md, whichever matches my computer) for installing the software and
flashing the board. Read those pages first — ask me to paste them if you can't
open the link. Then ask me which operating system I'm on and follow only that
page.

Rules for you:
- One step at a time. After each step, tell me exactly what I should see if it
  worked, and wait for me to confirm before the next one.
- Never ask me to paste my WiFi password, Telegram bot token, or any other
  secret into this chat. Tell me which file to put it in on my own computer
  instead, and exactly what line to change.
- If something doesn't look like the guide says, stop and help me figure out
  why rather than telling me to skip it.
- Tell me plainly if something I'm asking for isn't possible or isn't a good
  idea, rather than guessing.

I'll tell you when I'm ready for step one.
````

**Option B — do it yourself, no AI.** Follow the written steps below, then
pick your operating system:

- [Windows](windows.md)
- [Mac](mac.md)
- [Linux](linux.md)

Both options use the exact same steps underneath — Option A just has someone
explaining each one to you as you go.

## What you're building

A small touchscreen box that plugs into your car's OBD-II port (the
diagnostic socket, usually under the dashboard near the steering column — the
same one a garage's scanner plugs into) via a small Bluetooth adapter. It
reads how much soot is in your diesel particulate filter and warns you before
it clogs — it doesn't change anything about how your car runs, it only reads
data the engine computer already has. It does **not** need your phone once
it's working, and it does **not** need you to pair anything in your
computer's or phone's Bluetooth settings — the board talks to the adapter
directly, on its own.

## Shopping list

1. **The CYD 3.5" board** — sold on Amazon/AliExpress as "CYD ESP32 S3
   Display 3.5 Inch IPS Capacitive Touch 320x480 ST77922 HMI Module,
   XiaoZhiAI" (around £20–25). A USB-C cable comes with most listings, but
   make sure you have one — see the warning below.
2. **A Bluetooth OBD-II adapter.** It must be **BLE** ("Bluetooth Low
   Energy" / "Bluetooth 4.0+"), not "Bluetooth Classic" — the board can only
   talk to the BLE kind. Look for one that says BLE or "Bluetooth 4.0/5.0" in
   its listing. Adapters from **Veepeak** or **Konnwei** that advertise BLE
   support are known to work with this project; a generic one whose listing
   mentions "BLE" should also work, since the board searches for anything
   with "OBD", "ELM", "VEEPEAK", "VLINK", "OBDLINK" or "KONNWEI" in its name.
   Around £15–20.
3. **A microSD card**, 32 GB or smaller, plus a card reader for your
   computer if it doesn't have a slot built in (many laptops don't — a USB
   card reader is a couple of pounds). **Larger than 32 GB will not work**
   without an extra step — see the FAT32 warning on your OS page.
4. **A USB cable that actually carries data, not just power.** Some cheap
   cables only charge and cannot transfer files — if your computer never
   sees the board after plugging it in, this is the first thing to try
   swapping.
5. Something to power the board once it's living in the car — a spare phone
   charger plugged into a 12V-to-USB adapter, or your car's own USB port if
   it has one, both work fine.

## Setting up your own secrets (do this after installing the software)

Every board needs to know your WiFi network name and password before it can
connect to the internet, and — if you want phone alerts — a Telegram bot
token. These live in **one file on your own computer**, never typed into a
chat with anyone, AI or human.

1. In the project folder, go to `boards/cyd-s3-3p5/firmware/src/`. You'll see
   a file called `config.example.h`. **Make a copy of it in the same folder
   and rename the copy to `config.h`** (not `config.example.h` — the board
   only reads `config.h`).
2. Open `config.h` in a plain text editor (VS Code, which you'll already
   have installed by this point, works fine — double-click the file).
3. Find these two lines and put your own WiFi network name and password
   between the quote marks, replacing the placeholder text exactly:
   ```
   #define WIFI_STA_SSID "Vitoi"
   #define WIFI_STA_PASS "YOUR_WIFI_PASSWORD"
   ```
4. **Optional — Telegram alerts on your phone.** If you want a message when
   a regeneration starts or finishes, or a daily report:
   - Open Telegram, search for **@BotFather**, and send it `/newbot`. Follow
     its prompts (it asks for a name, then a username ending in `bot`). It
     replies with a long token that looks like
     `123456789:AAFxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx` — copy it.
   - Send your new bot any message (e.g. "hi") so it knows who you are.
   - In a browser, go to
     `https://api.telegram.org/bot<your token>/getUpdates` (with your real
     token in place of `<your token>`). Look for `"chat":{"id":` in the page
     that loads — the number right after it is your chat ID.
   - Back in `config.h`, fill in:
     ```
     #define TG_BOT_TOKEN "your token here"
     #define TG_CHAT_ID "your chat id here"
     ```
   - Don't want Telegram? Leave both lines exactly as they are
     (`YOUR_TELEGRAM_BOT_TOKEN` / `YOUR_CHAT_ID`) — the feature switches
     itself off when the token is empty-looking like that.
5. Leave every other line in `config.h` as it is unless your OS-specific page
   tells you otherwise. **Never share your `config.h` file, and never commit
   it or paste its contents anywhere** — it's listed in this project's
   `.gitignore` specifically so a normal "save/commit my changes" step can't
   accidentally publish it.

## Formatting the microSD card

The board needs the card formatted as **FAT32**. This matters because if the
card is blank, unformatted, or in the wrong format, the board **will not
warn you** — it silently falls back to its own tiny internal memory (room
for a few minutes of driving) instead. Each OS page below has the exact
formatting steps for that operating system — do this before the card's first
use.

## Checking it's actually working

After you've flashed the board (your OS page covers this) and inserted the
formatted SD card:

1. Power the board from USB. You should hear a short boot jingle and the
   screen should light up.
2. With no car connected yet, the screen shows **"NO LOGGER DATA"** — that's
   expected; it means the board itself is alive but hasn't found an OBD
   adapter yet.
3. Plug your Bluetooth OBD adapter into the car and turn the ignition to
   "on" (engine doesn't need to be running). Within a few seconds to a
   minute, the screen should change to show live numbers on a coloured
   background:
   - **Green** — everything normal, soot is low.
   - **Amber** — soot is building up, keep an eye on it.
   - **Red** — soot is close to the point where the car will want to
     regenerate (burn it off) on its own.
   - **A dark screen saying "ACTIVE REGENERATION"** — it's burning the soot
     off right now; the screen tells you not to switch the engine off until
     it finishes.
4. If you set up WiFi, the board also joins your home network and your
   phone can see a live dashboard. Each OS page shows how to find its
   address.

If none of that happens, each OS page has a troubleshooting section for the
most common first-time issues.

## A note on safety

This is a hobby project, not a diagnostic tool — it reads numbers the car's
engine computer already has and cannot tell you the filter is actually
blocked or safe. See [disclaimer.md](../disclaimer.md) for the full version.
Don't interact with the screen or your phone while driving.

## Not ready to build one yourself?

If all of this still sounds like too much, that's completely fine — ask
whoever gave you this link about getting a board that's already set up and
tested, ready to just plug in.
