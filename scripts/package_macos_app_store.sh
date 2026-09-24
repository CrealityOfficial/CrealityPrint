#!/bin/bash

# Sign a built .app for the Mac App Store and wrap it in a signed .pkg.
# The required identities are supplied by the CI keychain, never committed.

set -euo pipefail

APP_PATH="${1:?usage: package_macos_app_store.sh /path/to/App.app [output.pkg]}"
OUTPUT_PATH="${2:-${APP_PATH%.*}.pkg}"
SIGN_ONLY="${MAC_APP_STORE_SIGN_ONLY:-0}"
ENTITLEMENTS="${APP_STORE_ENTITLEMENTS:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/app_store.entitlements}"
APP_IDENTITY="${MAC_APP_SIGNING_IDENTITY:?set MAC_APP_SIGNING_IDENTITY to a Mac App Distribution identity}"
INSTALLER_IDENTITY="${MAC_INSTALLER_SIGNING_IDENTITY:?set MAC_INSTALLER_SIGNING_IDENTITY to a Mac Installer Distribution identity}"
PROVISIONING_PROFILE="${MAC_APP_PROVISIONING_PROFILE:?set MAC_APP_PROVISIONING_PROFILE to a Mac App Store provisioning profile}"
SIGNING_KEYCHAIN="${MAC_APP_SIGNING_KEYCHAIN:-}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS." >&2
    exit 1
fi
if [[ ! -d "$APP_PATH" || "${APP_PATH##*.}" != "app" ]]; then
    echo "Not a macOS app bundle: $APP_PATH" >&2
    exit 1
fi
if [[ ! -f "$ENTITLEMENTS" ]]; then
    echo "Entitlements file not found: $ENTITLEMENTS" >&2
    exit 1
fi
if [[ ! -f "$PROVISIONING_PROFILE" ]]; then
    echo "Provisioning profile not found: $PROVISIONING_PROFILE" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT_PATH")"

SIGNING_KEYCHAIN_ARGS=()
if [[ -n "$SIGNING_KEYCHAIN" ]]; then
    [[ -f "$SIGNING_KEYCHAIN" ]] || { echo "Signing keychain not found: $SIGNING_KEYCHAIN" >&2; exit 1; }
    SIGNING_KEYCHAIN_ARGS=(--keychain "$SIGNING_KEYCHAIN")
fi

TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/crealityprint-app-store.XXXXXX")"
trap 'rm -rf "$TEMP_DIR"' EXIT
PROFILE_PLIST="$TEMP_DIR/profile.plist"
SIGNED_ENTITLEMENTS="$TEMP_DIR/entitlements.plist"
ACTUAL_ENTITLEMENTS="$TEMP_DIR/actual-entitlements.plist"

security cms -D -i "$PROVISIONING_PROFILE" > "$PROFILE_PLIST"

BUNDLE_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP_PATH/Contents/Info.plist")"
BUNDLE_SHORT_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP_PATH/Contents/Info.plist")"
BUNDLE_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$APP_PATH/Contents/Info.plist")"
PROFILE_APP_ID="$(/usr/libexec/PlistBuddy -c 'Print :Entitlements:com.apple.application-identifier' "$PROFILE_PLIST" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c 'Print :Entitlements:application-identifier' "$PROFILE_PLIST")"
PROFILE_TEAM_ID="$(/usr/libexec/PlistBuddy -c 'Print :Entitlements:com.apple.developer.team-identifier' "$PROFILE_PLIST")"
EXPECTED_APP_ID="${PROFILE_TEAM_ID}.${BUNDLE_ID}"

if [[ ! "$BUNDLE_SHORT_VERSION" =~ ^[0-9]+(\.[0-9]+){0,2}$ ]] || [[ ! "$BUNDLE_VERSION" =~ ^[0-9]+(\.[0-9]+){0,2}$ ]]; then
    echo "CFBundleShortVersionString and CFBundleVersion must contain one to three numeric components." >&2
    exit 1
fi

if [[ "$PROFILE_APP_ID" != "$EXPECTED_APP_ID" ]]; then
    echo "Provisioning profile application identifier '$PROFILE_APP_ID' does not match '$EXPECTED_APP_ID'." >&2
    exit 1
fi

# The profile is part of the signed bundle. Derive the signing-only identifiers
# from it so the CI configuration never hard-codes a Team ID.
cp "$ENTITLEMENTS" "$SIGNED_ENTITLEMENTS"
/usr/libexec/PlistBuddy -c "Set :com.apple.application-identifier $PROFILE_APP_ID" "$SIGNED_ENTITLEMENTS" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Add :com.apple.application-identifier string $PROFILE_APP_ID" "$SIGNED_ENTITLEMENTS"
/usr/libexec/PlistBuddy -c "Set :com.apple.developer.team-identifier $PROFILE_TEAM_ID" "$SIGNED_ENTITLEMENTS" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Add :com.apple.developer.team-identifier string $PROFILE_TEAM_ID" "$SIGNED_ENTITLEMENTS"
cp "$PROVISIONING_PROFILE" "$APP_PATH/Contents/embedded.provisionprofile"

# Sign nested code first, then apply App Store entitlements to the bundle.
codesign --deep --force --options runtime --timestamp "${SIGNING_KEYCHAIN_ARGS[@]}" --sign "$APP_IDENTITY" "$APP_PATH"
codesign --force --options runtime --timestamp "${SIGNING_KEYCHAIN_ARGS[@]}" --entitlements "$SIGNED_ENTITLEMENTS" --sign "$APP_IDENTITY" "$APP_PATH"
codesign --verify --deep --strict --verbose=2 "$APP_PATH"
codesign --display --entitlements :- "$APP_PATH" 2>/dev/null > "$ACTUAL_ENTITLEMENTS"

if [[ "$(/usr/libexec/PlistBuddy -c 'Print :com.apple.security.app-sandbox' "$ACTUAL_ENTITLEMENTS")" != "true" ]] || \
   [[ "$(/usr/libexec/PlistBuddy -c 'Print :com.apple.application-identifier' "$ACTUAL_ENTITLEMENTS")" != "$PROFILE_APP_ID" ]]; then
    echo "Signed app is missing the required Mac App Store entitlements." >&2
    exit 1
fi

if [[ "$SIGN_ONLY" == "1" ]]; then
    echo "Signed and verified $APP_PATH"
    exit 0
fi

productbuild --component "$APP_PATH" /Applications --sign "$INSTALLER_IDENTITY" "$OUTPUT_PATH"
pkgutil --check-signature "$OUTPUT_PATH"
echo "Created $OUTPUT_PATH"
