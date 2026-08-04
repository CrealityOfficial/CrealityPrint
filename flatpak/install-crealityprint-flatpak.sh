#!/usr/bin/env bash
set -euo pipefail

APP_ID="io.github.crealityofficial.CrealityPrint"
APP_NAME="CrealityPrint"

script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
bundle_path="${1:-}"

if [ -z "$bundle_path" ]; then
    bundle_path=$(find "$script_dir" -maxdepth 1 -type f -name 'CrealityPrint-Linux-flatpak_*.flatpak' -printf '%T@ %p\n' 2>/dev/null | sort -nr | awk 'NR == 1 {print $2}')
fi

if [ -z "$bundle_path" ] || [ ! -f "$bundle_path" ]; then
    echo "Could not find a CrealityPrint flatpak bundle."
    echo "Usage: $0 /path/to/CrealityPrint-Linux-flatpak_*.flatpak"
    exit 1
fi

if ! command -v flatpak >/dev/null 2>&1; then
    echo "flatpak is not installed. Please install flatpak first."
    exit 1
fi

install_scope="${CREALITYPRINT_FLATPAK_INSTALLATION:-user}"
case "$install_scope" in
    user)
        flatpak_install_args=(--user)
        ;;
    system)
        flatpak_install_args=(--system)
        ;;
    *)
        echo "CREALITYPRINT_FLATPAK_INSTALLATION must be 'user' or 'system'."
        exit 1
        ;;
esac

echo "Installing $APP_NAME from $bundle_path..."
flatpak install "${flatpak_install_args[@]}" -y "$bundle_path"

deploy_dir=$(flatpak info --show-location "$APP_ID" 2>/dev/null || true)
if [ -z "$deploy_dir" ] && [ "$install_scope" = "user" ]; then
    deploy_dir=$(flatpak info --user --show-location "$APP_ID" 2>/dev/null || true)
fi
if [ -z "$deploy_dir" ] && [ "$install_scope" = "system" ]; then
    deploy_dir=$(flatpak info --system --show-location "$APP_ID" 2>/dev/null || true)
fi

if [ -z "$deploy_dir" ] || [ ! -d "$deploy_dir" ]; then
    echo "Installed $APP_NAME, but could not locate the Flatpak deployment."
    exit 1
fi

xdg_data_home="${XDG_DATA_HOME:-$HOME/.local/share}"
desktop_dir="$xdg_data_home/applications"
icon_dir="$xdg_data_home/icons"
mkdir -p "$desktop_dir" "$icon_dir"

desktop_dst="$desktop_dir/$APP_ID.desktop"
desktop_src=""
for candidate in \
    "$deploy_dir/export/share/applications/$APP_ID.desktop" \
    "$deploy_dir/files/share/applications/$APP_ID.desktop" \
    "$deploy_dir/files/share/applications/$APP_NAME.desktop"; do
    if [ -f "$candidate" ]; then
        desktop_src="$candidate"
        break
    fi
done

if [ -n "$desktop_src" ]; then
    install -m 0644 "$desktop_src" "$desktop_dst"
else
    cat > "$desktop_dst" <<EOF
[Desktop Entry]
Name=$APP_NAME
GenericName=3D Printing Software
Icon=$APP_ID
Exec=flatpak run --branch=master --arch=x86_64 --command=entrypoint --file-forwarding $APP_ID @@u %U @@
Terminal=false
Type=Application
MimeType=model/stl;model/3mf;application/vnd.ms-3mfdocument;application/prs.wavefront-obj;application/x-amf;x-scheme-handler/crealityprintlink;
Categories=Graphics;Utility;3DGraphics;Engineering;
Keywords=3D;Printing;Slicer;slice;3D;printer;convert;gcode;stl;obj;amf;SLA
StartupNotify=false
StartupWMClass=$APP_NAME
X-Flatpak=$APP_ID
EOF
    chmod 0644 "$desktop_dst"
fi

for icon_root in "$deploy_dir/export/share/icons" "$deploy_dir/files/share/icons"; do
    [ -d "$icon_root" ] || continue
    while IFS= read -r -d '' icon_src; do
        rel_path=${icon_src#"$icon_root/"}
        install -Dm644 "$icon_src" "$icon_dir/$rel_path"
    done < <(find "$icon_root" -type f \( -name "$APP_ID.*" -o -name "$APP_NAME.*" \) -print0)
done

if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$desktop_dst" || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$desktop_dir" >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1 && [ -d "$icon_dir/hicolor" ]; then
    gtk-update-icon-cache -q -f "$icon_dir/hicolor" >/dev/null 2>&1 || true
fi

if command -v systemctl >/dev/null 2>&1; then
    systemctl --user import-environment XDG_DATA_HOME XDG_DATA_DIRS >/dev/null 2>&1 || true
fi

echo "$APP_NAME has been installed."
echo "A launcher was installed to $desktop_dst."
