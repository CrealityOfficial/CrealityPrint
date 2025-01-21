!include "MUI2.nsh"

!define WebView2Installer "@CPACK_WEBVIEW2_PATH@"

OutFile "MyAppInstaller.exe"
InstallDir "$PROGRAMFILES\\MyApp"

Section "Install"
  SetOutPath $INSTDIR

  # 检查 WebView2 是否安装
  ExecDos::exec '"$WINDIR\\System32\\reg.exe" query "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F8B38191-1DC4-4389-53A6-A2AMC1C2CBF9ED9B}"'"" ExitCode
  StrCmp $ExitCode "0" WebView2Installed WebView2NotInstalled

WebView2Installed:
  ; 如果 WebView2 已安装，执行其他安装步骤或提示成功信息
  MessageBox MB_OK "WebView2 已安装。" IDOK
  Goto end

WebView2NotInstalled:
  ; 如果未安装 WebView2，则运行安装
  ExecWait '"$INSTDIR\\${WebView2Installer}" /silent /install'
  
  ; 再次检查是否成功安装
  ExecDos::exec '"$WINDIR\\System32\\reg.exe" query "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F8B38191-1DC4-4389-53A6-A2AMC1C2CBF9ED9B}"'"" ExitCode
  StrCmp $ExitCode "0" WebView2InstallSuccess WebView2InstallFailure

WebView2InstallSuccess:
  MessageBox MB_OK  "WebView2 成功安装。" IDOK
  Goto end

WebView2InstallFailure:
  MessageBox MB_OK|MB_ICONEXCLAMATION "WebView2安装失败， 请手动安装。"