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
| vcpkg manifest working (libtorrent 2.1.2, no deprecated API) | done |
| libtorrent 2.1 API port (deprfun removed) | done |
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
- [x] libtorrent builds against the real 2.1 API (`deprfun` removed).
- [x] Port the code to the libtorrent 2.1 API properly (drop `deprfun`).
- [ ] Bump `CMAKE_CXX_STANDARD` from 17 to 20.
- [x] Move `vendor/wx` to vcpkg's wxWidgets (upstream PR 1264).
- [ ] Evaluate moving `vendor/vcpkg` to a standalone vcpkg (`VCPKG_ROOT`).

**Dependencies**
- [x] libtorrent: on 2.1.2.
- [x] wxWidgets: on 3.3.3 (via vcpkg).
- [x] OpenSSL: on 3.6.5.
- [ ] Revisit Crashpad/Sentry wiring (crash reports currently go to PicoTorrent's
      Sentry project).

**Correctness / housekeeping**
- [ ] Repoint upstream endpoints to `api.netorrent.org` once that service exists
      (the update-check and GeoIP defaults reference `api.picotorrent.org`).
      Deliberately left intact for now.
- [ ] New NeTorrent icon set (current icons still show the PicoTorrent logo).
- [ ] Update `docs/` for the new name/URLs.
- [ ] CMake 4.x compatibility audit (low priority; build pins VS-bundled CMake 3.31).

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

Also implemented from upstream **issues**: #1288 — "Downloaded" and
"Uploaded" columns in the torrent list.

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
