#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var CROSS_COMPILE
need curl
need make

version=2.0.28
archive="$build_dir/kexec-tools-$version.tar.xz"
src="$build_dir/kexec-tools-$version"
headers="$build_dir/linux-headers"
[ -f "$archive" ] || curl -fL --retry 2 -o "$archive" \
    "https://www.kernel.org/pub/linux/utils/kernel/kexec/kexec-tools-$version.tar.xz"
[ -d "$src" ] || tar -xf "$archive" -C "$build_dir"
make -C "$build_dir/linux-4.4.3" O="$build_dir/kernel" ARCH=arm \
    INSTALL_HDR_PATH="$headers" headers_install

prefix=${CROSS_COMPILE%*-}
host=${prefix##*/}
PATH=$(dirname "${CROSS_COMPILE}gcc"):$PATH
export PATH
(
    cd "$src"
    LDFLAGS=-static CFLAGS="-O2 -std=gnu11 -DSYS_memfd_create=__NR_memfd_create -I$headers/include" \
        ./configure --host="$host" --without-zlib --without-lzma --without-xen
    make -j"${JOBS:-4}"
)
cp "$src/build/sbin/kexec" "$dist_dir/kexec-arm"
"${CROSS_COMPILE}strip" "$dist_dir/kexec-arm"
sha256sum "$archive" "$dist_dir/kexec-arm"
