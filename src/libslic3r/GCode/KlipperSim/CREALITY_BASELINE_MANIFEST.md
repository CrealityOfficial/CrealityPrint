# creality_klipper phase 0 baseline

This manifest fixes the authoritative firmware inputs for the streaming
pipeline refactor. `119_klipper` is intentionally excluded.

| SHA-256 | File |
|---|---|
| `72754c5f5a1aaed2384650b360296b609be8f3c283521a1fb31b8be2d50f7a72` | `klippy/gcode.py` |
| `0f7cc78c1dfc7181034258b4354b06e4500aba2936a5e0e7f89e0020a2d3d2d1` | `klippy/extras/virtual_sdcard.py` |
| `e6fa7c2fdd52c837b43470dc6b57f4b817559e03d5f5e1794aaba42895f0ac77` | `klippy/extras/gcode_move.py` |
| `9ff056015c0d0add094069c6f0a98cf1af092fa092baf02b189427d8527a08ac` | `klippy/toolhead.py` |
| `434256436260cbc783606e0dd28acd6712a90137056e59239c501573e2cd7805` | `klippy/mcu.py` |
| `17ca804a7dd586d5db622e4a0d66de057eb5cba45ebf58e38de526664ccecd5b` | `klippy/chelper/trapq.c` |
| `473e01a2ea248681bde7e7dcedbc9b0415bb2308c5cc16335b92140693762aba` | `klippy/chelper/itersolve.c` |
| `196a0bdc3ad194c5e18130eeb1a9a424fe35e253eb7903a92def808ed7bf05b6` | `klippy/chelper/stepcompress.c` |
| `5626fc9a48f0c45c0cce2e8f92888f1781a2f4ce339dbfcf7c5dfeb58992cf52` | `klippy/chelper/serialqueue.c` |

Repository prefix for every path above:

```text
klipper_src/creality_klipper/klipper/
```

Initial regression corpus:

- `testdata/streaming_phase0/continuous.gcode`
- `testdata/streaming_phase0/microsegments.gcode`
- `testdata/streaming_phase0/barriers.gcode`
- `testdata/streaming_phase0/aux_interleave.gcode`
- `testdata/streaming_phase0/state_changes.gcode`
- `testdata/streaming_phase0/homing.gcode`

Existing high-load queue corpus (kept in place because the files are large):

| SHA-256 | File |
|---|---|
| `d90cdc4868600f38206259480b9d0420672c64b129dde37eb2da0d943b3dd207` | `klipper_src/failed.gcode` |
| `e69734ea755add6947a0c526ea0fe6093156d62e426f51d439bb916ad54d3dcd` | `klipper_src/successful.gcode` |

The phase 0/1/2/3 test executable validates parser state, immediate move delivery,
barrier delivery, lookahead separation, `G4` time advancement, G92 physical
coordinates, batch-adapter equivalence, per-window A/B/E step clocks, compact
versus diagnostic results, and bounded trapq history release.

Non-interactive test command (run at repository root):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File src/libslic3r/GCode/KlipperSim/run_streaming_phase_tests.ps1
```

Stable expectations encoded by the test are: 4 continuous moves with joined
lookahead, 10 microsegments, 5 moves separated by 4 barriers, and 7 auxiliary
events which do not create synchronization boundaries.
