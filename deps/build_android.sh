#!/bin/bash -e
# build_android.sh - Compiles dependencies for Android
#
# You may need to change some of the variables in this script for your
# system/target, specifically, PLATFORM, ARCH, TOOLCHAIN_VERSION, and
# AUTOMAKE_DIR.

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

# Where to find config.guess and config.sub (needed to fix packages with old
# automake files)
AUTOMAKE_DIR="/usr/share/automake-1.12"

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

TOOLCHAIN_PATH="$ANDROID_NDK/toolchains/$TOOLCHAIN-$TOOLCHAIN_VERSION/prebuilt/$TOOLCHAIN_ARCH/bin/"
export PATH="$TOOLCHAIN_PATH:$PATH"

SYSROOT="$ANDROID_NDK/platforms/android-$PLATFORM/arch-$ARCH/"

export CC="$TOOLCHAIN-gcc --sysroot=$SYSROOT"
export CXX="$TOOLCHAIN-g++ --sysroot=$SYSROOT"
#export LD="$TOOLCHAIN-ld --sysroot=$SYSROOT"
export LD="$CC"

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export LD_LIBRARY_PATH="$PREFIX/lib"
export LDFLAGS="-L$PREFIX/lib"
export CPPFLAGS="-I$PREFIX/include"

TARGETS="util_linux expat openssl"

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
    #autoreconf -fi
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
    cp "$AUTOMAKE_DIR/config.guess" "$AUTOMAKE_DIR/config.sub" $1
}

build_target() {
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
    -p     Android NDK platform to build for (Default: $PLATFORM)
               (see $ANDROID_NDK/platforms)
    -t     Toolchain version (Default: $TOOLCHAIN_VERSION)
               (see $ANDROID_NDK/toolchains)
    -a     Automake directory (used to update some build systems)
               (Default: $AUTOMAKE_DIR)
    -m     Quoted flags to pass to make
    -h     This help output

Targets: $TARGETS
EOF
}

while getopts "p:t:m:h" flag
do
    case "$flag" in
        p)
            PLATFORM="$OPTARG"
            ;;
        t)
            TOOLCHAIN_VERSION="$OPTARG"
            ;;
        m)
            MAKEFLAGS="$OPTARG"
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
