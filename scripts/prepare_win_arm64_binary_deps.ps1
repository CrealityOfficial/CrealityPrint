param(
    [string]$VcpkgRoot = "C:\work\vcpkg",
    [string]$RepoRoot = (Resolve-Path "$PSScriptRoot\..").Path,
    [string]$OrcaSlicerRoot = "C:\work\OrcaSlicer",
    [switch]$SkipCrTpmsCheck
)

$ErrorActionPreference = "Stop"
trap {
    Write-Error $_
    exit 1
}

$vsRoot = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
$msvcRoot = Join-Path $vsRoot "VC\Tools\MSVC"
$msvcVersion = Get-ChildItem $msvcRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
if (-not $msvcVersion) {
    throw "MSVC tools not found under: $msvcRoot"
}

$toolDir = Join-Path $msvcVersion.FullName "bin\Hostx64\arm64"
$dumpbin = Join-Path $toolDir "dumpbin.exe"
$lib = Join-Path $toolDir "lib.exe"
foreach ($tool in @($dumpbin, $lib)) {
    if (-not (Test-Path $tool)) {
        throw "Required ARM64 MSVC tool is missing: $tool"
    }
}

$gmpBin = Join-Path $VcpkgRoot "installed\arm64-windows-release\bin\gmp-10.dll"
$mpfrBin = Join-Path $VcpkgRoot "packages\mpfr_arm64-windows-release\bin\mpfr-6.dll"
foreach ($path in @($gmpBin, $mpfrBin)) {
    if (-not (Test-Path $path)) {
        throw "Required ARM64 vcpkg dependency file is missing: $path"
    }
}

$gmpDest = Join-Path $RepoRoot "deps\GMP\gmp\lib\win-arm64"
$mpfrDest = Join-Path $RepoRoot "deps\MPFR\mpfr\lib\winARM64"

foreach ($dir in @($gmpDest, $mpfrDest)) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
}

Copy-Item -Force $gmpBin (Join-Path $gmpDest "libgmp-10.dll")
Copy-Item -Force $mpfrBin (Join-Path $mpfrDest "libmpfr-4.dll")

$work = Join-Path $RepoRoot "build-arm64-deps-importlibs"
New-Item -ItemType Directory -Force -Path $work | Out-Null

function New-ImportLibrary {
    param(
        [string]$DllPath,
        [string]$DllName,
        [string]$OutLib
    )

    $defPath = Join-Path $work ([IO.Path]::GetFileNameWithoutExtension($DllName) + ".def")
    $dumpPath = Join-Path $work ([IO.Path]::GetFileNameWithoutExtension($DllName) + ".exports.txt")

    & $dumpbin /exports $DllPath | Set-Content -Encoding ASCII $dumpPath

    $exports = Get-Content $dumpPath |
        ForEach-Object {
            if ($_ -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)(?:\s+=.*)?$') {
                $matches[1]
            }
        } |
        Where-Object { $_ -and $_ -notmatch '^\?' } |
        Sort-Object -Unique

    if (-not $exports) {
        throw "No exports found in $DllPath"
    }

    @("LIBRARY $DllName", "EXPORTS") + $exports | Set-Content -Encoding ASCII $defPath
    & $lib /machine:ARM64 /def:$defPath /out:$OutLib | Out-Host
    if (-not (Test-Path $OutLib)) {
        throw "Failed to create import library: $OutLib"
    }

    $expPath = [IO.Path]::ChangeExtension($OutLib, ".exp")
    if (Test-Path $expPath) {
        Remove-Item -Force $expPath
    }
}

New-ImportLibrary `
    -DllPath (Join-Path $gmpDest "libgmp-10.dll") `
    -DllName "libgmp-10.dll" `
    -OutLib (Join-Path $gmpDest "libgmp-10.lib")

New-ImportLibrary `
    -DllPath (Join-Path $mpfrDest "libmpfr-4.dll") `
    -DllName "libmpfr-4.dll" `
    -OutLib (Join-Path $mpfrDest "libmpfr-4.lib")

$webView2Source = Join-Path $OrcaSlicerRoot "deps\WebView2\lib\win-arm64"
$webView2Dest = Join-Path $RepoRoot "deps\WebView2\lib\winARM64"
if (Test-Path $webView2Source) {
    New-Item -ItemType Directory -Force -Path $webView2Dest | Out-Null
    Copy-Item -Force (Join-Path $webView2Source "*") $webView2Dest
} elseif (-not (Test-Path (Join-Path $webView2Dest "WebView2Loader.dll"))) {
    throw "ARM64 WebView2Loader.dll is missing. Expected source: $webView2Source"
}

$crTpmsDir = Join-Path $RepoRoot "deps\CR_TPMS\cr_tpms\lib\winARM64"
if (-not $SkipCrTpmsCheck -and -not (Test-Path (Join-Path $crTpmsDir "cr_tpms_library.lib"))) {
    throw "Missing Windows ARM64 CR_TPMS library: $crTpmsDir. Build cannot link TPMS without cr_tpms_library.lib/.dll."
}

Remove-Item -Recurse -Force $work

Write-Host "Prepared Windows ARM64 binary dependencies."
