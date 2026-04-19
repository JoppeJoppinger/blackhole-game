; BlackHoleGame NSIS Installer Script — Godot 4 Build
; Compiled with makensis

Unicode True
SetCompressor /SOLID lzma
RequestExecutionLevel admin

!include "MUI2.nsh"
!include "LogicLib.nsh"

;--------------------------------
; General

Name "Black Hole Game"
OutFile "BlackHoleGame-Setup.exe"
InstallDir "C:\Games\BlackHoleGame"
InstallDirRegKey HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame" "InstallPath"
BrandingText "JoppeJoppinger - Black Hole Game v1.0.0"

;--------------------------------
; MUI Settings

!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Black Hole Game Installer"
!define MUI_WELCOMEPAGE_TEXT "This wizard will install the Black Hole Game on your computer.$\r$\n$\r$\nA hyper-realistic Kerr black hole simulator built with Godot 4.$\r$\n$\r$\nControls: W=Thrust, S=Brake, Mouse=Aim, Q/E=Roll, V=Camera, Esc=Mouse$\r$\n$\r$\nClick Next to continue."
!define MUI_FINISHPAGE_RUN "$INSTDIR\BlackHoleGame.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Black Hole Game"

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

  ; Install the Godot-built game exe
  File "BlackHoleGame.exe"

  ; Write registry entries for Add/Remove Programs
  WriteRegStr HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "DisplayName" "Black Hole Game"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "DisplayVersion" "1.0.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "Publisher" "JoppeJoppinger"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "URLInfoAbout" "https://github.com/JoppeJoppinger/blackhole-game"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame" "NoRepair" 1

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Create Desktop shortcut pointing to the game exe
  CreateShortcut "$DESKTOP\Black Hole Game.lnk" "$INSTDIR\BlackHoleGame.exe"

  ; Create Start Menu folder
  CreateDirectory "$SMPROGRAMS\Black Hole Game"
  CreateShortcut "$SMPROGRAMS\Black Hole Game\Black Hole Game.lnk" "$INSTDIR\BlackHoleGame.exe"
  CreateShortcut "$SMPROGRAMS\Black Hole Game\Uninstall Black Hole Game.lnk" "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"
  Delete "$INSTDIR\BlackHoleGame.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$DESKTOP\Black Hole Game.lnk"
  RMDir /r "$SMPROGRAMS\Black Hole Game"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlackHoleGame"
  DeleteRegKey HKLM "SOFTWARE\JoppeJoppinger\BlackHoleGame"
SectionEnd
