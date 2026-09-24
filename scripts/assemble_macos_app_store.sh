#!/bin/bash

# Assemble two CMake-produced bundles into one signed Mac App Store app.
# productbuild is deliberately excluded: it must run in the GUI LaunchAgent.

set -euo pipefail

ARM_APP="${1:?usage: assemble_macos_app_store.sh <arm64.app> <x86_64.app> <output-dir>}"
X86_APP="${2:?usage: assemble_macos_app_store.sh <arm64.app> <x86_64.app> <output-dir>}"
OUTPUT_DIR="${3:?usage: assemble_macos_app_store.sh <arm64.app> <x86_64.app> <output-dir>}"
APP_NAME="${APP_NAME:-CrealityPrint}"
VERSION="${APP_STORE_VERSION:?set APP_STORE_VERSION}"
BUILD_NUMBER="${APP_STORE_BUILD_NUMBER:?set APP_STORE_BUILD_NUMBER}"
FIREBASE_CONFIG="${APP_STORE_FIREBASE_CONFIG:?set APP_STORE_FIREBASE_CONFIG}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_APP="$OUTPUT_DIR/$APP_NAME.app"
INFO_PLIST="$OUTPUT_APP/Contents/Info.plist"

for app in "$ARM_APP" "$X86_APP"; do
    [[ -d "$app/Contents/MacOS" ]] || { echo "Invalid app bundle: $app" >&2; exit 1; }
done
[[ -f "$FIREBASE_CONFIG" ]] || { echo "App Store Firebase configuration is missing" >&2; exit 1; }

rm -rf "$OUTPUT_APP"
mkdir -p "$OUTPUT_DIR"
ditto "$ARM_APP" "$OUTPUT_APP"
install -m 600 "$FIREBASE_CONFIG" "$OUTPUT_APP/Contents/Resources/GoogleService-Info.plist"
lipo -create \
    "$ARM_APP/Contents/MacOS/$APP_NAME" \
    "$X86_APP/Contents/MacOS/$APP_NAME" \
    -output "$OUTPUT_APP/Contents/MacOS/$APP_NAME"

plist_value() {
    /usr/libexec/PlistBuddy -c "Print :$1" "$INFO_PLIST"
}

[[ "$(plist_value CFBundleExecutable)" == "$APP_NAME" ]] || { echo "Unexpected CFBundleExecutable" >&2; exit 1; }
[[ "$(plist_value CFBundleShortVersionString)" == "$VERSION" ]] || { echo "Unexpected CFBundleShortVersionString" >&2; exit 1; }
[[ "$(plist_value CFBundleVersion)" == "$BUILD_NUMBER" ]] || { echo "Unexpected CFBundleVersion" >&2; exit 1; }
[[ "$(plist_value LSApplicationCategoryType)" == "public.app-category.graphics-design" ]] || { echo "Unexpected bundle category" >&2; exit 1; }
[[ "$(plist_value ITSAppUsesNonExemptEncryption)" == "false" ]] || { echo "Missing encryption declaration" >&2; exit 1; }
[[ -f "$OUTPUT_APP/Contents/Resources/GoogleService-Info.plist" ]] || { echo "GoogleService-Info.plist is missing" >&2; exit 1; }

xattr -cr "$OUTPUT_APP"
chmod -R go+r "$OUTPUT_APP"
export MAC_APP_STORE_SIGN_ONLY=1
bash "$SCRIPT_DIR/package_macos_app_store.sh" "$OUTPUT_APP"

lipo -archs "$OUTPUT_APP/Contents/MacOS/$APP_NAME" | grep -Eq '(^| )arm64( |$).*x86_64|(^| )x86_64( |$).*arm64'
echo "Created signed universal app: $OUTPUT_APP"
