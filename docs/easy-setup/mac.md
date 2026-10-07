# Easy setup — Mac

New to this? Start at the [easy-setup hub page](README.md) for the shopping
list and the config walkthrough.

This page covers two things only: installing the software on your Mac, and
getting the firmware onto the CYD 3.5" board. It assumes you've already read
the hub page and have the board, a data-capable USB-C cable, and (ideally) a
32 GB or smaller microSD card in hand.

Everything below is clicking and typing into windows — no Terminal, except
one optional aside near the end that you can skip entirely.

## 1. Install Visual Studio Code

VS Code is the program you'll use to write the firmware onto the board. It's
free, made by Microsoft, and has nothing to do with the actual car-reading
code — it's just the editor/toolbox.

1. Open your web browser and go to **code.visualstudio.com**.
2. Click the big **Download for macOS** button (the site detects you're on a
   Mac and offers the right file automatically).
3. A file ending in `.zip` downloads. Click it in your browser's download
   list, or open your **Downloads** folder and double-click it — macOS will
   unzip it automatically into a file called `Visual Studio Code.app`.
4. Drag **Visual Studio Code.app** onto your **Applications** folder (you can
   open Applications from Finder's sidebar, then drag the icon across).
5. Open **Applications** and double-click **Visual Studio Code**.
6. The first time you do this, macOS will show a warning like *"Visual Studio
   Code" is an app downloaded from the Internet. Are you sure you want to open
   it?* — this is normal (it's called Gatekeeper, and it shows up for almost
   any app you download outside the App Store, not just this one). Click
   **Open**.

VS Code should now launch and show an empty window with a welcome screen.

## 2. Install the PlatformIO extension

PlatformIO is the piece that actually knows how to build the firmware and
send it to the board. It installs as an extension *inside* VS Code — you
don't download it separately.

1. In VS Code, look at the strip of icons down the left edge. Click the one
   that looks like four squares (the **Extensions** icon).
2. A search box appears at the top of the panel. Type **PlatformIO IDE**.
3. The official result is called **PlatformIO IDE**, published by
   PlatformIO. Click **Install** on it.
4. Wait. The first install downloads quite a lot (compilers, tools for every
   chip PlatformIO supports) and **can take several minutes**, even on a
   good connection. Don't close VS Code while it's working.
5. When it finishes, VS Code will likely ask to **reload** the window, or
   just reload itself. Let it. After reloading, you should see a new icon in
   the left-edge strip that looks like an alien head/ant — that's the
   PlatformIO icon, and it means the install worked.

## 3. Get the project files

You need the `obd-esp32` project folder on your Mac. Depending on how you
were given access to it, use whichever of these applies:

**If you have a repository URL** (a Git address, e.g. something ending in
`.git`, from a Gitea or GitHub-style server):

1. Click the PlatformIO icon in the left strip, then look for **Open** or a
   **Clone** option in the PlatformIO home screen — PlatformIO's home tab has
   a **"Clone Git Project"** button. Click it.
2. Paste the URL you were given, choose a folder on your Mac to save it into
   (your **Documents** folder is a sensible choice), and confirm.

**If you were given a folder or a `.zip` file directly** (e.g. a USB stick,
an email attachment, a shared drive):

1. If it's a `.zip` file, double-click it — macOS extracts it automatically
   into a regular folder with the same name, in the same location.
2. Move that extracted folder somewhere easy to find, such as your
   **Documents** folder.

Either way, you should end up with a folder named something like `obd-esp32`
somewhere on your Mac, containing (among other things) a `boards` folder.

## 4. Set up your secrets

Before building anything, go do the `config.h` step now: copy
`config.example.h` to `config.h` inside
`boards/cyd-s3-3p5/firmware/src/` and fill in your WiFi (and optionally
Telegram) details. This is fully explained in the
[hub page's "Setting up your own secrets" section](README.md#setting-up-your-own-secrets-do-this-after-installing-the-software)
— do it now, then come back here.

## 5. Open the right project folder in PlatformIO

This repository holds the firmware for **several different boards**, each
in its own subfolder. You must open the specific subfolder for the CYD 3.5"
board — not the top-level `obd-esp32` folder — or PlatformIO won't know
which firmware you mean.

1. In VS Code, go to the **File** menu and choose **Open Folder…**.
2. Navigate into the project folder you got in step 3, then down into
   `boards` → `cyd-s3-3p5` → `firmware`.
3. Select the **`firmware`** folder itself (not a file inside it) and click
   **Open**.

You'll know you got it right if, after it opens, PlatformIO's icon in the
left strip shows a project, and a file called `platformio.ini` is visible at
the top level of the file list in the sidebar.

**One important warning:** that `platformio.ini` file pins an exact tool
version (`platform = espressif32@7.1.3`). This is deliberate — a newer
version is known to crash this firmware's Bluetooth code. If VS Code,
PlatformIO, or anything else ever offers to "update" the platform, say no.
Don't edit that line.

## 6. Format the microSD card

The board needs its microSD card formatted as **FAT32** — not exFAT, not
NTFS, not "Mac OS Extended". If the card isn't formatted this way, the board
won't tell you — it'll just quietly log to its own tiny internal memory
instead (room for a few minutes of driving), which defeats the point.

1. Put the microSD card into a card reader connected to your Mac.
2. Open **Disk Utility** (press **Cmd+Space**, type "Disk Utility", press
   Return).
3. In the sidebar, click on the card itself (the physical device entry, not
   a volume indented underneath it).
4. Click **Erase** in the toolbar.
5. Give it a name if you like, and for **Format**, choose **MS-DOS (FAT)**
   — this is macOS's name for FAT32.
6. Click **Erase**, then **Done**.

**If your card is larger than 32 GB**, Disk Utility's GUI will grey out or
simply not offer FAT32 as an option — this is a known macOS limitation, not
a bug in the card. The simplest fix is to use a 32 GB or smaller card
instead. If you specifically want to use a larger card, see the optional
Terminal workaround at the bottom of this page.

Once formatted, insert the card into the board's SD slot before you power it
on for the first time.

## 7. Connect the board

Plug the CYD 3.5" board into your Mac using a USB-C cable that you know
carries data (not a charge-only cable — see the shopping list on the hub
page).

What should happen: macOS may show a brief notification about a new USB
device, and the board's screen should light up. This board uses the
ESP32-S3 chip's own built-in USB connection — there's no separate
USB-to-serial adapter chip to worry about, so on a modern Mac it should just
work with no driver to install.

To check PlatformIO can see it: click the **PlatformIO** icon in the left
strip, then look under the **PIO Home** → **Devices** section (or use the
alien-head icon's quick-access menu) for a device list. You're looking for
something named like `/dev/cu.usbmodemXXXX` (a string of numbers/letters
after `usbmodem`). **I haven't personally verified this exact board model
showing up this way on a brand-new Mac (unverified)** — if it doesn't
appear, see the troubleshooting table below before assuming something is
broken.

## 8. Build and upload the firmware

At the bottom of the VS Code window, once the `firmware` folder is open as
your project, PlatformIO adds a thin blue toolbar with several small icons.
The two you need:

- A **checkmark (✓)** icon — this **builds** the firmware (compiles the code
  without sending it anywhere yet).
- A **right-arrow (→)** icon — this **builds and uploads** (sends it to the
  board over USB). You can click this one directly; it builds first
  automatically.

1. Click the **right-arrow** icon.
2. A terminal panel opens at the bottom of the window and starts printing a
   lot of text — this is normal. **The first build can take several
   minutes** (it's compiling everything from scratch).
3. Watch for the end of the output:
   - A line reading **`SUCCESS`** (in green) means it built and uploaded.
   - Red text and a non-zero exit/error means it failed — see
     troubleshooting below.

## 9. Open the Serial Monitor

The "serial monitor" is just a window that shows text the board prints over
the same USB cable while it's running — useful for confirming it booted
correctly.

1. In the same bottom toolbar, click the **plug-shaped** icon.
2. A new terminal panel opens and starts printing the board's startup
   messages.
3. You should see boot text scroll past, and the board's screen should show
   its boot jingle / light up around the same time. Soon after, it should
   start scanning for the Bluetooth OBD adapter.

Opening the monitor resets the board and starts a fresh session, so this is
a good moment to confirm things are alive before taking the board out to the
car.

## 10. Troubleshooting (macOS-specific)

| Symptom | Likely cause | Fix |
|---|---|---|
| No device/port appears in PlatformIO's device list | Charge-only USB-C cable | Swap to a cable you know transfers data (e.g. one you've used to sync a phone). |
| Still no port after swapping cables | Mac isn't seeing the hardware at all | Open **Apple menu → About This Mac → More Info → System Information**, then **USB** under Hardware. If the board doesn't appear there either, it's a hardware/cable/power problem, not software. **(unverified exact wording of this menu may vary by macOS version)** |
| Port appears but upload fails partway through | Board didn't enter its upload mode, or a loose connection | Unplug and replug the board, close any other app that might have the port open (another VS Code window, a serial terminal), and try the upload again. |
| "Permission denied" style error on the port | Uncommon on macOS compared to Linux, but **(unverified)** another process may be holding the port open | Quit other apps/terminals that might be using the port, replug the board, retry. |
| PlatformIO extension install hangs or fails | Slow/interrupted connection during the large first-time download | Check your internet connection and retry the **Install** button; if it partially installed, try **Uninstall** then **Install** again from the Extensions panel. |
| Gatekeeper blocks VS Code or something else from opening | macOS's standard "downloaded from the internet" warning | Right-click the app, choose **Open**, then confirm in the dialog that appears (this remembers your choice after the first time). |
| A dialog about an unsigned/blocked system extension appears | This board shouldn't need one — it uses the chip's native USB connection, not a separate driver | **(unverified)** This almost certainly means something unrelated is happening. Don't grant the extension in **System Settings → Privacy & Security** just to make this board work; if you're stuck here, double-check you're using the right cable and port first. |

## What's next

Once the build says `SUCCESS` and the serial monitor shows the board
booting, go back to the hub page's
[**"Checking it's actually working"** section](README.md#checking-its-actually-working)
for what you should see on the screen with the SD card in, and then with the
car's Bluetooth adapter connected.

---

### Optional, advanced: using a microSD card bigger than 32 GB

Skip this section entirely if you used a 32 GB or smaller card — it's not
needed.

macOS's Disk Utility app refuses to offer FAT32 for cards above 32 GB
through its normal window. There's a workaround, but it needs one Terminal
command. If you've never used Terminal before, it's the plain black (or
white) window app you can open from **Applications → Utilities → Terminal**,
where you type commands instead of clicking buttons.

The command below erases the card and reformats it as FAT32 regardless of
size. **This permanently deletes everything on the card** — only run it on
the card you intend to use for this project.

1. Open **Disk Utility**, click your card, and note its **identifier** at
   the bottom (something like `disk4`) — not a partition like `disk4s1`, the
   whole-disk one.
2. Open **Terminal** and type this, replacing `CARDNAME` with any name
   (letters/numbers, no spaces) and `diskN` with the identifier you noted
   (e.g. `disk4`):
   ```
   diskutil eraseDisk MS-DOS CARDNAME diskN
   ```
   This command tells macOS: "erase this entire disk and format it as
   MS-DOS/FAT32, naming it CARDNAME." It's the same operation Disk Utility's
   GUI does for smaller cards — this is just the way to force it above the
   32 GB limit.
3. Press Return, confirm the identifier is really your card (not your
   Mac's own disk — double- and triple-check this), and let it finish.

If any of that is unclear, it's genuinely simpler and safer to just use a
32 GB or smaller card instead.
