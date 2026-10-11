#Requires -Version 5.1
<#
.SYNOPSIS
    Configure and build NeTorrent.

.DESCRIPTION
    A lightweight alternative to the Cake build that mirrors what CI does:
    locate MSVC, bootstrap the vendored vcpkg, then drive CMake directly.

.PARAMETER Arch
    Target architecture: x64 (default) or x86.

.PARAMETER Configuration
    Build configuration: Release (default) or Debug.

.PARAMETER Target
    Optional CMake target to build (default: the whole solution).

.PARAMETER Clean
    Delete the build directory before configuring.

.PARAMETER Version
    Override the semantic version (default is derived from git tags).

.EXAMPLE
    ./scripts/build.ps1
    ./scripts/build.ps1 -Arch x64 -Configuration Debug -Target NeTorrent
#>
[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86')][string]$Arch = 'x64',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$Target = '',
    [switch]$Clean,
    [string]$Version = ''
)

$ErrorActionPreference = 'Stop'
# In PowerShell 7.3+, native stderr would otherwise become a terminating error
# under ErrorActionPreference = 'Stop'; we want to inspect the exit code instead.
if ($PSVersionTable.PSVersion.Major -ge 7) { $PSNativeCommandUseErrorActionPreference = $false }
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$Platform      = if ($Arch -eq 'x86') { 'Win32' } else { 'x64' }
$Triplet       = "$Arch-windows-static-md-rel"
$BuildDir      = Join-Path $Root "build-$Arch"
$VsGenerator   = 'Visual Studio 17 2022'

Write-Host "==> NeTorrent build" -ForegroundColor Cyan
Write-Host "    root          : $Root"
Write-Host "    arch          : $Arch ($Platform)"
Write-Host "    configuration : $Configuration"
Write-Host "    triplet       : $Triplet"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "==> Cleaning $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

# --- Locate MSVC environment -------------------------------------------------
function Get-VcVarsPath {
    $candidates = @()
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $inst = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($inst) { $candidates += (Join-Path $inst 'VC\Auxiliary\Build\vcvarsall.bat') }
    }
    foreach ($pf in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
        $candidates += (Join-Path $pf 'Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat')
        $candidates += (Join-Path $pf 'Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat')
        $candidates += (Join-Path $pf 'Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat')
        $candidates += (Join-Path $pf 'Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat')
    }
    foreach ($c in $candidates) { if ($c -and (Test-Path $c)) { return $c } }
    return $null
}

$vcvars = Get-VcVarsPath
if (-not $vcvars) { throw "Could not find vcvarsall.bat. Install Visual Studio 2022 (or its Build Tools) with the C++ workload." }

Write-Host "==> Importing MSVC environment from $vcvars"
$vcArch = if ($Arch -eq 'x86') { 'x86' } else { 'x64' }
cmd /c "`"$vcvars`" $vcArch && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($matches[1])" -Value $matches[2] }
}

# --- Locate CMake ------------------------------------------------------------
# Prefer the CMake bundled with Visual Studio (3.x). CMake 4.x removed
# compatibility with cmake_minimum_required < 3.5, which the older vendored
# subprojects (wxWidgets, crashpad, antlr4) still declare.
function Get-CMake {
    $vs = $env:VSINSTALLDIR
    if ($vs) {
        $bundled = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        if (Test-Path $bundled) { return $bundled }
    }
    if (Get-Command cmake -ErrorAction SilentlyContinue) { return (Get-Command cmake).Source }
    throw "Could not find cmake. Install CMake or the 'C++ CMake tools for Windows' VS component."
}
$cmake = Get-CMake
Write-Host "==> Using CMake: $cmake"

# --- Bootstrap vcpkg ---------------------------------------------------------
$vcpkgDir = Join-Path $Root 'vendor/vcpkg'
$vcpkgExe = Join-Path $vcpkgDir 'vcpkg.exe'
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "==> Bootstrapping vcpkg"
    & (Join-Path $vcpkgDir 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE -ne 0) { throw "vcpkg bootstrap failed" }
}

# --- Vendored nested dependencies --------------------------------------------
# wxWidgets keeps its third-party libs (zlib, png, expat, tiff, jpeg, scintilla...)
# in nested submodules. A shallow top-level fetch can leave them empty, which
# shows up later as "No SOURCES given to target: wxzlib".
$wxZlibSource = Join-Path $Root 'vendor/wx/src/zlib/adler32.c'
if (-not (Test-Path $wxZlibSource)) {
    Write-Host "==> Initializing wxWidgets nested submodules"
    & git -C (Join-Path $Root 'vendor/wx') submodule update --init --recursive --depth 1
    if ($LASTEXITCODE -ne 0) { throw "failed to initialize wxWidgets submodules" }
}

# --- Version information -----------------------------------------------------
function Get-BuildVersion {
    param([string]$Override)
    if ($Override) { return $Override }
    $tag = (& git describe --tags --match 'v[0-9]*' --abbrev=0 2>$null | Select-Object -First 1)
    if ($LASTEXITCODE -eq 0 -and $tag) { return $tag.TrimStart('v') }
    return '0.26.0'
}
$semver  = Get-BuildVersion -Override $Version
$parts   = $semver.Split('.')
$verMajor = $parts[0]
$verMinor = if ($parts.Count -gt 1) { $parts[1] } else { '0' }
$verPatch = if ($parts.Count -gt 2) { $parts[2] } else { '0' }
$version  = "$verMajor.$verMinor.$verPatch"
$shortSha = (& git rev-parse --short=7 HEAD).Trim()
$branch   = (& git rev-parse --abbrev-ref HEAD).Trim()

Write-Host "==> Version $version ($branch @ $shortSha)"

# --- Configure ---------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$configureLog = Join-Path $BuildDir 'configure.log'
Write-Host "==> Configuring CMake (log: $configureLog)"
& $cmake -S $Root -B $BuildDir -G $VsGenerator -A $Platform `
    "-DVCPKG_TARGET_TRIPLET=$Triplet" `
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" `
    "-DGITVERSION_VAR_BRANCHNAME=$branch" `
    "-DGITVERSION_VAR_SEMVER=$semver" `
    "-DGITVERSION_VAR_SHORTSHA=$shortSha" `
    "-DGITVERSION_VAR_VERSION_MAJOR=$verMajor" `
    "-DGITVERSION_VAR_VERSION_MINOR=$verMinor" `
    "-DGITVERSION_VAR_VERSION_PATCH=$verPatch" `
    "-DGITVERSION_VAR_VERSION=$version" 2>&1 | Tee-Object -FilePath $configureLog
if ($LASTEXITCODE -ne 0) {
    Write-Host "--- last 40 lines of configure.log ---" -ForegroundColor Yellow
    Get-Content $configureLog -Tail 40
    throw "CMake configure failed (see $configureLog)"
}

# --- Build -------------------------------------------------------------------
$buildArgs = @('--build', $BuildDir, '--config', $Configuration)
if ($Target) { $buildArgs += @('--target', $Target) }
$buildLog = Join-Path $BuildDir 'build.log'
Write-Host "==> Building ($Configuration, log: $buildLog)"
& $cmake @buildArgs 2>&1 | Tee-Object -FilePath $buildLog
if ($LASTEXITCODE -ne 0) {
    Write-Host "--- last 40 lines of build.log ---" -ForegroundColor Yellow
    Get-Content $buildLog -Tail 40
    throw "CMake build failed (see $buildLog)"
}

Write-Host "==> Done. Artifacts in $BuildDir\$Configuration" -ForegroundColor Green
