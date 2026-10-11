# NeTorrent roadmap

NeTorrent is a Windows BitTorrent client forked from
[PicoTorrent](https://github.com/picotorrent/picotorrent) (last upstream release
**v0.25.0, May 2021**). The goal is a maintained, modernized clone built on the
still-healthy [`arvidn/libtorrent`](https://github.com/arvidn/libtorrent) engine.

## Status

| Area | State |
|------|-------|
| Upstream imported (fork of `picotorrent` @ `24e3e86`) | done, kept as `upstream` remote |
| Rebranded to NeTorrent | done |
| Build toolchain (VS 2022 + CMake + vcpkg) | via `scripts/build.ps1` |
| First green build | in progress |
| MVP features | not started |

The fork keeps upstream history so we can diff and cherry-pick. `upstream` points
at `picotorrent/picotorrent`; `origin` is where the NeTorrent repo will live.

## Modernization backlog

**Toolchain / build**
- [ ] Confirm a green `scripts/build.ps1` build of the rebranded tree.
- [ ] Bump `CMAKE_CXX_STANDARD` from 17 to 20 once the baseline is green.
- [ ] Pin a vcpkg baseline in `vcpkg.json` for reproducible dependency resolution.
- [ ] Evaluate moving `vendor/wx` and `vendor/vcpkg` to vcpkg-provided packages
      (removes two heavy submodules; keeps CMake importing `wxWidgets::*`).
- [ ] Drop the ANTLR/PQL submodule from the default build until PQL is needed.

**Dependencies**
- [ ] libtorrent: track 2.1.x (currently whatever vcpkg resolves; upstream targeted 2.0).
- [ ] wxWidgets: track 3.2.x.
- [ ] OpenSSL: 3.x (upstream docs still reference 1.1.1).
- [ ] Revisit Crashpad/Sentry wiring (upstream endpoint is PicoTorrent's).

**Correctness / housekeeping**
- [ ] Replace upstream service endpoints (`api.picotorrent.org` update-check and
      GeoIP mirror, Sentry DSN) or disable them by default.
- [ ] New NeTorrent icon set (current icons still show the PicoTorrent logo).
- [ ] Update `docs/` for the new name/URLs.
- [ ] CMake 4.x compatibility audit (old vendored subprojects declare low
      `cmake_minimum_required`).

## MVP scope (first milestone)

1. Session management (libtorrent session lifecycle, settings persistence).
2. Add torrent / add magnet link.
3. Torrent list: progress, speeds, status, ETA; start/pause/remove.
4. Details tabs: overview, files, peers, trackers.
5. Basic preferences: download paths, connection, bandwidth.

Out of scope for the MVP (later): PQL, labels, RSS, torrent creation wizard,
plugin API/updater, translations beyond en-US, installer packaging.

## Layout

```
src/netorrent/         application + UI + session wiring
  bittorrent/          libtorrent session/torrent wrappers
  core/                configuration, sqlite, environment, http, geoip
  ui/                  wxWidgets UI (frame, views, dialogs, models)
src/pql/               PQL query language (ANTLR)
src/plugins/updater/   plugin-based updater
vendor/                submodules: vcpkg, wx, sentry-crashpad, antlr4
res/dbmigrations/      sqlite schema migrations
scripts/build.ps1      local build entry point
```
