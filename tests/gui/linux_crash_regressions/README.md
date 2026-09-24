# Linux font and filament drag regressions

The Ubuntu 26 dumps from CrealityPrint 7.3.0.6123 identified two independent
paths: Pango 1.57.0 looking up a missing font family while GTK populates its emoji
chooser, and nested GTK native drag loops in the filament grouping dialog.

AppImage packages a private, coherent Pango 1.52.2 runtime with an explicit
missing-family guard. This version builds against the Ubuntu 24 packaging
host's GLib, HarfBuzz and Fontconfig. The private installation does not replace
the headers or pkg-config files used to build wxWidgets. The corresponding
upstream fix for Pango 1.57.0 is
https://github.com/GNOME/pango/commit/c7dcffdc86a2ac4fe8e176aac3a6b71cb79adbe4.

Filament grouping now uses mouse capture within the dialog, without a native
drag event loop. Card rebuilds wait until capture ends. Cancel, capture loss,
dialog close and Escape release capture before controls can be destroyed.

## Build and run

On the Linux packaging host, after configuring the dependency build:

```sh
cmake --build deps/build --target dep_Pango -j2
prefix="$PWD/deps/build/destdir/usr/local"
cmake -S tests/gui/linux_crash_regressions -B /tmp/creality-crash-tests \
  -DCMAKE_PREFIX_PATH="$prefix" \
  -DwxWidgets_CONFIG_EXECUTABLE="$prefix/bin/wx-config" \
  -DPANGO_RUNTIME_DIR="$prefix/lib/creality-pango/lib"
cmake --build /tmp/creality-crash-tests -j2
ctest --test-dir /tmp/creality-crash-tests --output-on-failure
```

The font test injects a missing family into a font pattern and treats GLib
criticals as fatal; it also lays out U+1FAEA, the character seen in the dump.
The drag test runs real wx controls under Xvfb and checks movement thresholds,
successful release, duplicate presses, cancellation, capture loss and source
destruction. Install `xvfb` to run it.
It also checks the shared in-dialog chip preview: showing it preserves focus
and mouse capture, dialog edges constrain it, and finishing or cancelling a
drag removes it. The preview uses child-window coordinates on all platforms.

The event source lifetime test exercises wx event delivery and weak reference
invalidation over 1000 cycles, with source-first destruction, listener-first
destruction and source replacement. This checks the wx lifetime contract used
by MCPChatPanel's export subscription; it does not instantiate MCPChatPanel.
Also verify application exit after opening AI chat and after recreating the
interface (for example, changing the language). Embedded AI chat must subscribe
to its owning Plater, rather than the previous frame's global Plater.

These tests do not replace Ubuntu 26 Wayland verification: open GTK's emoji
chooser and repeatedly move filament chips between cards, release outside a
card, cancel with Escape, and close/reopen the dialog. Verify that incompatible
cards reject drops and that confirmed filament assignments remain correct.
