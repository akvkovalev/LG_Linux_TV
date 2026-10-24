#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var CROSS_COMPILE

out="$build_dir/kernel"
[ -f "$out/Makefile" ] || die "build the kernel first"
make -C "$out" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    M="$repo_dir/modules/gdma-trigger" modules
cp "$repo_dir/modules/gdma-trigger/gdma-trigger.ko" "$dist_dir/gdma-trigger.ko"
printf 'GDMA trigger copied to %s\n' "$dist_dir"
