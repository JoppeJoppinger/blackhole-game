# Black Hole Game — Windows Installer

This document explains how to build, use, and customize the Windows installer for the **Black Hole Game** UE5 project.

The installer is built with **WiX Toolset v4** and produces:
- `BlackHoleGame-1.0.0-Setup.exe` — Full bootstrapper that installs prerequisites + game files
- `BlackHoleGame-1.0.0.msi` — Standalone MSI for silent/enterprise deployments

---

## Prerequisites (Build Machine)

| Tool | Version | Where |
|------|---------|-------|
| .NET SDK | 8.0+ | https://dotnet.microsoft.com/download |
| WiX Toolset | v4 (auto-installed) | `dotnet tool install --global wix` |
| Windows OS | 10/11 | Required for MSI/EXE building |

> **Note:** The build script automatically installs WiX v4 if not present.

---

## Building the Installer

1. **Clone / copy the repository** to a Windows machine
2. Open **PowerShell** (not cmd) in the `Installer\` directory
3. Run:

```powershell
.\build.ps1
```

That's it. The script will:
1. Install WiX Toolset v4 (if needed) via `dotnet tool`
2. Add required WiX extensions (Bal, UI, Util)
3. Create placeholder `Resources/` assets (replace these before release — see below)
4. Build `Output\BlackHoleGame-1.0.0.msi`
5. Build `Output\BlackHoleGame-1.0.0-Setup.exe` (bootstrapper)
6. Print SHA256 checksums of all output files

### Build options

```powershell
# Build only the MSI (skip the Bundle/bootstrapper EXE)
.\build.ps1 -SkipBundle

# Clean output directory before building
.\build.ps1 -CleanFirst

# Verbose output
.\build.ps1 -Verbose
```

---

## Before Release: Replace Placeholder Assets

The build script creates minimal placeholder assets. **Replace these before shipping:**

| File | Required Size | Purpose |
|------|--------------|---------|
| `Resources\icon.ico` | 256×256 (ICO) | App icon (taskbar, shortcuts, Add/Remove Programs) |
| `Resources\logo.png` | 480×70 px (PNG) | Bundle welcome screen logo |
| `Resources\banner.bmp` | 500×63 px (BMP) | MSI dialog top banner |
| `Resources\dialog.bmp` | 493×312 px (BMP) | MSI welcome/finish dialog background |
| `LICENSE.rtf` | — | License text shown during install |

---

## What the Installer Does (Step by Step)

### Bootstrapper EXE (`BlackHoleGame-1.0.0-Setup.exe`)

1. **Welcome screen** — Shows game logo and name
2. **License agreement** — Displays `LICENSE.rtf`
3. **Prerequisite detection & install:**
   - **Git for Windows** — Checks `HKLM\SOFTWARE\GitForWindows`. Downloads and installs silently if missing.
   - **Visual C++ 2022 Redistributable x64** — Checks `HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64`. Downloads from Microsoft if needed.
   - **Visual Studio 2022 Build Tools** — Checks `HKLM\SOFTWARE\Microsoft\VisualStudio\17.0`. Downloads and installs with C++ Game workload if missing.
4. **Install directory selection** — Default: `C:\Program Files\BlackHoleGame\`
5. **File installation** — Copies all project files
6. **Shortcuts** — Creates Desktop and Start Menu shortcuts
7. **Registry** — Writes install path and version info
8. **Finish screen** — Summary with option to open project

### What gets installed

```
[Program Files]\BlackHoleGame\
├── Source\BlackHoleGame\      ← All .h, .cpp source files + Build.cs
├── Config\                    ← DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini, etc.
├── Shaders\                   ← AccretionDisk.usf, GravitationalLens.usf, StarField.usf
├── Content\                   ← Blueprint/Map/Material/Particle README files
├── BlackHoleGame.uproject     ← UE5 project descriptor
├── README.md
├── SETUP.md
└── setup_windows.ps1          ← Post-install setup helper
```

**Shortcuts created:**
- `Desktop\Black Hole Game.lnk` → Opens project in UE5
- `Start Menu\Black Hole Game\Open in UE5.lnk`
- `Start Menu\Black Hole Game\Uninstall Black Hole Game.lnk`

**Registry keys written:**
```
HKLM\SOFTWARE\JoppeJoppinger\BlackHoleGame\
  InstallPath = C:\Program Files\BlackHoleGame\
  Version     = 1.0.0.0
  UE5Path     = (detected from HKLM\SOFTWARE\EpicGames\Unreal Engine\5.3\InstalledDirectory)
```

---

## Installation Options

### Interactive (recommended for end users)
```
BlackHoleGame-1.0.0-Setup.exe
```

### Silent install (no UI)
```
BlackHoleGame-1.0.0-Setup.exe /quiet /norestart
```

### Silent install to custom directory
```
msiexec /i BlackHoleGame-1.0.0.msi INSTALLFOLDER="D:\Games\BlackHole\" /quiet /norestart
```

### Silent install with custom UE5 path
```
msiexec /i BlackHoleGame-1.0.0.msi UEINSTALLPATH="C:\Program Files\Epic Games\UE_5.3\" /quiet /norestart
```

### Log the installation
```
BlackHoleGame-1.0.0-Setup.exe /quiet /log install.log
```

---

## MSI Properties Reference

| Property | Default | Description |
|----------|---------|-------------|
| `INSTALLFOLDER` | `[ProgramFilesFolder]\BlackHoleGame\` | Installation directory |
| `UEINSTALLPATH` | (auto-detected from registry) | Path to Unreal Engine 5.3 |

---

## Uninstalling

### Via Windows Settings
1. Open **Settings → Apps → Installed apps**
2. Search for "Black Hole Game"
3. Click **Uninstall**

### Via command line (MSI)
```
msiexec /x {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
```

### Via bootstrapper EXE
```
BlackHoleGame-1.0.0-Setup.exe /uninstall /quiet
```

---

## Project Structure

```
Installer/
├── BlackHoleGame.wixproj          WiX v4 MSBuild project
├── build.ps1                      Build script (run this)
│
├── Bundle/
│   └── Bundle.wxs                 Bootstrapper EXE definition
│
├── Product/
│   ├── Product.wxs                MSI package definition
│   ├── Directories.wxs            Install directory tree
│   ├── Features.wxs               All file components
│   ├── Shortcuts.wxs              Desktop + Start Menu shortcuts
│   ├── RegistryEntries.wxs        Registry values
│   └── EnvironmentVariables.wxs   BLACKHOLEGAME_PATH env var
│
├── UI/
│   └── CustomUI.wxs               UI customization hooks
│
├── Prerequisites/
│   ├── Git.wxs                    Git detection documentation
│   └── VCRedist.wxs               VCRedist detection documentation
│
└── Output/                        ← Build output (git-ignored)
    ├── BlackHoleGame-1.0.0.msi
    └── BlackHoleGame-1.0.0-Setup.exe
```

---

## Troubleshooting

### "wix: command not found" after install
Restart your terminal or run: `$env:PATH += ";$env:USERPROFILE\.dotnet\tools"`

### ICE validation errors during build
Add the ICE code to `SuppressIces` in `BlackHoleGame.wixproj`. Common ones: `ICE69` (shortcut targets).

### Bundle fails to build "Cannot find SourceFile"
Build the MSI first (`.\build.ps1 -SkipBundle`), then build the Bundle.
The Bundle references `Output\BlackHoleGame-1.0.0.msi` which must exist first.

### Prerequisites download during build
The bootstrapper downloads prerequisite installers at runtime (on the user's machine), not at build time. The build machine only needs .NET SDK + WiX v4.

---

## WiX v4 Notes

- Namespace: `http://wixtoolset.org/schemas/v4/wxs`
- Distribution: `dotnet tool install --global wix`
- Extensions: `wix extension add WixToolset.Bal.wixext` etc.
- Documentation: https://wixtoolset.org/docs/
