Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define PATCH_NAME "Creality Print WebView2 Compatibility Patch"
!define APP_EXE "CrealityPrint.exe"
!define REGISTRY_KEY "CrealityPrint-7"
!define VENDOR "Creality"
!define EDGE_FIXED_SOURCE "C:\work\C3DSlicer\out\weiyusuo-relwithdeb\build\src\edge_fixed"

Name "${PATCH_NAME}"
OutFile "C:\work\C3DSlicer\out\weiyusuo-relwithdeb\build\CrealityPrint_WebView2_CompatibilityPatch.exe"
InstallDir "$PROGRAMFILES64\Creality\Creality Print 7.0 Alpha"
InstallDirRegKey HKLM "Software\${VENDOR}\${REGISTRY_KEY}" ""
ShowInstDetails show
BrandingText "Creality"

!define MUI_ICON "icon\NSIS.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "icon\NSISHeader.ico"
!define MUI_ABORTWARNING
!define MUI_PAGE_CUSTOMFUNCTION_PRE DirectoryPre
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

Var DetectedDir

Function .onInit
  SetShellVarContext all
  StrCpy $DetectedDir ""

  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  ReadRegStr $DetectedDir HKLM "Software\${VENDOR}\${REGISTRY_KEY}" ""
  ${If} "$DetectedDir" == ""
    ReadRegStr $DetectedDir HKCU "Software\${VENDOR}\${REGISTRY_KEY}" ""
  ${EndIf}
  ${If} "$DetectedDir" == ""
    ReadRegStr $DetectedDir HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}" "InstallLocation"
  ${EndIf}
  ${If} "$DetectedDir" == ""
    ReadRegStr $DetectedDir HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}" "InstallLocation"
  ${EndIf}

  ${If} "$DetectedDir" == ""
    ${If} ${FileExists} "$PROGRAMFILES64\Creality\Creality Print 7.2\${APP_EXE}"
      StrCpy $DetectedDir "$PROGRAMFILES64\Creality\Creality Print 7.2"
    ${ElseIf} ${FileExists} "$PROGRAMFILES64\Creality\Creality Print 7.1\${APP_EXE}"
      StrCpy $DetectedDir "$PROGRAMFILES64\Creality\Creality Print 7.1"
    ${ElseIf} ${FileExists} "$PROGRAMFILES64\Creality\Creality Print 7.0 Alpha\${APP_EXE}"
      StrCpy $DetectedDir "$PROGRAMFILES64\Creality\Creality Print 7.0 Alpha"
    ${ElseIf} ${FileExists} "$PROGRAMFILES64\Creality\Creality Print\${APP_EXE}"
      StrCpy $DetectedDir "$PROGRAMFILES64\Creality\Creality Print"
    ${EndIf}
  ${EndIf}

  ${If} "$DetectedDir" != ""
    StrCpy $INSTDIR "$DetectedDir"
  ${EndIf}
FunctionEnd

Function DirectoryPre
  ${If} "$DetectedDir" == ""
    MessageBox MB_ICONEXCLAMATION|MB_OK "Creality Print install directory was not found automatically. Please select the folder that contains ${APP_EXE}."
  ${EndIf}
FunctionEnd

Section "Install edge_fixed runtime" SecInstall
  ${IfNot} ${FileExists} "$INSTDIR\${APP_EXE}"
    MessageBox MB_ICONSTOP|MB_OK "The selected directory does not contain ${APP_EXE}:$\r$\n$INSTDIR$\r$\n$\r$\nPlease run this patch again and select the Creality Print install directory."
    Abort
  ${EndIf}

  DetailPrint "Installing WebView2 fixed runtime to: $INSTDIR\edge_fixed"
  RMDir /r "$INSTDIR\edge_fixed"
  SetOutPath "$INSTDIR\edge_fixed"
  File /r "${EDGE_FIXED_SOURCE}\*.*"

  DetailPrint "Done. Creality Print will use $INSTDIR\edge_fixed for WebView2."
  MessageBox MB_ICONINFORMATION|MB_OK "WebView2 compatibility runtime has been installed.$\r$\n$\r$\nPlease restart Creality Print."
SectionEnd
