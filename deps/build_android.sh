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

#export CC="$TOOLCHAIN-gcc --sysroot=$SYSROOT -Wl,-rpath-link=$PREFIX/lib"
export CC="$TOOLCHAIN-gcc --sysroot=$SYSROOT"
export CXX="$TOOLCHAIN-g++ --sysroot=$SYSROOT"
export LD="$TOOLCHAIN-ld --sysroot=$SYSROOT"

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export LD_LIBRARY_PATH="$PREFIX/lib"
export LDFLAGS="-L$PREFIX/lib"
export CPPFLAGS="-I$PREFIX/include"

# Versions of dependent libraries
UTIL_LINUX_BASE_VER=2.21
UTIL_LINUX=util-linux-2.21.2
EXPAT_VER=2.1.0
EXPAT="expat-$EXPAT_VER"
OPENSSL=openssl-1.0.1c

# Simple function to download (if they haven't been already) and extract
# packages if they don't exist.
download_package() {
    if [ ! -f "$1" ]
    then
        curl -LO --disable-epsv "$2"
    fi
    tar -xf "$1"
}

mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR"

# util_linux - For libuuid
download_package "$UTIL_LINUX.tar.xz" "http://www.kernel.org/pub/linux/utils/util-linux/v$UTIL_LINUX_BASE_VER/$UTIL_LINUX.tar.xz"
pushd "$UTIL_LINUX"
## Need to update Autoconf's OS detection support for Android
cp "$AUTOMAKE_DIR/config.guess" "$AUTOMAKE_DIR/config.sub" config/
#autoreconf -fi
./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
    --disable-libblkid --disable-libmount --disable-mount \
    --disable-losetup --disable-fsck --disable-partx --disable-uuidd \
    --disable-mountpoint --disable-fallocate --disable-unshare \
    --disable-agetty --disable-cramfs --disable-switch_root \
    --disable-pivot_root --disable-kill --disable-rename \
    --disable-schedutils --disable-wall --disable-write \
    --without-ncurses --disable-shared
pushd libuuid
make $@
make install
popd
popd

# Expat - For XML parsing
download_package "$EXPAT.tar.gz" "http://downloads.sourceforge.net/project/expat/expat/$EXPAT_VER/$EXPAT.tar.gz"
pushd "$EXPAT"
./configure --host=arm-linux-androideabi --prefix="$PREFIX" --disable-shared
make $@
make install
popd

# openssl for SSL/TLS support
download_package "$OPENSSL.tar.gz" "http://www.openssl.org/source/$OPENSSL.tar.gz"
pushd "$OPENSSL"
./Configure --prefix="$PREFIX" android no-idea no-cast no-seed no-md2 no-sha0 no-whrlpool no-whirlpool -DL_ENDIAN
# The dependencies seem screwed up or something, so make fails the
# first time...
make MAKEDEPPROG="$CC" depend
# Can't do parallel make for some reason.  It seems unreliable
make
make install
popd

popd
