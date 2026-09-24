# FillTensor static libraries

Source: [CR_Close_library build #6](http://172.20.180.14:8081/job/CR_Close_library/6/artifact/artifacts/).
Jenkins result: `SUCCESS`; source revision: `a2b46ff4278bca9f6eb1c0862ac48cf5d1775a2a`; configuration: `Release`.

The four exported headers have identical contents after normalizing line endings.
The public API is unchanged from the feature branch; the header additionally supports
`CR_FILLTENSOR_STATIC_DEFINE`. All consumers must use that definition when linking these
archives. `libslic3r` publishes the definition and link dependency through CMake.
No FillTensor DLL is required.

| Jenkins platform | Repository library path under `cr_FillTensor/lib/` | SHA-256 |
| --- | --- | --- |
| windows-x64 | `win64/cr_FillTensor_library.lib` | `823249dfde71dc1e2e4759f8472c1ef6d9e3d6a5058b068ea94424a2e18095ec` |
| linux-x86_64 | `linux/x86_64/libcr_FillTensor_library.a` | `9b2afd15dedde11baa29f91d3b9a1a36e2496e928fcb7cbc3ca9f17a4cf99b1b` |
| macos-arm64 | `mac/arm64/libcr_FillTensor_library.a` | `a109f89772754488397729fb3a5bf9cfdddcad36cbb320bda4bd21d9e5ce74af` |
| macos-x86_64 | `mac/x86_64/libcr_FillTensor_library.a` | `3db9477a73b323100bc488338cb34b9b4fbb5020afbd1b6f51dc26b215c0ebfd` |

The `linux/x86_64` archive was recompiled from the same source revision with
`-fPIC` (CMake `CMAKE_POSITION_INDEPENDENT_CODE=ON`). The artifact published by
build #6 was compiled without position-independent code, so ld rejected it when
linking `liborcaservice.so`: `relocation R_X86_64_PC32 against symbol
'_ZSt7nothrow@@GLIBCXX_3.4' can not be used when making a shared object;
recompile with -fPIC`. The Jenkinsfile should pass
`-DCMAKE_POSITION_INDEPENDENT_CODE=ON` for the Linux and macOS targets so future
artifacts stay usable from shared libraries.

Build `dep_CR_FILLTENSOR` to install the header and archive into the dependency prefix,
then regenerate the application build. An old Windows import library with the same name
must be replaced by this static archive. The copy rule tracks both source files through
`DEPENDS` so updates are installed on subsequent builds.

The macOS selector uses `CMAKE_OSX_ARCHITECTURES` when set, to support separate target builds
from a different host architecture. Build arm64 and x86_64 dependencies separately; this
artifact set does not contain a universal archive, Windows ARM64, or Linux ARM64 libraries.

Validation performed on Windows: archive machine formats, all four platform selection/copy
rules, SHA-256 identity after installation, and native Windows static linking and execution.
The execution check covered all three cell types, 125 query points each, single/batch query
agreement, and 16 concurrent queries per cell type. These checks do not replace native
macOS/Linux application builds or full slicing regression tests.
