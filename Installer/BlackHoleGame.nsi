; BlackHoleGame NSIS Installer Script
; Compiled with makensis

Unicode True
SetCompressor /SOLID lzma
RequestExecutionLevel admin

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"

;--------------------------------
; General

Name "Black Hole Game - UE5 Project Installer"
OutFile "BlackHoleGame-Setup.exe"
InstallDir "C:\Games\BlackHoleGame"
InstallDirRegKey HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame" "InstallPath"
BrandingText "JoppeJoppinger - Black Hole Game v1.0.0"

;--------------------------------
; MUI Settings

!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Black Hole Game - UE5 Project Installer"
!define MUI_WELCOMEPAGE_TEXT "This wizard will install the Black Hole Game UE5 project on your computer.$\r$\n$\r$\nThe installer will also download and install required prerequisites (Git for Windows, Visual C++ 2022 Redistributable) if not already present.$\r$\n$\r$\nClick Next to continue."
!define MUI_FINISHPAGE_RUN_TEXT "Open README"
!define MUI_FINISHPAGE_RUN "$INSTDIR\README.md"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Section

Section "Black Hole Game" SecMain
  SectionIn RO

  SetOutPath "$INSTDIR"

  ; Install prerequisites
  
  ; Check Git
  ReadRegStr $0 HKLM "SOFTWARE\GitForWindows" "InstallPath"
  ${If} $0 == ""
    DetailPrint "Downloading Git for Windows..."
    NSISdl::download "https://github.com/git-for-windows/git/releases/download/v2.44.0.windows.1/Git-2.44.0-64-bit.exe" "$TEMP\GitInstaller.exe"
    Pop $0
    ${If} $0 == "success"
      DetailPrint "Installing Git for Windows silently..."
      ExecWait '"$TEMP\GitInstaller.exe" /VERYSILENT /NORESTART /NOCANCEL /SP-'
    ${Else}
      DetailPrint "Git download failed: $0 - skipping"
    ${EndIf}
  ${Else}
    DetailPrint "Git for Windows already installed, skipping."
  ${EndIf}

  ; Check VCRedist
  ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64" "Installed"
  ${If} $0 != 1
    DetailPrint "Downloading Visual C++ 2022 Redistributable..."
    NSISdl::download "https://aka.ms/vs/17/release/vc_redist.x64.exe" "$TEMP\VCRedist.exe"
    Pop $0
    ${If} $0 == "success"
      DetailPrint "Installing Visual C++ 2022 Redistributable silently..."
      ExecWait '"$TEMP\VCRedist.exe" /install /quiet /norestart'
    ${Else}
      DetailPrint "VCRedist download failed: $0 - skipping"
    ${EndIf}
  ${Else}
    DetailPrint "Visual C++ Redistributable already installed, skipping."
  ${EndIf}

  ; Install project files
  DetailPrint "Installing project files..."

  ; Individual files
  File "..\BlackHoleGame.uproject"
  File "..\README.md"
  File "..\SETUP.md"
  File "..\setup_windows.ps1"

  ; Source directory
  SetOutPath "$INSTDIR\Source"
  File /r "..\Source\*.*"

  ; Config directory
  SetOutPath "$INSTDIR\Config"
  File /r "..\Config\*.*"

  ; Shaders directory
  SetOutPath "$INSTDIR\Shaders"
  File /r "..\Shaders\*.*"

  ; Content directory
  SetOutPath "$INSTDIR\Content"
  File /r "..\Content\*.*"

  ; Return to install root
  SetOutPath "$INSTDIR"

  ; Write registry entries for Add/Remove Programs
  WriteRegStr HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "DisplayName" "Black Hole Game"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "DisplayVersion" "1.0.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "Publisher" "JoppeJoppinger"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "URLInfoAbout" "https://github.com/JoppeJoppinger/blackhole-game"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "NoRepair" 1

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Create Desktop shortcut
  CreateShortcut "$DESKTOP\Black Hole Game.lnk" \
    "C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe" \
    '"$INSTDIR\BlackHoleGame.uproject"' \
    "$INSTDIR\BlackHoleGame.uproject" 0

  ; Create Start Menu folder
  CreateDirectory "$SMPROGRAMS\Black Hole Game"
  CreateShortcut "$SMPROGRAMS\Black Hole Game\Black Hole Game.lnk" \
    "C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe" \
    '"$INSTDIR\BlackHoleGame.uproject"' \
    "$INSTDIR\BlackHoleGame.uproject" 0
  CreateShortcut "$SMPROGRAMS\Black Hole Game\Uninstall Black Hole Game.lnk" \
    "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\Black Hole Game.lnk"
  RMDir /r "$SMPROGRAMS\Black Hole Game"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame"
  DeleteRegKey HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame"
SectionEnd
