; Euro1943.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install Euro1943.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "Euro1943"

; The file to write
OutFile "Euro1943_install.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Euro1943

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Euro1943" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Euro1943 (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File /r /x Euro1943_install.* *.*
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Euro1943 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euro1943" "DisplayName" "Euro1943"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euro1943" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euro1943" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euro1943" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Euro1943"
  CreateShortCut "$SMPROGRAMS\Euro1943\Euro1943.lnk" "$INSTDIR\Euro1943.exe" "" "$INSTDIR\Euro1943.exe" 0
  CreateShortCut "$SMPROGRAMS\Euro1943\Map Editor.lnk" "$INSTDIR\mapedit.exe" "" "$INSTDIR\mapedit.exe" 0
  CreateShortCut "$SMPROGRAMS\Euro1943\Readme.lnk" "$INSTDIR\readme.txt" "" "$INSTDIR\readme.txt" 0
  CreateShortCut "$SMPROGRAMS\Euro1943\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
;  SetOutPath "$INSTDIR\server"
;  CreateShortCut "$SMPROGRAMS\Euro1943\Start Server.lnk" "$INSTDIR\server\server.exe" "" "$INSTDIR\server\server.exe" 0
  CreateShortCut "$SMPROGRAMS\Euro1943\Start Server.lnk" "$INSTDIR\server.exe" "" "$INSTDIR\server.exe" 0
;  SetOutPath "$INSTDIR"
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euro1943"
  DeleteRegKey HKLM SOFTWARE\Euro1943

  ; Remove files and uninstaller
  ;Delete $INSTDIR\*.*

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Euro1943\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Euro1943"
  RMDir /r "$INSTDIR"

SectionEnd
