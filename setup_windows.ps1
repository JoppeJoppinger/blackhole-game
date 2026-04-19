#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Black Hole Game — Vollautomatisches Windows Setup
.DESCRIPTION
    Installiert alle Voraussetzungen und richtet das Projekt ein:
    - Git
    - Visual Studio 2022 (mit C++ Game-Workload)
    - Epic Games Launcher + Unreal Engine 5.3
    - Klont das Repository
    - Generiert Projektdateien
    - Kompiliert das Projekt
.NOTES
    Ausfuehren als Administrator:
    Right-click -> "Run with PowerShell as Administrator"
    oder:
    powershell.exe -ExecutionPolicy Bypass -File setup_windows.ps1
#>

param(
    [string]$UEVersion    = "5.3",
    [string]$UEPath       = "C:\Program Files\Epic Games\UE_5.3",
    [string]$RepoURL      = "https://github.com/JoppeJoppinger/blackhole-game.git",
    [string]$InstallDir   = "$env:USERPROFILE\Projects\blackhole-game"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference    = "SilentlyContinue"

# ── Farben & Hilfsfunktionen ────────────────────────────────────────────────
function Write-Step  { param($msg) Write-Host "`n>>> $msg" -ForegroundColor Cyan }
function Write-OK    { param($msg) Write-Host "    [OK]  $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "    [!!]  $msg" -ForegroundColor Yellow }
function Write-Fail  { param($msg) Write-Host "    [XX]  $msg" -ForegroundColor Red }

function Test-Command { param($cmd) return [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }

function Install-WingetPackage {
    param([string]$Id, [string]$Name)
    Write-Host "    Installiere $Name via winget..." -ForegroundColor Yellow
    winget install --id $Id --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        Write-Warn "$Name Installation moeglicherweise fehlgeschlagen (Exit: $LASTEXITCODE). Weiter..."
    }
}

# ─────────────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════╗" -ForegroundColor Magenta
Write-Host "║       Black Hole Game — Windows Setup Script         ║" -ForegroundColor Magenta
Write-Host "║       Unreal Engine 5 / NVIDIA + AMD Support         ║" -ForegroundColor Magenta
Write-Host "╚══════════════════════════════════════════════════════╝" -ForegroundColor Magenta
Write-Host ""

# ── 1. Installationsprotokoll ────────────────────────────────────────────────
$LogFile = "$env:TEMP\blackhole_setup_log.txt"
$Installed = @()
Start-Transcript -Path $LogFile -Append | Out-Null
Write-OK "Log: $LogFile"

# ── 2. winget pruefen / installieren ────────────────────────────────────────
Write-Step "Pruefe winget (Windows Package Manager)..."
if (-not (Test-Command "winget")) {
    Write-Warn "winget nicht gefunden. Installiere App Installer aus dem Microsoft Store..."
    Start-Process "ms-windows-store://pdp/?productid=9NBLGGH4NNS1" -Wait
    Write-Host "    Bitte App Installer installieren, dann dieses Skript neu starten." -ForegroundColor Yellow
    exit 1
}
$wgVer = (winget --version)
Write-OK "winget $wgVer gefunden"

# ── 3. Git ───────────────────────────────────────────────────────────────────
Write-Step "Pruefe Git..."
if (Test-Command "git") {
    $gitVer = (git --version)
    Write-OK "$gitVer bereits installiert"
} else {
    Install-WingetPackage "Git.Git" "Git for Windows"
    $Installed += "Git.Git"
    # PATH aktualisieren fuer diese Session
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path","User")
    Write-OK "Git installiert"
}

# ── 4. Visual Studio 2022 ────────────────────────────────────────────────────
Write-Step "Pruefe Visual Studio 2022..."
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsFound = $false
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -version "[17.0,18.0)" -property installationPath 2>$null
    if ($vsPath) {
        Write-OK "Visual Studio 2022 gefunden: $vsPath"
        $vsFound = $true

        # Workload pruefen
        $workloads = & $vsWhere -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath 2>$null
        if (-not $workloads) {
            Write-Warn "Workload 'Game development with C++' fehlt. Installiere nach..."
            $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vs_installer.exe"
            Start-Process $vsInstaller -ArgumentList `
                "modify --installPath `"$vsPath`" --add Microsoft.VisualStudio.Workload.NativeGame --quiet --norestart" `
                -Wait -NoNewWindow
            Write-OK "Workload installiert"
        } else {
            Write-OK "Workload 'Game development with C++' vorhanden"
        }
    }
}
if (-not $vsFound) {
    Write-Warn "Visual Studio 2022 nicht gefunden. Installiere Community Edition..."
    Install-WingetPackage "Microsoft.VisualStudio.2022.Community" "Visual Studio 2022 Community"
    $Installed += "Microsoft.VisualStudio.2022.Community"
    Write-Warn "VS2022 wurde installiert. Bitte manuell Workload 'Game development with C++' hinzufuegen:"
    Write-Host "    Visual Studio Installer -> Modify -> 'Game development with C++' -> Install" -ForegroundColor Yellow
    Write-Host "    Danach dieses Skript erneut ausfuehren." -ForegroundColor Yellow
    Start-Process "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vs_installer.exe"
    Read-Host "    [ENTER] druecken sobald die Workload installiert wurde..."
}

# ── 5. Epic Games Launcher pruefen ──────────────────────────────────────────
Write-Step "Pruefe Epic Games Launcher..."
$epicLauncher = "$env:ProgramFiles\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe"
if (-not (Test-Path $epicLauncher)) {
    Write-Warn "Epic Games Launcher nicht gefunden. Installiere..."
    $launcherInstaller = "$env:TEMP\EpicInstaller.msi"
    Invoke-WebRequest -Uri "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi" `
        -OutFile $launcherInstaller -UseBasicParsing
    Start-Process msiexec.exe -ArgumentList "/i `"$launcherInstaller`" /quiet /norestart" -Wait
    $Installed += "Epic Games Launcher (MSI)"
    Write-OK "Epic Games Launcher installiert"
} else {
    Write-OK "Epic Games Launcher gefunden"
}

# ── 6. Unreal Engine 5.3 pruefen ────────────────────────────────────────────
Write-Step "Pruefe Unreal Engine $UEVersion..."
$ueBuildBat = "$UEPath\Engine\Build\BatchFiles\Build.bat"
if (-not (Test-Path $ueBuildBat)) {
    Write-Warn "Unreal Engine $UEVersion nicht gefunden unter: $UEPath"
    Write-Host ""
    Write-Host "    ┌─────────────────────────────────────────────────────┐" -ForegroundColor Yellow
    Write-Host "    │  MANUELLE AKTION ERFORDERLICH                       │" -ForegroundColor Yellow
    Write-Host "    │                                                     │" -ForegroundColor Yellow
    Write-Host "    │  1. Epic Games Launcher oeffnen                     │" -ForegroundColor Yellow
    Write-Host "    │  2. 'Unreal Engine' -> 'Library'                    │" -ForegroundColor Yellow
    Write-Host "    │  3. '+' klicken -> Version 5.3 auswaehlen           │" -ForegroundColor Yellow
    Write-Host "    │  4. 'Install' -> Standardpfad bestaetigen           │" -ForegroundColor Yellow
    Write-Host "    │  5. (~30-60 GB Download, dauert je nach Verbindung) │" -ForegroundColor Yellow
    Write-Host "    └─────────────────────────────────────────────────────┘" -ForegroundColor Yellow
    Write-Host ""
    Start-Process "$env:ProgramFiles\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe"
    Read-Host "    [ENTER] druecken sobald UE 5.3 vollstaendig installiert ist..."

    if (-not (Test-Path $ueBuildBat)) {
        Write-Fail "UE5.3 immer noch nicht gefunden. Pruefe den Pfad:"
        Write-Host "    Erwartet: $UEPath" -ForegroundColor Red
        Write-Host "    Passe den Parameter -UEPath beim Skriptaufruf an." -ForegroundColor Yellow
        exit 1
    }
}
Write-OK "Unreal Engine $UEVersion gefunden: $UEPath"

# ── 7. Repository klonen / aktualisieren ─────────────────────────────────────
Write-Step "Repository einrichten..."
if (-not (Test-Path $InstallDir)) {
    Write-Host "    Klone $RepoURL..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path (Split-Path $InstallDir) -Force | Out-Null
    git clone $RepoURL $InstallDir
    Write-OK "Repository geklont nach: $InstallDir"
} else {
    Write-Host "    Repository gefunden, aktualisiere..." -ForegroundColor Yellow
    Push-Location $InstallDir
    git pull
    Pop-Location
    Write-OK "Repository aktualisiert"
}

# ── 8. Projektdateien generieren ─────────────────────────────────────────────
Write-Step "Generiere Visual Studio Projektdateien..."
Push-Location $InstallDir
& "$UEPath\Engine\Build\BatchFiles\GenerateProjectFiles.bat" `
    "$InstallDir\BlackHoleGame.uproject" -Game 2>&1 | ForEach-Object { Write-Host "    $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Fail "Projektdateien konnten nicht generiert werden."
    exit 1
}
Write-OK "Visual Studio Solution erstellt: BlackHoleGame.sln"

# ── 9. Kompilieren ───────────────────────────────────────────────────────────
Write-Step "Kompiliere BlackHoleGame (kann 5-20 Minuten dauern)..."
Write-Host "    CPU-Kerne: $([System.Environment]::ProcessorCount)" -ForegroundColor Gray
& "$UEPath\Engine\Build\BatchFiles\Build.bat" `
    BlackHoleGameEditor Win64 Development `
    "$InstallDir\BlackHoleGame.uproject" -Waitmutex `
    2>&1 | ForEach-Object { Write-Host "    $_" }

if ($LASTEXITCODE -ne 0) {
    Write-Fail "Kompilierung fehlgeschlagen. Pruefe die Ausgabe oben."
    Write-Host "    Log: $LogFile" -ForegroundColor Yellow
    exit 1
}
Write-OK "Kompilierung erfolgreich!"

# ── 10. Verknuepfung auf Desktop erstellen ───────────────────────────────────
Write-Step "Erstelle Desktop-Verknuepfung..."
$shortcutPath = "$env:USERPROFILE\Desktop\Black Hole Game.lnk"
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath  = "$UEPath\Engine\Binaries\Win64\UnrealEditor.exe"
$shortcut.Arguments   = "`"$InstallDir\BlackHoleGame.uproject`""
$shortcut.Description = "Black Hole Space Game - Unreal Engine 5"
$shortcut.Save()
Write-OK "Verknuepfung erstellt: $shortcutPath"

# ── 11. Installationsprotokoll speichern ─────────────────────────────────────
$installLog = @"
=== Black Hole Game — Installationsprotokoll ===
Datum:       $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Installiert: $($Installed -join ", ")
UE-Pfad:     $UEPath
Projektpfad: $InstallDir
GitHub:      $RepoURL

=== Deinstallation ===
1. Projektordner loeschen: $InstallDir
2. Folgende Pakete deinstallieren (falls durch dieses Skript installiert):
$($Installed | ForEach-Object { "   - $_" } | Out-String)
3. UE5.3 ueber Epic Games Launcher entfernen: Library -> UE 5.3 -> Uninstall
"@

$installLog | Out-File "$InstallDir\INSTALLED.txt" -Encoding UTF8
Write-OK "Protokoll gespeichert: $InstallDir\INSTALLED.txt"

# ── Fertig! ───────────────────────────────────────────────────────────────────
Stop-Transcript | Out-Null

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║              SETUP ABGESCHLOSSEN!                    ║" -ForegroundColor Green
Write-Host "╚══════════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "  Starten:" -ForegroundColor White
Write-Host "  1. Desktop-Verknuepfung 'Black Hole Game' doppelklicken" -ForegroundColor Gray
Write-Host "     ODER" -ForegroundColor Gray
Write-Host "  2. PowerShell:" -ForegroundColor Gray
Write-Host "     & `"$UEPath\Engine\Binaries\Win64\UnrealEditor.exe`" `"$InstallDir\BlackHoleGame.uproject`"" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Steuerung im Spiel:" -ForegroundColor White
Write-Host "  W/S = Schub vor/zurueck  |  A/D = Strafe links/rechts" -ForegroundColor Gray
Write-Host "  Q/E = Rollen             |  Maus = Ausrichten" -ForegroundColor Gray
Write-Host "  Shift = Boost            |  V = Kamera wechseln" -ForegroundColor Gray
Write-Host "  H = HUD an/aus           |  Space = Notbremse" -ForegroundColor Gray
Write-Host ""
Write-Host "  Projekt: $InstallDir" -ForegroundColor DarkGray
Write-Host "  GitHub:  https://github.com/JoppeJoppinger/blackhole-game" -ForegroundColor DarkGray
Write-Host "  Log:     $LogFile" -ForegroundColor DarkGray
Write-Host ""
