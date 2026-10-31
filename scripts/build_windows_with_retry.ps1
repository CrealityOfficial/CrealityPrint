[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string]$Configuration,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, 2147483647)]
    [int]$Parallel,

    [string]$Target
)

$ErrorActionPreference = 'Stop'
$cmakePath = (Get-Command 'cmake.exe' -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$maxAttempts = 3
$buildArguments = @('--build', $BuildDirectory, '--config', $Configuration, '--parallel', $Parallel)
if (-not [string]::IsNullOrWhiteSpace($Target)) {
    $buildArguments += @('--target', $Target)
}

for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
    $manifestWriteFailed = $false
    Write-Host "Windows build attempt $attempt/$maxAttempts"

    # Windows PowerShell wraps native stderr in ErrorRecord objects. Keep it
    # streaming to Jenkins without treating stderr alone as a build failure.
    $ErrorActionPreference = 'Continue'
    try {
        & $cmakePath @buildArguments 2>&1 |
            ForEach-Object {
                $line = $_.ToString()
                Write-Host $line
                # Match the error code regardless of localization or mt.exe exit-code formatting.
                if ($line -match '\bc101008d\b') {
                    $manifestWriteFailed = $true;
                }
            }
        $buildExitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = 'Stop'
    }

    if ($buildExitCode -eq 0) {
        exit 0
    }
    if (-not $manifestWriteFailed -or $attempt -eq $maxAttempts) {
        Write-Host "Windows build failed with exit code $buildExitCode after $attempt attempt(s)."
        exit $buildExitCode
    }

    Write-Host 'Manifest resource update failed (C101008D); retrying the build in 2 seconds...'
    Start-Sleep -Seconds 2
}
