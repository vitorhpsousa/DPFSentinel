# Applying the licence

> **Not legal advice.** Steps for the recommended option (GPL-3.0-or-later). Adjust if you pick another.

## 1. LICENSE file

1. Download the official text of your chosen licence (GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.txt; Apache-2.0: https://www.apache.org/licenses/LICENSE-2.0.txt; AGPL-3.0: https://www.gnu.org/licenses/agpl-3.0.txt; MPL-2.0: https://www.mozilla.org/MPL/2.0/; MIT: https://opensource.org/license/mit). Do not retype or edit it.
2. Save it as `LICENSE` at the repository root, unmodified.
3. Fill in your name and year only where the licence template asks (MIT, Apache appendix, per-file headers).
4. Replace "To be decided by the owner." in `README.md` under "Licence" with a one-line statement, for example:

   > Code: GPL-3.0-or-later (see `LICENSE`). Documentation and images: CC BY-SA 4.0. The project name and logo are not licensed; see `TRADEMARKS.md`.

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

## 4. Docs and images

Put `LICENSE-docs` (CC BY-SA 4.0 legal code from https://creativecommons.org/licenses/by-sa/4.0/legalcode.txt) next to `docs/`, or state in `docs/README` that docs are CC BY-SA 4.0. Confirm no image contains a plate, VIN, or third-party screenshot you do not own.

## 5. TRADEMARKS.md

Copy `TRADEMARKS.draft.md`, fill in the name once chosen. Consider a name search before publishing (company registers, GitHub, app stores). Do not claim "registered" unless it is (use "TM" for unregistered).

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

- [ ] Pick licence; decide DCO vs CLA
- [ ] `LICENSE` (unmodified official text) at root
- [ ] README "Licence" section updated
- [ ] SPDX header on each source file
- [ ] Dependency table verified (`pip-licenses`, `pio pkg list`)
- [ ] `NOTICE` completed
- [ ] `TRADEMARKS.md` with an actual name
- [ ] `DCO` file and contributing note
- [ ] Docs licence stated, images checked
- [ ] Secrets audit done (`_repo_prep/secrets_audit.md`)
- [ ] Optional: lawyer review
