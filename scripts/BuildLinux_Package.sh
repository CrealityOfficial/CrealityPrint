#!/bin/bash

# scripts/BuildLinux_Package.sh -sir 6.0.0.100 CrealityPrint Alpha 0 0

echo xxx
export ROOT="$(pwd)" #$(dirname $(readlink -f ${0}))

DPS_PATH="${PWD}/deps/build/destdir"
echo "Using DPS_PATH=${DPS_PATH}"

set -e # exit on first error

function check_available_memory_and_disk() {
    FREE_MEM_GB=$(free -g -t | grep 'Mem' | rev | cut -d" " -f1 | rev)
    MIN_MEM_GB=10

    FREE_DISK_KB=$(df -k . | tail -1 | awk '{print $4}')
    MIN_DISK_KB=$((10 * 1024 * 1024))

    if [ ${FREE_MEM_GB} -le ${MIN_MEM_GB} ]; then
        echo -e "\nERROR: CrealityPrint Builder requires at least ${MIN_MEM_GB}G of 'available' mem (systen has only ${FREE_MEM_GB}G available)"
        echo && free -h && echo
        exit 2
    fi

    if [[ ${FREE_DISK_KB} -le ${MIN_DISK_KB} ]]; then
        echo -e "\nERROR: CrealityPrint Builder requires at least $(echo ${MIN_DISK_KB} |awk '{ printf "%.1fG\n", $1/1024/1024; }') (systen has only $(echo ${FREE_DISK_KB} | awk '{ printf "%.1fG\n", $1/1024/1024; }') disk free)"
        echo && df -h . && echo
        exit 1
    fi
}

function usage() {
    echo "Usage: ./BuildLinux.sh [-1][-b][-c][-d][-i][-r][-s][-u] [version] [app_name] [version_extra] [slicer_header] [rebuild_deps]"
    echo "   -1: limit builds to 1 core (where possible)"
    echo "   -b: build in debug mode"
    echo "   -c: force a clean build"
    echo "   -d: build deps (optional)"
    echo "   -h: this help output"
    echo "   -i: Generate appimage (optional)"
    echo "   -e: Generate deb package (optional)"
    echo "   -T <toolchain>: choose compiler (gcc|clang), default: clang"
    echo "   -r: skip ram and disk checks (low ram compiling)"
    echo "   -s: build orca-slicer (optional)"
    echo "   -u: update and build dependencies (optional and need sudo)"
    echo "   rebuild_deps: positional arg, 1/true/yes/rebuild to clean and build deps"
    echo "For a first use, you want to 'sudo ./BuildLinux.sh -u'"
    echo "   and then './BuildLinux.sh -dsi'"
}

unset name
TOOLCHAIN="clang"
while getopts ":1bcdghirseuT:" opt; do
  case ${opt} in
    1 )
        export CMAKE_BUILD_PARALLEL_LEVEL=1
        ;;
    b )
        BUILD_DEBUG="1"
        ;;
    c )
        CLEAN_BUILD=1
        ;;
    d )
        BUILD_DEPS="1"
        ;;
    h ) usage
        exit 0
        ;;
    i )
        BUILD_IMAGE="1"
        ;;
    e )
        BUILD_DEB="1"
        ;;
    T )
        case "${OPTARG}" in
            gcc|clang)
                TOOLCHAIN="${OPTARG}"
                ;;
            *)
                echo "Unsupported toolchain '${OPTARG}'. Use 'gcc' or 'clang'."; exit 1;;
        esac
        ;;
    r )
	    SKIP_RAM_CHECK="1"
	;;
    s )
        BUILD_ORCA="1"
        ;;
    u )
        UPDATE_LIB="1"
        ;;
  esac
done

if [ ${OPTIND} -eq 1 ]
then
    usage
    exit 0
fi

DISTRIBUTION=$(awk -F= '/^ID=/ {print $2}' /etc/os-release)
# treat ubuntu as debian
if [ "${DISTRIBUTION}" == "ubuntu" ]
then
    DISTRIBUTION="debian"
fi
if [ ! -f ./linux.d/${DISTRIBUTION} ]
then
    echo "Your distribution does not appear to be currently supported by these build scripts"
    exit 1
fi
source ./linux.d/${DISTRIBUTION}

echo "FOUND_GTK3=${FOUND_GTK3}"
if [[ -z "${FOUND_GTK3_DEV}" ]]
then
    echo "Error, you must install the dependencies before."
    echo "Use option -u with sudo"
    exit 1
fi

echo "Changing date in version..."
{
    # change date in version
    sed -i "s/+UNKNOWN/_$(date '+%F')/" version.inc
}
echo "done"

#
# Normalize positional arguments after options
shift $((OPTIND - 1))

VERSION_TAG_NAME=$1
APPNAME=$2
VERSION_EXTRA=$3
SLICER_HEADER=$4
REBUILD_DEPS=$5
if  [ -z "$SLICER_HEADER" ]; then
    export SLICER_HEADER=1
fi 
if [ -z "$VERSION_TAG_NAME" ]; then
    export VERSION_TAG_NAME="6.0.0"
fi

if [ -z "$APPNAME" ]; then
    export APPNAME="CrealityPrint"
fi

if [ -z "$VERSION_EXTRA" ]; then
    export APPNAME="Alpha"
fi

case "${REBUILD_DEPS}" in
    1|true|TRUE|yes|YES|y|Y|rebuild|REBUILD)
        BUILD_DEPS="1"
        CLEAN_BUILD=1
        ;;
esac
#

#
echo VERSION_TAG_NAME=$VERSION_TAG_NAME
echo APPNAME=$APPNAME
echo VERSION_EXTRA=$VERSION_EXTRA
echo SLICER_HEADER=$SLICER_HEADER
echo REBUILD_DEPS=${REBUILD_DEPS:-0}
#
#
#

# Select compiler according to -T (default clang)
if [[ "${TOOLCHAIN}" == "gcc" ]]; then
    C_COMPILER="gcc"; CXX_COMPILER="g++"
else
    C_COMPILER="clang"; CXX_COMPILER="clang++"
fi
echo "Using compiler: C=${C_COMPILER} CXX=${CXX_COMPILER}"

if ! [[ -n "${SKIP_RAM_CHECK}" ]]
then
    check_available_memory_and_disk
fi

if [[ -n "${BUILD_DEPS}" ]]
then
    echo "Configuring dependencies..."
    BUILD_ARGS="-DDEP_WX_GTK3=ON -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
    if [[ -n "${CLEAN_BUILD}" ]]
    then
        rm -fr deps/build
    fi
    if [ ! -d "deps/build" ]
    then
        mkdir deps/build
    fi
    if [[ -n "${BUILD_DEBUG}" ]]
    then
        # have to build deps with debug & release or the cmake won't find everything it needs
        mkdir deps/build/release
        cmake -S deps -B deps/build/release -G Ninja -DDESTDIR="${DPS_PATH}" ${BUILD_ARGS}
        cmake --build deps/build/release
        BUILD_ARGS="${BUILD_ARGS} -DCMAKE_BUILD_TYPE=Debug"
    fi

    echo "cmake -S deps -B deps/build -G Ninja ${BUILD_ARGS}"
    cmake -S deps -B deps/build -G Ninja -DDESTDIR="${DPS_PATH}" ${BUILD_ARGS}
    cmake --build deps/build
fi


if [[ -n "${BUILD_ORCA}" ]]
then
    echo "Configuring CrealityPrint..."
    if [[ -n "${CLEAN_BUILD}" ]]
    then
        rm -fr build
    fi
    BUILD_ARGS=""
    if [[ -n "${FOUND_GTK3_DEV}" ]]
    then
        BUILD_ARGS="-DSLIC3R_GTK=3 -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
    fi
    if [[ -n "${BUILD_DEBUG}" ]]
    then
        BUILD_ARGS="${BUILD_ARGS} -DCMAKE_BUILD_TYPE=Debug -DBBL_INTERNAL_TESTING=1"
    else
        BUILD_ARGS="${BUILD_ARGS} -DBBL_RELEASE_TO_PUBLIC=1 -DBBL_INTERNAL_TESTING=0 -DUPDATE_ONLINE_MACHINES=1"
    fi
    echo -e "cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="${DPS_PATH}/usr/local" -DSLIC3R_STATIC=1 ${BUILD_ARGS}"
    cmake -S . -B build -G Ninja \
        -DCMAKE_PREFIX_PATH="${DPS_PATH}/usr/local" \
        -DSLIC3R_STATIC=1 \
        -DORCA_TOOLS=ON \
        -DPROCESS_NAME=$APPNAME \
        -DGENERATE_ORCA_HEADER=$SLICER_HEADER \
        -DCREALITYPRINT_VERSION=$VERSION_TAG_NAME \
        -DPROJECT_VERSION_EXTRA=$VERSION_EXTRA  \
        ${BUILD_ARGS}
    echo "done"
    echo "Building CrealityPrint ..."
    cmake --build build --target CrealityPrint
    #echo "Building CrealityPrint_profile_validator .."
    #cmake --build build --target CrealityPrint_profile_validator
    ./run_gettext.sh
    echo "done"
fi

if [[ -e ${ROOT}/build/src/BuildLinuxImage.sh ]]; then
# Give proper permissions to script
chmod 755 ${ROOT}/build/src/BuildLinuxImage.sh

echo "[9/9] Generating Linux app..."
    pushd build
        if [[ -n "${BUILD_IMAGE}" ]]
        then
            ${ROOT}/build/src/BuildLinuxImage.sh -i
        else
            ${ROOT}/build/src/BuildLinuxImage.sh
        fi
        # Optionally build Debian package if requested and script exists
        if [[ -n "${BUILD_DEB}" ]] && [[ -e ${ROOT}/build/src/BuildLinuxDeb.sh ]]
        then
            chmod 755 ${ROOT}/build/src/BuildLinuxDeb.sh
            ${ROOT}/build/src/BuildLinuxDeb.sh
        fi
    popd
echo "done"
else 
    echo not find ${ROOT}/build/src/BuildLinuxImage.sh
fi

