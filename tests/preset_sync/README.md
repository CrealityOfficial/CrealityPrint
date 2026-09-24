# Preset version and filename regression

Build and run without the GUI:

```powershell
cmake -S tests/preset_sync -B out/preset-sync-fix -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<dependency-prefix>
cmake --build out/preset-sync-fix
ctest --test-dir out/preset-sync-fix --output-on-failure
```

The dependency prefix must provide Boost headers. The tests exercise the production
selection and filename policy with:

- All six arrival orders of the three same-name cloud records from the log.
- Equal timestamps, invalid timestamps, and deterministic record-ID selection.
- Actual Windows case-insensitive file creation using the `WHITE` / `White` pair.
- Recovery of a legacy filename whose embedded name was overwritten.
- Separate JSON and `.info` ownership and logical names after re-reading files.
- Repeated saves, occupied fallback filenames, long names, and legacy manual renames.

Only temporary test files are written. These tests do not authenticate with the
cloud or run the full GUI preset-loading flow.
