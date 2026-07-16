[CmdletBinding()]
param(
    [string]$DependencyRoot = (Join-Path $PSScriptRoot "..\\.bridgeengine-deps"),
    [string]$VcpkgRoot,
    [string]$Triplet = $env:VCPKG_TARGET_TRIPLET
)

$ErrorActionPreference = "Stop"

if (-not $Triplet) {
    $Triplet = "x64-windows"
}

function Stop-WithMessage([string]$Message) {
    Write-Error $Message
    exit 1
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Stop-WithMessage "Git is required to bootstrap vcpkg. Install Git for Windows and run this script again."
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\\Installer\\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Stop-WithMessage "Visual Studio Build Tools with C++ desktop development are required. Install them from https://visualstudio.microsoft.com/downloads/."
}

$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) {
    Stop-WithMessage "Install the Visual Studio Build Tools C++ workload, then run this script again."
}

if (-not $VcpkgRoot) {
    $VcpkgRoot = Join-Path $DependencyRoot "vcpkg"
}

if (-not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $VcpkgRoot) | Out-Null
    git clone --depth 1 https://github.com/microsoft/vcpkg.git $VcpkgRoot
}

$vcpkg = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkg)) {
    & (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics
}

& $vcpkg install "ffmpeg:$Triplet"

$toolchain = Join-Path $VcpkgRoot "scripts\\buildsystems\\vcpkg.cmake"
Write-Host "FFmpeg is ready. Configure BridgeEngine with:"
Write-Host "cmake --preset default -DCMAKE_TOOLCHAIN_FILE=`"$toolchain`" -DVCPKG_TARGET_TRIPLET=$Triplet"
