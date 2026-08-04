#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  scripts/build_flatpak_arm_lima.sh [VERSION] [APP_NAME] [RTYPE]

Examples:
  scripts/build_flatpak_arm_lima.sh 7.1.0.123 CrealityPrint Beta

Environment overrides:
  LIMA_VM_NAME        Lima VM name. Default: flatpak-arm
  LIMA_TEMPLATE       Lima template used when the VM does not exist. Default: template:ubuntu-24.04
  LIMA_CPUS           VM CPU count. Default: all host CPUs
  LIMA_MEMORY_GIB     VM memory in GiB. Default: host memory minus 2GiB
  LIMA_DISK_GIB       VM disk size in GiB. Default: 250
  HOST_RESERVED_GIB   Host memory kept outside the VM. Default: 2
  BUILD_JOBS          Build parallelism inside the VM. Default: VM CPU count
  FLATPAK_STATE_DIR   Persistent flatpak-builder state dir in the VM. Default: /var/tmp/C3DSlicer-flatpak-builder-state
  KEEP_LIMA_RUNNING   Set to 1 to keep the VM running after the build. Default: 0
EOF
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    die "this script must run on the M2 macOS host, not inside Linux or Windows"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

VERSION="${1:-}"
APP_NAME="${2:-CrealityPrint}"
RTYPE="${3:-Beta}"

if [[ -z "${VERSION}" ]]; then
    if git -C "${ROOT_DIR}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        TAG_NAME="$(git -C "${ROOT_DIR}" describe --tags --abbrev=0 2>/dev/null || echo 7.1.0)"
        TAGNUMB="$(git -C "${ROOT_DIR}" rev-list HEAD --count)"
        VERSION="${TAG_NAME}.${TAGNUMB}"
    else
        VERSION="7.1.0.local"
    fi
fi

VM_NAME="${LIMA_VM_NAME:-flatpak-arm}"
LIMA_TEMPLATE="${LIMA_TEMPLATE:-template:ubuntu-24.04}"
LIMA_DISK_GIB="${LIMA_DISK_GIB:-250}"
HOST_RESERVED_GIB="${HOST_RESERVED_GIB:-2}"

if command -v limactl >/dev/null 2>&1; then
    LIMACTL="$(command -v limactl)"
elif [[ -x "${HOME}/.local/bin/limactl" ]]; then
    LIMACTL="${HOME}/.local/bin/limactl"
else
    die "limactl not found. Install Lima first or add limactl to PATH"
fi

HOST_CPUS="$(sysctl -n hw.ncpu)"
HOST_MEM_BYTES="$(sysctl -n hw.memsize)"
HOST_MEM_GIB="$((HOST_MEM_BYTES / 1024 / 1024 / 1024))"

VM_CPUS="${LIMA_CPUS:-${HOST_CPUS}}"
VM_MEMORY_GIB="${LIMA_MEMORY_GIB:-$((HOST_MEM_GIB - HOST_RESERVED_GIB))}"
if (( VM_MEMORY_GIB < 2 )); then
    VM_MEMORY_GIB=2
fi
BUILD_JOBS="${BUILD_JOBS:-${VM_CPUS}}"
BUNDLE_NAME="${APP_NAME}-V${VERSION}-linux-aarch64-${RTYPE}.flatpak"
GUEST_ROOT="${GUEST_ROOT:-/var/tmp/C3DSlicer-flatpak-src}"
FLATPAK_STATE_DIR="${FLATPAK_STATE_DIR:-/var/tmp/C3DSlicer-flatpak-builder-state}"

VM_DIR="${HOME}/.lima/${VM_NAME}"
VM_YAML="${VM_DIR}/lima.yaml"

update_lima_yaml() {
    local yaml="$1"
    python3 - "$yaml" "$VM_CPUS" "$VM_MEMORY_GIB" "$LIMA_DISK_GIB" <<'PY'
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
cpus = sys.argv[2]
memory = f"{sys.argv[3]}GiB"
disk = f"{sys.argv[4]}GiB"
text = path.read_text()

def set_key(src, key, value):
    pattern = rf"(?m)^{key}:\s*.*$"
    replacement = f"{key}: {value}"
    if re.search(pattern, src):
        return re.sub(pattern, replacement, src, count=1)
    return src.rstrip() + f"\n{replacement}\n"

def ensure_home_mount_writable(src):
    lines = src.splitlines()
    out = []
    i = 0
    changed = False
    while i < len(lines):
        line = lines[i]
        out.append(line)
        if re.match(r"\s*-\s+location:\s*['\"]?~['\"]?\s*$", line):
            i += 1
            while i < len(lines) and (not lines[i].strip() or re.match(r"\s+writable:\s*", lines[i])):
                i += 1
            out.append("  writable: true")
            changed = True
            continue
        i += 1
    if not changed:
        out.extend(["mounts:", '- location: "~"', "  writable: true"])
    return "\n".join(out) + "\n"

text = set_key(text, "cpus", cpus)
text = set_key(text, "memory", memory)
text = set_key(text, "disk", disk)
text = ensure_home_mount_writable(text)
path.write_text(text)
PY
}

stop_vm() {
    if [[ "${KEEP_LIMA_RUNNING:-0}" == "1" ]]; then
        echo "KEEP_LIMA_RUNNING=1, leave Lima VM '${VM_NAME}' running."
        return
    fi
    echo "Stopping Lima VM '${VM_NAME}'..."
    "${LIMACTL}" stop "${VM_NAME}" >/dev/null 2>&1 || true
}

trap stop_vm EXIT

if ! "${LIMACTL}" list "${VM_NAME}" >/dev/null 2>&1; then
    echo "Creating Lima VM '${VM_NAME}' with ${VM_CPUS} CPUs, ${VM_MEMORY_GIB}GiB memory, ${LIMA_DISK_GIB}GiB disk..."
    "${LIMACTL}" start -y \
        --name "${VM_NAME}" \
        --arch aarch64 \
        --cpus "${VM_CPUS}" \
        --memory "${VM_MEMORY_GIB}" \
        --disk "${LIMA_DISK_GIB}" \
        --mount-writable \
        "${LIMA_TEMPLATE}"
else
    echo "Preparing Lima VM '${VM_NAME}' with ${VM_CPUS} CPUs and ${VM_MEMORY_GIB}GiB memory..."
    "${LIMACTL}" stop "${VM_NAME}" >/dev/null 2>&1 || true
    update_lima_yaml "${VM_YAML}"
    "${LIMACTL}" start "${VM_NAME}"
fi

echo "Build settings:"
echo "  VM:       ${VM_NAME}"
echo "  CPUs:     ${VM_CPUS}"
echo "  Memory:   ${VM_MEMORY_GIB}GiB"
echo "  Jobs:     ${BUILD_JOBS}"
echo "  Version:  ${VERSION}"
echo "  App:      ${APP_NAME}"
echo "  RTYPE:    ${RTYPE}"
echo

"${LIMACTL}" shell "${VM_NAME}" bash -s -- \
    "${ROOT_DIR}" "${VERSION}" "${APP_NAME}" "${RTYPE}" "${BUILD_JOBS}" "${GUEST_ROOT}" "${BUNDLE_NAME}" "${FLATPAK_STATE_DIR}" <<'GUEST_SCRIPT'
set -euo pipefail

HOST_ROOT="$1"
VERSION="$2"
APP_NAME="$3"
RTYPE="$4"
BUILD_JOBS="$5"
GUEST_ROOT="$6"
BUNDLE_NAME="$7"
FLATPAK_STATE_DIR="$8"

if [[ "$(uname -m)" != "aarch64" ]]; then
    echo "ERROR: guest architecture must be aarch64, got $(uname -m)" >&2
    exit 1
fi

if command -v growpart >/dev/null 2>&1 && command -v resize2fs >/dev/null 2>&1; then
    sudo growpart /dev/vda 1 >/dev/null 2>&1 || true
    sudo resize2fs /dev/vda1 >/dev/null 2>&1 || true
fi

if ! command -v flatpak-builder >/dev/null 2>&1 || ! command -v flatpak >/dev/null 2>&1 || ! command -v ccache >/dev/null 2>&1; then
    echo "Installing Flatpak build tools inside the Lima VM..."
    sudo apt-get update
    sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y \
        flatpak flatpak-builder git git-lfs python3 python3-pip \
        ca-certificates curl rsync jq gettext desktop-file-utils \
        build-essential cmake ninja-build file unzip xz-utils bzip2 patch ccache
fi

for tool in flatpak flatpak-builder python3 git; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "ERROR: ${tool} is not installed inside the Lima VM" >&2
        exit 1
    }
done

sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

if ! flatpak list --runtime --columns=application,branch,arch | grep -q $'org.gnome.Platform\t50\taarch64' ||
   ! flatpak list --runtime --columns=application,branch,arch | grep -q $'org.gnome.Sdk\t50\taarch64' ||
   ! flatpak list --runtime --columns=application,branch,arch | grep -q $'org.freedesktop.Sdk.Extension.llvm21\t25.08\taarch64'; then
    echo "Installing Flatpak runtimes inside the Lima VM..."
    sudo flatpak install -y flathub \
        org.gnome.Platform//50 \
        org.gnome.Sdk//50 \
        org.freedesktop.Sdk.Extension.llvm21
fi

echo "Syncing source into guest disk..."
mkdir -p "$(dirname "${GUEST_ROOT}")"
rsync -a --delete \
    --exclude='.git/' \
    --exclude='.flatpak-builder/' \
    --exclude='build/' \
    --exclude='build-tests/' \
    --exclude='build_check_*/' \
    --exclude='out/' \
    --exclude='deps/DL_CACHE/' \
    --exclude='deps/build*/' \
    "${HOST_ROOT}/" "${GUEST_ROOT}/"

cd "${GUEST_ROOT}"

export FLATPAK_BUILDER_N_JOBS="${BUILD_JOBS}"
export CMAKE_BUILD_PARALLEL_LEVEL="${BUILD_JOBS}"

APP_ID="io.github.crealityofficial.CrealityPrint"
SOURCE_MANIFEST="flatpak/${APP_ID}.yml"
BUILD_ROOT="build/flatpak-arm"
FLATPAK_BUILD_DIR="${BUILD_ROOT}/builder"
FLATPAK_REPO_DIR="${BUILD_ROOT}/repo"
GENERATED_MANIFEST="flatpak/${APP_ID}.generated.yml"
BUNDLE_PATH="build/${BUNDLE_NAME}"

rm -rf "${FLATPAK_BUILD_DIR}"
mkdir -p "${BUILD_ROOT}" "${FLATPAK_REPO_DIR}" "${FLATPAK_STATE_DIR}" build
cp "${SOURCE_MANIFEST}" "${GENERATED_MANIFEST}"

python3 - "${GENERATED_MANIFEST}" "${VERSION}" "${RTYPE}" <<'PY'
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
version = sys.argv[2]
rtype = sys.argv[3]
text = path.read_text()
text = re.sub(r"-DCREALITYPRINT_VERSION=[^ \\\n]+", f"-DCREALITYPRINT_VERSION={version}", text)
text = re.sub(r"-DPROJECT_VERSION_EXTRA=[^ \\\n]+", f"-DPROJECT_VERSION_EXTRA={rtype}", text)
text = re.sub(r"\n\s*rm -r /run/build/creality_deps/external-packages\s*\n", "\n", text)
path.write_text(text)
PY

echo "Guest build environment:"
echo "  arch: $(uname -m)"
echo "  flatpak: $(flatpak --version)"
echo "  flatpak-builder: $(flatpak-builder --version)"
echo "  jobs: ${BUILD_JOBS}"
echo "  state: ${FLATPAK_STATE_DIR}"
echo

flatpak-builder \
    --force-clean \
    --ccache \
    --disable-updates \
    --state-dir="${FLATPAK_STATE_DIR}" \
    --arch=aarch64 \
    --install-deps-from=flathub \
    --repo="${FLATPAK_REPO_DIR}" \
    "${FLATPAK_BUILD_DIR}" \
    "${GENERATED_MANIFEST}"

rm -f "${BUNDLE_PATH}"
flatpak build-bundle \
    --arch=aarch64 \
    "${FLATPAK_REPO_DIR}" \
    "${BUNDLE_PATH}" \
    "${APP_ID}"

echo
echo "Flatpak bundle created:"
ls -lh "${BUNDLE_PATH}"
GUEST_SCRIPT

mkdir -p "${ROOT_DIR}/build"
"${LIMACTL}" copy "${VM_NAME}:${GUEST_ROOT}/build/${BUNDLE_NAME}" "${ROOT_DIR}/build/${BUNDLE_NAME}"

echo
echo "Build finished successfully."
echo "Host bundle: ${ROOT_DIR}/build/${BUNDLE_NAME}"
