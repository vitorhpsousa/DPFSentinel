# Applying the licence

> **Not legal advice.** Steps for the recommended option (GPL-3.0-or-later). Adjust if you pick another.
>
> **Status check (this repository, re-verified against the actual tree):** steps 1, 2, 3 and 6 below are all **done**. Only step 4 (CC BY-SA for docs/images) remains genuinely deferred, as non-blocking housekeeping. `TRADEMARKS.md` (step 5) is **not applicable** — the owner isn't claiming any trademark or brand ownership over the project name or anything else, only the GPL copyright on the code itself.

## 1. LICENSE file — done

1. Download the official text of your chosen licence (GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.txt; Apache-2.0: https://www.apache.org/licenses/LICENSE-2.0.txt; AGPL-3.0: https://www.gnu.org/licenses/agpl-3.0.txt; MPL-2.0: https://www.mozilla.org/MPL/2.0/; MIT: https://opensource.org/license/mit). Do not retype or edit it.
2. Save it as `LICENSE` at the repository root, unmodified. **Done**: the root `LICENSE` file exists and its header matches the official GPL-3.0 text.
3. Fill in your name and year only where the licence template asks (MIT, Apache appendix, per-file headers). GPL-3.0's own licence text has no such blank to fill in; this applies to SPDX per-file headers instead (see step 2 below, not yet done).
4. Replace "To be decided by the owner." in `README.md` under "Licence" with a one-line statement. **Done**: the README's "Licence" section now reads:

   > GPL-3.0-or-later. See \[LICENSE](LICENSE).

   (Simpler than the suggested CC-BY-SA/TRADEMARKS wording below, since those pieces are not in place yet — see steps 4 and 5.)

## 2. SPDX headers — done

Add one line at the top of each source file (Python, C/C++, JS, shell). This makes licences machine-readable (REUSE tooling).

```python
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 the DPF Sentinel project
```

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
```

**Done**: every `.cpp`/`.h`/`.py` source file across all three firmware targets and `tools/` now carries this header (99 files). The copyright line names the project rather than a personal name, since the owner is the sole copyright holder and preferred not to put a personal name in every file.

- Docs: `<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->` — not yet added; tracked under step 4 below, since docs aren't CC BY-SA licensed yet either.
- Third-party or generated files (for example `tg_root_ca.h`, the embedded Telegram root CA certificate): deliberately **excluded** — it's public certificate data, not this project's own expression, so it keeps no header at all rather than being mislabelled.
- Optional: run `reuse lint` (the FSFE REUSE tool) to check every file has a licence. Not installed or run here.

## 3. NOTICE (or THIRD_PARTY.md) — done

For GPL it is not mandatory, but list third-party components and their licences. Apache-2.0 requires that a NOTICE file shipped by a dependency be preserved when you redistribute; if you bundle NimBLE or firmware binaries, include it.

**Done**: a root-level [`NOTICE.md`](../../NOTICE.md) now lists the four compiled libraries (NimBLE-Arduino, Adafruit GFX Library, GFX Library for Arduino, the Arduino-ESP32 core) with their licences — two verified directly against the installed package's own licence file (NimBLE-Arduino, Adafruit GFX), two cited from the library's upstream repository since the installed copy ships no licence file of its own (GFX Library for Arduino, the Arduino-ESP32 core). `boards/cyd-s3-3p5/firmware/NOTICE.md` and `boards/waveshare-s3-touch-lcd-2/firmware/NOTICE.md` remain separate, per-board files for hardware bring-up facts (pin numbers, register maps) — a different kind of credit, cross-linked from the new root file rather than merged into it.

## 4. Docs and images

Put `LICENSE-docs` (CC BY-SA 4.0 legal code from https://creativecommons.org/licenses/by-sa/4.0/legalcode.txt) next to `docs/`, or state in `docs/README` that docs are CC BY-SA 4.0. Confirm no image contains a plate, VIN, or third-party screenshot you do not own.

## 5. TRADEMARKS.md — not applicable

The owner is not claiming trademark or brand ownership over "DPF Sentinel" or anything else about this project — only the GPL copyright on the code. Skip this step; delete `TRADEMARKS.draft.md` if it's confusing to have around.

## 6. DCO sign-off — done

1. Add `DCO` (text of Developer Certificate of Origin 1.1, from https://developercertificate.org/) at the repo root or under `docs/`. **Done**: the official text is at the repo root as [`DCO`](../../DCO), copied verbatim.
2. Add to `docs/contributing.md`. **Done**: see its "Sign off your commits" section, with the `git commit -s` instruction.
3. Enforcement: on GitHub the "DCO" GitHub App, or a small CI check; on Gitea, a pre-receive hook or a CI job that greps `Signed-off-by`. **Still not set up** — reviewed by hand for now, per `docs/contributing.md`. Set this up once outside contributions actually start arriving, not before.
4. Note the sign-off email becomes public in history. Use a noreply address if the owner does not want their real one exposed. (Relevant the day someone else signs off a commit — the project owner's own commits aren't currently signed off, since contributions are solo so far.)

## 7. Hardware and other assets

- Any hardware design files go in `hardware/` with their own `LICENSE` (CERN-OHL-S-2.0 if you chose GPL).
- Logo: own file with its licence note or "all rights reserved".
- Sample logs, if ever added: check for odometer, timestamps, VIN, and licence them separately (CC0 is common for data; but the facts about your own car are your own privacy choice).

## 8. Changing your mind later

- While you are the only copyright holder you may relicense.
- Once outside contributions arrive, keep a record of contributors. With DCO only, relicensing needs each contributor's agreement (or removing their code). With a CLA, the CLA text governs.
- Already-published versions stay under the licence they were published with; you cannot revoke it.

## 9. Checklist

Re-checked against this repository's actual state (not just the original plan):

- [x] Pick licence: GPL-3.0-or-later.
- [x] `LICENSE` (unmodified official text) at root — present, GPL-3.0 text confirmed.
- [x] README "Licence" section updated — reads "GPL-3.0-or-later. See \[LICENSE](LICENSE)." (no longer "To be decided").
- [x] `TRADEMARKS.md` — **not applicable**, no trademark claim is being made; `TRADEMARKS.draft.md` can be deleted.
- [x] SPDX header on each source file — all 99 `.cpp`/`.h`/`.py` files across all three firmware targets and `tools/` now carry one (`tg_root_ca.h`, third-party certificate data, deliberately excluded).
- [ ] Dependency table verified (`pip-licenses`, `pio pkg list`) — deferred; [dependency-compatibility.md](dependency-compatibility.md) still marks most rows **UNVERIFIED**, though [NOTICE.md](../../NOTICE.md) now verifies the two biggest ones (NimBLE-Arduino, Adafruit GFX) directly against their installed licence files.
- [x] `NOTICE` completed — root [`NOTICE.md`](../../NOTICE.md) now covers all four compiled libraries. The per-board `NOTICE.md` files remain separate (hardware bring-up credits, a different kind of citation).
- [x] `DCO` file and contributing note — [`DCO`](../../DCO) at the repo root (official text) and the "Sign off your commits" section in `docs/contributing.md`. Enforcement (a GitHub App or CI check) is still not set up — reviewed by hand while contributions are solo.
- [ ] Docs licence stated, images checked — deferred; `docs/` is not yet marked CC BY-SA anywhere in this repository (drafts only, in this same folder).
- [x] Secrets audit — this repository's own secrets handling is described in the README's "Secrets" section: every target's `src/config.h` is git-ignored and copied from its own tracked `src/config.example.h`. There is no separate audit document in this repository (an earlier pre-split draft referenced one that lived outside both published repos).
- [ ] Optional: lawyer review

None of the deferred items block going live: the `LICENSE` file is what actually governs anyone's rights to use, modify or redistribute the code, and that's in place and correct. The rest is housekeeping that can be added at any time without changing those rights retroactively.
