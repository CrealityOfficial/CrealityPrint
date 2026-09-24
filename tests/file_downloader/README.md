# Preset download regression

This standalone target builds the production `CurlConnectionPool` with the
repository's bundled Boost.Nowide headers. It does not require the GUI.

```powershell
cmake -S tests/file_downloader -B out/preset-download-fix -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<dependency-prefix>
cmake --build out/preset-download-fix
ctest --test-dir out/preset-download-fix --output-on-failure
```

The dependency prefix must supply Boost headers and libcurl with its dependencies.
The standalone build requests static zlib. On a reused Windows build directory,
if CMake has cached the zlib import library, configure with
`-DZLIB_LIBRARY_RELEASE=<dependency-prefix>/lib/zlibstatic.lib` as well.

The tests use a loopback HTTP server and temporary output directories. They check
1000 queued files, an actual concurrency limit of three, bounded process handle
growth on Windows, exact downloaded bytes, UTF-8 paths, truncated responses,
cancellation with partial-file cleanup, failed file opens, an empty queue, and a
nonpositive concurrency setting.

Optionally exercise real preset files without uploading them to a remote server:

```powershell
python tests/file_downloader/regression.py out/preset-download-fix/file_downloader_test_client.exe --presets <preset-directory>
```

This verifies file transfer integrity; it does not exercise GUI preset loading or
authenticate against the cloud service.
