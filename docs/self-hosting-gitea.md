# Self-hosting Gitea on CasaOS

Information reflects the Gitea docs as of 2026-09-23 (docs example image: 1.27.3; a web search reported 1.27.2 as latest stable, so re-check Docker Hub tags). Placeholders: `<casaos-ip>`, `<user>`, `<repo>`.

## Why Gitea (or Forgejo)
Single Go binary, tiny RAM footprint, SQLite is enough for one user, web UI like GitHub, and pull requests/issues/Actions use GitHub-like concepts, so moving to GitHub later is just another `git remote`. Forgejo (a Gitea fork) works nearly identically; this guide only covers Gitea and its commands were not tested with Forgejo.

## Ports (avoid clashes)
CasaOS web UI uses port 80. Use host ports **3300** (web) and **2222** (SSH); the container listens on 3000/22 internally. Host port 22 is the host's own sshd, so never map Gitea to 22. Change 3300/2222 in the compose file if taken.

## Install, route A: CasaOS custom app import
1. On the host: `sudo mkdir -p /DATA/AppData/gitea/data`
2. CasaOS UI: App Store -> Custom Install (the "+" / import icon) -> Docker Compose -> paste `gitea/docker-compose.yml`, replace `<casaos-ip>`, submit.
3. CasaOS may add its own labels/metadata (`x-casaos`); a plain compose file should import without them, but I could not verify the current import dialog wording.

## Install, route B: plain docker compose (over SSH to the host)
```
sudo mkdir -p /DATA/AppData/gitea/data
sudo mkdir -p /DATA/AppData/gitea && cd /DATA/AppData/gitea
# copy docker-compose.yml here, edit <casaos-ip>
sudo docker compose up -d
sudo docker compose logs -f gitea
```
Apps started this way still show in CasaOS as containers, though not with the app-store tile.

## First-run setup
1. Open `http://<casaos-ip>:3300/`. The installer page appears.
2. Database: **SQLite3** (path preset). Confirm Server Domain/SSH port/Base URL match the compose values (SSH port 2222).
3. Expand "Administrator account settings" and create the admin (strong password). Expand "Server and third party settings" and tick **Disable self-registration** (the compose file already sets `DISABLE_REGISTRATION=true`). Install.
4. Create the repository `<repo>` (empty: no README/license/.gitignore) under your user. Set it Private.
5. Optional: add your SSH public key under Settings -> SSH/GPG Keys.

## Push the local repo
Review the script `git_first_push.sh`; it refuses to run if secrets or `azure-snapshots`, `logs`, `.pio`, `venv` exist, and never pushes. Then:
```
bash git_first_push.sh /Users/vitor/Claude/OBD
git remote add origin http://<casaos-ip>:3300/<user>/<repo>.git   # HTTP
# or: git remote add origin ssh://git@<casaos-ip>:2222/<user>/<repo>.git
git push -u origin main
```
For HTTP, use a Gitea access token (Settings -> Applications) as the password; macOS keychain will remember it. Store it nowhere in the repo.

## Adding GitHub later
Second remote (you push to both):
```
git remote add github git@github.com:<gh-user>/<repo>.git
git push github main
git remote set-url --add --push origin ...   # only if you want one `git push` to hit both; skip otherwise
```
Or a Gitea push mirror: repo Settings -> Repository -> Mirror Settings -> "Push Mirrors" with a GitHub personal access token (mirrors on each commit/interval). Not tested here.

## Backups
Everything lives in `/DATA/AppData/gitea/data` (repos, SQLite DB, app.ini, attachments).
- Simple: stop, copy, start:
  `sudo docker compose stop && sudo tar czf /DATA/backups/gitea-$(date +%F).tgz -C /DATA/AppData/gitea data && sudo docker compose start`
- Official: `gitea dump` inside the container as the `git` user (docs: run with the RUN_USER; see docs.gitea.com/administration/backup-and-restore for the exact docker exec form).
- Copy backups off the box (USB/other machine); restoring means extracting to the volume, and `gitea admin regenerate hooks` if paths change. Also: your local clone plus GitHub is itself a backup.

## Updating
Edit the image tag in the compose file (read the release notes/blog for breaking changes, especially across minor versions), back up first, then `sudo docker compose pull && sudo docker compose up -d`. Stay on the rootful image (don't switch to `-rootless`).

## Optional: Gitea Actions runner
Actions are enabled in recent Gitea; a runner (`gitea/act_runner`) is a separate container using a registration token from Admin -> Actions -> Runners. It needs the Docker socket, which effectively gives it root on the host, so only do this if you need CI. Not covered or tested here; see docs.gitea.com/usage/actions.

## LAN-only and remote access
Do not port-forward 3300/2222 on the router. Keep it LAN only. To use it away from home, install **Tailscale** (or WireGuard) on the CasaOS host and your laptop, then use the host's Tailscale IP/name in the remote URL (`http://<tailscale-name>:3300/...`). If you do, you can set ROOT_URL to that name, or keep two remotes. Keep registration disabled and use a strong admin password.

## Could not verify
- Current CasaOS custom-install UI steps (wiki page 404'd; relied on search snippets), `x-casaos` details.
- Latest Docker tag: sources disagree (1.27.2 vs 1.27.3).
- Exact `gitea dump` docker command, push-mirror UI, Actions setup, Forgejo differences.
- Nothing was run against any host.
