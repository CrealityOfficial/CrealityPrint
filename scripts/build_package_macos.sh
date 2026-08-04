#!/bin/bash

# scripts/build_package_macos.sh 6.0.0.100 CrealityPrint Alpha 0 F002
#
# Usage: build_package_macos.sh [VERSION] [APPNAME] [VERSION_EXTRA] [SLICER_HEADER] [CUSTOM_TYPE] [--clean]
#
# Parameters:
#   VERSION        - Version tag (default: 6.0.0)
#   APPNAME        - Application name (default: CrealityPrint)
#   VERSION_EXTRA  - Extra version info
#   SLICER_HEADER  - Slicer header flag (default: 1)
#   CUSTOM_TYPE    - Custom build type
#   --clean        - Clean build directories before building
#
# Environment Variables:
#   CLEAN_ORIGINAL_SYMBOLS - Set to 'true' to remove original symbol files after compression
#   PROCESS_NAME          - Process name for DMG package (default: APPNAME)
#   CREALITYPRINT_VERSION - Version for DMG package (default: VERSION)
#   CMAKE_OSX_ARCHITECTURES - Architecture for DMG package (default: ARCH)
#   PROJECT_VERSION_EXTRA - Extra version for DMG package (default: Alpha)
#
# Examples:
#   ./build_package_macos.sh                           # Normal incremental build
#   ./build_package_macos.sh --clean                   # Clean build
#   CLEAN_ORIGINAL_SYMBOLS=true ./build_package_macos.sh --clean  # Clean build with symbol cleanup
#   ./build_package_macos.sh 6.1.0 MyApp Alpha 1 --clean  # Clean build with custom parameters

set -e
set -o pipefail

# Check for clean build flag
CLEAN_BUILD=false
for arg in "$@"; do
    if [ "$arg" = "--clean" ]; then
        CLEAN_BUILD=true
        break
    fi
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
  export BUILD_TARGET="slicer"
fi

if [ -z "$SLICER_CMAKE_GENERATOR" ]; then
  export SLICER_CMAKE_GENERATOR="Ninja"
fi

if [ -z "$SLICER_BUILD_TARGET" ]; then
  export SLICER_BUILD_TARGET="all"
fi

if [ -z "$DEPS_CMAKE_GENERATOR" ]; then
  export DEPS_CMAKE_GENERATOR="Ninja"
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Error: ninja is required for macOS builds."
    exit 1
fi
if ! command -v ccache >/dev/null 2>&1; then
    echo "Error: ccache is required for macOS builds."
    exit 1
fi
CCACHE_PROGRAM="$(command -v ccache)"
export CCACHE_BASEDIR="${CCACHE_BASEDIR:-$(pwd)}"
export CCACHE_NOHASHDIR="${CCACHE_NOHASHDIR:-true}"

if [ -z "$OSX_DEPLOYMENT_TARGET" ]; then
  export OSX_DEPLOYMENT_TARGET="11.3"
fi

VERSION_TAG_NAME=$1
APPNAME=$2
VERSION_EXTRA=$3
SLICER_HEADER=$4
CUSTOM_TYPE=$5
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
    export VERSION_EXTRA="Alpha"
fi
if [ -z "$CUSTOM_TYPE" ]; then
    export CUSTOM_TYPE=""
fi
echo "Build params:"
echo " - ARCH: $ARCH"
echo " - BUILD_CONFIG: $BUILD_CONFIG"
echo " - BUILD_TARGET: $BUILD_TARGET"
echo " - CMAKE_GENERATOR: $SLICER_CMAKE_GENERATOR for Slicer, $DEPS_CMAKE_GENERATOR for deps"
echo " - CCACHE: $CCACHE_PROGRAM"
echo " - CCACHE_DIR: ${CCACHE_DIR:-$HOME/.cache/ccache}"
echo " - OSX_DEPLOYMENT_TARGET: $OSX_DEPLOYMENT_TARGET"
echo " - SLICER_BUILD_TARGET=$SLICER_BUILD_TARGET"
echo " - BUILD_TARGET=$BUILD_TARGET"

REBUILD_DEPS_REQUESTED=0
case "${REBUILD_DEPS:-0}" in
    1|true|TRUE|yes|YES|on|ON)
        REBUILD_DEPS_REQUESTED=1
        ;;
esac
echo " - REBUILD_DEPS=$REBUILD_DEPS_REQUESTED"


PROJECT_DIR="$(pwd)" #"$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_BUILD_DIR="$PROJECT_DIR/build_$ARCH"
DEPS_DIR="$PROJECT_DIR/deps"
DEPS_BUILD_DIR="$DEPS_DIR/build_$ARCH"
DEPS="$DEPS_BUILD_DIR/dep_$ARCH"

BUILD_CONFIG="Release"
echo " - PROJECT_BUILD_DIR=$PROJECT_BUILD_DIR"

# Fix for Multi-config generators
if [ "$SLICER_CMAKE_GENERATOR" == "Xcode" ]; then
    export BUILD_DIR_CONFIG_SUBDIR="/$BUILD_CONFIG"
else
    export BUILD_DIR_CONFIG_SUBDIR=""
fi

# 读取环境变量 MY_DIR
DEPS_PATH=$DEPS_ENV_DIR
if [ -z "${DEPS_PATH}" ]; then
    echo "DEPS_ENV_DIR is empty, use workspace deps."
else
    DEPS=$DEPS_PATH
fi
if [ "$REBUILD_DEPS_REQUESTED" = "1" ]; then
    export BUILD_TARGET="all"
elif [ ! -d "$DEPS/usr/local" ]; then
    echo "Deps directory is missing, build deps: $DEPS"
    export BUILD_TARGET="all"
else
    export BUILD_TARGET="slicer"
fi
echo "=====DEPS=====: $DEPS"

function reset_build_dir_on_generator_change() {
    local build_dir="$1"
    local expected_generator="$2"
    local cache_file="$build_dir/CMakeCache.txt"

    if [ ! -f "$cache_file" ]; then
        return
    fi

    local cached_generator
    cached_generator="$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$cache_file" | head -n 1)"
    if [ -n "$cached_generator" ] && [ "$cached_generator" != "$expected_generator" ]; then
        echo "CMake generator changed from $cached_generator to $expected_generator; cleaning $build_dir"
        rm -rf "$build_dir"
    fi
}

reset_build_dir_on_generator_change "$PROJECT_BUILD_DIR" "$SLICER_CMAKE_GENERATOR"
reset_build_dir_on_generator_change "$DEPS_BUILD_DIR" "$DEPS_CMAKE_GENERATOR"

#DEPS_PATH="/Users/creality/Orca_work/c3d_6.0/C3DSlicer/deps/build_x86_64/dep_x86_64"


function build_deps() {
    echo "Building deps..."
    (
        set -x
        
        # Clean deps build directory if requested
        if [ "$CLEAN_BUILD" = true ] || [ "$REBUILD_DEPS_REQUESTED" = "1" ]; then
            echo "🧹 Cleaning deps build directory: $DEPS_BUILD_DIR"
            rm -rf "$DEPS_BUILD_DIR"
            rm -rf "$DEPS"
            echo "✓ Deps build directory cleaned"
        fi
        
        mkdir -p "$DEPS_BUILD_DIR"
        cd "$DEPS_BUILD_DIR"
        if [ "1." != "$BUILD_ONLY". ]; then
            cmake .. \
                -G "${DEPS_CMAKE_GENERATOR}" \
                -DDESTDIR="$DEPS" \
                -DOPENSSL_ARCH="darwin64-${ARCH}-cc" \
                -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
                -DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_PROGRAM" \
                -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_PROGRAM" \
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
# 生成和处理 dSYM 调试符号
function process_debug_symbols() {
    echo "Processing debug symbols..."
    
    local app_path="$PROJECT_BUILD_DIR/src$BUILD_DIR_CONFIG_SUBDIR/$APPNAME.app"
    local binary_path="$app_path/Contents/MacOS/$APPNAME"
    local symbols_dir="$PROJECT_BUILD_DIR/symbols"
    
    # 检查二进制文件是否存在
    if [ ! -f "$binary_path" ]; then
        echo "Error: Binary file not found at $binary_path"
        return 1
    fi
    
    echo "Found binary: $binary_path"
    
    # 创建临时工作目录（在 .app 包外部）
    local temp_work_dir="$PROJECT_BUILD_DIR/temp_symbol_processing"
    mkdir -p "$temp_work_dir"
    cd "$temp_work_dir"
    
    # 复制二进制文件到临时目录进行处理
    cp "$binary_path" "./$APPNAME"
    echo "✓ Binary copied to temporary directory for symbol processing"
    
    # 步骤1: 生成 dSYM 文件
    echo "Step 1: Generating dSYM file..."
    if dsymutil "$APPNAME" -o "$APPNAME.dSYM"; then
        echo "✓ dSYM file generated successfully: $APPNAME.dSYM"
    else
        echo "✗ Failed to generate dSYM file"
        return 1
    fi
    
    # 步骤2: 生成 Breakpad 符号文件
    echo "Step 2: Generating Breakpad symbol file..."
    
    # 查找 dump_syms 工具的函数
    find_dump_syms_for_symbols() {
        # 首先检查已知的硬编码路径
        local hardcoded_paths=(
            "/Users/qprj/breakpad/breakpad/src/tools/mac/dump_syms/build/Release/dump_syms"
            "/Users/creality/breakpad/breakpad/src/tools/mac/dump_syms/build/Release/dump_syms"
        )
        
        for path in "${hardcoded_paths[@]}"; do
            if [ -f "$path" ] && [ -x "$path" ]; then
                echo "$path"
                return 0
            fi
        done
        
        # 然后检查是否已在 PATH 中
        if command -v dump_syms &> /dev/null; then
            echo "$(which dump_syms)"
            return 0
        fi
        
        # 尝试加载 .bashrc 配置文件
        echo "dump_syms not found in PATH, trying to load .bashrc..." >&2
        if [ -f "$HOME/.bashrc" ]; then
            echo "  Sourcing: $HOME/.bashrc" >&2
            source "$HOME/.bashrc" 2>/dev/null || true
            
            # 检查是否现在可以找到 dump_syms
            if command -v dump_syms &> /dev/null; then
                echo "$(which dump_syms)"
                return 0
            fi
        fi
        
        # 最后尝试项目本地路径
        local local_path="$PROJECT_DIR/tools/breakpad/dump_syms"
        if [ -f "$local_path" ] && [ -x "$local_path" ]; then
            echo "$local_path"
            return 0
        fi
        
        return 1
    }
    
    # 查找 dump_syms 工具
    local dump_syms_tool=""
    if dump_syms_tool=$(find_dump_syms_for_symbols); then
        echo "✓ Found dump_syms at: $dump_syms_tool"
        
        if "$dump_syms_tool" "$APPNAME.dSYM" > "$APPNAME.sym"; then
            echo "✓ Breakpad symbol file generated: $APPNAME.sym"
        else
            echo "✗ Failed to generate Breakpad symbol file"
            return 1
        fi
    else
        echo "✗ dump_syms tool not found"
        echo "  Please ensure dump_syms is installed and available in your PATH"
        echo "  You may need to add it to your shell configuration file"
        echo "Skipping Breakpad symbol generation"
        return 1
    fi
    
    # 步骤3: 创建符号目录结构
    echo "Step 3: Creating symbol directory structure..."
    local symbol_id=$(head -n 1 "$APPNAME.sym" | awk '{print $4}')
    if [ -z "$symbol_id" ]; then
        echo "✗ Failed to extract symbol ID from .sym file"
        return 1
    fi
    
    echo "Symbol ID: $symbol_id"
    
    # 创建符号目录并复制文件
    local target_dir="symbols/$APPNAME/$symbol_id"
    mkdir -p "$target_dir"
    cp "$APPNAME.sym" "$target_dir/$APPNAME.sym"
    echo "✓ Symbol file copied to: $target_dir/$APPNAME.sym"
    
    # 步骤4: 剥离二进制文件中的调试信息
    echo "Step 4: Stripping debug symbols from binary..."
    local original_size=$(stat -f%z "$APPNAME" 2>/dev/null || echo "0")
    
    if strip -S "$APPNAME"; then
        local stripped_size=$(stat -f%z "$APPNAME" 2>/dev/null || echo "0")
        echo "✓ Debug symbols stripped from binary"
        echo "  Original size: $(numfmt --to=iec $original_size 2>/dev/null || echo "${original_size} bytes")"
        echo "  Stripped size: $(numfmt --to=iec $stripped_size 2>/dev/null || echo "${stripped_size} bytes")"
        
        if [ "$original_size" -gt "$stripped_size" ]; then
            local saved_bytes=$((original_size - stripped_size))
            echo "  Space saved: $(numfmt --to=iec $saved_bytes 2>/dev/null || echo "${saved_bytes} bytes")"
        fi
    else
        echo "✗ Failed to strip debug symbols"
        return 1
    fi
    
    # 移动符号文件到项目构建目录
    if [ -d "symbols" ]; then
        # 先删除目标位置的旧符号文件
        if [ -d "$symbols_dir" ]; then
            echo "  Removing old symbols directory: $symbols_dir"
            rm -rf "$symbols_dir"
        fi
        mv "symbols" "$symbols_dir"
        echo "✓ Symbols moved to: $symbols_dir"
    fi
    
    # 移动 dSYM 文件到项目构建目录
    if [ -d "$APPNAME.dSYM" ]; then
        # 先删除目标位置的旧 dSYM 文件
        local target_dsym="$PROJECT_BUILD_DIR/$APPNAME.dSYM"
        if [ -d "$target_dsym" ]; then
            echo "  Removing old dSYM file: $target_dsym"
            rm -rf "$target_dsym"
        fi
        mv "$APPNAME.dSYM" "$target_dsym"
        echo "✓ dSYM moved to: $target_dsym"
    fi
    
    # 可选：将剥离后的二进制文件复制回 .app 包（减小应用大小）
    echo "Step 5: Optionally copying stripped binary back to .app bundle..."
    if cp "$APPNAME" "$binary_path"; then
        echo "✓ Stripped binary copied back to: $binary_path"
        echo "  This reduces the final application size"
    else
        echo "⚠ Warning: Failed to copy stripped binary back"
        echo "  Original binary with debug symbols remains in .app bundle"
    fi
    
    # 清理临时工作目录
    cd "$PROJECT_BUILD_DIR"
    rm -rf "$temp_work_dir"
    echo "✓ Temporary working directory cleaned up"
    
    echo "✓ Debug symbol processing completed successfully!"
    echo ""
    echo "Generated files (outside .app bundle):"
    echo "  - dSYM: $PROJECT_BUILD_DIR/$APPNAME.dSYM"
    echo "  - Symbols: $symbols_dir"
    echo "  - App binary: $binary_path (stripped)"
    
    return 0
}

# 新增函数：压缩和重命名符号文件
function compress_and_rename_symbols() {
    echo "🗜️ Compressing and renaming symbol files..."
    
    local symbols_dir="$PROJECT_BUILD_DIR/symbols"
    local dsym_path="$PROJECT_BUILD_DIR/$APPNAME.dSYM"
    
    # 获取DMG包的基础名称（不含扩展名）
    # 使用与macx.cmake中相同的命名规则
    local process_name="${PROCESS_NAME:-$APPNAME}"
    local version="${CREALITYPRINT_VERSION:-$VERSION_TAG_NAME}"
    local arch="${CMAKE_OSX_ARCHITECTURES:-$ARCH}"
    local extra="${VERSION_EXTRA:-Alpha}"
    
    local dmg_base_name="${process_name}-${version}-macx-${arch}-${extra}"
    
    echo "📦 DMG base name: $dmg_base_name"
    echo "   Process name: $process_name"
    echo "   Version: $version"
    echo "   Architecture: $arch"
    echo "   Extra: $extra"
    
    # 压缩symbols目录
    if [ -d "$symbols_dir" ]; then
        echo "Step 1: Compressing symbols directory..."
        cd "$PROJECT_BUILD_DIR"
        
        # 创建symbols压缩包
        local symbols_archive="${dmg_base_name}.sym.tar.gz"
        if tar -czf "$symbols_archive" -C . symbols; then
            echo "✓ Symbols compressed: $symbols_archive"
            
            # 计算压缩前后大小
            local original_size=$(du -sh symbols | cut -f1)
            local compressed_size=$(du -sh "$symbols_archive" | cut -f1)
            echo "  Original size: $original_size"
            echo "  Compressed size: $compressed_size"
        else
            echo "✗ Failed to compress symbols directory"
            return 1
        fi
    else
        echo "⚠ Warning: Symbols directory not found: $symbols_dir"
    fi
    
    # 压缩dSYM文件
    if [ -d "$dsym_path" ]; then
        echo "Step 2: Compressing dSYM file..."
        cd "$PROJECT_BUILD_DIR"
        
        # 创建dSYM压缩包
        local dsym_archive="${dmg_base_name}.dSYM.tar.gz"
        if tar -czf "$dsym_archive" "$APPNAME.dSYM"; then
            echo "✓ dSYM compressed: $dsym_archive"
            
            # 计算压缩前后大小
            local original_dsym_size=$(du -sh "$APPNAME.dSYM" | cut -f1)
            local compressed_dsym_size=$(du -sh "$dsym_archive" | cut -f1)
            echo "  Original dSYM size: $original_dsym_size"
            echo "  Compressed dSYM size: $compressed_dsym_size"
        else
            echo "✗ Failed to compress dSYM file"
            return 1
        fi
    else
        echo "⚠ Warning: dSYM file not found: $dsym_path"
    fi
    
    # 自动清理原始文件以节省空间
    echo "Step 3: Cleaning original symbol files..."
    if [ -d "$symbols_dir" ]; then
        rm -rf "$symbols_dir"
        echo "✓ Original symbols directory removed"
    fi
    if [ -d "$dsym_path" ]; then
        rm -rf "$dsym_path"
        echo "✓ Original dSYM file removed"
    fi
    echo "💾 Disk space saved by removing original files"
    
    echo "✅ Symbol file compression completed"
    echo "📁 Compressed files location: $PROJECT_BUILD_DIR"
    echo "   - Symbols: ${dmg_base_name}.sym.tar.gz"
    echo "   - dSYM: ${dmg_base_name}.dSYM.tar.gz"
}

function pack_slicer() {
    echo "Packing slicer"
    (
        cd "$PROJECT_BUILD_DIR"
        cmake --build . --target package --config $BUILD_CONFIG
    )
}
function build_slicer() {
    echo "Verify localization with gettext..."
    (
        cd "$PROJECT_DIR"
        # ./run_gettext.sh
        if [ -z "$5" ]; then
            bash run_gettext.sh || exit 1
        else
            echo "customum gettext"
            # bash ./customized/$CUSTOM_TYPE/copy_resources.sh || exit 1
            bash ./customized/$CUSTOM_TYPE/run_gettext.sh || exit 1
        fi

    )

    echo "Building slicer..."
    (
        # echo " - SLICER_HEADER=$SLICER_HEADER"
        set -x
        
        # Clean build directory if requested
        if [ "$CLEAN_BUILD" = true ]; then
            echo "🧹 Cleaning build directory: $PROJECT_BUILD_DIR"
            rm -rf "$PROJECT_BUILD_DIR"
            echo "✓ Build directory cleaned"
        fi
        
        mkdir -p "$PROJECT_BUILD_DIR"
        cd "$PROJECT_BUILD_DIR"
        if [ "1." != "$BUILD_ONLY". ]; then
            cmake .. \
                -G "${SLICER_CMAKE_GENERATOR}" \
                -DBBL_RELEASE_TO_PUBLIC=1 \
                -DUPDATE_ONLINE_MACHINES=1 \
                -DGENERATE_ORCA_HEADER=$SLICER_HEADER \
                -DCMAKE_PREFIX_PATH="$DEPS/usr/local" \
                -DCMAKE_INSTALL_PREFIX="$PWD/CrealityPrint" \
                -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
                -DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_PROGRAM" \
                -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_PROGRAM" \
                -DCMAKE_MACOSX_RPATH=ON \
                -DCMAKE_INSTALL_RPATH="${DEPS}/usr/local" \
                -DCMAKE_MACOSX_BUNDLE=ON \
                -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
                -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET}" \
                -DPROCESS_NAME=$APPNAME \
                -DCREALITYPRINT_VERSION=$VERSION_TAG_NAME \
                -DPROJECT_VERSION_EXTRA=$VERSION_EXTRA    \
                -DCUSTOM_TYPE=$CUSTOM_TYPE
        fi
        cmake --build . --config "$BUILD_CONFIG" --target "$SLICER_BUILD_TARGET" || exit -2
    )


    # echo "Fix macOS app package..."
    # (
    #     cd "$PROJECT_BUILD_DIR"
    #     mkdir -p CrealityPrint
    #     cd CrealityPrint
    #     # remove previously built app
    #     # rm -rf ./CrealityPrint.app
    #     cp -pR  "../src$BUILD_DIR_CONFIG_SUBDIR/CrealityPrint.app" ./
    # )
}

case "${BUILD_TARGET}" in
    all)
        build_deps
        build_slicer
        process_debug_symbols
        compress_and_rename_symbols
        pack_slicer
        ;;
    deps)
        build_deps
        ;;
    slicer)
        build_slicer
        process_debug_symbols
        compress_and_rename_symbols
        pack_slicer
        ;;
    *)
        echo "Unknown target: $BUILD_TARGET. Available targets: deps, slicer, all."
        exit 1
        ;;
esac

"$CCACHE_PROGRAM" --show-stats || true
