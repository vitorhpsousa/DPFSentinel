# Licensing options

> **This is not legal advice.** It is a plain-language comparison written by an AI assistant from general knowledge and from reading this repository. Licences are legal instruments and rules differ by country. Before you publish, read the licence texts yourself and, if money or a company is involved, ask a lawyer (or your national open-source body, or the SFC / FSF / OSI / Open Source Initiative resources). Nothing here has been checked by a lawyer.

## 1. What you are deciding

The owner's goal, as stated: **stay open source, but protect the project.** "Protect" can mean several different things, and no licence does all of them:

| Worry | Which tool addresses it |
|---|---|
| Someone takes the code, closes it, sells it | Copyleft (GPL / AGPL / MPL for files) |
| Someone runs it as a hosted service and never shares changes | AGPL only (or a commercial dual licence) |
| A contributor or user sues you over a patent | Patent clause (Apache-2.0, GPL-3.0, AGPL-3.0, MPL-2.0 all have one; MIT does not say) |
| Someone uses your name/logo for a look-alike or misleading product | Trademark policy, not a copyright licence |
| Someone claims the code is theirs / removes your credit | Any licence requires keeping the copyright notice; Apache adds NOTICE |
| Someone builds and sells hardware from your design | Hardware licence (CERN-OHL), not a software licence |
| Someone uses the logs for something bad | No licence can control this; the disclaimer and a safety warning are what you have |
| Nobody can be held liable if it damages a car | All of these disclaim warranty and liability; see `docs/disclaimer.md` (a disclaimer is not a guarantee against a claim) |

A licence never stops a person from *using* the code privately. Open source, by definition, cannot restrict who uses it or for what field.

## 2. The five candidates

### MIT
- Very short. Anyone may use, modify, sell, relicense under any terms (including closed source) as long as the copyright notice and licence text are kept.
- Protects: attribution and the no-warranty clause. That is all.
- Does not protect: no copyleft (a company can take it closed), no explicit patent grant (courts in some places would infer one; it is not spelled out), no SaaS clause, no trademark grant statement.
- Compatible with nearly everything and is the easiest to adopt.

### Apache-2.0
- Permissive like MIT, but adds: an **explicit patent grant** from every contributor, and a **patent retaliation** clause (if you sue claiming the software infringes a patent, you lose your patent licence); a `NOTICE` file mechanism for attributions; a clause that contributions submitted to the project are under the same licence (Section 5); and an explicit statement that it grants **no trademark rights** (Section 6).
- Protects: attribution, patents, clarity of contributions, name (via Section 6, only partly).
- Does not protect: no copyleft, so a closed-source fork is allowed; no SaaS clause.
- One-way compatible with GPL-3.0 (Apache-2.0 code can go into a GPL-3.0 project; not GPL-2.0-only).

### GPL-3.0 (or "GPL-3.0-or-later")
- **Strong copyleft.** If you distribute a program that is a derivative of GPL code (source or binary, including firmware flashed into a device you sell), you must offer the complete corresponding source under the GPL and cannot add extra restrictions. Section 6 also covers "installation information" for consumer products (anti-tivoisation).
- Includes an explicit patent licence from contributors and a patent-retaliation effect.
- Protects: against closed-source forks of anything **distributed**. Someone shipping a product with your firmware must release their changes.
- Does not protect: **the hosted/SaaS loophole**. If someone modifies the Pi dashboard and runs it on their own server for users over the network, they are not "distributing", so they need not share. (For this project, which is a local logger and a device, this loophole matters less than for a web service.) Also does not prevent someone selling the software or hardware, as long as they comply.
- Cannot be combined into closed products, which some companies avoid. That is partly the point.
- Compatibility: GPL-3.0 can include MIT, BSD, Apache-2.0, LGPL code. Cannot include GPL-2.0-only code.

### AGPL-3.0
- GPL-3.0 plus Section 13: if users interact with a **modified** version over a network, you must offer them the source. Closes the SaaS loophole.
- Protects: everything GPL does, plus hosted use.
- Costs: many companies ban AGPL outright, which shrinks the pool of contributors and adopters. For a tool that runs on your own Pi/ESP32 it adds little, since nobody hosts it as a service. It would matter if a future cloud dashboard were added.
- Section 13 obligations apply to the *dashboard code served to users*; for this project that is `webui/` (`obd-pi` repo) and the ESP32 web page (`obd-esp32` repo), which are the network-facing parts.

### MPL-2.0
- **Weak, file-level copyleft.** Changes to MPL-covered *files* must be shared when distributed; you can combine those files with closed code in a larger work.
- Includes a patent grant. Has an explicit "Secondary Licences" compatibility mechanism so MPL code can be combined with GPL code.
- Protects: the files you wrote, not the whole product built around them. A middle path.
- Does not protect against a company using the code in a closed product as long as they publish their modifications to the MPL files themselves. No SaaS clause.

## 3. Comparison at a glance

| | MIT | Apache-2.0 | MPL-2.0 | GPL-3.0 | AGPL-3.0 |
|---|---|---|---|---|---|
| Closed-source fork allowed | Yes | Yes | Only outside the MPL files | No (if distributed) | No (if distributed or network-used) |
| Explicit patent grant | No | Yes | Yes | Yes | Yes |
| SaaS / hosted loophole | Open | Open | Open | Open | Closed |
| Attribution required | Yes | Yes (+ NOTICE) | Yes | Yes | Yes |
| Trademark statement | No | Yes (none granted) | Yes (none granted) | Sec. 7(e) allows refusing | Same |
| Company acceptance | Highest | High | Medium | Medium | Low |
| Simplicity | Highest | Medium | Medium | Lower | Lower |
| Firmware on a sold device must ship source | No | No | Modified MPL files only | Yes | Yes |

## 4. Patents

Cars are covered by patents held by manufacturers and suppliers. This project reads data the car already reports and decodes it; whether that touches any patent, anywhere, is unknown and outside what this document can determine. What the licence does: Apache-2.0, GPL-3.0, AGPL-3.0 and MPL-2.0 each state that contributors grant users a licence to *their own* patents in their contribution. That prevents a contributor from later suing users of the project. It does not protect against a patent held by a third party. MIT is silent.

## 5. Dual licensing

The owner can offer the same code under two licences (for example AGPL/GPL for everyone, and a paid commercial licence for companies that cannot accept copyleft). Requirements:

- The owner must hold the **rights to all the code**. Outside contributions must therefore be assigned or licensed to you with the right to relicense. A DCO alone does **not** give that (see below).
- Practical for a hobby project only if you actually expect commercial interest. It can be added later **only if** you kept sole copyright, or collected a CLA. If you take contributions under a plain GPL with only a DCO now, later dual licensing needs each contributor's consent.

## 6. Contributor agreements: CLA versus DCO

- **DCO (Developer Certificate of Origin)**: each commit carries `Signed-off-by: Name <email>` (`git commit -s`), certifying the contributor wrote it or has the right to submit it under the project licence. Lightweight, no paperwork, used by the Linux kernel. Contributors keep their copyright; the project cannot relicense without asking them.
- **CLA (Contributor Licence Agreement)**: a signed agreement giving the project a licence (or ownership) broad enough to relicense. Needed for dual licensing or a later licence change. Adds friction and needs a process (a bot or manual records). Some contributors refuse.
- **Recommendation for a one-person hobby project**: DCO. Take a CLA only if you decide on dual licensing.

## 7. Trademark: the name and the logo

Copyright licences do not cover names. If you want to control who can call something "<ProjectName>":

- Pick a distinctive name and check it is not already used (search company registers, GitHub, package indexes and the UK/EU trademark databases). The name "OBD DPF Logger" is descriptive and would be hard to protect; a coined name is easier.
- Say in the repository (a `TRADEMARKS.md`, draft in this folder) that the licence covers code only, and set out what forks may do (state it is a fork, do not imply endorsement).
- Registration is optional and costs money and time; unregistered use still gives some rights in some countries, weaker. Ask a professional before relying on it.
- The logo should have its own licence statement (for example "all rights reserved" or CC-BY-ND) separate from the code licence.

## 8. Hardware: CERN-OHL

If you publish schematics, PCB files, enclosure models or a wiring design (the ESP32-C3 round-screen board plan, for instance), a software licence is the wrong tool. The CERN Open Hardware Licence v2 comes in three flavours:

- **CERN-OHL-P**: permissive (like MIT/Apache).
- **CERN-OHL-W**: weakly reciprocal (like MPL).
- **CERN-OHL-S**: strongly reciprocal (like GPL): anyone who distributes products or modified designs must provide the source of the design.

If you choose GPL for the code, **CERN-OHL-S** is the matching choice. Currently the repository contains no hardware design files (only descriptions of off-the-shelf boards), so this applies only if you add some. Alternatively use TAPR OHL or Solderpad. Check what applies to the third-party boards' own documentation: their vendors' pinouts and drawings are not yours to relicense.

## 9. Documentation and images: CC-BY-SA

Words and pictures are not code. Common practice: **CC BY-SA 4.0** for the docs (share-alike, attribution). Beware:

- CC licences are not recommended for software, which is why code and docs are separated.
- CC BY-SA 4.0 is one-way compatible with GPL-3.0 (you can adapt CC BY-SA material into a GPL-3.0 work, per the CC compatibility statement), not the other way round. Confirm on the Creative Commons compatibility page before relying on it.
- Screenshots in `docs/img/` are the owner's own renders; check no plate, VIN or location appears (see `docs/contributing.md`, Privacy). Do not license third-party screenshots (Car Scanner's) you do not own.
- If you want docs to be copyable into anything, use CC0 or the same code licence instead.

## 10. Recommendation

**Code: GPL-3.0-or-later.** Add **DCO** sign-off for contributions, a **TRADEMARKS.md** for the name and logo, **CC BY-SA 4.0** for `docs/` and `docs/img/`, and **CERN-OHL-S-2.0** for any hardware files if they are ever added.

Reasoning:

1. **"Open but protected" points at copyleft.** The main realistic risk is someone selling a locked-down dongle or app built on this code. GPL-3.0 forces them to publish their changes when they ship. MIT/Apache do not.
2. **The project is mostly distributed as source or as a device.** Firmware flashed to an ESP32, and a Pi image, are exactly the cases where GPL's "distribute" trigger applies. The SaaS loophole (AGPL's target) needs a hosted service; this project has none. If the owner later adds a hosted cloud dashboard, switch the dashboard directory to AGPL-3.0 or relicense while still sole copyright holder.
3. **All the verified dependencies are compatible** with GPL-3.0 (see [dependency-compatibility.md](dependency-compatibility.md)). NimBLE-Arduino is Apache-2.0 (verified from its shipped LICENSE file), which is fine inside a GPL-3.0 whole. The important thing is to use "**-3.0**" not "-2.0-only".
4. **Patent clause** is present (GPL-3.0 Sec. 11), which is a better base than MIT for a project touching a regulated industry.
5. **Sole-copyright now = options later.** As long as the owner is the only copyright holder, the licence can change or a commercial licence can be added. Once outside patches arrive under DCO only, it cannot without asking each contributor.

**When to choose something else:**

- If **wide adoption matters more than protection** (car-tuning shops, other open projects embedding your decoders, easier for companies): choose **Apache-2.0**. It is the best permissive choice thanks to the patent and NOTICE features. Do not choose MIT for a project with this patent exposure unless you want the shortest text.
- If a **hosted service** is planned: **AGPL-3.0**.
- If you want a **middle path** that protects your files while letting others combine them with closed apps: **MPL-2.0**.
- If you might sell commercial licences: keep sole copyright (or a CLA) and dual-license, either GPL/AGPL plus commercial.

**A caveat about GPL and PID data.** The list of PIDs, request bytes and scale factors are facts about a car, and facts are generally not protected by copyright in many jurisdictions (the *expression*, such as comments and structure, is). This is a general statement, not a legal opinion; jurisdictions differ (the EU has a database right). The PID table is the part other people are most likely to copy, so the licence protects it weakly whichever you choose. Keeping a **verified, documented** table is the real advantage, not the licence.

## 11. Dependencies

See [dependency-compatibility.md](dependency-compatibility.md) for the table and its verification status.

## 12. How to apply the choice

See [applying.md](applying.md): LICENSE file, SPDX headers, NOTICE, TRADEMARKS.md, DCO, and where the draft files go.

## 13. Draft files in this folder

- `LICENSE.draft-README` explains the placeholder approach used for the text of GPL-3.0 (the full text is not reproduced; fetch the official one).
- `LICENSE.draft-docs-CC-BY-SA` pointer and notice text.
- `NOTICE.draft`
- `TRADEMARKS.draft.md`
- `DCO.draft`

None of these is at the repository root; they are ready to copy once the owner decides.
