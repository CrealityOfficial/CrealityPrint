#requires -Version 7.0

[CmdletBinding()]
param(
    [Parameter()]
    [ValidateSet('All', 'Dependencies', 'Application')]
    [string]$Target = 'All',

    [Parameter()]
    [switch]$Fresh,

    [Parameter()]
    [string]$VisualStudioPath,

    [Parameter()]
    [string]$PerlPath = 'C:\Strawberry\perl\bin\perl.exe',

    [Parameter()]
    [string]$ToolsetVersion = '14.44.35207',

    [Parameter()]
    [string]$WindowsSdkVersion = '10.0.26100.0',

    [Parameter()]
    [ValidateRange(1, 64)]
    [int]$Jobs = 8,

    [Parameter()]
    [ValidateRange(1, 1024)]
    [int]$MinimumFreeGiB = 150,

    [Parameter()]
    [string]$BuildRecordPath,

    [Parameter()]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:OutputMarkerSchema = 'crealityprint-vs2026-generated-output/v1'
$script:PresetBase = 'windows-vs2026-v143-x64-release'

function Resolve-ContainedGeneratedPath {
    param(
        [Parameter(Mandatory)]
        [string]$Candidate,

        [Parameter(Mandatory)]
        [string]$OutputRoot
    )

    $fullCandidate = [IO.Path]::GetFullPath($Candidate).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar
    )
    $fullOutputRoot = [IO.Path]::GetFullPath($OutputRoot).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar
    )
    $prefix = $fullOutputRoot + [IO.Path]::DirectorySeparatorChar

    if (-not $fullCandidate.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Generated path is outside the repository output root: $fullCandidate"
    }

    if ($fullCandidate.Equals($fullOutputRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The output root itself is never a valid generated-directory target.'
    }

    return $fullCandidate
}

function Assert-NoReparsePointInOutputPath {
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$OutputRoot
    )

    $current = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetFullPath($OutputRoot)

    while ($current.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        if (Test-Path -LiteralPath $current) {
            $item = Get-Item -LiteralPath $current -Force
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing generated-path operation through a reparse point: $current"
            }
        }

        if ($current.Equals($root, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }

        $parent = [IO.Directory]::GetParent($current)
        if ($null -eq $parent) {
            throw "Unable to prove generated-path ancestry for: $Path"
        }
        $current = $parent.FullName
    }
}

function Assert-NoReparsePointDescendants {
    param([Parameter(Mandatory)][string]$Root)

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        return
    }
    foreach ($item in Get-ChildItem -LiteralPath $Root -Force -Recurse) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing recursive cleanup of a tree containing a reparse point: $($item.FullName)"
        }
    }
}

function Get-ContainedDescendantReparsePoints {
    param([Parameter(Mandatory)][string]$Root)

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        return @()
    }
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $fullRoot + [IO.Path]::DirectorySeparatorChar
    $items = @(
        Get-ChildItem -LiteralPath $fullRoot -Force -Recurse |
            Where-Object {
                ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
            } |
            Sort-Object { $_.FullName.Length } -Descending
    )
    foreach ($item in $items) {
        $fullPath = [IO.Path]::GetFullPath($item.FullName)
        if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Descendant reparse point escaped the generated root: $fullPath"
        }
    }
    return @($items)
}

function Remove-ContainedDescendantReparsePoints {
    param([Parameter(Mandatory)][string]$Root)

    $items = @(Get-ContainedDescendantReparsePoints -Root $Root)
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $fullRoot + [IO.Path]::DirectorySeparatorChar
    foreach ($item in $items) {
        $current = Get-Item -LiteralPath $item.FullName -Force
        $fullPath = [IO.Path]::GetFullPath($current.FullName)
        if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) -or
            ($current.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) {
            throw "Generated descendant changed during guarded reparse-point cleanup: $fullPath"
        }
        if ($current.PSIsContainer) {
            [IO.Directory]::Delete($fullPath, $false)
        }
        else {
            [IO.File]::Delete($fullPath)
        }
    }
    Assert-NoReparsePointDescendants -Root $fullRoot
}

function Get-OutputMarkerPath {
    param(
        [Parameter(Mandatory)]
        [string]$OutputRoot,

        [Parameter(Mandatory)]
        [string]$Preset,

        [Parameter(Mandatory)]
        [string]$Kind
    )

    return Join-Path $OutputRoot ".crealityprint-vs2026-markers\$Preset-$Kind.json"
}

function Write-OutputMarker {
    param(
        [Parameter(Mandatory)]
        [string]$MarkerPath,

        [Parameter(Mandatory)]
        [string]$OutputRoot,

        [Parameter(Mandatory)]
        [string]$RepositoryRoot,

        [Parameter(Mandatory)]
        [string]$TargetPath,

        [Parameter(Mandatory)]
        [string]$Kind
    )

    $safeMarkerPath = Resolve-ContainedGeneratedPath -Candidate $MarkerPath -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safeMarkerPath -OutputRoot $OutputRoot
    $markerDirectory = Split-Path -Parent $safeMarkerPath
    $null = New-Item -ItemType Directory -Path $markerDirectory -Force
    [ordered]@{
        schema = $script:OutputMarkerSchema
        repository = [IO.Path]::GetFullPath($RepositoryRoot)
        target = [IO.Path]::GetFullPath($TargetPath)
        kind = $Kind
    } | ConvertTo-Json | Set-Content -LiteralPath $safeMarkerPath -Encoding utf8NoBOM
}

function Assert-OutputMarker {
    param(
        [Parameter(Mandatory)]
        [string]$MarkerPath,

        [Parameter(Mandatory)]
        [string]$OutputRoot,

        [Parameter(Mandatory)]
        [string]$RepositoryRoot,

        [Parameter(Mandatory)]
        [string]$TargetPath
    )

    $safeMarkerPath = Resolve-ContainedGeneratedPath -Candidate $MarkerPath -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safeMarkerPath -OutputRoot $OutputRoot
    if (-not (Test-Path -LiteralPath $safeMarkerPath -PathType Leaf)) {
        throw "Refusing to delete an unmarked generated directory: $TargetPath"
    }

    $marker = Get-Content -LiteralPath $safeMarkerPath -Raw | ConvertFrom-Json
    if ($marker.schema -ne $script:OutputMarkerSchema) {
        throw "Unexpected output marker schema: $safeMarkerPath"
    }
    if (-not ([IO.Path]::GetFullPath([string]$marker.repository)).Equals(
            [IO.Path]::GetFullPath($RepositoryRoot),
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Output marker belongs to another repository: $safeMarkerPath"
    }
    if (-not ([IO.Path]::GetFullPath([string]$marker.target)).Equals(
            [IO.Path]::GetFullPath($TargetPath),
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Output marker target does not match: $safeMarkerPath"
    }
}

function Assert-GeneratedDirectoryResetPreflight {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string]$MarkerPath,
        [Parameter(Mandatory)][string]$RepositoryRoot
    )

    $safePath = Resolve-ContainedGeneratedPath -Candidate $Path -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safePath -OutputRoot $OutputRoot
    $safeMarkerPath = Resolve-ContainedGeneratedPath -Candidate $MarkerPath -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safeMarkerPath -OutputRoot $OutputRoot
    if (Test-Path -LiteralPath $safePath) {
        Assert-OutputMarker `
            -MarkerPath $safeMarkerPath `
            -OutputRoot $OutputRoot `
            -RepositoryRoot $RepositoryRoot `
            -TargetPath $safePath
        $null = @(Get-ContainedDescendantReparsePoints -Root $safePath)
    }
}

function Reset-GeneratedDirectory {
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$OutputRoot,

        [Parameter(Mandatory)]
        [string]$MarkerPath,

        [Parameter(Mandatory)]
        [string]$RepositoryRoot,

        [Parameter(Mandatory)]
        [string]$Kind
    )

    $safePath = Resolve-ContainedGeneratedPath -Candidate $Path -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safePath -OutputRoot $OutputRoot

    if (Test-Path -LiteralPath $safePath) {
        Assert-OutputMarker -MarkerPath $MarkerPath -OutputRoot $OutputRoot -RepositoryRoot $RepositoryRoot -TargetPath $safePath
        Remove-ContainedDescendantReparsePoints -Root $safePath
        Remove-Item -LiteralPath $safePath -Recurse -Force
    }

    Write-OutputMarker -MarkerPath $MarkerPath -OutputRoot $OutputRoot -RepositoryRoot $RepositoryRoot -TargetPath $safePath -Kind $Kind
}

function Register-GeneratedDirectory {
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$OutputRoot,

        [Parameter(Mandatory)]
        [string]$MarkerPath,

        [Parameter(Mandatory)]
        [string]$RepositoryRoot,

        [Parameter(Mandatory)]
        [string]$Kind
    )

    $safePath = Resolve-ContainedGeneratedPath -Candidate $Path -OutputRoot $OutputRoot
    Assert-NoReparsePointInOutputPath -Path $safePath -OutputRoot $OutputRoot

    if (Test-Path -LiteralPath $safePath) {
        Assert-OutputMarker `
            -MarkerPath $MarkerPath `
            -OutputRoot $OutputRoot `
            -RepositoryRoot $RepositoryRoot `
            -TargetPath $safePath
    }
    else {
        Write-OutputMarker `
            -MarkerPath $MarkerPath `
            -OutputRoot $OutputRoot `
            -RepositoryRoot $RepositoryRoot `
            -TargetPath $safePath `
            -Kind $Kind
    }
}

function Invoke-CheckedNativeCommand {
    param(
        [Parameter(Mandatory)]
        [string]$FilePath,

        [Parameter()]
        [string[]]$ArgumentList = @(),

        [Parameter(Mandatory)]
        [string]$WorkingDirectory
    )

    Write-Host "> $FilePath $($ArgumentList -join ' ')"
    Push-Location -LiteralPath $WorkingDirectory
    try {
        & $FilePath @ArgumentList
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "Command failed with exit code ${exitCode}: $FilePath"
    }
}

function Find-VisualStudio18 {
    param([string]$RequestedPath)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        return [IO.Path]::GetFullPath($RequestedPath).TrimEnd('\')
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "vswhere.exe was not found: $vswhere"
    }

    $paths = @(
        & $vswhere -latest -products '*' -version '[18.0,19.0)' -property installationPath |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($LASTEXITCODE -ne 0 -or $paths.Count -ne 1) {
        throw 'A unique Visual Studio 18 installation could not be discovered.'
    }

    return [IO.Path]::GetFullPath($paths[0]).TrimEnd('\')
}

function Enter-PinnedDeveloperShell {
    param(
        [Parameter(Mandatory)]
        [string]$InstallationPath,

        [Parameter(Mandatory)]
        [string]$VcVersion,

        [Parameter(Mandatory)]
        [string]$SdkVersion
    )

    $launcher = Join-Path $InstallationPath 'Common7\Tools\Launch-VsDevShell.ps1'
    if (-not (Test-Path -LiteralPath $launcher -PathType Leaf)) {
        throw "Visual Studio Developer PowerShell launcher was not found: $launcher"
    }

    & $launcher -VsInstallationPath $InstallationPath -Arch amd64 -HostArch amd64 -SkipAutomaticLocation -NoLogo | Out-Null
    if ($null -eq (Get-Command Enter-VsDevShell -ErrorAction SilentlyContinue)) {
        throw 'Enter-VsDevShell was not imported by Visual Studio.'
    }

    Enter-VsDevShell `
        -VsInstallPath $InstallationPath `
        -Arch amd64 `
        -HostArch amd64 `
        -SkipAutomaticLocation `
        -DevCmdArguments "-vcvars_ver=$VcVersion -winsdk=$SdkVersion -no_logo" |
        Out-Null
}

function Get-CMakeCacheValue {
    param(
        [Parameter(Mandatory)]
        [string]$CachePath,

        [Parameter(Mandatory)]
        [string]$Name
    )

    $match = Select-String -LiteralPath $CachePath -Pattern "^$([regex]::Escape($Name)):[^=]*=(.*)$" |
        Select-Object -First 1
    if ($null -eq $match) {
        throw "Required CMake cache entry is absent: $Name"
    }
    return $match.Matches[0].Groups[1].Value
}

function Test-CMakeTrueValue {
    param([Parameter(Mandatory)][string]$Value)

    return $Value -in @('1', 'ON', 'TRUE', 'YES', 'Y')
}

function Get-CMakeCompilerMetadata {
    param([Parameter(Mandatory)][string]$BuildDirectory)

    $cmakeFiles = Join-Path $BuildDirectory 'CMakeFiles'
    $metadataFiles = @(
        Get-ChildItem -LiteralPath $cmakeFiles -Recurse -File -Filter 'CMakeCXXCompiler.cmake'
    )
    if ($metadataFiles.Count -ne 1) {
        throw "Expected exactly one CMake C++ compiler metadata file in $BuildDirectory; found $($metadataFiles.Count)."
    }

    $content = Get-Content -LiteralPath $metadataFiles[0].FullName -Raw
    $compilerMatch = [regex]::Match(
        $content,
        '(?m)^set\(CMAKE_CXX_COMPILER "([^"]+)"\)\s*$')
    $versionMatch = [regex]::Match(
        $content,
        '(?m)^set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)\s*$')
    if (-not $compilerMatch.Success -or -not $versionMatch.Success) {
        throw "CMake compiler metadata is incomplete: $($metadataFiles[0].FullName)"
    }

    return [pscustomobject]@{
        Path = [IO.Path]::GetFullPath($compilerMatch.Groups[1].Value)
        Version = $versionMatch.Groups[1].Value
        MetadataPath = $metadataFiles[0].FullName
    }
}

function Get-ToolDescriptor {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Path
    )

    $fullPath = [IO.Path]::GetFullPath($Path)
    $item = Get-Item -LiteralPath $fullPath -Force
    return [ordered]@{
        name = $Name
        path = $fullPath
        file_version = [string]$item.VersionInfo.FileVersion
        product_version = [string]$item.VersionInfo.ProductVersion
        sha256 = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash
    }
}

function Get-TranslationContract {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$GitPath
    )

    $sourceRoot = Join-Path $RepositoryRoot 'localization\i18n'
    $outputRoot = Join-Path $RepositoryRoot 'resources\i18n'
    $trackedPoPaths = @(& $GitPath -C $RepositoryRoot -c 'core.quotepath=false' `
        ls-files -- ':(glob)localization/i18n/*/CrealityPrint*.po')
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to enumerate tracked PO inputs.'
    }
    $trackedPoSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($relative in $trackedPoPaths | Where-Object { $_ -ne '' }) {
        $null = $trackedPoSet.Add([IO.Path]::GetFullPath((Join-Path $RepositoryRoot $relative)))
    }
    $records = [Collections.Generic.List[object]]::new()
    foreach ($directory in Get-ChildItem -LiteralPath $sourceRoot -Directory | Sort-Object Name) {
        $poFiles = @(Get-ChildItem -LiteralPath $directory.FullName -File -Filter 'CrealityPrint*.po')
        if ($poFiles.Count -ne 1) {
            throw "Expected exactly one CrealityPrint PO file for '$($directory.Name)'; found $($poFiles.Count)."
        }
        if (-not $trackedPoSet.Contains([IO.Path]::GetFullPath($poFiles[0].FullName))) {
            throw "Translation input must be tracked by Git: $($poFiles[0].FullName)"
        }
        $records.Add([pscustomobject]@{
            Language = $directory.Name
            SourcePath = $poFiles[0].FullName
            OutputPath = Join-Path $outputRoot "$($directory.Name)\CrealityPrint.mo"
        })
    }
    if ($records.Count -eq 0) {
        throw 'The translation contract contains no languages.'
    }
    if ($records.Count -ne $trackedPoSet.Count) {
        throw 'The translation directory set does not exactly match the tracked PO input set.'
    }
    return @($records)
}

function Assert-TranslationTreeContract {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$GitPath,
        [Parameter(Mandatory)][object[]]$Contract,
        [Parameter()][switch]$PreflightClean,
        [Parameter()][switch]$CleanExpectedOutputs
    )

    $outputRoot = Join-Path $RepositoryRoot 'resources\i18n'
    if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
        throw "Translation output root is missing: $outputRoot"
    }

    foreach ($item in @((Get-Item -LiteralPath $outputRoot -Force)) +
        @(Get-ChildItem -LiteralPath $outputRoot -Force -Recurse)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Reparse points are not permitted in the translation output tree: $($item.FullName)"
        }
    }

    $allowed = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $expectedMoPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $tracked = @(
        & $GitPath -C $RepositoryRoot -c 'core.quotepath=false' ls-files -- 'resources/i18n'
    )
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to enumerate tracked translation output files.'
    }
    foreach ($relative in $tracked | Where-Object { $_ -ne '' }) {
        $null = $allowed.Add([IO.Path]::GetFullPath((Join-Path $RepositoryRoot $relative)))
    }
    foreach ($entry in $Contract) {
        $expectedPath = [IO.Path]::GetFullPath([string]$entry.OutputPath)
        if ($allowed.Contains($expectedPath)) {
            throw "Expected generated translation must not be tracked: $expectedPath"
        }
        $relative = [IO.Path]::GetRelativePath($RepositoryRoot, $expectedPath).Replace('\', '/')
        & $GitPath -C $RepositoryRoot check-ignore --quiet -- $relative
        if ($LASTEXITCODE -ne 0) {
            throw "Expected generated translation is not covered by a Git ignore rule: $relative"
        }
        $null = $allowed.Add($expectedPath)
        $null = $expectedMoPaths.Add($expectedPath)
    }

    foreach ($file in Get-ChildItem -LiteralPath $outputRoot -Force -Recurse -File) {
        $fullFilePath = [IO.Path]::GetFullPath($file.FullName)
        if ($file.Extension.Equals('.mo', [StringComparison]::OrdinalIgnoreCase) -and
            -not $expectedMoPaths.Contains($fullFilePath)) {
            throw "Unexpected MO file outside the PO-derived translation contract: $($file.FullName)"
        }
        if (-not $allowed.Contains($fullFilePath)) {
            throw "Unexpected untracked file in translation output tree: $($file.FullName)"
        }
    }

    if ($PreflightClean -and $CleanExpectedOutputs) {
        throw 'Translation cleanup preflight and execution switches are mutually exclusive.'
    }
    if ($CleanExpectedOutputs) {
        foreach ($entry in $Contract) {
            if (Test-Path -LiteralPath $entry.OutputPath -PathType Leaf) {
                Remove-Item -LiteralPath $entry.OutputPath -Force
            }
        }
    }
    elseif (-not $PreflightClean) {
        foreach ($entry in $Contract) {
            if (-not (Test-Path -LiteralPath $entry.OutputPath -PathType Leaf)) {
                throw "Expected generated translation is missing: $($entry.OutputPath)"
            }
        }
        $actualMoFiles = @(
            Get-ChildItem -LiteralPath $outputRoot -Force -Recurse -File -Filter '*.mo'
        )
        if ($actualMoFiles.Count -ne $Contract.Count) {
            throw "Generated translation count is $($actualMoFiles.Count); expected $($Contract.Count)."
        }
    }
}

function Assert-CMakeCacheAbsolutePathContract {
    param(
        [Parameter(Mandatory)][string]$CachePath,
        [Parameter(Mandatory)][string[]]$AllowedRoots,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$AllowedFiles
    )

    $normalizedRoots = @(
        $AllowedRoots | ForEach-Object {
            [IO.Path]::GetFullPath($_).TrimEnd(
                [IO.Path]::DirectorySeparatorChar,
                [IO.Path]::AltDirectorySeparatorChar)
        }
    )
    $normalizedFiles = @(
        $AllowedFiles | ForEach-Object { [IO.Path]::GetFullPath($_) }
    )
    $violations = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)

    foreach ($line in Get-Content -LiteralPath $CachePath) {
        if ($line -notmatch '^([^#/][^:]*):(PATH|FILEPATH|STRING)=(.*)$') {
            continue
        }
        $name = $Matches[1]
        foreach ($rawPart in $Matches[3] -split ';') {
            $part = $rawPart.Trim().Trim('"')
            if ([string]::IsNullOrWhiteSpace($part) -or
                -not [IO.Path]::IsPathFullyQualified($part)) {
                continue
            }
            $fullPath = [IO.Path]::GetFullPath($part)
            $allowed = @($normalizedFiles | Where-Object {
                    $fullPath.Equals($_, [StringComparison]::OrdinalIgnoreCase)
                }).Count -gt 0
            if (-not $allowed) {
                foreach ($root in $normalizedRoots) {
                    if ($fullPath.Equals($root, [StringComparison]::OrdinalIgnoreCase) -or
                        $fullPath.StartsWith(
                            $root + [IO.Path]::DirectorySeparatorChar,
                            [StringComparison]::OrdinalIgnoreCase)) {
                        $allowed = $true
                        break
                    }
                }
            }
            if (-not $allowed) {
                $null = $violations.Add($name)
            }
        }
    }

    if ($violations.Count -ne 0) {
        $names = @($violations) -join ', '
        throw "CMake cache contains absolute paths outside the accepted roots in: $names"
    }
}

function Assert-CMakeCacheContract {
    param(
        [Parameter(Mandatory)]
        [string]$BuildDirectory,

        [Parameter(Mandatory)]
        [string]$SdkVersion,

        [Parameter(Mandatory)]
        [string]$VisualStudioPath,

        [Parameter(Mandatory)]
        [int]$ExpectedJobs,

        [Parameter(Mandatory)]
        [string]$ExpectedCompilerPath,

        [Parameter(Mandatory)]
        [string]$ExpectedCompilerVersion,

        [Parameter(Mandatory)]
        [string]$ExpectedPerlPath,

        [Parameter(Mandatory)]
        [string]$ExpectedMsgfmtPath,

        [Parameter(Mandatory)]
        [string]$ExpectedMsgmergePath,

        [Parameter(Mandatory)]
        [bool]$IsDependency,

        [Parameter()]
        [string]$ExpectedDependencyPrefix = '',

        [Parameter()]
        [string]$ExpectedRepositoryRoot = '',

        [Parameter()]
        [string]$ExpectedGitPath = ''
    )

    $cachePath = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "CMake cache was not generated: $cachePath"
    }

    $expected = [ordered]@{
        CMAKE_GENERATOR = 'Visual Studio 18 2026'
        CMAKE_GENERATOR_PLATFORM = 'x64'
        CMAKE_GENERATOR_TOOLSET = 'v143,host=x64'
        CMAKE_GENERATOR_INSTANCE = [IO.Path]::GetFullPath($VisualStudioPath).Replace('\', '/')
        CMAKE_SYSTEM_VERSION = $SdkVersion
        CMAKE_BUILD_TYPE = 'Release'
        CMAKE_CONFIGURATION_TYPES = 'Release'
        CREALITYPRINT_BUILD_JOBS = [string]$ExpectedJobs
    }

    foreach ($entry in $expected.GetEnumerator()) {
        $actual = Get-CMakeCacheValue -CachePath $cachePath -Name $entry.Key
        if ($actual -ne $entry.Value) {
            throw "Unexpected $($entry.Key): expected '$($entry.Value)', got '$actual'"
        }
    }

    $breakpad = Get-CMakeCacheValue -CachePath $cachePath -Name 'ENABLE_BREAKPAD'
    if (-not (Test-CMakeTrueValue -Value $breakpad)) {
        throw "ENABLE_BREAKPAD must be true for the supported VS2026 route; got '$breakpad'."
    }

    if ($IsDependency) {
        $strictMatch = Get-CMakeCacheValue `
            -CachePath $cachePath `
            -Name 'CREALITYPRINT_STRICT_PATH_TOOLCHAIN_MATCH'
        if (-not (Test-CMakeTrueValue -Value $strictMatch)) {
            throw "CREALITYPRINT_STRICT_PATH_TOOLCHAIN_MATCH must be true; got '$strictMatch'."
        }
        $cachedPerl = Get-CMakeCacheValue -CachePath $cachePath -Name 'OPENSSL_PERL_EXECUTABLE'
        if (-not ([IO.Path]::GetFullPath($cachedPerl)).Equals(
                [IO.Path]::GetFullPath($ExpectedPerlPath),
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unexpected OPENSSL_PERL_EXECUTABLE: $cachedPerl"
        }
        foreach ($entry in ([ordered]@{
                CREALITYPRINT_MSGFMT_EXECUTABLE = $ExpectedMsgfmtPath
                CREALITYPRINT_MSGMERGE_EXECUTABLE = $ExpectedMsgmergePath
            }).GetEnumerator()) {
            $actual = Get-CMakeCacheValue -CachePath $cachePath -Name $entry.Key
            if (-not ([IO.Path]::GetFullPath($actual)).Equals(
                    [IO.Path]::GetFullPath($entry.Value),
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "Unexpected $($entry.Key): $actual"
            }
        }
    }
    else {
        if ([string]::IsNullOrWhiteSpace($ExpectedDependencyPrefix) -or
            [string]::IsNullOrWhiteSpace($ExpectedRepositoryRoot) -or
            [string]::IsNullOrWhiteSpace($ExpectedGitPath)) {
            throw 'The application cache contract requires exact source, Git, and dependency paths.'
        }
        $offlineWebView2Required = Get-CMakeCacheValue `
            -CachePath $cachePath `
            -Name 'CREALITYPRINT_REQUIRE_OFFLINE_WEBVIEW2_INSTALLER'
        if (Test-CMakeTrueValue -Value $offlineWebView2Required) {
            throw 'The public VS2026 source route must explicitly omit the unavailable offline WebView2 installer.'
        }
        $expectedPrefix = [IO.Path]::GetFullPath($ExpectedDependencyPrefix)
        $cachedPrefix = Get-CMakeCacheValue -CachePath $cachePath -Name 'CMAKE_PREFIX_PATH'
        if (-not ([IO.Path]::GetFullPath($cachedPrefix)).Equals(
                $expectedPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unexpected CMAKE_PREFIX_PATH: $cachedPrefix"
        }
        $cachedNlopt = Get-CMakeCacheValue -CachePath $cachePath -Name 'NLopt_LIBS'
        $expectedNlopt = Join-Path $expectedPrefix 'lib\nlopt.lib'
        if (-not ([IO.Path]::GetFullPath($cachedNlopt)).Equals(
                [IO.Path]::GetFullPath($expectedNlopt),
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "NLopt resolved outside the accepted dependency prefix: $cachedNlopt"
        }

        $findSettings = [ordered]@{
            CMAKE_FIND_USE_PACKAGE_ROOT_PATH = $false
            CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH = $false
            CMAKE_FIND_USE_PACKAGE_REGISTRY = $false
            CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY = $false
            CMAKE_EXPORT_NO_PACKAGE_REGISTRY = $true
        }
        foreach ($entry in $findSettings.GetEnumerator()) {
            $actual = Get-CMakeCacheValue -CachePath $cachePath -Name $entry.Key
            if ((Test-CMakeTrueValue -Value $actual) -ne $entry.Value) {
                throw "Unexpected $($entry.Key): $actual"
            }
        }

        Assert-CMakeCacheAbsolutePathContract `
            -CachePath $cachePath `
            -AllowedRoots @(
                $expectedPrefix,
                [IO.Path]::GetFullPath($ExpectedRepositoryRoot),
                [IO.Path]::GetFullPath($VisualStudioPath),
                [IO.Path]::GetFullPath((Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'))) `
            -AllowedFiles @([IO.Path]::GetFullPath($ExpectedGitPath))
    }

    $compiler = Get-CMakeCompilerMetadata -BuildDirectory $BuildDirectory
    if (-not $compiler.Path.Equals(
            [IO.Path]::GetFullPath($ExpectedCompilerPath),
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "CMake selected an unexpected compiler: $($compiler.Path)"
    }
    if ($compiler.Version -ne $ExpectedCompilerVersion) {
        throw "CMake selected compiler version '$($compiler.Version)'; expected '$ExpectedCompilerVersion'."
    }
}

function Assert-DependencyPrefix {
    param([Parameter(Mandatory)][string]$Prefix)

    $sentinels = @(
        (Join-Path $Prefix 'include\boost-1_84\boost\version.hpp'),
        (Join-Path $Prefix 'include\openssl\ssl.h'),
        (Join-Path $Prefix 'include\client\windows\handler\exception_handler.h'),
        (Join-Path $Prefix 'include\nlopt.hpp'),
        (Join-Path $Prefix 'include\wx\wx.h'),
        (Join-Path $Prefix 'include\zlib.h'),
        (Join-Path $Prefix 'lib\cmake\wxWidgets-3.3\wxWidgetsConfig.cmake'),
        (Join-Path $Prefix 'lib\libcrypto.lib'),
        (Join-Path $Prefix 'lib\libbreakpad_client.lib'),
        (Join-Path $Prefix 'lib\libssl.lib'),
        (Join-Path $Prefix 'lib\nlopt.lib'),
        (Join-Path $Prefix 'lib\vc_x64_lib\wxbase33u.lib'),
        (Join-Path $Prefix 'lib\vc_x64_lib\wxmsw33u_core.lib'),
        (Join-Path $Prefix 'lib\zlib.lib')
    )
    foreach ($path in $sentinels) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Dependency installation is incomplete; missing: $path"
        }
    }
}

function Invoke-SelfTests {
    $temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) 'crealityprint-safe-output-test'
    $accepted = Resolve-ContainedGeneratedPath `
        -Candidate (Join-Path $temporaryRoot 'child') `
        -OutputRoot $temporaryRoot
    if (-not $accepted.EndsWith('child', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Contained generated path self-test failed.'
    }

    foreach ($rejected in @($temporaryRoot, (Split-Path -Parent $temporaryRoot))) {
        $didThrow = $false
        try {
            $null = Resolve-ContainedGeneratedPath -Candidate $rejected -OutputRoot $temporaryRoot
        }
        catch {
            $didThrow = $true
        }
        if (-not $didThrow) {
            throw "Unsafe generated path was accepted by self-test: $rejected"
        }
    }

    $cacheTestRoot = Join-Path ([IO.Path]::GetTempPath()) (
        'crealityprint-cache-contract-' + [guid]::NewGuid().ToString('N'))
    try {
        $allowedRoot = Join-Path $cacheTestRoot 'allowed'
        $cachePath = Join-Path $cacheTestRoot 'CMakeCache.txt'
        $null = New-Item -ItemType Directory -Path $allowedRoot -Force
        [IO.File]::WriteAllText(
            $cachePath,
            "Boost_DIR:PATH=$($allowedRoot.Replace('\', '/'))/lib/cmake/Boost`n")
        Assert-CMakeCacheAbsolutePathContract `
            -CachePath $cachePath `
            -AllowedRoots @($allowedRoot) `
            -AllowedFiles @()

        $outside = Join-Path ([IO.Path]::GetTempPath()) 'ambient-boost\lib\cmake\Boost'
        [IO.File]::WriteAllText(
            $cachePath,
            "Boost_DIR:PATH=$($outside.Replace('\', '/'))`n")
        $didThrow = $false
        try {
            Assert-CMakeCacheAbsolutePathContract `
                -CachePath $cachePath `
                -AllowedRoots @($allowedRoot) `
                -AllowedFiles @()
        }
        catch {
            $didThrow = $true
        }
        if (-not $didThrow) {
            throw 'An ambient package path was accepted by the CMake cache self-test.'
        }
    }
    finally {
        if (Test-Path -LiteralPath $cacheTestRoot) {
            Remove-Item -LiteralPath $cacheTestRoot -Recurse -Force
        }
    }

    if ($IsWindows) {
        $reparseTestRoot = Join-Path ([IO.Path]::GetTempPath()) (
            'crealityprint-reparse-contract-' + [guid]::NewGuid().ToString('N'))
        try {
            $generatedRoot = Join-Path $reparseTestRoot 'generated'
            $externalRoot = Join-Path $reparseTestRoot 'external'
            $survivor = Join-Path $externalRoot 'survivor.txt'
            $junction = Join-Path $generatedRoot 'linked-directory'
            $null = New-Item -ItemType Directory -Path $generatedRoot, $externalRoot -Force
            [IO.File]::WriteAllText($survivor, 'survive')
            $null = New-Item -ItemType Junction -Path $junction -Target $externalRoot
            Remove-ContainedDescendantReparsePoints -Root $generatedRoot
            if ((Test-Path -LiteralPath $junction) -or
                -not (Test-Path -LiteralPath $survivor -PathType Leaf)) {
                throw 'Guarded reparse-point cleanup did not preserve the linked target.'
            }
        }
        finally {
            if (Test-Path -LiteralPath $reparseTestRoot) {
                Remove-Item -LiteralPath $reparseTestRoot -Recurse -Force
            }
        }
    }

    Write-Host 'Self-tests passed.'
}

function Invoke-WindowsVs2026Build {
    $repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    $outputRoot = Join-Path $repositoryRoot 'out'
    $depsSource = Join-Path $repositoryRoot 'deps'

    $safeBuildRecordPath = $null
    if (-not [string]::IsNullOrWhiteSpace($BuildRecordPath)) {
        $safeBuildRecordPath = Resolve-ContainedGeneratedPath `
            -Candidate $BuildRecordPath `
            -OutputRoot $outputRoot
        Assert-NoReparsePointInOutputPath -Path $safeBuildRecordPath -OutputRoot $outputRoot
        if (Test-Path -LiteralPath $safeBuildRecordPath) {
            throw "Build record path already exists: $safeBuildRecordPath"
        }
    }
    $preset = $script:PresetBase

    $paths = [ordered]@{
        DepsBuild = Join-Path $outputRoot "build\deps\$preset"
        DepsInstall = Join-Path $outputRoot "install\deps\$preset"
        AppBuild = Join-Path $outputRoot "build\app\$preset"
        AppInstall = Join-Path $outputRoot "install\app\$preset"
        Temp = Join-Path $outputRoot "temp\$preset"
    }

    foreach ($path in $paths.Values) {
        $null = Resolve-ContainedGeneratedPath -Candidate $path -OutputRoot $outputRoot
        Assert-NoReparsePointInOutputPath -Path $path -OutputRoot $outputRoot
    }

    $vs = Find-VisualStudio18 -RequestedPath $VisualStudioPath
    if (-not (Test-Path -LiteralPath $PerlPath -PathType Leaf)) {
        throw "Native Strawberry Perl was not found: $PerlPath"
    }

    $freeGiB = [math]::Floor((Get-PSDrive -Name ([IO.Path]::GetPathRoot($repositoryRoot).TrimEnd(':\'))).Free / 1GB)
    if ($freeGiB -lt $MinimumFreeGiB) {
        throw "Insufficient free disk space: ${freeGiB} GiB available, ${MinimumFreeGiB} GiB required."
    }

    if (Test-Path -LiteralPath (Join-Path $repositoryRoot 'UnitTest\.git')) {
        throw 'The private UnitTest submodule must not be initialized for this public build route.'
    }

    Enter-PinnedDeveloperShell -InstallationPath $vs -VcVersion $ToolsetVersion -SdkVersion $WindowsSdkVersion

    $originalEnvironment = @{}
    $packageNames = @(
        'Boost', 'TBB', 'zstd', 'OpenSSL', 'CURL', 'ZLIB', 'Eigen3', 'EXPAT',
        'PNG', 'GLEW', 'glfw3', 'cereal', 'NLopt', 'OpenVDB', 'FFmpeg',
        'libnoise', 'wxWidgets', 'JPEG', 'TIFF', 'PahoMqttCpp', 'Qhull',
        'CGAL', 'OpenCV', 'OpenCASCADE', 'Freetype', 'Blosc', 'Log4cplus',
        'IlmBase', 'OpenEXR'
    )
    $packageHintNames = [Collections.Generic.List[string]]::new()
    foreach ($packageName in $packageNames) {
        foreach ($prefix in @($packageName, $packageName.ToUpperInvariant())) {
            $packageHintNames.Add("${prefix}_ROOT")
            $packageHintNames.Add("${prefix}_DIR")
        }
    }
    $scopedNames = @(
        'PATH', 'TEMP', 'TMP', 'TZ', 'VSLANG',
        'DUMPTOOL_USER', 'DUMPTOOL_PASS', 'DUMPTOOL_HOST', 'DUMPTOOL_TO',
        'CXAGENT_API_BASE', 'CXAGENT_DEBUG_API_BASE', 'CXAGENT_RELEASE_API_BASE',
        'BUILD_ID', 'VLD_HOME', 'NLOPT',
        'CMAKE_PREFIX_PATH', 'CMAKE_INCLUDE_PATH', 'CMAKE_LIBRARY_PATH',
        'CMAKE_PROGRAM_PATH', 'CMAKE_FRAMEWORK_PATH', 'CMAKE_APPBUNDLE_PATH',
        'CMAKE_FIND_ROOT_PATH', 'CMAKE_IGNORE_PATH', 'CMAKE_SYSTEM_IGNORE_PATH',
        'CMAKE_TOOLCHAIN_FILE',
        'CMAKE_PROJECT_INCLUDE', 'CMAKE_PROJECT_INCLUDE_BEFORE',
        'CMAKE_PROJECT_TOP_LEVEL_INCLUDES',
        'CMAKE_FIND_USE_PACKAGE_ROOT_PATH',
        'CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH',
        'CMAKE_FIND_USE_PACKAGE_REGISTRY',
        'CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY',
        'CMAKE_EXPORT_NO_PACKAGE_REGISTRY',
        'CMAKE_GENERATOR', 'CMAKE_GENERATOR_INSTANCE',
        'CMAKE_GENERATOR_PLATFORM', 'CMAKE_GENERATOR_TOOLSET',
        'BOOST_ROOT', 'BOOST_INCLUDEDIR', 'BOOST_LIBRARYDIR',
        'OPENSSL_ROOT_DIR', 'wxWidgets_ROOT_DIR', 'WXWIN', 'WXCFG', 'WXRC',
        'VCPKG_ROOT', 'VCPKG_INSTALLATION_ROOT',
        'CC', 'CXX', 'RC', 'ASM', 'ASM_MASM',
        'CFLAGS', 'CXXFLAGS', 'LDFLAGS', 'CL', '_CL_', 'LINK', '_LINK_',
        'CMAKE_BUILD_PARALLEL_LEVEL', 'CMAKE_INSTALL_PARALLEL_LEVEL',
        'CL_MPCount', 'UseMultiToolTask', 'EnforceProcessCountAcrossBuilds',
        'MultiProcMaxCount'
    ) + @($packageHintNames)
    $scopedNames = @($scopedNames | Select-Object -Unique)
    foreach ($name in $scopedNames) {
        $originalEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
    }

    try {
        foreach ($name in $scopedNames | Where-Object {
                $_ -notin @('PATH', 'TEMP', 'TMP', 'TZ', 'VSLANG') }) {
            [Environment]::SetEnvironmentVariable($name, $null, 'Process')
        }

        $toolsDirectory = Join-Path $repositoryRoot 'tools'
        $perlDirectory = Split-Path -Parent ([IO.Path]::GetFullPath($PerlPath))
        $env:PATH = "$perlDirectory;$toolsDirectory;$env:PATH"
        $env:TZ = 'UTC'
        $env:VSLANG = '1033'
        $env:CMAKE_BUILD_PARALLEL_LEVEL = [string]$Jobs
        $env:CMAKE_INSTALL_PARALLEL_LEVEL = [string]$Jobs
        $env:CL_MPCount = [string]$Jobs

        $resolved = [ordered]@{}
        foreach ($name in @(
                'cmake', 'cl', 'link', 'lib', 'rc', 'msbuild', 'nmake',
                'git', 'perl', 'msgfmt', 'msgmerge')) {
            $command = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($null -eq $command) {
                throw "Required build command is unavailable: $name"
            }
            $resolved[$name] = $command.Source
        }
        $resolved.pwsh = Join-Path $PSHOME 'pwsh.exe'
        if (-not (Test-Path -LiteralPath $resolved.pwsh -PathType Leaf)) {
            throw "The active PowerShell interpreter is missing: $($resolved.pwsh)"
        }

        $expectedResolvedTools = [ordered]@{
            cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            perl = [IO.Path]::GetFullPath($PerlPath)
            msgfmt = Join-Path $repositoryRoot 'tools\msgfmt.exe'
            msgmerge = Join-Path $repositoryRoot 'tools\msgmerge.exe'
            rc = Join-Path $env:WindowsSdkDir "bin\$WindowsSdkVersion\x64\rc.exe"
        }
        foreach ($entry in $expectedResolvedTools.GetEnumerator()) {
            if (-not ([IO.Path]::GetFullPath($resolved[$entry.Key])).Equals(
                    [IO.Path]::GetFullPath($entry.Value),
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "Unexpected $($entry.Key) executable: $($resolved[$entry.Key])"
            }
        }
        foreach ($name in @('cl', 'link', 'lib', 'nmake')) {
            if (-not $resolved[$name].StartsWith(
                    $env:VCToolsInstallDir,
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "$name is not from the selected VCToolsInstallDir: $($resolved[$name])"
            }
        }
        if (-not $resolved.msbuild.StartsWith($vs, [StringComparison]::OrdinalIgnoreCase)) {
            throw "msbuild.exe is not from the selected Visual Studio instance: $($resolved.msbuild)"
        }
        $activeToolsetVersion = Split-Path -Leaf $env:VCToolsInstallDir.TrimEnd('\')
        if ($activeToolsetVersion -ne $ToolsetVersion) {
            throw "The active compiler toolset is '$activeToolsetVersion'; expected '$ToolsetVersion'."
        }
        if ($env:WindowsSDKVersion.TrimEnd('\') -ne $WindowsSdkVersion) {
            throw "Unexpected Windows SDK: $env:WindowsSDKVersion"
        }

        $cmakeVersionLine = @(& $resolved.cmake --version)[0]
        if ($LASTEXITCODE -ne 0 -or $cmakeVersionLine -notmatch 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)') {
            throw 'Unable to determine CMake version.'
        }
        if ([version]$Matches[1] -lt [version]'4.2.0') {
            throw "CMake 4.2 or newer is required; found $($Matches[1])."
        }

        $capabilities = @(& $resolved.cmake -E capabilities) -join [Environment]::NewLine |
            ConvertFrom-Json
        if (@($capabilities.generators | Where-Object { $_.name -eq 'Visual Studio 18 2026' }).Count -ne 1) {
            throw 'The selected CMake does not advertise the Visual Studio 18 2026 generator.'
        }

        $toolDescriptors = @(
            foreach ($entry in $resolved.GetEnumerator()) {
                Get-ToolDescriptor -Name $entry.Key -Path $entry.Value
            }
        )
        $compilerDescriptor = $toolDescriptors | Where-Object { $_.name -eq 'cl' } |
            Select-Object -First 1
        if ($null -eq $compilerDescriptor -or [string]::IsNullOrWhiteSpace($compilerDescriptor.file_version)) {
            throw 'Unable to determine the exact compiler file version.'
        }
        $translationContract = Get-TranslationContract `
            -RepositoryRoot $repositoryRoot `
            -GitPath $resolved.git

        $selectedKinds = if ($Target -eq 'Dependencies') {
            @('DepsBuild', 'DepsInstall', 'Temp')
        }
        elseif ($Target -eq 'Application') {
            @('AppBuild', 'AppInstall', 'Temp')
        }
        else {
            @('DepsBuild', 'DepsInstall', 'AppBuild', 'AppInstall', 'Temp')
        }


        if ($Fresh) {
            foreach ($kind in $selectedKinds) {
                $marker = Get-OutputMarkerPath -OutputRoot $outputRoot -Preset $preset -Kind $kind
                Assert-GeneratedDirectoryResetPreflight `
                    -Path $paths[$kind] `
                    -OutputRoot $outputRoot `
                    -MarkerPath $marker `
                    -RepositoryRoot $repositoryRoot
            }
            if ($Target -in @('All', 'Application')) {
                Assert-TranslationTreeContract `
                    -RepositoryRoot $repositoryRoot `
                    -GitPath $resolved.git `
                    -Contract $translationContract `
                    -PreflightClean
            }
            foreach ($kind in $selectedKinds) {
                $marker = Get-OutputMarkerPath -OutputRoot $outputRoot -Preset $preset -Kind $kind
                Reset-GeneratedDirectory `
                    -Path $paths[$kind] `
                    -OutputRoot $outputRoot `
                    -MarkerPath $marker `
                    -RepositoryRoot $repositoryRoot `
                    -Kind $kind
            }
            if ($Target -in @('All', 'Application')) {
                Assert-TranslationTreeContract `
                    -RepositoryRoot $repositoryRoot `
                    -GitPath $resolved.git `
                    -Contract $translationContract `
                    -CleanExpectedOutputs
            }
        }
        else {
            foreach ($kind in $selectedKinds) {
                $marker = Get-OutputMarkerPath -OutputRoot $outputRoot -Preset $preset -Kind $kind
                Register-GeneratedDirectory `
                    -Path $paths[$kind] `
                    -OutputRoot $outputRoot `
                    -MarkerPath $marker `
                    -RepositoryRoot $repositoryRoot `
                    -Kind $kind
            }
        }

        $null = New-Item -ItemType Directory -Path $paths.Temp -Force
        $env:TEMP = $paths.Temp
        $env:TMP = $paths.Temp

        if ($Target -in @('All', 'Dependencies')) {
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--preset', $preset, '--fresh',
                    "-DCREALITYPRINT_BUILD_JOBS:STRING=$Jobs",
                    "-DOPENSSL_PERL_EXECUTABLE:FILEPATH=$([IO.Path]::GetFullPath($PerlPath))",
                    "-DCREALITYPRINT_MSGFMT_EXECUTABLE:FILEPATH=$($resolved.msgfmt)",
                    "-DCREALITYPRINT_MSGMERGE_EXECUTABLE:FILEPATH=$($resolved.msgmerge)") `
                -WorkingDirectory $depsSource
            Assert-CMakeCacheContract `
                -BuildDirectory $paths.DepsBuild `
                -SdkVersion $WindowsSdkVersion `
                -VisualStudioPath $vs `
                -ExpectedJobs $Jobs `
                -ExpectedCompilerPath $resolved.cl `
                -ExpectedCompilerVersion $compilerDescriptor.file_version `
                -ExpectedPerlPath $PerlPath `
                -ExpectedMsgfmtPath $resolved.msgfmt `
                -ExpectedMsgmergePath $resolved.msgmerge `
                -IsDependency $true
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--build', $paths.DepsBuild, '--config', 'Release',
                    '--target', 'deps', '--parallel', '1', '--',
                    "/p:CL_MPCount=$Jobs") `
                -WorkingDirectory $depsSource
            Assert-DependencyPrefix -Prefix (Join-Path $paths.DepsInstall 'usr\local')
        }

        if ($Target -in @('All', 'Application')) {
            Assert-DependencyPrefix -Prefix (Join-Path $paths.DepsInstall 'usr\local')
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--preset', $preset, '--fresh',
                    "-DCREALITYPRINT_BUILD_JOBS:STRING=$Jobs") `
                -WorkingDirectory $repositoryRoot
            Assert-CMakeCacheContract `
                -BuildDirectory $paths.AppBuild `
                -SdkVersion $WindowsSdkVersion `
                -VisualStudioPath $vs `
                -ExpectedJobs $Jobs `
                -ExpectedCompilerPath $resolved.cl `
                -ExpectedCompilerVersion $compilerDescriptor.file_version `
                -ExpectedPerlPath $PerlPath `
                -ExpectedMsgfmtPath $resolved.msgfmt `
                -ExpectedMsgmergePath $resolved.msgmerge `
                -IsDependency $false `
                -ExpectedDependencyPrefix (Join-Path $paths.DepsInstall 'usr\local') `
                -ExpectedRepositoryRoot $repositoryRoot `
                -ExpectedGitPath $resolved.git
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--build', $paths.AppBuild, '--config', 'Release',
                    '--parallel', "$Jobs", '--', "/p:CL_MPCount=$Jobs") `
                -WorkingDirectory $repositoryRoot
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--build', $paths.AppBuild, '--config', 'Release',
                    '--target', 'gettext_po_to_mo', '--parallel', "$Jobs", '--',
                    "/p:CL_MPCount=$Jobs") `
                -WorkingDirectory $repositoryRoot
            Assert-TranslationTreeContract `
                -RepositoryRoot $repositoryRoot `
                -GitPath $resolved.git `
                -Contract $translationContract
            Invoke-CheckedNativeCommand `
                -FilePath $resolved.cmake `
                -ArgumentList @(
                    '--build', $paths.AppBuild, '--config', 'Release',
                    '--target', 'install', '--parallel', "$Jobs", '--',
                    "/p:CL_MPCount=$Jobs") `
                -WorkingDirectory $repositoryRoot

            foreach ($sentinel in @('CrealityPrint.exe', 'CrealityPrint_Slicer.dll')) {
                $path = Join-Path $paths.AppInstall $sentinel
                if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                    throw "Installed application sentinel is missing: $path"
                }
            }
        }


        $buildRecordSha256 = $null
        if ($null -ne $safeBuildRecordPath) {
            Assert-NoReparsePointInOutputPath -Path $safeBuildRecordPath -OutputRoot $outputRoot
            $recordDirectory = Split-Path -Parent $safeBuildRecordPath
            $null = New-Item -ItemType Directory -Path $recordDirectory -Force
            $sourceCommit = @(& $resolved.git -C $repositoryRoot rev-parse HEAD)
            if ($LASTEXITCODE -ne 0 -or $sourceCommit.Count -ne 1) {
                throw 'Unable to record the baseline Git commit.'
            }
            [ordered]@{
                schema = 'crealityprint-vs2026-build-record/v1'
                preset = $preset
                target = $Target
                configuration = 'Release'
                reproducible = $false
                source_commit = $sourceCommit[0]
                source_date_epoch = ''
                visual_studio = [IO.Path]::GetFullPath($vs)
                generator = 'Visual Studio 18 2026'
                platform = 'x64'
                generator_toolset = 'v143,host=x64'
                vc_tools_version = $activeToolsetVersion
                compiler_version = [string]$compilerDescriptor.file_version
                windows_sdk = $WindowsSdkVersion
                jobs = [ordered]@{
                    requested = $Jobs
                    dependency_top_level = 1
                    dependency_child = $Jobs
                    application = $Jobs
                    gettext = $Jobs
                    install = $Jobs
                    cl_mp_count = $Jobs
                }
                breakpad = $true
                package_resolution = [ordered]@{
                    package_root_hints_enabled = $false
                    cmake_environment_paths_enabled = $false
                    user_package_registry_enabled = $false
                    system_package_registry_enabled = $false
                    package_registry_export_enabled = $false
                    absolute_cache_paths_restricted = $true
                    dependency_prefix = [IO.Path]::GetFullPath(
                        (Join-Path $paths.DepsInstall 'usr\local'))
                }
                translation_count = $translationContract.Count
                translation_outputs = @(
                    $translationContract | ForEach-Object {
                        [IO.Path]::GetRelativePath(
                            (Join-Path $repositoryRoot 'resources\i18n'),
                            $_.OutputPath
                        ).Replace('\', '/')
                    }
                )
                locally_produced_artifacts_are_unsigned = $true
                tools = @($toolDescriptors)
            } | ConvertTo-Json -Depth 8 | Set-Content `
                -LiteralPath $safeBuildRecordPath -Encoding utf8NoBOM
            $buildRecordSha256 = (Get-FileHash -LiteralPath $safeBuildRecordPath -Algorithm SHA256).Hash
        }

        [pscustomobject]@{
            Preset = $preset
            VisualStudio = $vs
            Compiler = $resolved.cl
            WindowsSdk = $WindowsSdkVersion
            Perl = $resolved.perl
            DependencyPrefix = Join-Path $paths.DepsInstall 'usr\local'
            ApplicationInstall = $paths.AppInstall
            BuildRecord = $safeBuildRecordPath
            BuildRecordSha256 = $buildRecordSha256
        }
    }
    finally {
        foreach ($name in $scopedNames) {
            [Environment]::SetEnvironmentVariable($name, $originalEnvironment[$name], 'Process')
        }
    }
}

if ($MyInvocation.InvocationName -ne '.') {
    try {
        if ($SelfTest) {
            Invoke-SelfTests
        }
        else {
            Invoke-WindowsVs2026Build
        }
    }
    catch {
        Write-Error $_
        exit 1
    }
}
