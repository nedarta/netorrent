# NeTorrent

![CI](https://github.com/nedarta/netorrent/workflows/CI/badge.svg)

A tiny, hackable BitTorrent client for Windows, written in modern C++ on top of
[Rasterbar-libtorrent](https://libtorrent.org) for high performance and low
memory usage.

NeTorrent is a maintained fork of
[PicoTorrent](https://github.com/picotorrent/picotorrent) — the last upstream
release was **v0.25.0 (May 2021)** — kept building against current toolchains
and actively developed.

<p align="center">
    <img src="res/screenshot1.png?raw=true" width="614" />
</p>


## Status

NeTorrent is a working, daily-usable client. It carries PicoTorrent's feature
set and adds maintenance and features. Highlights since the fork:

- **Ported to libtorrent 2.1** using the current API (the deprecated-API shim
  PicoTorrent needed is gone).
- **wxWidgets now comes from [vcpkg](https://vcpkg.io/)** (3.3.x) instead of a
  vendored submodule.
- Builds with **Visual Studio 2022 + CMake + vcpkg** (Boost 1.92, OpenSSL 3.6,
  SQLite, fmt, nlohmann-json).
- **RSS feed monitoring** — watch one or more feeds and automatically add
  torrents whose titles match each feed's include/exclude filters.
- **Downloaded / Uploaded** columns in the torrent list.
- Fixed a startup crash in the advanced libtorrent settings.
- New Azerbaijani translation; updated Czech and Vietnamese translations.


## Quick facts

- Full support for BitTorrent 2.0 ([BEP-52](http://bittorrent.org/beps/bep_0052.html)), v1, v2 and v1+v2 hybrid torrents.
- Supports DHT, PeX, LSD, UPnP.
- RSS feed monitoring with include/exclude filters.
- (Azureus-style) peer ID: `-NT-`. Example: `-NT0151-` (major: 0, minor: 15, patch: 1).
- User agent: `NeTorrent/x.y.z`.
- Native look-and-feel on Windows.
- Easy to use with high performance.

*The portable build requires the Microsoft Visual C++ Redistributable.*


## Download for Windows

[![Download zip](https://custom-icon-badges.demolab.com/badge/-Download-blue?style=for-the-badge&logo=download&logoColor=white "Download zip")](https://github.com/nedarta/netorrent/releases)


## Building NeTorrent

NeTorrent is a Windows application built with CMake and vcpkg. You need:

 * [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (or the
   **Build Tools**) with the *Desktop development with C++* workload.
 * [CMake](https://cmake.org/download/) — the copy bundled with Visual Studio is
   used automatically if it is not on `PATH`.
 * Git, for version stamping and the vendored submodules.

OpenSSL, Boost, libtorrent, fmt, nlohmann-json, SQLite and wxWidgets are pulled
in by [vcpkg](https://vcpkg.io/) using the manifest in `vcpkg.json` — no manual
installation is required.

```powershell
# One-time: fetch the vendored dependencies (vcpkg, crashpad, antlr4)
git submodule update --init --recursive

# Configure + build (defaults to x64 Release)
./scripts/build.ps1

# Other configurations
./scripts/build.ps1 -Arch x86 -Configuration Debug
./scripts/build.ps1 -Target NeTorrent
```

The resulting binary is written to `build-<arch>/<Configuration>/`.

### Legacy build (Cake)

The original Cake pipeline is still present for packaging (installer, portable
and symbols zips):

```
λ dotnet tool restore
λ dotnet cake --platform=[x86|x64] --configuration=[Debug|Release]
```


## Translations

Translation files live in `lang/`. Community translations are very welcome —
open a pull request against the repository to add or improve a language. Missing
keys fall back to `en-US`.


## Credits

NeTorrent is a fork of [PicoTorrent](https://github.com/picotorrent/picotorrent)
by Viktor Elofsson and contributors. The upstream history is preserved, and
`upstream` points at the original repository for reference.


## License

Copyright (c) Viktor Elofsson and PicoTorrent contributors.

NeTorrent changes copyright (c) the NeTorrent contributors. NeTorrent is
provided as-is under the MIT license. For more information see [LICENSE](LICENSE).
