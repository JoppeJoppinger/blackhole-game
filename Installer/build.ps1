#Requires -Version 5.1
<#
.SYNOPSIS
    Build script for the Black Hole Game WiX v4 installer.

.DESCRIPTION
    Builds the BlackHoleGame-1.0.0.msi (MSI package) and
    BlackHoleGame-1.0.0-Setup.exe (WiX Bundle bootstrapper).

.PREREQUISITES
    - .NET 8 SDK  (https://dotnet.microsoft.com/download)
    - WiX Toolset v4  (installed automatically by this script)
    - Windows OS (building .msi/.exe requires Windows toolchain)

.NOTES
    Run this script from the Installer\ directory or use -InstallerDir.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Configuration Release -Verbose
#>

param(
    [string]$InstallerDir = $PSScriptRoot,
    [string]$Configuration = "Release",
    [switch]$SkipBundle,
    [switch]$CleanFirst
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────

function Write-Step { param([string]$msg) Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-OK   { param([string]$msg) Write-Host "    [OK] $msg" -ForegroundColor Green }
function Write-Warn { param([string]$msg) Write-Host "    [WARN] $msg" -ForegroundColor Yellow }
function Write-Fail { param([string]$msg) Write-Host "    [FAIL] $msg" -ForegroundColor Red }

function Get-FileSummary {
    param([string]$path)
    if (Test-Path $path) {
        $info = Get-Item $path
        $hash = (Get-FileHash $path -Algorithm SHA256).Hash
        $size = "{0:N0} KB" -f ($info.Length / 1KB)
        Write-Host ("    {0,-50} {1,10}   SHA256: {2}" -f $info.Name, $size, $hash) -ForegroundColor White
    } else {
        Write-Warn "Not found: $path"
    }
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 1: Check / install WiX Toolset v4
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Checking WiX Toolset v4"

$wixVersion = $null
try {
    $wixVersion = & wix --version 2>$null
} catch {}

if (-not $wixVersion) {
    Write-Warn "WiX Toolset not found. Installing via dotnet tool..."

    # Check .NET SDK first
    try { & dotnet --version | Out-Null }
    catch {
        Write-Fail ".NET SDK not found. Install from https://dotnet.microsoft.com/download"
        exit 1
    }

    & dotnet tool install --global wix
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "Failed to install WiX. Check dotnet tool output above."
        exit 1
    }

    # Refresh PATH
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User") + ";" + $env:PATH
    $wixVersion = & wix --version 2>$null
}

Write-OK "WiX version: $wixVersion"

# ─────────────────────────────────────────────────────────────────────────────
# Step 2: Ensure required WiX extensions
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Checking WiX extensions"

$extensions = @(
    "WixToolset.Bal.wixext",
    "WixToolset.UI.wixext",
    "WixToolset.Util.wixext"
)

foreach ($ext in $extensions) {
    $extList = & wix extension list 2>$null
    if ($extList -notmatch [regex]::Escape($ext)) {
        Write-Warn "$ext not installed — adding..."
        & wix extension add $ext
        if ($LASTEXITCODE -ne 0) {
            Write-Fail "Failed to add extension: $ext"
            exit 1
        }
    }
    Write-OK $ext
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 3: Ensure Resources/ directory and placeholder assets
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Checking Resources directory"

$repoRoot     = Split-Path $InstallerDir -Parent
$resourcesDir = Join-Path $repoRoot "Resources"

if (-not (Test-Path $resourcesDir)) {
    New-Item -ItemType Directory -Path $resourcesDir | Out-Null
    Write-OK "Created: $resourcesDir"
}

# Placeholder icon (1x1 transparent ICO — must be replaced for production)
$iconPath = Join-Path $resourcesDir "icon.ico"
if (-not (Test-Path $iconPath)) {
    Write-Warn "icon.ico not found — creating placeholder (replace with real 256x256 ICO for production)"
    # Minimal valid ICO header: 1 image, 1x1, 32-bit
    [byte[]]$icoBytes = @(
        0x00,0x00,         # Reserved
        0x01,0x00,         # Type: ICO
        0x01,0x00,         # Count: 1 image
        0x01,              # Width: 1px
        0x01,              # Height: 1px
        0x00,              # Color count: 0 (32-bit)
        0x00,              # Reserved
        0x01,0x00,         # Planes
        0x20,0x00,         # Bit count: 32
        0x28,0x00,0x00,0x00, # Size of image data
        0x16,0x00,0x00,0x00  # Offset to image data
    )
    # 40-byte BITMAPINFOHEADER + 4 bytes ARGB pixel + 4 bytes AND mask
    [byte[]]$bmpHeader = @(
        0x28,0x00,0x00,0x00, # Header size: 40
        0x01,0x00,0x00,0x00, # Width: 1
        0x02,0x00,0x00,0x00, # Height: 2 (x2 for ICO)
        0x01,0x00,           # Planes: 1
        0x20,0x00,           # Bit count: 32
        0x00,0x00,0x00,0x00, # Compression: none
        0x00,0x00,0x00,0x00, # Image size: 0 (uncompressed)
        0x00,0x00,0x00,0x00, # X pixels/meter
        0x00,0x00,0x00,0x00, # Y pixels/meter
        0x00,0x00,0x00,0x00, # Colors used
        0x00,0x00,0x00,0x00  # Important colors
    )
    [byte[]]$pixel    = @(0x00,0x00,0x00,0x00)  # Transparent BGRA pixel
    [byte[]]$andMask  = @(0x00,0x00,0x00,0x00)  # AND mask
    [System.IO.File]::WriteAllBytes($iconPath, $icoBytes + $bmpHeader + $pixel + $andMask)
    Write-OK "Placeholder icon.ico created"
}

# Placeholder logo.png (note for user)
$logoPath = Join-Path $resourcesDir "logo.png"
if (-not (Test-Path $logoPath)) {
    Write-Warn "logo.png not found — creating placeholder note"
    @"
PLACEHOLDER: Replace this file with a real PNG logo.
Required dimensions: 480 x 70 pixels (PNG format).
This logo is displayed in the WiX Bundle bootstrapper welcome screen.
"@ | Set-Content (Join-Path $resourcesDir "logo.png.README.txt")
    # Create a minimal 1x1 PNG as placeholder so the build doesn't fail
    # PNG signature + IHDR + IDAT + IEND for 1x1 transparent pixel
    [byte[]]$pngBytes = @(
        0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,  # PNG signature
        0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,  # IHDR chunk length + type
        0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,  # 1x1
        0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53, # 8-bit RGB + CRC
        0xDE,0x00,0x00,0x00,0x0C,0x49,0x44,0x41,  # IDAT chunk
        0x54,0x08,0xD7,0x63,0xF8,0xCF,0xC0,0x00,
        0x00,0x00,0x02,0x00,0x01,0xE2,0x21,0xBC,
        0x33,0x00,0x00,0x00,0x00,0x49,0x45,0x4E, # IEND chunk
        0x44,0xAE,0x42,0x60,0x82
    )
    [System.IO.File]::WriteAllBytes($logoPath, $pngBytes)
    Write-Warn "Minimal placeholder logo.png created (replace with 480x70 branded PNG)"
}

# Placeholder banner and dialog bitmaps for MSI UI
$bannerPath = Join-Path $resourcesDir "banner.bmp"
$dialogPath = Join-Path $resourcesDir "dialog.bmp"

foreach ($bmpPath in @($bannerPath, $dialogPath)) {
    if (-not (Test-Path $bmpPath)) {
        $bmpName = Split-Path $bmpPath -Leaf
        Write-Warn "$bmpName not found — creating placeholder"
        # Minimal valid BMP (1x1 black pixel, 24-bit)
        [byte[]]$bmpData = @(
            0x42,0x4D,           # BM signature
            0x1E,0x00,0x00,0x00, # File size: 30 bytes
            0x00,0x00,0x00,0x00, # Reserved
            0x1A,0x00,0x00,0x00, # Pixel data offset: 26
            0x0C,0x00,0x00,0x00, # BITMAPCOREHEADER size: 12
            0x01,0x00,           # Width: 1
            0x01,0x00,           # Height: 1
            0x01,0x00,           # Planes: 1
            0x18,0x00,           # Bit count: 24
            0x00,0x00,0x00,0x00  # 1 pixel: black (BGR + pad)
        )
        [System.IO.File]::WriteAllBytes($bmpPath, $bmpData)
        Write-Warn "Placeholder $bmpName created (replace with branded BMP for production)"
    }
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 4: Ensure LICENSE.rtf exists
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Checking LICENSE.rtf"

$licensePath = Join-Path $repoRoot "LICENSE.rtf"
if (-not (Test-Path $licensePath)) {
    Write-Warn "LICENSE.rtf not found — creating MIT license placeholder"
    @'
{\rtf1\ansi\deff0
{\fonttbl{\f0\fswiss\fprq2\fcharset0 Segoe UI;}{\f1\fmodern\fprq1\fcharset0 Courier New;}}
{\colortbl;\red0\green0\blue0;}
\f0\fs20
{\b\fs28 MIT License}\par
\par
Copyright (c) 2024 JoppeJoppinger\par
\par
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:\par
\par
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.\par
\par
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.\par
}
'@ | Set-Content $licensePath -Encoding UTF8
    Write-OK "LICENSE.rtf created"
} else {
    Write-OK "LICENSE.rtf exists"
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 5: Clean output directory
# ─────────────────────────────────────────────────────────────────────────────

$outputDir = Join-Path $InstallerDir "Output"

if ($CleanFirst -and (Test-Path $outputDir)) {
    Write-Step "Cleaning output directory"
    Remove-Item $outputDir -Recurse -Force
    Write-OK "Cleaned: $outputDir"
}

if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 6: Build the MSI
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Building BlackHoleGame MSI"

$msiOutput = Join-Path $outputDir "BlackHoleGame-1.0.0.msi"
$wixSources = @(
    "Product\Product.wxs",
    "Product\Directories.wxs",
    "Product\Features.wxs",
    "Product\Shortcuts.wxs",
    "Product\RegistryEntries.wxs",
    "Product\EnvironmentVariables.wxs",
    "UI\CustomUI.wxs",
    "Prerequisites\Git.wxs",
    "Prerequisites\VCRedist.wxs"
)

Push-Location $InstallerDir
try {
    $wixArgs = @("build") + $wixSources + @(
        "-ext", "WixToolset.UI.wixext",
        "-ext", "WixToolset.Util.wixext",
        "-o", $msiOutput
    )

    Write-Host "    Running: wix $($wixArgs -join ' ')" -ForegroundColor DarkGray
    & wix @wixArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Fail "MSI build failed (exit code $LASTEXITCODE)"
        Pop-Location
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}

Write-OK "MSI built: $msiOutput"

# ─────────────────────────────────────────────────────────────────────────────
# Step 7: Build the Bundle (bootstrapper EXE)
# ─────────────────────────────────────────────────────────────────────────────

if (-not $SkipBundle) {
    Write-Step "Building BlackHoleGame Bundle (bootstrapper EXE)"

    $exeOutput = Join-Path $outputDir "BlackHoleGame-1.0.0-Setup.exe"

    Push-Location $InstallerDir
    try {
        $bundleArgs = @(
            "build",
            "Bundle\Bundle.wxs",
            "-ext", "WixToolset.Bal.wixext",
            "-ext", "WixToolset.Util.wixext",
            "-o", $exeOutput
        )

        Write-Host "    Running: wix $($bundleArgs -join ' ')" -ForegroundColor DarkGray
        & wix @bundleArgs

        if ($LASTEXITCODE -ne 0) {
            Write-Fail "Bundle build failed (exit code $LASTEXITCODE)"
            Pop-Location
            exit $LASTEXITCODE
        }
    } finally {
        Pop-Location
    }

    Write-OK "Bundle EXE built: $exeOutput"
} else {
    Write-Warn "Skipping Bundle build (SkipBundle flag set)"
}

# ─────────────────────────────────────────────────────────────────────────────
# Step 8: Output summary
# ─────────────────────────────────────────────────────────────────────────────

Write-Step "Build complete — output files"
Write-Host ""
Write-Host "    File                                               Size         SHA256" -ForegroundColor Gray
Write-Host "    " + ("-" * 95) -ForegroundColor Gray

Get-FileSummary (Join-Path $outputDir "BlackHoleGame-1.0.0.msi")
if (-not $SkipBundle) {
    Get-FileSummary (Join-Path $outputDir "BlackHoleGame-1.0.0-Setup.exe")
}

Write-Host ""
Write-Host "Done! Installer files are in: $outputDir" -ForegroundColor Green
Write-Host ""
Write-Host "Silent install:   BlackHoleGame-1.0.0-Setup.exe /quiet /norestart" -ForegroundColor Yellow
Write-Host "MSI only:         msiexec /i BlackHoleGame-1.0.0.msi /quiet /norestart" -ForegroundColor Yellow
Write-Host "Custom dir:       msiexec /i BlackHoleGame-1.0.0.msi INSTALLFOLDER=""C:\Games\BlackHole\"" /quiet" -ForegroundColor Yellow
