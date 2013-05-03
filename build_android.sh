#!/bin/bash
# build_android.sh - Compiles dependencies for Android

if [ -z "$ANDROID_TOOLCHAIN" -o ! -d "$ANDROID_TOOLCHAIN" ]
then
    echo "Please set the ANDROID_TOOLCHAIN environment variable."
    echo "See 'docs/STANDALONE-TOOLCHAIN in Android NDK."
    exit 1
fi

# Get the directory of where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

TOP_DIR="$SCRIPT_DIR/android"

# Directory where the dependencies will be built
BUILD_DIR="$TOP_DIR/build"

# Directory where the dependencies will be installed
PREFIX_BASE="$TOP_DIR/prefix"

# Flags to pass to Make
MAKEFLAGS=""

# Version of ARM to compile for, 5 or 7
ARM_VERSION="7"

##############################################
# Shouldn't need to change anything below here
##############################################

TARGETS="util_linux expat openssl libev xmp3"

build_util_linux() {
    local base_ver=2.22
    local ver=2.22.2
    local dir="util-linux-$ver"
    local pkg="$dir.tar.xz"
    local url="http://www.kernel.org/pub/linux/utils/util-linux/v$base_ver/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-agetty \
        --disable-chsh-only-listed \
        --disable-cramfs \
        --disable-eject \
        --disable-fallocate \
        --disable-fsck \
        --disable-kill \
        --disable-libblkid \
        --disable-libmount \
        --disable-login \
        --disable-losetup \
        --disable-mount \
        --disable-mountpoint \
        --disable-nls \
        --disable-partx \
        --disable-pg-bell \
        --disable-pivot_root \
        --disable-rename \
        --disable-require-password \
        --disable-schedutils \
        --disable-su \
        --disable-sulogin \
        --disable-switch_root \
        --disable-unshare \
        --disable-use-tty-group \
        --disable-utmpdump \
        --disable-uuidd \
        --disable-wall \
        --disable-wdctl \
        --disable-write \
        --disable-shared --without-ncurses
    make libuuid.la

    install -D .libs/libuuid.a "$PREFIX/lib/libuuid.a"
    install -D libuuid/src/uuid.h "$PREFIX/include/uuid/uuid.h"
}

build_expat() {
    local ver=2.1.0
    local dir="expat-$ver"
    local pkg="$dir.tar.gz"
    local url="http://downloads.sourceforge.net/project/expat/expat/$ver/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-shared
    make
    make install
}

build_openssl() {
    local ver=1.0.1e
    local dir="openssl-$ver"
    local pkg="$dir.tar.gz"
    local url="http://www.openssl.org/source/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    if [ "$ARCH" = "arm" ]
    then
        ./Configure --prefix="$PREFIX" android-armv7
    else
        ./Configure --prefix="$PREFIX" android-x86
    fi

    # Force -j1 since parallel builds seem totally unreliable
    unset MAKEFLAGS
    make -j1
    make install
}

build_libev() {
    local ver=4.15
    local dir="libev-$ver"
    local pkg="$dir.tar.gz"
    local url="http://dist.schmorp.de/libev/$pkg"

    download_package "$pkg" "$url"
    cd "$dir"

    update_autotools .
    ./configure --host=arm-linux-androideabi --prefix="$PREFIX" \
        --disable-shared --enable-static
    make CFLAGS="$CFLAGS -DEV_SELECT_USE_FD_SET"
    make install
}

build_xmp3() {
    cd "$SCRIPT_DIR"
    ./waf configure build
}

# Simple function to download (if they haven't been already) and extract
# packages if they don't exist.
download_package() {
    local pkg="$1"; shift
    local url="$1"; shift

    pushd "$TOP_DIR"

    if [ ! -f "$pkg" ]
    then
        curl -LO --disable-epsv "$url"
    fi

    popd

    tar -xf "$TOP_DIR/$pkg"
}

# For packages with outdated autotools scripts which don't have Android support
update_autotools() {
    local dest="$1"; shift

    pushd "$TOP_DIR"
    if [ ! -f "config.guess" ]
    then
        curl -L -o config.guess "http://git.savannah.gnu.org/cgit/automake.git/plain/lib/config.guess?h=maint"
    fi

    if [ ! -f "config.sub" ]
    then
        curl -L -o config.sub "http://git.savannah.gnu.org/cgit/automake.git/plain/lib/config.sub?h=maint"
    fi
    popd

    cp "$TOP_DIR/config.guess" "$TOP_DIR/config.sub" "$dest"
}

export_environment() {
    ABI=$(ls "$ANDROID_TOOLCHAIN/bin" | grep 'gcc$' | sed 's/-gcc//')
    ARCH=$(echo $ABI | awk -F- '{print $1}')

    if [ "$ARCH" = "arm" ]
    then
        PREFIX="$PREFIX_BASE/$ABI-$ARM_VERSION"
    else
        PREFIX="$PREFIX_BASE/$ABI"
    fi

    PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
    LD_LIBRARY_PATH="$PREFIX/lib"
    LDFLAGS="-L$PREFIX/lib"
    CPPFLAGS="-I$PREFIX/include"

    if [ "$ARCH" = "arm" ]
    then
        CFLAGS+=" -fpic -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes"
        LDFLAGS+=" -no-canonical-prefixes"

        # For thumb release mode
        CPPFLAGS+=" -DNDEBUG"
        CFLAGS+=" -mthumb -Os -g -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64"

        # For arm release mode
        #CPPFLAGS+=" -DNDEBUG"
        #CFLAGS+=" -O2 -g -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300"

        if [ "$ARM_VERSION" = "7" ]
        then
            CFLAGS+=" -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
            LDFLAGS+=" -march=armv7-a -Wl,--fix-cortex-a8"
        else
            CFLAGS+=" -march=armv5te -mtune=xscale -msoft-float"
        fi

    elif [ "$ARCH" = "mipsel" ]
    then
        CPPFLAGS+=" -DNDEBUG"
        CFLAGS+=" -fpic -fno-strict-aliasing -finline-functions -ffunction-sections -funwind-tables -fmessage-length=0 -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers -no-canonical-prefixes -O2 -g -fomit-frame-pointer -funswitch-loops -finline-limit=300"
        LDFLAGS+=" -no-canonical-prefixes"
    elif [ "$ARCH" = "i686" ]
    then
        CPPFLAGS+=" -DNDEBUG"
        CFLAGS+=" -ffunction-sections -funwind-tables -no-canonical-prefixes -fstack-protector -O2 -g -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300"
        LDFLAGS+=" -no-canonical-prefixes"
    fi

    CXXFLAGS="$CFLAGS"

    export PATH="$ANDROID_TOOLCHAIN/bin:$PATH"

    export CC="$ABI-gcc"
    export CXX="$ABI-g++"

    export ARCH
    export ABI
    export PREFIX

    export MAKEFLAGS
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

    if [ "$ARCH" = "arm" ]
    then
        mkdir -p "$BUILD_DIR/$ABI-$ARM_VERSION"
        pushd "$BUILD_DIR/$ABI-$ARM_VERSION"
    else
        mkdir -p "$BUILD_DIR/$ABI"
        pushd "$BUILD_DIR/$ABI"
    fi

    for t in $targets
    do
        (set -x; eval "build_$t")
    done
}

get_help() {
    cat << EOF
Usage: $0 [options] [targets...]

Options:
    -a <5|7>        When building for ARM, which version to build for.
    -m "<args>"     Quoted flags to pass to make
    -j <n>          Shortcut to add "-j <n>" to MAKEFLAGS
    -h              This help output

Targets: $TARGETS
EOF
}

# Exit if any command fails
set -e

while getopts "a:m:j:h" flag
do
    case "$flag" in
        a)
            ARM_VERSION="$OPTARG"
            ;;
        m)
            MAKEFLAGS="$OPTARG"
            ;;
        j)
            MAKEFLAGS="$MAKEFLAGS -j$OPTARG"
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
