[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,

    [Parameter(Mandatory = $true)]
    [string]$SignServiceUrl
)

$ErrorActionPreference = 'Stop'

function Resolve-SignTool {
    $command = Get-Command 'signtool.exe' -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    foreach ($root in @('C:\Program Files (x86)\Windows Kits\10\bin', 'C:\Program Files\Windows Kits\10\bin')) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) { continue }
        $candidate = Get-ChildItem -LiteralPath $root -Directory |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'x64\signtool.exe' } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) { return $candidate }
    }
    throw 'signtool.exe was not found.'
}

function Invoke-Curl {
    param([string[]]$Arguments)
    $output = & 'C:\curl.exe' @Arguments
    if ($LASTEXITCODE -ne 0) { throw "curl.exe failed with exit code $LASTEXITCODE." }
    return $output
}

$resolvedPath = (Resolve-Path -LiteralPath $FilePath).Path
$temporaryFile = "$resolvedPath.signed.tmp"
$fileName = [Uri]::EscapeDataString([IO.Path]::GetFileName($resolvedPath))
$serviceUrl = $SignServiceUrl.TrimEnd('/')
$signTool = Resolve-SignTool
$verified = $false

try {
    for ($attempt = 1; $attempt -le 3 -and -not $verified; $attempt++) {
        Remove-Item -LiteralPath $temporaryFile -Force -ErrorAction SilentlyContinue
        Write-Host "[SIGN] $resolvedPath (upload $attempt/3)"
        $response = Invoke-Curl -Arguments @(
            '--fail', '--show-error', '--silent', '-X', 'POST',
            '-F', "file=@$resolvedPath", "$serviceUrl/sign"
        )
        $responseText = (@($response) -join "`n").Trim()
        if ($responseText -ne 'ok') {
            throw "The signing service rejected $resolvedPath. Response: $responseText"
        }

        Start-Sleep -Seconds 1
        for ($poll = 1; $poll -le 10; $poll++) {
            $cacheBuster = [DateTime]::UtcNow.Ticks
            Invoke-Curl -Arguments @(
                '--fail', '--show-error', '--silent', '--location',
                '--header', 'Cache-Control: no-cache',
                "$serviceUrl/exe/${fileName}?cacheBuster=$cacheBuster",
                '--output', $temporaryFile
            ) | Out-Null

            & $signTool verify /pa /q $temporaryFile *> $null
            if ($LASTEXITCODE -eq 0) {
                $verified = $true
                break
            }
            if ($poll -lt 10) { Start-Sleep -Seconds 1 }
        }

        if (-not $verified -and $attempt -lt 3) { Start-Sleep -Seconds 2 }
    }

    if (-not $verified) {
        & $signTool verify /pa /v $temporaryFile
        throw "The signing service returned a file that failed verification after 3 attempts: $resolvedPath"
    }

    Move-Item -LiteralPath $temporaryFile -Destination $resolvedPath -Force
    Write-Host "Signed file verified successfully: $resolvedPath"
}
finally {
    Remove-Item -LiteralPath $temporaryFile -Force -ErrorAction SilentlyContinue
}
