# SETUP.md — Automated Setup & Driver Guide

## Table of Contents
1. [Windows Setup (PowerShell)](#windows-setup)
2. [Linux Setup (Bash)](#linux-setup)
3. [NVIDIA-Specific Notes](#nvidia-specific-notes)
4. [AMD-Specific Notes](#amd-specific-notes)
5. [Vulkan Configuration (Linux/AMD)](#vulkan-configuration)
6. [Uninstall](#uninstall)

---

## Windows Setup

Save the following as `setup_windows.ps1` and run in **PowerShell (Admin)**:

```powershell
#Requires -RunAsAdministrator
param(
    [string]$UEPath = "C:\Program Files\Epic Games\UE_5.3",
    [string]$RepoURL = "https://github.com/JoppeJoppinger/blackhole-game.git",
    [string]$InstallDir = "$env:USERPROFILE\Projects\blackhole-game"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host "=== Black Hole Game — Windows Setup ===" -ForegroundColor Cyan

# 1. Verify UE5.3 installation
if (-not (Test-Path "$UEPath\Engine\Build\BatchFiles\Build.bat")) {
    Write-Error "UE5.3 not found at '$UEPath'. Install via Epic Games Launcher first."
    exit 1
}

# 2. Check Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    Write-Host "Visual Studio found: $vsPath" -ForegroundColor Green
} else {
    Write-Warning "Visual Studio not found. Install VS 2022 with 'Game development with C++' workload."
    Write-Host "Download: https://visualstudio.microsoft.com/downloads/"
}

# 3. Clone repository
if (-not (Test-Path $InstallDir)) {
    Write-Host "Cloning repository..." -ForegroundColor Yellow
    git clone $RepoURL $InstallDir
} else {
    Write-Host "Repository already exists at $InstallDir, pulling latest..." -ForegroundColor Yellow
    Push-Location $InstallDir
    git pull
    Pop-Location
}

Push-Location $InstallDir

# 4. Generate Visual Studio project files
Write-Host "Generating project files..." -ForegroundColor Yellow
& "$UEPath\Engine\Build\BatchFiles\GenerateProjectFiles.bat" `
    BlackHoleGame.uproject -Game 2>&1 | Write-Host

# 5. Build editor target
Write-Host "Building BlackHoleGame (this takes 5-15 minutes)..." -ForegroundColor Yellow
& "$UEPath\Engine\Build\BatchFiles\Build.bat" `
    BlackHoleGameEditor Win64 Development `
    "$InstallDir\BlackHoleGame.uproject" -Waitmutex 2>&1 | Write-Host

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Check output above."
    exit 1
}

Write-Host ""
Write-Host "=== Build successful! ===" -ForegroundColor Green
Write-Host "Launch with:" -ForegroundColor Cyan
Write-Host "  `"$UEPath\Engine\Binaries\Win64\UnrealEditor.exe`" `"$InstallDir\BlackHoleGame.uproject`""

Pop-Location
```

### What It Installs
- Clones the repository to `~/Projects/blackhole-game`
- Generates Visual Studio `.sln` file
- Compiles all C++ source (BlackHoleActor, SpaceshipPawn, shaders, etc.)

---

## Linux Setup

```bash
#!/bin/bash
set -euo pipefail

UE_PATH="${UE_PATH:-$HOME/UnrealEngine}"
REPO_URL="https://github.com/JoppeJoppinger/blackhole-game.git"
INSTALL_DIR="${INSTALL_DIR:-$HOME/Projects/blackhole-game}"

echo "=== Black Hole Game — Linux Setup ==="

# ── Verify UE5 ────────────────────────────────────────────────────────────────
if [ ! -f "$UE_PATH/Engine/Build/BatchFiles/Linux/Build.sh" ]; then
    echo "ERROR: UE5 not found at $UE_PATH"
    echo "Set UE_PATH environment variable to your UE5 installation."
    echo "Build from source: https://github.com/EpicGames/UnrealEngine"
    exit 1
fi

# ── System dependencies ────────────────────────────────────────────────────────
echo "Installing system dependencies..."
if command -v apt-get &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y \
        build-essential clang-14 libc++-14-dev libc++abi-14-dev \
        libvulkan-dev vulkan-tools vulkan-validationlayers \
        libx11-dev libxrandr-dev libxi-dev libxcursor-dev \
        libxinerama-dev libssl-dev git cmake ninja-build
elif command -v dnf &>/dev/null; then
    sudo dnf install -y \
        clang libcxx-devel vulkan-devel vulkan-loader vulkan-tools \
        vulkan-validation-layers libX11-devel git cmake ninja-build
elif command -v pacman &>/dev/null; then
    sudo pacman -S --noconfirm \
        clang libc++ vulkan-devel vulkan-icd-loader vulkan-tools \
        vulkan-validation-layers libx11 git cmake ninja
fi

# ── Clone ─────────────────────────────────────────────────────────────────────
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Cloning repository..."
    git clone "$REPO_URL" "$INSTALL_DIR"
else
    echo "Repository exists, updating..."
    cd "$INSTALL_DIR" && git pull
fi

cd "$INSTALL_DIR"

# ── Generate project files ────────────────────────────────────────────────────
echo "Generating project files..."
"$UE_PATH/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh" \
    BlackHoleGame.uproject -Game

# ── Build ─────────────────────────────────────────────────────────────────────
echo "Building BlackHoleGame (5-20 minutes depending on hardware)..."
"$UE_PATH/Engine/Build/BatchFiles/Linux/Build.sh" \
    BlackHoleGameEditor Linux Development BlackHoleGame.uproject

echo ""
echo "=== Build successful! ==="
echo "Launch with:"
echo "  $UE_PATH/Engine/Binaries/Linux/UnrealEditor $INSTALL_DIR/BlackHoleGame.uproject"
```

---

## NVIDIA-Specific Notes

### Driver Requirements
- **Minimum**: NVIDIA driver 520+ (for DX12/Vulkan + UE5 RT support)
- **Recommended**: NVIDIA driver 545+ (latest Game Ready or Studio driver)
- Download: https://www.nvidia.com/drivers

### Hardware Ray Tracing
- Automatically enabled on RTX 2080+ by `GraphicsQualityManager`
- Requires DX12 mode (set automatically on Windows)
- If RT causes issues: set `bUseForcePreset=true`, `ForcedPreset=High` in BP_BlackHole

### DLSS / Frame Generation
- DLSS 3 can be integrated via the DLSS UE5 plugin (NVIDIA Streamline)
- Not included in base project; add as optional plugin
- With DLSS, reduce `r.ScreenPercentage` to 67% for significant performance gain

---

## AMD-Specific Notes

### Driver Requirements (Windows)
- **Minimum**: AMD Adrenalin 23.1.1 (January 2023)
- **Recommended**: **AMD Adrenalin 23.12.1 or newer**
  - Earlier drivers have issues with Vulkan compute shaders used by Lumen SW-RT
- Download: https://www.amd.com/en/support

### Driver Requirements (Linux)
- **Mesa 23.0+** (open-source, included in Ubuntu 23.04 / Fedora 38+)
  - Check version: `glxinfo | grep "OpenGL version"`
  - On Ubuntu 22.04, install Mesa backport:
    ```bash
    sudo add-apt-repository ppa:kisak/kisak-mesa
    sudo apt update && sudo apt upgrade
    ```
- **AMDGPU-PRO 23.10+** (proprietary, recommended for Vulkan Compute on RDNA2)
  - Download from https://www.amd.com/en/support/linux-drivers
  - Install: `./amdgpu-install --usecase=workstation --vulkan=pro`

### Vulkan Configuration (AMD Linux)
The game uses Vulkan on Linux. Verify your setup:

```bash
# Check Vulkan ICD loaders
vulkaninfo | head -30

# Verify AMD device is listed
vulkaninfo 2>/dev/null | grep "deviceName"

# If multiple GPUs, force AMD device:
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json

# Launch editor with explicit Vulkan:
export DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1=1
$UE_PATH/Engine/Binaries/Linux/UnrealEditor BlackHoleGame.uproject -vulkan
```

### AMD Performance Tips
- The `High` preset uses **Software Lumen** which performs well on RDNA2+
- For RDNA1 (RX 5000), use `Medium` preset; Software Lumen is slow on RDNA1
- Set `r.Lumen.ScreenProbeGather.RadianceCache.NumMipmaps 3` (down from 4) if FPS drops
- AMD FSR 2.1+ is compatible with TSR-off configurations — enable via console:
  ```
  r.AntiAliasingMethod 2  (TAA)
  r.FidelityFX.FSR.Enable 1
  ```
  (Requires integrating FidelityFX Super Resolution plugin from GPUOpen)

### AMD Known Issues
- **VSM (Virtual Shadow Maps)**: Occasional artifacts on RDNA1 with driver < 23.5.
  Workaround: `r.Shadow.Virtual.Enable 0` (auto-applied on RDNA1 by quality manager)
- **Shader compilation**: First launch compiles ~2000 shaders; takes 5-10 minutes on
  AMD. Subsequent launches use the shader cache. Do not interrupt first launch.
- **DX11 fallback**: If DX12 causes crashes on RX 5000, add `-dx11` to launch args.
  The Medium/Low presets are designed to work on DX11 SM5.

---

## Vulkan Configuration

### Setting Vulkan as Default RHI (Linux)
Add to `Config/DefaultEngine.ini` (already set):
```ini
[/Script/LinuxTargetPlatform.LinuxTargetSettings]
TargetedRHIs=SF_VULKAN_SM5
TargetedRHIs=SF_VULKAN_SM6
```

Or launch with: `UnrealEditor BlackHoleGame.uproject -vulkan`

### Vulkan Validation Layers (Development)
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
./UnrealEditor BlackHoleGame.uproject -vulkan -VulkanDebug
```

---

## Uninstall

### Windows
```powershell
# Remove build artifacts (keep source)
Remove-Item -Recurse -Force .\Binaries, .\Intermediate, .\Saved

# Full removal
Remove-Item -Recurse -Force $InstallDir
```

### Linux
```bash
# Remove build artifacts
rm -rf Binaries/ Intermediate/ Saved/

# Full removal
rm -rf "$INSTALL_DIR"
```
