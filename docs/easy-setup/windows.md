# Windows — install the software and flash the board

New to this? Start at the [easy-setup hub page](README.md) for the shopping
list and the config walkthrough.

This page covers two things only: getting the tools installed on your
Windows computer, and getting the firmware onto the CYD 3.5" board. It is
100% point-and-click — there is no command line in this page.

Everything below has a specific place in the order. Do them in order; later
steps (like the config file) assume earlier ones (like having the project
folder) are already done.

## 1. Install Visual Studio Code

VS Code is the program you'll use for everything else on this page — editing
the config file, building the firmware, and sending it to the board.

1. Go to [code.visualstudio.com](https://code.visualstudio.com) in your
   browser.
2. Click the big **Download for Windows** button.
3. Once it's downloaded, double-click the installer file (it will be in your
   **Downloads** folder, named something like `VSCodeUserSetup-x64-....exe`).
4. Click through the installer: accept the licence agreement, then click
   **Next** on every screen without changing anything. The default options
   are fine.
5. Click **Install**, then **Finish**. VS Code should open on its own.

## 2. Install the PlatformIO extension

PlatformIO is the piece that understands how to build this project and talk
to the board. It installs as an extension inside VS Code — you don't download
it separately.

1. In VS Code, look at the row of icons down the far left edge of the
   window. Click the one that looks like four squares (the **Extensions**
   icon).
2. A search box appears at the top of the panel that opens. Type
   **PlatformIO IDE**.
3. Click on **PlatformIO IDE** in the results (it's normally the top result,
   published by "PlatformIO"), then click the blue **Install** button.
4. Wait. This downloads and sets up a fair amount of software in the
   background — it can take **several minutes** the first time, even on a
   fast connection. You'll see progress notifications in the bottom-right
   corner of the window.
5. VS Code may ask to **reload** the window partway through, or do it on its
   own. Let it. When it's done, you should see a new icon that looks like an
   alien head (the PlatformIO icon) added to that same left-hand strip of
   icons.

## 3. Get the project files

You need the `obd-esp32` project on your own computer. Which of these two you
do depends on how you were given the project:

**If you have a repository URL** (e.g. a Gitea or GitHub-style link someone
gave you):

1. In VS Code's left-hand icon strip, click the PlatformIO (alien head) icon.
2. In the PlatformIO Home tab that opens, look for a **Clone Git Project** (or
   similar wording) option, or use VS Code's own built-in Git support: press
   `Ctrl+Shift+P`, type **Git: Clone**, press Enter, then paste the URL you
   were given and press Enter again.
3. When asked, pick a folder to save it into — anywhere you'll remember, such
   as your **Documents** folder.
4. When it finishes, VS Code will likely offer to open the folder it just
   downloaded. Don't open it yet — the folder you actually need to open in
   the next step is a subfolder, not this top-level one.

**If you were given a folder or a ZIP file directly** (e.g. on a USB stick or
downloaded link):

1. If it's a ZIP file, right-click it in File Explorer and choose **Extract
   All...**, then choose a destination such as your **Documents** folder and
   click **Extract**.
2. You should end up with a folder named something like `obd-esp32`
   containing files like `platformio.ini`, a `boards` folder, and so on.

Either way, note where this `obd-esp32` folder ends up — you'll need to find
it again in the next step.

## 4. Open the correct project folder in PlatformIO

This repository holds the firmware for **several different boards**, not
just one. Each board's own PlatformIO project lives in its own subfolder, and
PlatformIO needs you to open that specific subfolder as the project — not the
repository's top-level folder — or it won't know which board you mean.

For the CYD 3.5" board, the folder you need is:

```
obd-esp32/boards/cyd-s3-3p5/firmware
```

To open it:

1. In VS Code, go to the **File** menu (top-left) and click **Open
   Folder...**.
2. Browse to wherever you put the `obd-esp32` project, then go into
   `boards`, then `cyd-s3-3p5`, then `firmware`. Select that `firmware`
   folder and click **Select Folder**.
3. VS Code will reload with that folder open. You should see a left-hand
   file list including `platformio.ini`, a `src` folder, and so on — if you
   instead see a long list of other boards' folders, you opened the wrong
   level; go back and open `firmware` specifically.
4. PlatformIO will notice the project and may spend a minute or two in the
   background preparing it the first time (downloading the specific tool
   versions this project needs). You can watch this in the bottom-right
   notifications.

A note for later: this project's `platformio.ini` deliberately pins an older
tool version (`platform = espressif32@7.1.3`) because a newer one is known to
crash this firmware's Bluetooth code. If PlatformIO or VS Code ever offers to
**update** this project's platform or toolchain, decline it — leave that line
exactly as it is.

## 5. Set up your WiFi and Telegram secrets

Now that you have the project open, do the `config.h` step before building
anything. This is already fully explained on the hub page — see [Setting up
your own secrets](README.md#setting-up-your-own-secrets-do-this-after-installing-the-software)
on the [easy-setup hub page](README.md). Come back here once `config.h` is
filled in.

## 6. Format the microSD card as FAT32

Do this before you power the board for the first time — an unformatted or
wrongly-formatted card fails silently (see the hub page's warning).

1. Put the microSD card into your computer's card slot, or into a USB card
   reader if your computer doesn't have one built in.
2. Open **File Explorer** (the folder icon in your taskbar) and find the
   card under **This PC** — it'll show up as a drive letter like `D:` or
   `E:`.
3. Right-click that drive and choose **Format...**.
4. In the **File system** dropdown, choose **FAT32**. (It may already be
   selected by default on a small card.)
5. Leave **Allocation unit size** on its default, and click **Start**. Click
   **OK** on the warning that this erases the card — that's expected for a
   new or repurposed card.
6. When it finishes, close the dialog and insert the card into the board.

**If your card is larger than 32 GB**, Windows' Format dialog will refuse to
offer FAT32 at all — this is a built-in limitation of Windows' own formatter,
not a mistake on your part. Your options are: use a 32 GB or smaller card
instead (the simplest fix), or use a third-party FAT32 formatting tool that
supports larger cards. This project doesn't need a large card, so switching
to a smaller one is the easiest route.

## 7. Connect the board over USB

1. Plug the board into your computer using a USB-C cable. Make sure it's a
   cable that actually carries data, not just power — the shopping list on
   the hub page has a note about this; a charge-only cable will power the
   screen on but your computer will never see it as a device.
2. Windows should recognise it within a few seconds. You might see a small
   notification bottom-right saying a device was set up, or nothing visible
   at all — both are normal.
3. This board has no separate USB-to-serial chip (no CP2102, no CH340/CH9102
   — if you've seen generic ESP32 guides telling you to install one of those
   drivers, that advice doesn't apply to this board). It talks to your
   computer through the ESP32-S3 chip's own built-in USB connection, which
   Windows 10 and 11 normally recognise automatically with no extra driver
   install. **This specific claim is unverified on a genuinely fresh Windows
   machine** — it's how this type of connection is supposed to behave, but it
   hasn't been personally confirmed for this exact board model start-to-finish.
4. To check it worked, open **Device Manager** (right-click the Start button,
   choose **Device Manager**) and look under **Ports (COM & LPT)** for a new
   entry like `COM3` or similar — note the number if you need it later (you
   usually won't; PlatformIO finds it automatically).

If no COM port shows up, see the troubleshooting table below before
continuing.

## 8. Build and upload the firmware

The controls you need live in the **blue toolbar at the very bottom of the
VS Code window** (not the top) once PlatformIO has finished preparing the
project.

1. Look at the bottom status bar. You should see a row of small icons,
   including a **checkmark (✓)** and a **right-pointing arrow (→)**.
2. Click the **checkmark icon** first — this is **Build**. It compiles the
   firmware without sending it to the board yet, which is a good first check
   that everything is set up correctly.
3. A panel opens at the bottom of the window (the **terminal pane**) showing
   progress text scrolling past. **The first build can take several
   minutes** — it's downloading and compiling a lot of underlying code the
   first time. Later builds are much faster.
4. When it finishes, look for a line in green text saying
   **`[SUCCESS]`**. If instead you see red text and the word **`[FAILED]`**
   or `Error`, something needs fixing before continuing — scroll up in that
   same pane to read the actual error message.
5. Once the build succeeds, click the **right-arrow icon (→)** — this is
   **Upload**. It builds again (reusing most of the previous work, so it's
   faster) and then sends the firmware to the board over USB.
6. Watch the same terminal pane. A successful upload also ends with a green
   **`[SUCCESS]`** line, after some output about connecting to the board and
   writing data to it. The board may flicker or restart during this — that's
   normal.

## 9. Open the Serial Monitor

The Serial Monitor is a window that shows you text the board itself is
printing as it runs — its own internal commentary on what it's doing. This
is how you confirm it booted properly without just guessing from the screen.

1. In the same bottom toolbar, find the icon that looks like an electrical
   **plug** — this is the Serial Monitor.
2. Click it. A new panel opens showing live text from the board.
3. Power-cycle the board (unplug and replug the USB cable, or just wait if it
   already restarted from the upload) and watch the text scroll by. You're
   looking for lines mentioning booting up, the screen jingle happening, and
   a Bluetooth/BLE scan starting — confirming the firmware is alive and
   looking for your OBD adapter.
4. You can close this panel once you've seen it boot successfully.

## 10. Troubleshooting (Windows-specific)

| Problem | Likely cause | What to try |
|---|---|---|
| No COM port appears in Device Manager | Charge-only USB cable, or the board isn't getting power | Swap to a different USB cable that you know carries data (not just power), and try a different USB port on your computer. |
| COM port appears but shows a yellow warning icon, or says "Unknown device" | Windows couldn't match it to a driver automatically **(unverified how often this happens on this board)** | Try unplugging and replugging first. If it persists, check for Windows Updates (Settings > Windows Update > Check for updates), since USB-CDC driver support sometimes ships that way. As a last resort, search the board listing/manufacturer page for any driver notes. |
| Upload fails with "Access is denied" or the port is busy | Another program has the COM port open | Close any other serial monitor, Arduino IDE window, or a second VS Code window that might also have PlatformIO's monitor open on the same port, then try Upload again. |
| Upload starts but fails partway through | Loose or poor-quality USB cable, or something interrupted the connection (closing a laptop lid, a USB hub losing power) | Try a shorter or known-good cable, plug directly into the computer rather than through a USB hub, and don't touch the cable during upload. |
| PlatformIO extension install hangs or fails | A patchy internet connection during the first-time download, or a work/school network blocking the download | Try again on a stable connection; check the notification area for a specific error message. **(unverified)** which proxies or filtered networks specifically cause this. |
| Upload fails and antivirus/Windows Defender flagged something | Some antivirus tools are known to interfere with flashing tools like esptool because they briefly write to the port in an unusual way **(unverified for this specific project)** | Temporarily pause real-time protection for the upload only, then re-enable it afterwards. Don't leave it disabled. |
| Build fails right after opening the folder, mentioning a missing platform or package | PlatformIO is still downloading the pinned toolchain in the background | Wait for the bottom-right notifications to finish, then try Build again. |

## What's next

Once the build and upload both say `[SUCCESS]` and the serial monitor showed
a clean boot, go back to the hub page's [Checking it's actually
working](README.md#checking-its-actually-working) section — it walks through
what you should see on the board's own screen once it's plugged into the car.
