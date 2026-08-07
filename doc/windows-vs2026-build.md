# Windows VS2026/v143 clean build

This is an additive Windows x64 Release route for Visual Studio 2026. It uses
the v143 compatibility toolset rather than the VS2026 default toolset, and it
does not replace the existing VS2019, VS2022, Ninja, ARM64, or CI build paths.

## Prerequisites

- Visual Studio 2026 with the Desktop development with C++ workload.
- The exact MSVC v143 `14.44.35207` x64/x64 tools.
- Windows SDK 10.0.26100.0.
- The CMake bundled with the selected Visual Studio 2026 instance. The Visual
  Studio 18 generator first appeared in CMake 4.2.
- Git.
- Native Windows Strawberry Perl at `C:\Strawberry\perl\bin\perl.exe`.
- At least 150 GiB of free space for a complete clean dependency and
  application build.

The script changes only its child process environment. It does not modify the
machine PATH or install software.

## Build

From PowerShell 7 at the repository root:

```powershell
pwsh -File scripts/build_windows_vs2026.ps1 -Target All -Fresh
```

Build only the dependency prefix:

```powershell
pwsh -File scripts/build_windows_vs2026.ps1 -Target Dependencies -Fresh
```

Build the application against an existing dependency prefix:

```powershell
pwsh -File scripts/build_windows_vs2026.ps1 -Target Application -Fresh
```

If Visual Studio or Perl is installed elsewhere, pass `-VisualStudioPath` or
`-PerlPath`. An explicit Perl path is authoritative for the OpenSSL configure
step as well as the wrapper PATH. The script verifies the generator, x64
platform, exact v143 toolset, SDK, compiler PATH, Perl, gettext tools, output
containment, and free disk before it deletes any marked generated directory.
For the application configure, CMake package-root hints and user/system package
registries are disabled, material package-specific environment hints are
cleared, and every absolute cache path must remain under the repository, the
fresh dependency prefix, the selected Visual Studio/SDK roots, or the exact
recorded Git executable. This prevents an ambient Boost, OpenSSL, wxWidgets,
NLopt, or other package installation from replacing the freshly built input.

`-Jobs` defaults to 8 and is a real concurrency contract: dependency child
builds, application compilation, translation generation, and installation use
that bound, while the top-level dependency graph remains serialized to avoid
nested oversubscription. Ambient CMake/MSBuild parallelism variables are
replaced for the child process and restored when the script exits.

## Presets and outputs

The dependency and application preset is
`windows-vs2026-v143-x64-release`. Its paths are:

```text
out/build/deps/windows-vs2026-v143-x64-release
out/install/deps/windows-vs2026-v143-x64-release/usr/local
out/build/app/windows-vs2026-v143-x64-release
out/install/app/windows-vs2026-v143-x64-release
out/temp/windows-vs2026-v143-x64-release
out/download-cache
```

The source download cache is intentionally retained by `-Fresh`; compiled
dependency output, application output, and both install trees are regenerated.
For any fresh application build, the wrapper also validates the source-tree
translation output directory, removes only the exact expected generated `.mo`
files, and requires the exact language set to be regenerated.
Cleanup is limited to exact paths carrying a repository-specific marker. The
guarded reset unlinks only reparse-point entries found beneath that validated
generated root, verifies none remain, and only then removes the ordinary tree.
The script never calls `git clean`, the private UnitTest submodule, private build
servers, packaging, signing, installers, printers, or devices.

The normal Release preset enables Breakpad and does not enable unit tests. Its
historical non-reproducible build-time behavior remains unchanged. A successful
local build proves this route only on the machine where it ran; the
existing GitHub Actions jobs continue to exercise their legacy VS2022 route.

The public VS2026 source preset explicitly allows the untracked offline
`MicrosoftEdgeWebView2RuntimeInstallerX64.exe` packaging input to be absent.
`MicrosoftEdgeSetup.exe`, the online bootstrap installer, is still installed,
but the resulting developer install tree cannot recover WebView2 without
network access. Other Windows routes retain the historical fail-closed
requirement by default. Packaging, installer execution, and GUI/runtime WebView2
recovery have not been validated by this build route.
