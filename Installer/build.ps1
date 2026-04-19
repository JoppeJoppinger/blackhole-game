#Requires -Version 5.1
<#
.SYNOPSIS
    Build script for the Black Hole Game WiX v4 installer.
.DESCRIPTION
    Builds BlackHoleGame-1.0.0.msi and BlackHoleGame-1.0.0-Setup.exe.
.PREREQUISITES
    - .NET 8 SDK  (https://dotnet.microsoft.com/download)
    - WiX Toolset v4  (installed automatically by this script)
    - Windows OS
.EXAMPLE
    .\build.ps1
    .\build.ps1 -SkipBundle
    .\build.ps1 -CleanFirst
#>

param(
    [string]$InstallerDir = $PSScriptRoot,
    [switch]$SkipBundle,
    [switch]$CleanFirst
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ── Helpers ───────────────────────────────────────────────────────────────────
function Write-Step { param([string]$m) Write-Host "`n==> $m" -ForegroundColor Cyan }
function Write-OK   { param([string]$m) Write-Host "    [OK]   $m" -ForegroundColor Green }
function Write-Warn { param([string]$m) Write-Host "    [WARN] $m" -ForegroundColor Yellow }
function Write-Fail { param([string]$m) Write-Host "    [FAIL] $m" -ForegroundColor Red }

function Get-FileSummary {
    param([string]$path)
    if (Test-Path $path) {
        $info = Get-Item $path
        $hash = (Get-FileHash $path -Algorithm SHA256).Hash
        $size = '{0:N0} KB' -f ($info.Length / 1KB)
        Write-Host ('    {0,-52} {1,10}   SHA256: {2}' -f $info.Name, $size, $hash) -ForegroundColor White
    } else {
        Write-Warn "Not found: $path"
    }
}

# ── Step 1: Check / install WiX v4 ───────────────────────────────────────────
Write-Step 'Checking WiX Toolset v4'

$wixVersion = $null
try { $wixVersion = & wix --version 2>$null } catch {}

if (-not $wixVersion) {
    Write-Warn 'WiX Toolset not found. Installing via dotnet tool...'
    try { & dotnet --version | Out-Null }
    catch {
        Write-Fail '.NET SDK not found. Install from https://dotnet.microsoft.com/download'
        exit 1
    }
    & dotnet tool install --global wix
    if ($LASTEXITCODE -ne 0) { Write-Fail 'Failed to install WiX.'; exit 1 }
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH','User') + ';' + $env:PATH
    $wixVersion = & wix --version 2>$null
}

Write-OK "WiX version: $wixVersion"

# ── Step 2: WiX extensions ───────────────────────────────────────────────────
Write-Step 'Checking WiX extensions'

$extensions = @('WixToolset.Bal.wixext','WixToolset.UI.wixext','WixToolset.Util.wixext')

foreach ($ext in $extensions) {
    $extList = (& wix extension list 2>$null) -join ' '
    if ($extList -notmatch [regex]::Escape($ext)) {
        Write-Warn "$ext not installed -> adding..."
        & wix extension add $ext
        if ($LASTEXITCODE -ne 0) { Write-Fail "Failed to add: $ext"; exit 1 }
    }
    Write-OK $ext
}

# ── Step 3: Resources ─────────────────────────────────────────────────────────
Write-Step 'Checking Resources directory'

$repoRoot     = Split-Path $InstallerDir -Parent
$resourcesDir = Join-Path $repoRoot 'Resources'
if (-not (Test-Path $resourcesDir)) {
    New-Item -ItemType Directory -Path $resourcesDir | Out-Null
    Write-OK "Created: $resourcesDir"
}

# Placeholder icon.ico
$iconPath = Join-Path $resourcesDir 'icon.ico'
if (-not (Test-Path $iconPath)) {
    Write-Warn 'icon.ico not found -> creating minimal placeholder (replace with real 256x256 ICO)'
    [byte[]]$icoBytes = @(
        0x00,0x00, 0x01,0x00, 0x01,0x00,
        0x01, 0x01, 0x00, 0x00,
        0x01,0x00, 0x20,0x00,
        0x28,0x00,0x00,0x00,
        0x16,0x00,0x00,0x00
    )
    [byte[]]$bmpHdr = @(
        0x28,0x00,0x00,0x00,
        0x01,0x00,0x00,0x00, 0x02,0x00,0x00,0x00,
        0x01,0x00, 0x20,0x00,
        0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
    )
    [byte[]]$pixel   = @(0x00,0x00,0x00,0x00)
    [byte[]]$andMask = @(0x00,0x00,0x00,0x00)
    [System.IO.File]::WriteAllBytes($iconPath, ($icoBytes + $bmpHdr + $pixel + $andMask))
    Write-OK 'Placeholder icon.ico created'
}

# Placeholder logo.png
$logoPath = Join-Path $resourcesDir 'logo.png'
if (-not (Test-Path $logoPath)) {
    Write-Warn 'logo.png not found -> creating minimal placeholder (replace with 480x70 branded PNG)'
    [byte[]]$pngBytes = @(
        0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,
        0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
        0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
        0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
        0xDE,0x00,0x00,0x00,0x0C,0x49,0x44,0x41,
        0x54,0x08,0xD7,0x63,0xF8,0xCF,0xC0,0x00,
        0x00,0x00,0x02,0x00,0x01,0xE2,0x21,0xBC,
        0x33,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,
        0x44,0xAE,0x42,0x60,0x82
    )
    [System.IO.File]::WriteAllBytes($logoPath, $pngBytes)
    Write-OK 'Placeholder logo.png created'
}

# Placeholder banner.bmp and dialog.bmp
[byte[]]$bmpData = @(
    0x42,0x4D, 0x1E,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00, 0x1A,0x00,0x00,0x00,
    0x0C,0x00,0x00,0x00,
    0x01,0x00, 0x01,0x00,
    0x01,0x00, 0x18,0x00,
    0x00,0x00,0x00,0x00
)
foreach ($bmpName in @('banner.bmp','dialog.bmp')) {
    $bmpPath = Join-Path $resourcesDir $bmpName
    if (-not (Test-Path $bmpPath)) {
        [System.IO.File]::WriteAllBytes($bmpPath, $bmpData)
        Write-Warn "Placeholder $bmpName created (replace with branded BMP for production)"
    }
}

# ── Step 4: LICENSE.rtf ───────────────────────────────────────────────────────
Write-Step 'Checking LICENSE.rtf'

$licensePath = Join-Path $repoRoot 'LICENSE.rtf'
if (-not (Test-Path $licensePath)) {
    $rtfContent = @'
{\rtf1\ansi\deff0
{\fonttbl{\f0\fswiss\fcharset0 Segoe UI;}}
\f0\fs20
{\b\fs28 MIT License}\par\par
Copyright (c) 2024 JoppeJoppinger\par\par
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:\par\par
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.\par\par
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\par
}
'@
    Set-Content -Path $licensePath -Value $rtfContent -Encoding UTF8
    Write-OK 'LICENSE.rtf created'
} else {
    Write-OK 'LICENSE.rtf exists'
}

# ── Step 5: Clean ─────────────────────────────────────────────────────────────
$outputDir = Join-Path $InstallerDir 'Output'

if ($CleanFirst -and (Test-Path $outputDir)) {
    Write-Step 'Cleaning output directory'
    Remove-Item $outputDir -Recurse -Force
    Write-OK "Cleaned: $outputDir"
}

if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# ── Step 6: Build MSI ────────────────────────────────────────────────────────
Write-Step 'Building BlackHoleGame MSI'

$msiOutput = Join-Path $outputDir 'BlackHoleGame-1.0.0.msi'

Push-Location $InstallerDir
try {
    $wixArgs = @(
        'build',
        'Product\Product.wxs',
        'Product\Directories.wxs',
        'Product\Features.wxs',
        'Product\Shortcuts.wxs',
        'Product\RegistryEntries.wxs',
        'Product\EnvironmentVariables.wxs',
        'UI\CustomUI.wxs',
        '-ext', 'WixToolset.UI.wixext',
        '-ext', 'WixToolset.Util.wixext',
        '-o', $msiOutput
    )
    Write-Host "    Running: wix $($wixArgs -join ' ')" -ForegroundColor DarkGray
    & wix @wixArgs
    if ($LASTEXITCODE -ne 0) { Write-Fail "MSI build failed (exit $LASTEXITCODE)"; exit $LASTEXITCODE }
} finally {
    Pop-Location
}

Write-OK "MSI built: $msiOutput"

# ── Step 7: Build Bundle EXE ─────────────────────────────────────────────────
if (-not $SkipBundle) {
    Write-Step 'Building bootstrapper EXE'
    $exeOutput = Join-Path $outputDir 'BlackHoleGame-1.0.0-Setup.exe'

    Push-Location $InstallerDir
    try {
        $bundleArgs = @(
            'build',
            'Bundle\Bundle.wxs',
            '-ext', 'WixToolset.Bal.wixext',
            '-ext', 'WixToolset.Util.wixext',
            '-o', $exeOutput
        )
        Write-Host "    Running: wix $($bundleArgs -join ' ')" -ForegroundColor DarkGray
        & wix @bundleArgs
        if ($LASTEXITCODE -ne 0) { Write-Fail "Bundle build failed (exit $LASTEXITCODE)"; exit $LASTEXITCODE }
    } finally {
        Pop-Location
    }

    Write-OK "Setup.exe built: $exeOutput"
} else {
    Write-Warn 'Skipping Bundle build (-SkipBundle)'
}

# ── Step 8: Summary ───────────────────────────────────────────────────────────
Write-Step 'Build complete'
Write-Host ''
Get-FileSummary (Join-Path $outputDir 'BlackHoleGame-1.0.0.msi')
if (-not $SkipBundle) {
    Get-FileSummary (Join-Path $outputDir 'BlackHoleGame-1.0.0-Setup.exe')
}

Write-Host ''
Write-Host 'Output directory:' -ForegroundColor White
Write-Host "  $outputDir" -ForegroundColor Cyan
Write-Host ''
Write-Host 'Usage examples:' -ForegroundColor White
Write-Host '  Full install (GUI):    BlackHoleGame-1.0.0-Setup.exe' -ForegroundColor Gray
Write-Host '  Silent install:        BlackHoleGame-1.0.0-Setup.exe /quiet /norestart' -ForegroundColor Gray
Write-Host '  MSI only:              msiexec /i BlackHoleGame-1.0.0.msi /quiet' -ForegroundColor Gray
$customDir = 'INSTALLFOLDER="C:\Games\BlackHole\"'
Write-Host "  Custom directory:      msiexec /i BlackHoleGame-1.0.0.msi $customDir /quiet" -ForegroundColor Gray
Write-Host ''
