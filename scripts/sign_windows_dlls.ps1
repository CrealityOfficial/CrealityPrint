[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SearchRoot,

    [Parameter(Mandatory = $true)]
    [string]$SignServiceUrl,

    [string]$SignToolPath,

    [string]$CacheRoot
)

$ErrorActionPreference = 'Stop'

function Resolve-SignTool {
    param([string]$RequestedPath)

    if ($RequestedPath -and (Test-Path -LiteralPath $RequestedPath -PathType Leaf)) {
        return (Resolve-Path -LiteralPath $RequestedPath).Path
    }

    $command = Get-Command 'signtool.exe' -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $kitRoots = @(
        'C:\Program Files (x86)\Windows Kits\10\bin',
        'C:\Program Files\Windows Kits\10\bin'
    )
    foreach ($kitRoot in $kitRoots) {
        if (-not (Test-Path -LiteralPath $kitRoot -PathType Container)) {
            continue
        }

        $candidate = Get-ChildItem -LiteralPath $kitRoot -Directory |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'x64\signtool.exe' } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    throw 'signtool.exe was not found; signed DLLs cannot be verified safely.'
}

function Invoke-Curl {
    param([string[]]$Arguments)

    $output = & 'C:\curl.exe' @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "curl.exe failed with exit code $LASTEXITCODE."
    }
    return $output
}

$resolvedRoot = (Resolve-Path -LiteralPath $SearchRoot).Path
$resolvedSignTool = Resolve-SignTool -RequestedPath $SignToolPath
$serviceUrl = $SignServiceUrl.TrimEnd('/')
$resolvedCacheRoot = $null
if (-not [string]::IsNullOrWhiteSpace($CacheRoot)) {
    try {
        $cacheRootItem = New-Item -ItemType Directory -Path $CacheRoot -Force
        $resolvedCacheRoot = $cacheRootItem.FullName.TrimEnd(
            [System.IO.Path]::DirectorySeparatorChar,
            [System.IO.Path]::AltDirectorySeparatorChar
        )
        $searchRootPrefix = $resolvedRoot.TrimEnd(
            [System.IO.Path]::DirectorySeparatorChar,
            [System.IO.Path]::AltDirectorySeparatorChar
        ) + [System.IO.Path]::DirectorySeparatorChar
        $cacheRootPrefix = $resolvedCacheRoot + [System.IO.Path]::DirectorySeparatorChar
        if ($cacheRootPrefix.StartsWith($searchRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "The signature cache must be outside the DLL search root: $resolvedCacheRoot"
        }
        Write-Host "Signature cache: $resolvedCacheRoot"
    }
    catch {
        Write-Warning "Signature cache is unavailable; continuing without it: $($_.Exception.Message)"
        $resolvedCacheRoot = $null
    }
}
$dlls = @(Get-ChildItem -LiteralPath $resolvedRoot -Filter '*.dll' -File -Recurse | Sort-Object FullName)

Write-Host "Checking $($dlls.Count) DLLs under $resolvedRoot"
$signedCount = 0
$skippedCount = 0
$cacheHitCount = 0
$cacheStoreCount = 0

foreach ($dll in $dlls) {
    $signature = Get-AuthenticodeSignature -LiteralPath $dll.FullName
    if ($signature.Status -eq [System.Management.Automation.SignatureStatus]::Valid) {
        Write-Host "[SKIP] Valid signature: $($dll.FullName)"
        $skippedCount++
        continue
    }
    if ($signature.Status -ne [System.Management.Automation.SignatureStatus]::NotSigned) {
        throw "DLL has an existing but invalid signature ($($signature.Status)): $($dll.FullName)"
    }

    $cachedFile = $null
    $temporaryFile = "$($dll.FullName).signed.tmp"
    # The application DLL changes with normal source builds and would create an
    # unbounded series of one-use entries. Cache stable dependency DLLs only.
    $cacheEligible = $resolvedCacheRoot -and ($dll.Name -notlike '*_Slicer.dll')
    if ($cacheEligible) {
        $unsignedHash = (Get-FileHash -LiteralPath $dll.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $cacheDirectory = Join-Path (Join-Path $resolvedCacheRoot $unsignedHash.Substring(0, 2)) $unsignedHash
        $cachedFile = Join-Path $cacheDirectory $dll.Name
        $cacheMetadataFile = "$cachedFile.sha256"

        if (Test-Path -LiteralPath $cachedFile -PathType Leaf) {
            $cacheIntegrityValid = $false
            if (Test-Path -LiteralPath $cacheMetadataFile -PathType Leaf) {
                try {
                    $expectedCacheHash = [System.IO.File]::ReadAllText($cacheMetadataFile).Trim()
                    $actualCacheHash = (Get-FileHash -LiteralPath $cachedFile -Algorithm SHA256).Hash
                    $cacheIntegrityValid = $actualCacheHash.Equals(
                        $expectedCacheHash,
                        [System.StringComparison]::OrdinalIgnoreCase
                    )
                }
                catch {
                    Write-Warning "Failed to validate cached signature integrity for $($dll.FullName): $($_.Exception.Message)"
                }
            }

            if ($cacheIntegrityValid) {
                $cachedSignature = Get-AuthenticodeSignature -LiteralPath $cachedFile
                if ($cachedSignature.Status -eq [System.Management.Automation.SignatureStatus]::Valid) {
                    try {
                        Copy-Item -LiteralPath $cachedFile -Destination $temporaryFile -Force
                        & $resolvedSignTool verify /pa /q $temporaryFile *> $null
                        if ($LASTEXITCODE -eq 0) {
                            Move-Item -LiteralPath $temporaryFile -Destination $dll.FullName -Force
                            Write-Host "[CACHE] $($dll.FullName)"
                            $cacheHitCount++
                            continue
                        }
                    }
                    catch {
                        Write-Warning "Failed to restore cached signature for $($dll.FullName): $($_.Exception.Message)"
                    }
                    finally {
                        Remove-Item -LiteralPath $temporaryFile -Force -ErrorAction SilentlyContinue
                    }
                }
            }

            Write-Warning "Removing invalid signature cache entry: $cachedFile"
            Remove-Item -LiteralPath $cachedFile -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath $cacheMetadataFile -Force -ErrorAction SilentlyContinue
        }
    }

    try {
        Write-Host "[SIGN] $($dll.FullName)"
        $verified = $false
        for ($attempt = 1; $attempt -le 3; $attempt++) {
            Remove-Item -LiteralPath $temporaryFile -Force -ErrorAction SilentlyContinue
            $response = Invoke-Curl -Arguments @(
                '--fail', '--show-error', '--silent',
                '-X', 'POST',
                '-F', "file=@$($dll.FullName)",
                "$serviceUrl/sign"
            )
            $responseText = (@($response) -join "`n").Trim()
            if ($responseText -ne 'ok') {
                throw "The signing service rejected $($dll.FullName). Response: $responseText"
            }

            # The service may return "ok" before the signed file is atomically published.
            # Poll the result before uploading the same unsigned DLL again.
            Start-Sleep -Seconds 1
            for ($poll = 1; $poll -le 10; $poll++) {
                $cacheBuster = [DateTime]::UtcNow.Ticks
                Invoke-Curl -Arguments @(
                    '--fail', '--show-error', '--silent', '--location',
                    '--header', 'Cache-Control: no-cache',
                    "$serviceUrl/exe/$([Uri]::EscapeDataString($dll.Name))?cacheBuster=$cacheBuster",
                    '--output', $temporaryFile
                ) | Out-Null

                & $resolvedSignTool verify /pa /q $temporaryFile *> $null
                if ($LASTEXITCODE -eq 0) {
                    $verified = $true
                    break
                }
                if ($poll -lt 10) {
                    Start-Sleep -Seconds 1
                }
            }
            if ($verified) {
                break
            }

            $returnedFile = Get-Item -LiteralPath $temporaryFile
            $returnedSignature = Get-AuthenticodeSignature -LiteralPath $temporaryFile
            Write-Warning "The signed result was not published for $($dll.FullName) (upload $attempt/3, returned bytes=$($returnedFile.Length), status=$($returnedSignature.Status))."
            if ($attempt -lt 3) {
                Start-Sleep -Seconds 2
            }
        }

        if (-not $verified) {
            & $resolvedSignTool verify /pa /v $temporaryFile
            throw "The signing service returned a DLL that failed signature verification after 3 attempts: $($dll.FullName)"
        }

        Move-Item -LiteralPath $temporaryFile -Destination $dll.FullName -Force
        $signedCount++

        if ($cachedFile) {
            New-Item -ItemType Directory -Path $cacheDirectory -Force | Out-Null
            $cachePublishId = "$PID.$([Guid]::NewGuid().ToString('N'))"
            $cacheTemporaryFile = "$cachedFile.$cachePublishId.tmp"
            $cacheMetadataTemporaryFile = "$cacheMetadataFile.$cachePublishId.tmp"
            try {
                Copy-Item -LiteralPath $dll.FullName -Destination $cacheTemporaryFile -Force
                & $resolvedSignTool verify /pa /q $cacheTemporaryFile *> $null
                if ($LASTEXITCODE -ne 0) {
                    throw "The signed DLL failed verification before caching: $($dll.FullName)"
                }
                $signedCacheHash = (Get-FileHash -LiteralPath $cacheTemporaryFile -Algorithm SHA256).Hash
                [System.IO.File]::WriteAllText($cacheMetadataTemporaryFile, $signedCacheHash)
                Move-Item -LiteralPath $cacheTemporaryFile -Destination $cachedFile -Force
                Move-Item -LiteralPath $cacheMetadataTemporaryFile -Destination $cacheMetadataFile -Force
                $cacheStoreCount++
            }
            catch {
                Write-Warning "Failed to update signature cache for $($dll.FullName): $($_.Exception.Message)"
            }
            finally {
                Remove-Item -LiteralPath $cacheTemporaryFile -Force -ErrorAction SilentlyContinue
                Remove-Item -LiteralPath $cacheMetadataTemporaryFile -Force -ErrorAction SilentlyContinue
            }
        }
    }
    finally {
        Remove-Item -LiteralPath $temporaryFile -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "DLL signing complete: signed=$signedCount, cache-hit=$cacheHitCount, cache-store=$cacheStoreCount, already-valid=$skippedCount, total=$($dlls.Count)"
