# Applying the licence

> **Not legal advice.** Steps for the recommended option (GPL-3.0-or-later). Adjust if you pick another.
>
> **Status check (this repository, re-verified against the actual tree):** steps 1 and 4 below are **done** — a root `LICENSE` file with the unmodified GPL-3.0 text exists, and `README.md`'s "Licence" section already reads "GPL-3.0-or-later. See \[LICENSE](LICENSE)." (no longer "To be decided by the owner."). The owner has decided the rest is **deliberately deferred, not an oversight**: SPDX headers, a root NOTICE and a DCO process are housekeeping polish that can be added later without affecting anyone's rights to use the code today, so they're not a blocker for going live. `TRADEMARKS.md` (step 5) is **not applicable** — the owner isn't claiming any trademark or brand ownership over the project name or anything else, only the GPL copyright on the code itself.

## 1. LICENSE file — done

1. Download the official text of your chosen licence (GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.txt; Apache-2.0: https://www.apache.org/licenses/LICENSE-2.0.txt; AGPL-3.0: https://www.gnu.org/licenses/agpl-3.0.txt; MPL-2.0: https://www.mozilla.org/MPL/2.0/; MIT: https://opensource.org/license/mit). Do not retype or edit it.
2. Save it as `LICENSE` at the repository root, unmodified. **Done**: the root `LICENSE` file exists and its header matches the official GPL-3.0 text.
3. Fill in your name and year only where the licence template asks (MIT, Apache appendix, per-file headers). GPL-3.0's own licence text has no such blank to fill in; this applies to SPDX per-file headers instead (see step 2 below, not yet done).
4. Replace "To be decided by the owner." in `README.md` under "Licence" with a one-line statement. **Done**: the README's "Licence" section now reads:

   > GPL-3.0-or-later. See \[LICENSE](LICENSE).

   (Simpler than the suggested CC-BY-SA/TRADEMARKS wording below, since those pieces are not in place yet — see steps 4 and 5.)

## 2. SPDX headers

Add one line at the top of each source file (Python, C/C++, JS, shell). This makes licences machine-readable (REUSE tooling).

```python
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 <Your Name>
```

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 <Your Name>
```

- Docs: `<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->`
- Third-party or generated files (for example `tg_root_ca.h`): keep their own notice; do not relabel.
- Optional: run `reuse lint` (the FSFE REUSE tool) to check every file has a licence. Not installed or run here.
- Keep the copyright line factual: your name, and the year of first publication.

## 3. NOTICE (or THIRD_PARTY.md)

For GPL it is not mandatory, but list third-party components and their licences (see `NOTICE.draft`). Apache-2.0 requires that a NOTICE file shipped by a dependency be preserved when you redistribute; if you bundle NimBLE or firmware binaries, include it. Fill in only what is verified in `dependency-compatibility.md`.

Note: `boards/cyd-s3-3p5/firmware/NOTICE.md` already exists, but it is a per-board acknowledgements file for hardware facts cross-checked during that board's bring-up (pin numbers, register maps) — it is not the project-wide dependency NOTICE described above (it does not, for example, cover NimBLE-Arduino's Apache-2.0 notice for the firmware binary as a whole). A root-level `NOTICE` covering all three ESP32-S3 targets' dependencies is still not done.

## 4. Docs and images

Put `LICENSE-docs` (CC BY-SA 4.0 legal code from https://creativecommons.org/licenses/by-sa/4.0/legalcode.txt) next to `docs/`, or state in `docs/README` that docs are CC BY-SA 4.0. Confirm no image contains a plate, VIN, or third-party screenshot you do not own.

## 5. TRADEMARKS.md — not applicable

The owner is not claiming trademark or brand ownership over "DPFGuardian" or anything else about this project — only the GPL copyright on the code. Skip this step; delete `TRADEMARKS.draft.md` if it's confusing to have around.

## 6. DCO sign-off

1. Add `DCO` (text of Developer Certificate of Origin 1.1, from https://developercertificate.org/) at the repo root or under `docs/`; see `DCO.draft`.
2. Add to `docs/contributing.md`:

   > By contributing you certify the Developer Certificate of Origin. Sign off every commit: `git commit -s` adds `Signed-off-by: Your Name <you@example.com>`.

3. Enforcement: on GitHub the "DCO" GitHub App, or a small CI check; on Gitea, a pre-receive hook or a CI job that greps `Signed-off-by`. Neither is set up. Do it manually at first while contributions are rare.
4. Note the sign-off email becomes public in history. Use a noreply address if the owner does not want their real one exposed.

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
- [ ] SPDX header on each source file — deferred, not blocking. (Checked: no `SPDX` string in any `.h`/`.cpp` file in this repository.)
- [ ] Dependency table verified (`pip-licenses`, `pio pkg list`) — deferred; [dependency-compatibility.md](dependency-compatibility.md) still marks most rows **UNVERIFIED**.
- [ ] `NOTICE` completed — deferred. A per-board `boards/cyd-s3-3p5/firmware/NOTICE.md` exists but is not the project-wide dependency NOTICE this step describes; no root `NOTICE` exists.
- [ ] `DCO` file and contributing note — deferred; no `DCO` file or sign-off process exists yet. Revisit once outside contributions actually start arriving.
- [ ] Docs licence stated, images checked — deferred; `docs/` is not yet marked CC BY-SA anywhere in this repository (drafts only, in this same folder).
- [x] Secrets audit — this repository's own secrets handling is described in the README's "Secrets" section: every target's `src/config.h` is git-ignored and copied from its own tracked `src/config.example.h`. There is no separate audit document in this repository (an earlier pre-split draft referenced one that lived outside both published repos).
- [ ] Optional: lawyer review

None of the deferred items block going live: the `LICENSE` file is what actually governs anyone's rights to use, modify or redistribute the code, and that's in place and correct. The rest is housekeeping that can be added at any time without changing those rights retroactively.
