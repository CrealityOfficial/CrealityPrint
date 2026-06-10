#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

unset GTK_PATH
unset GIO_MODULE_DIR
unset GDK_PIXBUF_MODULE_FILE
unset GDK_PIXBUF_MODULEDIR
unset LD_LIBRARY_PATH
unset LD_PRELOAD
unset SNAP
unset SNAP_ARCH
unset SNAP_COMMON
unset SNAP_CONTEXT
unset SNAP_COOKIE
unset SNAP_DATA
unset SNAP_INSTANCE_KEY
unset SNAP_INSTANCE_NAME
unset SNAP_LIBRARY_PATH
unset SNAP_NAME
unset SNAP_REAL_HOME
unset SNAP_REEXEC
unset SNAP_REVISION
unset SNAP_USER_COMMON
unset SNAP_USER_DATA
unset SNAP_VERSION

export XDG_DATA_DIRS="/usr/share/ubuntu:/usr/share/gnome:/usr/local/share:/usr/share:/var/lib/snapd/desktop"

exec "$ROOT_DIR/build/src/CrealityPrint" "$@"
