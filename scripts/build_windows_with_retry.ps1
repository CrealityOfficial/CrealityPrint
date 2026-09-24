[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string]$Configuration,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, 2147483647)]
    [int]$Parallel
)

$ErrorActionPreference = 'Stop'
$cmakePath = (Get-Command 'cmake.exe' -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$maxAttempts = 3

for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
    $manifestWriteFailed = $false
    $manifestAccessDenied = $false
    Write-Host "Windows build attempt $attempt/$maxAttempts"

    # Windows PowerShell wraps native stderr in ErrorRecord objects. Keep it
    # streaming to Jenkins without treating stderr alone as a build failure.
    $ErrorActionPreference = 'Continue'
    try {
        & $cmakePath --build $BuildDirectory --config $Configuration --parallel $Parallel 2>&1 |
            ForEach-Object {
                $line = $_.ToString()
                Write-Host $line
                if ($line -match 'mt\.exe\s*:\s*general error c101008d:.*Failed to write the updated manifest') {
                    $manifestWriteFailed = $true
                }
                # CMake reports the exit code independently of the localized
                # Windows error text, which can be garbled on Jenkins agents.
                if ($line -match '^MT: command .*mt\.exe.* failed \(exit code 0x1f\)') {
                    $manifestAccessDenied = $true
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
    if (-not ($manifestWriteFailed -and $manifestAccessDenied) -or $attempt -eq $maxAttempts) {
        Write-Host "Windows build failed with exit code $buildExitCode after $attempt attempt(s)."
        exit $buildExitCode
    }

    Write-Host 'Manifest resource update failed (c101008d, mt.exe exit code 0x1f); retrying the build in 2 seconds...'
    Start-Sleep -Seconds 2
}
