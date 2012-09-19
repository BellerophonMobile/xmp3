#!/bin/bash
# build_android.sh - Compiles dependencies for Android
#
# You may need to change some of the variables in this script for your
# system/target, specifically, PLATFORM, ARCH, TOOLCHAIN_VERSION, and
# AUTOMAKE_DIR.

set -e

if [ -z "$ANDROID_NDK" ]
then
    echo "Please set the ANDROID_NDK environment variable."
    exit 1
fi

# Get the directory of where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Directory where the dependencies will be built
BUILD_DIR="$SCRIPT_DIR/android-build"

# Directory where the dependencies will be installed
PREFIX="$SCRIPT_DIR/android-prefix"

# Target platforms: 3, 4, 5, 8, 9, 14
PLATFORM=9

# Target architectures: arm, mips, x86
ARCH=arm

# Android NDK toolchain options
TOOLCHAIN=arm-linux-androideabi
# 4.6 is in NDK version 8b July 2012
TOOLCHAIN_VERSION=4.6

# Flags to pass to Make
MAKEFLAGS=""

## For armv7 platforms
#FLAGS="-march=armv7-a -mfloat-abi=softfp"
#export CFLAGS="$FLAGS"
#export CXXFLAGS="$FLAGS"
#export LDFLAGS="-Wl,--fix-cortex-a8"

##############################################
# Shouldn't need to change anything below here
##############################################

# Modify TOOLCHAIN_ARCH based on host system
if [ "$(uname)" = "Linux" ]
then
    TOOLCHAIN_ARCH=linux-x86
elif [ "$(uname)" = "Darwin" ]
then
    TOOLCHAIN_ARCH=darwin-x86
fi

TARGETS="util_linux expat openssl libev"

# util_linux - For libuuid
build_util_linux() {
    local base_ver=2.21
    local ver=2.21.2
    local dir="util-linux-$ver"
    local pkg="$dir.tar.xz"
    local url="http://www.kernel.org/pub/linux/utils/util-linux/v$base_ver/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    update_autotools config/
    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-libblkid --disable-libmount --disable-mount \
        --disable-losetup --disable-fsck --disable-partx --disable-uuidd \
        --disable-mountpoint --disable-fallocate --disable-unshare \
        --disable-agetty --disable-cramfs --disable-switch_root \
        --disable-pivot_root --disable-kill --disable-rename \
        --disable-schedutils --disable-wall --disable-write \
        --without-ncurses --disable-shared
    cd libuuid
    make $MAKEFLAGS
    make install
}

# expat - For XML parsing
build_expat() {
    local ver=2.1.0
    local dir="expat-$ver"
    local pkg="$dir.tar.gz"
    local url="http://downloads.sourceforge.net/project/expat/expat/$ver/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-shared
    make $MAKEFLAGS
    make install
}

# openssl - For TLS support
build_openssl() {
    local ver=1.0.1c
    local dir="openssl-$ver"
    local pkg="$dir.tar.gz"
    local url="http://www.openssl.org/source/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    #./Configure --prefix="$PREFIX" android no-idea no-cast no-seed no-md2 no-sha0 no-whrlpool no-whirlpool -DL_ENDIAN
    ./Configure --prefix="$PREFIX" android
    make MAKEDEPPROG="$CC" depend
    # Parallel makes are totally unreliable it seems
    make
    make install
}

# libev - Event loop
build_libev() {
    local ver=4.11
    local dir="libev-$ver"
    local pkg="$dir.tar.gz"
    local url="http://dist.schmorp.de/libev/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    update_autotools .
    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-shared --enable-static
    make $MAKEFLAGS CFLAGS="$CFLAGS -DEV_SELECT_USE_FD_SET"
    make install
}

# Simple function to download (if they haven't been already) and extract
# packages if they don't exist.
download_package() {
    if [ ! -f "$1" ]
    then
        curl -LO --disable-epsv "$2"
    fi
    tar -xf "$1"
}

# For packages with outdated autotools scripts which don't have Android support
update_autotools() {
    pushd "$BUILD_DIR"
    if [ ! -f "config.guess" ]
    then
        curl -L -o config.guess "http://git.savannah.gnu.org/cgit/automake.git/plain/lib/config.guess?h=maint"
    fi

    if [ ! -f "config.sub" ]
    then
        curl -L -o config.sub "http://git.savannah.gnu.org/cgit/automake.git/plain/lib/config.sub?h=maint"
    fi
    popd

    cp "$BUILD_DIR/config.guess" "$BUILD_DIR/config.sub" $1
}

export_environment() {
    PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
    LD_LIBRARY_PATH="$PREFIX/lib"
    LDFLAGS="-L$PREFIX/lib"
    CPPFLAGS="-I$PREFIX/include"

    if [ "$ARCH" = "arm" -o "$ARCH" = "armv7" ]
    then
        CFLAGS="-fpic -ffunction-sections -funwind-tables -fstack-protector -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__"
        # For thumb mode
        CFLAGS+=" -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64"
        if [ "$ARCH" = "armv7" ]
        then
            ARCH="arm"
            CFLAGS+=" -march=armv7-a -mfloat-abi=softfp -mfpu=vfp"
            CXXFLAGS+=" -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64"
            LDFLAGS+=" -Wl,--fix-cortex-a8"
        elif [ "$ARCH" = "arm" ]
        then
            CFLAGS+=" -march=armv5te -mtune=xscale -msoft-float"
            CXXFLAGS+=" -march=armv5te -mtune=xscale -msoft-float"
        fi
    elif [ "$ARCH" = "mips" ]
    then
        CFLAGS="-fpic -fno-strict-aliasing -finline-functions -ffunction-sections -funwind-tables -fmessage-length=0 -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers -O2 -fomit-frame-pointer -funswitch-loops -finline-limit=300"
    elif [ "$ARCH" = "x86" ]
    then
        CFLAGS="-ffunction-sections -funwind-tables -O2 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300"
    fi

    CXXFLAGS="$CFLAGS"

    local toolchain_path="$ANDROID_NDK/toolchains/$TOOLCHAIN-$TOOLCHAIN_VERSION/prebuilt/$TOOLCHAIN_ARCH/bin/"
    local sysroot="$ANDROID_NDK/platforms/android-$PLATFORM/arch-$ARCH/"

    export PATH="$toolchain_path:$PATH"

    export CC="$TOOLCHAIN-gcc --sysroot=$sysroot"
    export CXX="$TOOLCHAIN-g++ --sysroot=$sysroot"
    export LD="$CC"

    export PKG_CONFIG_PATH
    export LD_LIBRARY_PATH
    export LDFLAGS
    export CPPFLAGS
    export CFLAGS
    export CXXFLAGS
}

build_target() {
    export_environment

    local targets
    if [ "$#" -eq 0 ]
    then
        targets="$TARGETS"
    else
        targets="$@"
    fi

    mkdir -p "$BUILD_DIR"
    pushd "$BUILD_DIR"

    for t in $targets
    do
        (eval "build_$t")
    done
}

get_help() {
    cat << EOF
Usage: $0 [options] [targets...]

Options:
    -r     Android processor architecture to build for (Default: $ARCH)
               (arm, armv7, mips, x86)
    -p     Android NDK platform to build for (Default: $PLATFORM)
               (see $ANDROID_NDK/platforms)
    -t     Toolchain version (Default: $TOOLCHAIN_VERSION)
               (see $ANDROID_NDK/toolchains)
    -m     Quoted flags to pass to make
    -j     Shortcut to add "-j <n>" to MAKEFLAGS
    -h     This help output

Targets: $TARGETS
EOF
}

while getopts "r:p:t:m:j:h" flag
do
    case "$flag" in
        r)
            ARCH="$OPTARG"
            ;;
        p)
            PLATFORM="$OPTARG"
            ;;
        t)
            TOOLCHAIN_VERSION="$OPTARG"
            ;;
        m)
            MAKEFLAGS="$OPTARG"
            ;;
        j)
            MAKEFLAGS="$MAKEFLAGS -j $OPTARG"
            ;;
        h)
            get_help
            exit 0
            ;;
        *)
            get_help
            exit 1
            ;;
    esac
done

# Clear all option flags
shift $(( OPTIND - 1 ))

build_target $@
