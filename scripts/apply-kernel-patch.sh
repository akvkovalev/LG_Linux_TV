#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"

require_var LG_KERNEL_ARCHIVE
require_var LG_KERNEL_SHA256
need tar
need patch
need sha256sum

actual=$(sha256sum "$LG_KERNEL_ARCHIVE" | awk '{print $1}')
[ "$actual" = "$LG_KERNEL_SHA256" ] || die "LG kernel archive checksum mismatch"

src="$build_dir/linux-4.4.3"
rm -rf "$src"
tar -xzf "$LG_KERNEL_ARCHIVE" -C "$build_dir"
[ -f "$src/Makefile" ] || die "archive did not produce linux-4.4.3"
patch -d "$src" -p1 < "$repo_dir/patches/0001-tested-native-boot.patch"
printf 'Prepared %s\n' "$src"
