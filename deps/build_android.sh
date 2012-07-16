#!/bin/sh

# Get the directory of where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Output prefix
if [ -z "$PREFIX" ]
then
    export PREFIX="$SCRIPT_DIR/android-prefix"
fi

# Target platforms: 3, 4, 5, 8, 9, 14
PLATFORM=9

# Target architectures: arm, mips, x86
ARCH=arm

TOOLCHAIN=arm-linux-androideabi
TOOLCHAIN_VERSION=4.4.3
TOOLCHAIN_ARCH=linux-x86

SYSROOT="$ANDROID_NDK/platforms/android-$PLATFORM/arch-$ARCH/"

## For armv7 platforms
#FLAGS="-march=armv7-a -mfloat-abi=softfp"
#export CFLAGS="$FLAGS"
#export CXXFLAGS="$FLAGS"
#export LDFLAGS="-Wl,--fix-cortex-a8"

# Shouldn't need to change anything below here

TOOLCHAIN_PATH="$ANDROID_NDK/toolchains/$TOOLCHAIN-$TOOLCHAIN_VERSION/prebuilt/$TOOLCHAIN_ARCH/bin/"
export PATH="$TOOLCHAIN_PATH:$PATH"

export CC="$TOOLCHAIN-gcc --sysroot=$SYSROOT"
export CXX="$TOOLCHAIN-g++ --sysroot=$SYSROOT"
export LD="$TOOLCHAIN-ld --sysroot=$SYSROOT"

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export LD_LIBRARY_PATH="$PREFIX/lib"
export LDFLAGS="-L$PREFIX/lib"
export CPPFLAGS="-I$PREFIX/include"

BUILD_DIR="$SCRIPT_DIR/android-build"

AUTOMAKE_DIR=/usr/share/automake-1.12

UTIL_LINUX_BASE_VER=2.21
UTIL_LINUX_VER=2.21.2

EXPAT_VER=2.1.0

OPENSSL_VER=1.0.1c

if [ ! -d "$BUILD_DIR" ]
then
    mkdir -p "$BUILD_DIR"
    pushd "$BUILD_DIR"

    # util-linux, for libuuid
    if [ ! -d "util-linux-$UTIL_LINUX_VER" ]
    then
        wget -c "http://www.kernel.org/pub/linux/utils/util-linux/v$UTIL_LINUX_BASE_VER/util-linux-$UTIL_LINUX_VER.tar.xz"
        tar xf "util-linux-$UTIL_LINUX_VER.tar.xz"
        pushd "util-linux-$UTIL_LINUX_VER"

        # Need to update Autoconf's OS detection support for Android
        cp "$AUTOMAKE_DIR/config.guess" "$AUTOMAKE_DIR/config.sub" config/
        autoreconf -fi
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
    fi

    # expat for XML parsing
    if [ ! -d "expat-$EXPAT_VER" ]
    then
        wget -c "http://downloads.sourceforge.net/project/expat/expat/$EXPAT_VER/expat-$EXPAT_VER.tar.gz"
        tar xf "expat-$EXPAT_VER.tar.gz"
        pushd "expat-$EXPAT_VER"
        ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
            --disable-shared
        make $@
        make install
        popd
    fi

    # openssl for SSL/TLS support
    if [ ! -d "openssl-$OPENSSL_VER" ]
    then
        wget -c "http://www.openssl.org/source/openssl-$OPENSSL_VER.tar.gz"
        tar xf "openssl-$OPENSSL_VER.tar.gz"
        pushd "openssl-$OPENSSL_VER"
        ANDROID_DEV="$SYSROOT/usr" ./Configure --prefix="$PREFIX" android -Wa,--noexecstack -DOPENSSL_NO_TLS1_2_CLIENT
        # The dependencies seem screwed up or something, so make fails the
        # first time...
        make deps
        make -k $@
        make $@
        make install
        popd
    fi

    popd
fi
