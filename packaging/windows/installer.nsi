!include "MUI2.nsh"
!include "FileFunc.nsh"

Unicode true
Target amd64-unicode
SetCompressor /SOLID lzma

!define PRODUCT_NAME "HexEd"
!define PRODUCT_VERSION "1.3"
!define PRODUCT_PUBLISHER "Rishav Ambasta"
!define PRODUCT_WEB_SITE "https://github.com/rishavambasta/hexed"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\hexed.exe"

!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

Name "${PRODUCT_NAME} v${PRODUCT_VERSION}"
OutFile "..\..\dist\HexEd-${PRODUCT_VERSION}-Setup-win64.exe"

InstallDir "$PROGRAMFILES64\HexEd"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

!define MUI_ICON "..\..\assets\icons\hexed.ico"
!define MUI_UNICON "..\..\assets\icons\hexed.ico"
!define MUI_ABORTWARNING

; Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\hexed.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "..\..\build\win64_staging\hexed.exe"
  File "..\..\build\win64_staging\README.md"
  File "..\..\build\win64_staging\*.dll"

  SetOutPath "$INSTDIR\etc"
  File /r "..\..\build\win64_staging\etc\*.*"

  SetOutPath "$INSTDIR\lib"
  File /r "..\..\build\win64_staging\lib\*.*"

  SetOutPath "$INSTDIR\share"
  File /r "..\..\build\win64_staging\share\*.*"

  SetOutPath "$INSTDIR\assets"
  File /r "..\..\build\win64_staging\assets\*.*"

  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\HexEd"
  CreateShortcut "$SMPROGRAMS\HexEd\HexEd.lnk" "$INSTDIR\hexed.exe" "" "$INSTDIR\assets\icons\hexed.ico" 0
  CreateShortcut "$SMPROGRAMS\HexEd\Uninstall HexEd.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
  CreateShortcut "$DESKTOP\HexEd.lnk" "$INSTDIR\hexed.exe" "" "$INSTDIR\assets\icons\hexed.ico" 0

  ; Write registry keys for Windows Add/Remove Programs
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\hexed.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME} v${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\hexed.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"

  WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

Section Uninstall
  Delete "$DESKTOP\HexEd.lnk"
  Delete "$SMPROGRAMS\HexEd\HexEd.lnk"
  Delete "$SMPROGRAMS\HexEd\Uninstall HexEd.lnk"
  RMDir "$SMPROGRAMS\HexEd"

  Delete "$INSTDIR\hexed.exe"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\uninst.exe"
  RMDir /r "$INSTDIR\etc"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\share"
  RMDir /r "$INSTDIR\assets"
  RMDir "$INSTDIR"

  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
