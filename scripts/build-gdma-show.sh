#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var CROSS_COMPILE

out="$build_dir/kernel"
[ -f "$out/Makefile" ] || die "build the kernel first"
make -C "$out" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    M="$repo_dir/modules/gdma-show" modules
cp "$repo_dir/modules/gdma-show/gdma-show.ko" "$dist_dir/gdma-show.ko"
printf 'GDMA OSD enabler copied to %s\n' "$dist_dir"
