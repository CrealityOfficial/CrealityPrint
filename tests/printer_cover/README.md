# Printer cover regression tests

Build this standalone target with the same Boost, wxWidgets and FFmpeg dependencies
as the application. Point `SLIC3R_GENERATED_INCLUDE_DIR` to the application's generated
`libslic3r_version.h` when using a build directory other than `build`.

```sh
cmake -S tests/printer_cover -B build/printer_cover_tests -DCMAKE_PREFIX_PATH=<dependency-prefix>
cmake --build build/printer_cover_tests --config Release
ctest --test-dir build/printer_cover_tests -C Release --output-on-failure
```

On Windows, run from a Visual Studio developer shell and make the dependency DLLs
available on `PATH`. On headless Linux, run the tests with a virtual display for
wxWidgets initialization.

The tests replace HTTP transport with deterministic responses and use temporary
directories, including non-ASCII paths. They exercise the production downloader,
PNG/JPEG conversion, WebP alpha (when FFmpeg is enabled), disk cache precedence,
duplicate nozzle downloads, URL changes, corrupt-image repair, failure cooldown,
preservation of old images, and the revision used to refresh same-model textures.

Manual application checks:

1. Install a model through parameter update that is absent from bundled profiles.
   Confirm its cover appears immediately and still appears after restarting offline.
2. Delete that model's user cover but retain its current parameter version and
   machine list. Restart the application; the cover should
   be downloaded without updating the parameter version.
3. Change the cover URL and update the parameter package. Verify the selected
   model and device cards refresh without switching models.
4. Simulate a failed image download. Parameter update should still complete, and
   an existing image or the default printer image should remain visible.
