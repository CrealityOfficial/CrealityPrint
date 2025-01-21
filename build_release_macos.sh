#!/bin/bash

set -e
set -o pipefail

while getopts ":dpa:snt:xbc:h" opt; do
  case "${opt}" in
    d )
        export BUILD_TARGET="deps"
        ;;
    p )
        export PACK_DEPS="1"
        ;;
    a )
        export ARCH="$OPTARG"
        ;;
    s )
        export BUILD_TARGET="slicer"
        ;;
    n )
        export NIGHTLY_BUILD="1"
        ;;
    t )
        export OSX_DEPLOYMENT_TARGET="$OPTARG"
        ;;
    x )
        export SLICER_CMAKE_GENERATOR="Ninja"
        export SLICER_BUILD_TARGET="all"
        export DEPS_CMAKE_GENERATOR="Ninja"
        ;;
    b )
        export BUILD_ONLY="1"
        ;;
    c )
        export BUILD_CONFIG="$OPTARG"
        ;;
    h ) echo "Usage: ./build_release_macos.sh [-d]"
        echo "   -d: Build deps only"
        echo "   -a: Set ARCHITECTURE (arm64 or x86_64)"
        echo "   -s: Build slicer only"
        echo "   -n: Nightly build"
        echo "   -t: Specify minimum version of the target platform, default is 11.3"
        echo "   -x: Use Ninja CMake generator, default is Xcode"
        echo "   -b: Build without reconfiguring CMake"
        echo "   -c: Set CMake build configuration, default is Release"
        exit 0
        ;;
    * )
        ;;
  esac
done

# Set defaults

if [ -z "$ARCH" ]; then
  ARCH="$(uname -m)"
  export ARCH
fi

if [ -z "$BUILD_CONFIG" ]; then
  export BUILD_CONFIG="Release"
fi

if [ -z "$BUILD_TARGET" ]; then
  export BUILD_TARGET="all"
fi

if [ -z "$SLICER_CMAKE_GENERATOR" ]; then
  export SLICER_CMAKE_GENERATOR="Xcode"
fi

if [ -z "$SLICER_BUILD_TARGET" ]; then
  export SLICER_BUILD_TARGET="ALL_BUILD"
fi

if [ -z "$DEPS_CMAKE_GENERATOR" ]; then
  export DEPS_CMAKE_GENERATOR="Unix Makefiles"
fi

if [ -z "$OSX_DEPLOYMENT_TARGET" ]; then
  export OSX_DEPLOYMENT_TARGET="11.3"
fi

echo "Build params:"
echo " - ARCH: $ARCH"
echo " - BUILD_CONFIG: $BUILD_CONFIG"
echo " - BUILD_TARGET: $BUILD_TARGET"
echo " - CMAKE_GENERATOR: $SLICER_CMAKE_GENERATOR for Slicer, $DEPS_CMAKE_GENERATOR for deps"
echo " - OSX_DEPLOYMENT_TARGET: $OSX_DEPLOYMENT_TARGET"
echo

# if which -s brew; then
# 	brew --prefix libiconv
# 	brew --prefix zstd
# 	export LIBRARY_PATH=$LIBRARY_PATH:$(brew --prefix zstd)/lib/
# elif which -s port; then
# 	port install libiconv
# 	port install zstd
# 	export LIBRARY_PATH=$LIBRARY_PATH:/opt/local/lib
# else
# 	echo "Need either brew or macports to successfully build deps"
# 	exit 1
# fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_BUILD_DIR="$PROJECT_DIR/build_check_$ARCH"
DEPS_DIR="$PROJECT_DIR/deps"
DEPS_BUILD_DIR="$DEPS_DIR/build_$ARCH"
DEPS="$DEPS_BUILD_DIR/dep_$ARCH"
#"/Users/creality/Orca_work/orca_2.0/OrcaSlicer/deps/build_x86_64/OrcaSlicer_dep_x86_64"
#"$DEPS_BUILD_DIR/dep_$ARCH"
DEPS_PATH="/Users/creality/Orca_work/c3d_6.0/C3DSlicer/deps/build_x86_64/dep_x86_64"
if [ -d "$DEPS_PATH" ]; then 
    DEPS=$DEPS_PATH
    export BUILD_TARGET="slicer"
else 
    echo "deps file is not exist" 
    export BUILD_TARGET="all"
fi

# Fix for Multi-config generators
if [ "$SLICER_CMAKE_GENERATOR" == "Xcode" ]; then
    export BUILD_DIR_CONFIG_SUBDIR="/$BUILD_CONFIG"
else
    export BUILD_DIR_CONFIG_SUBDIR=""
fi

function build_deps() {
    echo "Building deps..."
    (
        set -x
        mkdir -p "$DEPS"
        cd "$DEPS_BUILD_DIR"
        if [ "1." != "$BUILD_ONLY". ]; then
            cmake .. \
                -G "${DEPS_CMAKE_GENERATOR}" \
                -DDESTDIR="$DEPS" \
                -DOPENSSL_ARCH="darwin64-${ARCH}-cc" \
                -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
                -DCMAKE_OSX_ARCHITECTURES:STRING="${ARCH}" \
                -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET}"
        fi
        cmake --build . --config "$BUILD_CONFIG" --target deps
    )
}

function pack_deps() {
    echo "Packing deps..."
    (
        set -x
        mkdir -p "$DEPS"
        cd "$DEPS_BUILD_DIR"
        tar -zcvf "dep_mac_${ARCH}_$(date +"%Y%m%d").tar.gz" "dep_$ARCH"
    )
}

function build_slicer() {
    echo "Building slicer..."
    (
        set -x
        mkdir -p "$PROJECT_BUILD_DIR"
        cd "$PROJECT_BUILD_DIR"
        if [ "1." != "$BUILD_ONLY". ]; then
            cmake .. \
                -G "${SLICER_CMAKE_GENERATOR}" \
                -DBBL_RELEASE_TO_PUBLIC=1 \
                -DGENERATE_ORCA_HEADER=0 \
                -DCMAKE_PREFIX_PATH="$DEPS/usr/local" \
                -DCMAKE_INSTALL_PREFIX="$PWD/CrealityPrint" \
                -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
                -DCMAKE_MACOSX_RPATH=ON \
                -DCMAKE_INSTALL_RPATH="${DEPS}/usr/local" \
                -DCMAKE_MACOSX_BUNDLE=ON \
                -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
                -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET}"
        fi
        cmake --build . --config "$BUILD_CONFIG" --target "$SLICER_BUILD_TARGET"
    )

    echo "Verify localization with gettext..."
    (
        cd "$PROJECT_DIR"
        ./run_gettext.sh
    )

    # echo "Fix macOS app package..."
    # (
    #     cd "$PROJECT_BUILD_DIR"
    #     mkdir -p CrealityPrint
    #     cd CrealityPrint
    #     # remove previously built app
    #     rm -rf ./CrealityPrint.app
    #     # fully copy newly built app
    #     cp -pR "../src$BUILD_DIR_CONFIG_SUBDIR/CrealityPrint.app" ./CrealityPrint.app
    #     # fix resources
    #     resources_path=$(readlink ./CrealityPrint.app/Contents/Resources)
    #     rm ./CrealityPrint.app/Contents/Resources
    #     cp -R "$resources_path" ./CrealityPrint.app/Contents/Resources
    #     # delete .DS_Store file
    #     find ./CrealityPrint.app/ -name '.DS_Store' -delete
    # )

    # extract version
    # export ver=$(grep '^#define CREALITYPRINT_VERSION' ../src/libslic3r/libslic3r_version.h | cut -d ' ' -f3)
    # ver="_V${ver//\"}"
    # echo $PWD
    # if [ "1." != "$NIGHTLY_BUILD". ];
    # then
    #     ver=${ver}_dev
    # fi

    # zip -FSr CrealityPrint${ver}_Mac_${ARCH}.zip CrealityPrint.app
}

case "${BUILD_TARGET}" in
    all)
        build_deps
        build_slicer
        ;;
    deps)
        build_deps
        ;;
    slicer)
        build_slicer
        ;;
    *)
        echo "Unknown target: $BUILD_TARGET. Available targets: deps, slicer, all."
        exit 1
        ;;
esac

if [ "1." == "$PACK_DEPS". ]; then
    pack_deps
fi
