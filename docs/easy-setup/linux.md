# Easy setup — Linux

New to this? Start at the [easy-setup hub page](README.md) for the shopping
list and the `config.h` walkthrough.

This page covers only two things: installing the software on your Linux
computer, and flashing the CYD 3.5" board. Everything else — what to buy,
what to put in `config.h`, and how to tell the finished board is working —
is on the hub page above.

**A note on distros.** The exact clicks below (software store names, app
icons, where "Activities" is) are for **Ubuntu with the default GNOME
desktop**, since that's the most common beginner distro. If you're on
something else (Fedora, Mint, Pop!_OS, Arch-based, KDE instead of GNOME,
etc.), the same steps exist but the app store and menus look different —
we'll call out the one place that's genuinely different by distro family
(the group name in the permissions step).

Almost everything here is point-and-click. There is exactly one place where
you have to type a command into a terminal (a program where you type
commands instead of clicking things) — step 8, for serial port permissions.
We explain exactly what it does before you type it, and you'll only ever
need to do it once.

## 1. Install Visual Studio Code

VS Code is the program you'll use to edit files and, with an add-on, build
and flash the firmware.

**On Ubuntu (or another GNOME-based distro):** open your software store app
— on Ubuntu this is **Ubuntu Software** (a shopping-bag icon in the
dock/Activities), on other GNOME distros it may just be called **Software**.
Search for **"Visual Studio Code"**, pick the one published by Microsoft,
and click **Install**. Wait for it to finish, then you can open it from
Activities the same way you'd open any other app.

**If your distro doesn't have VS Code in its software store** (or you'd
rather not use it), go to
[code.visualstudio.com](https://code.visualstudio.com) in your browser and
download the `.deb` package (Ubuntu, Mint, Pop!_OS and other Debian-based
distros) or the `.rpm` package (Fedora and other Red Hat-based distros).
Double-clicking the downloaded file normally opens your distro's package
installer with an **Install** button — click it.

Exact steps vary by distro and desktop environment; the above describes
Ubuntu specifically.

## 2. Install the PlatformIO IDE extension

PlatformIO is the add-on that teaches VS Code how to build and upload this
project's firmware.

1. Open VS Code.
2. Click the **Extensions** icon in the left-hand sidebar (it looks like
   four small squares, one pulled away from the others).
3. In the search box at the top, type **PlatformIO IDE**.
4. Click **Install** on the result published by PlatformIO.
5. Wait. The first install can take several minutes — it's downloading
   toolchains in the background, not just the extension itself. VS Code may
   ask to **reload** partway through; let it.

When it's done you'll see a new alien-head icon in the left sidebar — that's
the PlatformIO home screen.

## 3. Get the project files

You need the `obd-esp32` project folder on your computer. Depending on how
you were given access to it, do one of these:

**If you have a repository URL** (e.g. a link to a Gitea or GitHub-style
address): in VS Code, open the **PlatformIO** home screen (the alien-head
icon), and look for a **Clone** / **Open Project** option — or use VS Code's
own **Source Control** icon in the sidebar, which has a **Clone
Repository** button. Paste the URL in, and pick a folder to save it into
(your home folder or **Documents** is fine).

**If you were given a folder or a ZIP file directly** (e.g. on a USB stick
or downloaded link): if it's a ZIP, right-click it in your file manager
(Files/Nautilus on Ubuntu) and choose **Extract Here** (or **Extract to...**
and pick a destination). Put the extracted folder somewhere easy to find
again, like your home folder or **Documents**. If it's already a plain
folder, just copy it there the normal way.

Either way, make a note of where the `obd-esp32` folder ended up — you'll
need to point VS Code at it in the next step.

## 4. Open the right project folder in PlatformIO

This repo holds firmware for **several different boards**, each in its own
subfolder. You must open the one for the CYD 3.5" board specifically, not
the repo's top-level folder — opening the wrong folder means PlatformIO
won't find the right `platformio.ini` and the build step later will fail or
build the wrong thing.

1. In VS Code, go to **File > Open Folder…**
2. Navigate into the `obd-esp32` folder you got in step 3, then into
   `boards/cyd-s3-3p5/firmware`.
3. Select that `firmware` folder (not any folder above or below it) and
   confirm.

VS Code will reload with that folder open, and after a moment PlatformIO
should recognise it as a PlatformIO project (you'll see PlatformIO-specific
icons appear in the bottom status bar).

## 5. Set up your WiFi and Telegram secrets

Before building, do the `config.h` step now. This is already fully covered
on the hub page — see [Setting up your own secrets](README.md#setting-up-your-own-secrets-do-this-after-installing-the-software)
— so we won't repeat it here. Come back to this page once that file exists.

## 6. Format the microSD card as FAT32

The board needs its microSD card formatted as **FAT32** (not exFAT, not
NTFS) — see the hub page's [FAT32 warning](README.md#formatting-the-microsd-card)
for why this matters (the board silently falls back to its own tiny internal
memory if the card isn't right, with no warning on screen).

On Ubuntu/GNOME, the built-in **Disks** app does this without a terminal:

1. Put the microSD card in your computer (in a card slot, or a USB card
   reader if your computer doesn't have a slot).
2. Open **Disks** from Activities (search "Disks" — it's pre-installed on
   Ubuntu; the icon looks like a hard drive).
3. In the list on the left, select your SD card — be careful to pick the
   card itself, not your computer's own internal disk. Check the size shown
   matches your card.
4. Click the **gear/menu icon** below the volume diagram and choose
   **Format Partition…** (or **Format Disk…** if you want to wipe any
   existing partitions first).
5. In the dialog, set the type to **FAT32** (sometimes labelled "Compatible
   with all systems and devices (FAT)"), give it any name you like, and
   confirm. This will erase anything currently on the card.

Unlike Windows' and macOS's built-in formatting tools, Disks does **not**
refuse to make a FAT32 partition on cards bigger than 32 GB, so this works
regardless of your card's size.

If your distro doesn't have Disks installed, **GParted** is a similar
point-and-click alternative — install it from your software store and use
its **Format to > fat32** option on the card's partition.

Once formatted, insert the card into the board before you power it on for
the first time.

## 7. Fix serial port permissions (one-time step)

This is the one unavoidable terminal step on Linux, and you'll only ever
need to do it once on this computer.

**Why this is needed:** the CYD board uses the ESP32-S3 chip's own built-in
USB connection — there's no separate USB-to-serial chip on this board. When
you plug it in, Linux should show it as a device named something like
`/dev/ttyACM0`. By default, though, your normal user account isn't allowed
to read or write that device — only the system administrator (root) and
members of a specific group are. If you skip this step, PlatformIO will
fail partway through uploading with a "permission denied" style error, even
though everything else is set up correctly. This is a permissions problem,
not a missing driver — nothing extra needs installing for this board.

The fix is to add your user account to the group that's allowed to use
serial devices, then make that change take effect.

1. **Open a terminal.** On Ubuntu, click **Activities** (top-left corner, or
   the Super/Windows key), type **Terminal**, and open the result — it's an
   app called **Terminal** with a dark window and a blinking cursor, already
   installed on Ubuntu.
2. Type this line exactly and press **Enter**:

   ```bash
   sudo usermod -a -G dialout $USER
   ```

   In plain English: this adds your user account to the `dialout` group,
   which is the group Linux uses to control who can open serial devices like
   `/dev/ttyACM0`. `sudo` means "do this as the administrator" — it will ask
   for your login password (the one you use to log into Ubuntu), which
   won't appear on screen as you type it; that's normal, just type it and
   press Enter.

   **If you're on an Arch-based distro** (Arch, Manjaro, EndeavourOS), use
   `uucp` instead of `dialout`:
   ```bash
   sudo usermod -a -G uucp $USER
   ```

3. **Log out and log back in, or restart your computer.** The group change
   doesn't take effect for your current login session — it only applies
   from your next login onward. This is the step people most often skip and
   then wonder why uploading still fails.

That's it — you won't need to touch a terminal again for the rest of this
guide, or for any future rebuilds.

## 8. Connect the board

Plug the CYD board into your computer using a USB-C cable that actually
carries data (see the warning on the hub page's shopping list — some cheap
cables only charge). Use the port on the board itself, not on any adapter
accessory it came with.

## 9. Build and upload the firmware

Back in VS Code, with the `boards/cyd-s3-3p5/firmware` folder open:

1. Look at the blue status bar along the **bottom** of the VS Code window —
   PlatformIO adds its own icons there.
2. Click the **checkmark icon** (✓) — this is **Build**. It compiles the
   firmware without uploading anything yet. A panel opens at the bottom
   showing progress.
3. Once Build finishes with no red errors, click the **right-arrow icon**
   (→) next to it — this is **Upload**. It builds again if needed and then
   sends the firmware to the board over USB.

**How to tell it worked:** the bottom panel ends with a green line saying
something like `======= [SUCCESS] =======`.

**How to tell it failed:** red text in that same panel. If the message
mentions "Permission denied" on a port like `/dev/ttyACM0`, that's almost
always step 7 — either you haven't logged out/in since running the command,
or the command didn't run. Log out and back in and try Upload again.

The **first** build is the slow one — it has to compile everything from
scratch and can take several minutes. Later builds, after small code
changes, are much faster.

Do not let PlatformIO or VS Code "update" anything related to the
`espressif32` platform version for this project if it offers to — this
project deliberately pins `platform = espressif32@7.1.3` in
`platformio.ini` because newer versions crash the Bluetooth library this
firmware uses. If you're ever asked to update the platform, say no.

## 10. Watch it boot with the Serial Monitor

Still in the same bottom toolbar, click the **plug icon** — this opens the
**Serial Monitor**, a live text view of whatever the board is printing as it
runs.

Opening it resets the board, so you should see it boot from the top: lines
about the screen initialising, the SD card mounting, and it starting to
scan for your Bluetooth OBD adapter. You should also hear the board's short
boot jingle and see its screen light up at the same time. This confirms the
firmware is actually running, separately from what's shown on the board's
own screen.

## Troubleshooting (Linux-specific)

| Problem | What to try |
|---|---|
| "Permission denied" on the port during Upload | You're missing step 7, or you did it but haven't logged out and back in yet (or restarted) since. Do that, then try Upload again. |
| No port appears at all / PlatformIO can't find the board | Try a different USB cable (many are charge-only, not data). Try a different USB port. Make sure the board is actually powered — screen should light up. **(unverified)** Some distros run background services called ModemManager or brltty that are sometimes reported to interfere with serial devices like this one; this is general community knowledge, not something confirmed on this specific project, so try the cable/port checks first. |
| Upload starts but fails partway through | Try again — sometimes a stale connection from a previous attempt causes this. If it keeps happening, unplug and replug the board and retry. |
| PlatformIO IDE extension install hangs or fails in VS Code | Check your internet connection — the first install downloads toolchains, which can be large. Close and reopen VS Code and try again; if it's still stuck, a full restart of your computer and reinstalling the extension has fixed similar extension issues in other projects **(unverified for this specific extension)**. |

## What's next

Once Upload succeeds and the Serial Monitor shows the board alive, go back
to the hub page's [Checking it's actually working](README.md#checking-its-actually-working)
section — it walks through what the screen should show once you plug the
board into your car.
