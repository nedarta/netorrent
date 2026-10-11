# NeTorrent

![CI](https://github.com/nedarta/netorrent/workflows/CI/badge.svg)
[![Discord](https://img.shields.io/discord/759537913205227580)](https://discord.gg/tV3dFrv)

A tiny, hackable BitTorrent client written in modern C++. Based on
Rasterbar-libtorrent to provide high performance and low memory usage.

<p align="center">
    <img src="res/screenshot1.png?raw=true" width="614" />
</p>


## Quick facts

- Full support for BitTorrent 2.0 ([BEP-52](http://bittorrent.org/beps/bep_0052.html)), v1, v2 and v1+v2 hybrid torrents.
- Supports DHT, PeX, LSD, UPnP.
- (Azureus-style) peer ID: `-NT-`. Example: `-NT0151-` (major: 0, minor: 15, patch: 1).
- User agent: `NeTorrent/x.y.z`.
- Native look-and-feel across Windows versions.
- Easy to use with high performance.

*The portable version of NeTorrent requires manual installation of [the Microsoft Visual C++ Redistributable for Visual Studio 2019](https://visualstudio.microsoft.com/downloads/).*


## Download for Windows

[![Download zip](https://custom-icon-badges.demolab.com/badge/-Download-blue?style=for-the-badge&logo=download&logoColor=white "Download zip")](https://github.com/nedarta/netorrent/releases)


## Building NeTorrent

NeTorrent is a Windows application built with CMake and vcpkg. You need:

 * [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (or the
   **Build Tools**) with the *Desktop development with C++* workload.
 * [CMake](https://cmake.org/download/) — the copy bundled with Visual Studio is
   used automatically if it is not on `PATH`.
 * Git, for version stamping and the vendored submodules.

OpenSSL, Boost, libtorrent, fmt, nlohmann-json and SQLite are pulled in by
[vcpkg](https://vcpkg.io/) using the manifest in `vcpkg.json` — no manual
installation is required.

```powershell
# One-time: fetch the vendored dependencies (vcpkg, wxWidgets, crashpad, antlr4)
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

## Translations

Translation files live in `lang/`. Community translations are very welcome —
open a pull request against the repository to add or improve a language.


## License

Copyright (c) Viktor Elofsson and contributors. NeTorrent is provided
as-is under the MIT license. For more information see [LICENSE](LICENSE).
