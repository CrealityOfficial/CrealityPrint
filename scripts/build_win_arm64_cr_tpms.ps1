param(
    [string]$RepoUrl = "http://hmiao@172.20.180.12:8050/a/yanfa4/core/cr_tpms_library",
    [string]$SourceDir = "C:\work\cr_tpms_library",
    [string]$C3DSlicerRoot = (Resolve-Path "$PSScriptRoot\..").Path
)

$ErrorActionPreference = "Stop"
trap {
    Write-Error $_
    exit 1
}

if (Test-Path $SourceDir) {
    git -C $SourceDir pull --ff-only
} else {
    git clone $RepoUrl $SourceDir
}

$vsRoot = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
$vcvars = Join-Path $vsRoot "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvars)) {
    throw "VS 2022 vcvarsall.bat not found: $vcvars"
}

$msvcRoot = Join-Path $vsRoot "VC\Tools\MSVC"
$msvcVersion = Get-ChildItem $msvcRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
if (-not $msvcVersion) {
    throw "MSVC tools not found under: $msvcRoot"
}

$cl = Join-Path $msvcVersion.FullName "bin\Hostx64\arm64\cl.exe"
if (-not (Test-Path $cl)) {
    throw "ARM64 compiler cl.exe not found: $cl"
}

$buildDir = Join-Path $SourceDir "build-arm64-dll"
$binDir = Join-Path $buildDir "bin"
$libDir = Join-Path $buildDir "lib"
$objDir = Join-Path $buildDir "obj"
New-Item -ItemType Directory -Force -Path $binDir, $libDir, $objDir | Out-Null

$source = Join-Path $SourceDir "src\cr_tpms_library.cpp"
$include = Join-Path $SourceDir "include"
$dll = Join-Path $binDir "cr_tpms_library.dll"
$lib = Join-Path $libDir "cr_tpms_library.lib"
$obj = Join-Path $objDir "cr_tpms_library.obj"

$cmd = @"
@echo on
set VSINSTALLDIR=
set VCINSTALLDIR=
set VCToolsInstallDir=
set VSCMD_ARG_TGT_ARCH=
set VSCMD_ARG_HOST_ARCH=
call "$vcvars" x64_arm64
cd /d "$SourceDir"
"$cl" /nologo /LD /EHsc /std:c++14 /DCR_TPMS_LIBRARY_EXPORTS /I"$include" "$source" /Fo"$obj" /Fe"$dll" /link /IMPLIB:"$lib"
"@

$cmdPath = Join-Path $buildDir "build-arm64.cmd"
Set-Content -Encoding ASCII -Path $cmdPath -Value $cmd
cmd /v:on /c "`"$cmdPath`""
if ($LASTEXITCODE -ne 0) {
    throw "Failed to build cr_tpms_library for Windows ARM64."
}

$dest = Join-Path $C3DSlicerRoot "deps\CR_TPMS\cr_tpms\lib\winARM64"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -Force $dll (Join-Path $dest "cr_tpms_library.dll")
Copy-Item -Force $lib (Join-Path $dest "cr_tpms_library.lib")
Copy-Item -Force (Join-Path $SourceDir "include\cr_tpms_library.h") `
    (Join-Path $C3DSlicerRoot "deps\CR_TPMS\cr_tpms\include\cr_tpms_library.h")

Write-Host "Installed Windows ARM64 CR_TPMS library to: $dest"
