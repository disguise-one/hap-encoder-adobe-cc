; Windows installer for the HAP plugins: After Effects output module + Premiere Pro / Media Encoder exporter.
; Build:  makensis /DVERSION=x.y.z /DAEX=<HAPPlugin.aex> /DPRM=<HAPPlugin.prm> installer\packaging\hap.nsi

!ifndef VERSION
  !error "pass /DVERSION=x.y.z"
!endif
!ifndef AEX
  !error "pass /DAEX=<path to HAPPlugin.aex>"
!endif
!ifndef PRM
  !error "pass /DPRM=<path to HAPPlugin.prm>"
!endif

Unicode true
!include "MUI2.nsh"
!include "x64.nsh"

!define PRODUCT "HAP for Adobe CC"
!define REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\HAPAdobeCC"
!define OLDREGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\HAPAfterEffects"
!define MEDIACORE "$PROGRAMFILES64\Adobe\Common\Plug-ins\7.0\MediaCore"
!define UNINSTALLER "Uninstall HAP.exe"

Name "${PRODUCT} ${VERSION}"
OutFile "HAP-Adobe-${VERSION}-win64.exe"
InstallDir "${MEDIACORE}\HAP"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_FINISHPAGE_TEXT "Restart After Effects, Premiere Pro and Media Encoder.$\r$\n$\r$\nAfter Effects: Render Queue > Output Module > Format > HAP Movie.$\r$\nPremiere Pro / Media Encoder: Export > Format > HAP Video."

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\license.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "${PRODUCT} needs 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd

Section "Install"
  ; the install dir is fixed: the Adobe apps only scan the MediaCore folder
  SetOutPath "${MEDIACORE}\HAP"
  ; a running Adobe app holds the plugins open; if a copy was skipped (Ignore, or a silent
  ; install), stop before touching registration so the old version stays consistently installed
  ClearErrors
  File "/oname=HAPPlugin.aex" "${AEX}"
  File "/oname=HAPPlugin.prm" "${PRM}"
  IfErrors 0 plugins_copied
    MessageBox MB_ICONSTOP "The HAP plugins could not be installed because they are in use. Close After Effects, Premiere Pro and Media Encoder, then run the installer again." /SD IDOK
    SetErrorLevel 2
    Abort
  plugins_copied:
  ; the exporter copies these into each Media Encoder version's preset folder on load
  SetOutPath "${MEDIACORE}\HAP\Presets"
  File "..\..\asset\encoder_preset\ame-12.0\*.epr"

  ; replace a 1.3.0 (After Effects-only) install rather than leaving two entries
  Delete "${MEDIACORE}\HAP\Uninstall HAP for After Effects.exe"
  DeleteRegKey HKLM "${OLDREGKEY}"

  WriteUninstaller "${MEDIACORE}\HAP\${UNINSTALLER}"
  WriteRegStr HKLM "${REGKEY}" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "${REGKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${REGKEY}" "Publisher" "HAP community"
  WriteRegStr HKLM "${REGKEY}" "URLInfoAbout" "https://github.com/jameslashmar/hap-encoder-adobe-cc"
  WriteRegStr HKLM "${REGKEY}" "InstallLocation" "${MEDIACORE}\HAP"
  WriteRegStr HKLM "${REGKEY}" "UninstallString" '"${MEDIACORE}\HAP\${UNINSTALLER}"'
  WriteRegDWORD HKLM "${REGKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REGKEY}" "NoRepair" 1
SectionEnd

Function un.onInit
  SetRegView 64
FunctionEnd

Section "Uninstall"
  ; a running Adobe app holds the plugins open; if either can't be removed, stop here so the
  ; uninstaller and its Add/Remove Programs entry survive for another try
  ClearErrors
  Delete "${MEDIACORE}\HAP\HAPPlugin.aex"
  Delete "${MEDIACORE}\HAP\HAPPlugin.prm"
  IfErrors 0 plugins_removed
    MessageBox MB_ICONSTOP "The HAP plugins are in use. Close After Effects, Premiere Pro and Media Encoder, then run the uninstaller again." /SD IDOK
    SetErrorLevel 2
    Abort
  plugins_removed:
  Delete "${MEDIACORE}\HAP\Presets\*.epr"
  RMDir "${MEDIACORE}\HAP\Presets"
  Delete "${MEDIACORE}\HAP\${UNINSTALLER}"
  RMDir "${MEDIACORE}\HAP"
  DeleteRegKey HKLM "${REGKEY}"
  ; presets already copied into Documents\Adobe\Adobe Media Encoder are the user's; left in place
SectionEnd
