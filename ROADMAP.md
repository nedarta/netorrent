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
| Build toolchain (VS 2022 Build Tools + CMake 3.31 + vcpkg) | done |
| vcpkg manifest working (`deprfun` for libtorrent 2.1.2 compat) | done |
| rag-rat semantic index (`src`, `include`, `src/pql`, docs) | done — 1040 chunks embedded |
| First green build | done — `NeTorrent.exe` runs, migrates DB, loads translations, listens on 6881 |
| MVP features | not started |

The fork keeps upstream history so we can diff and cherry-pick. `upstream` points
at `picotorrent/picotorrent`; `origin` is where the NeTorrent repo will live.

## Modernization backlog

**Toolchain / build**
- [x] Confirm a green `scripts/build.ps1` build of the rebranded tree. (in flight)
- [x] Bump `vendor/vcpkg` to current master; old vcpkg could not detect VS 2022 Build Tools.
- [x] Pin the vcpkg manifest `builtin-baseline`.
- [x] Fix `src/pql`'s dead `git://` URL (GitHub dropped git protocol).
- [x] Build script auto-initializes wxWidgets' nested third-party submodules.
- [x] libtorrent builds against the 2.0-era API via the vcpkg `deprfun` feature
      (interim; see the port item below).
- [ ] Port the code to the libtorrent 2.1 API properly (drop `deprfun`).
- [ ] Bump `CMAKE_CXX_STANDARD` from 17 to 20 once the baseline is green.
- [ ] Evaluate moving `vendor/wx` and `vendor/vcpkg` to vcpkg-provided packages.

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

## Ported upstream PRs

Open PRs from `picotorrent/picotorrent` that were never merged upstream, ported
into NeTorrent (rebranded, and where needed adjusted for the modernized stack):

- **#1287** more advanced libtorrent settings (`announce_ip`,
  `rate_limit_ip_overhead`, uTP/TCP toggles, `dht_privacy_lookups`).
- **#1246 / #1203 / #1251** translations — cs-CZ fix, vi-VN Weblate update,
  new Azerbaijani (`az-AZ`).
- **#1264** use vcpkg's wxWidgets instead of the `vendor/wx` submodule.

Deliberately not ported: #1002 / #1243 (Linux), #1125 (client/server split —
too large/WIP), #1225 (libtorrent bump — already superseded), #1010 (AppX
packaging), #931 (RSS monitor — upstream PR is an incomplete stub).

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
