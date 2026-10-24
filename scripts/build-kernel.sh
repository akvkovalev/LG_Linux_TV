#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"

require_var CROSS_COMPILE
need make

src="$build_dir/linux-4.4.3"
out="$build_dir/kernel"
[ -f "$src/Makefile" ] || die "run scripts/apply-kernel-patch.sh first"
mkdir -p "$out"
cp "$repo_dir/configs/rtd288o-tested.config" "$out/.config"

make -C "$src" O="$out" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    HOSTCFLAGS='-O2 -fcommon' olddefconfig
make -C "$src" O="$out" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    HOSTCFLAGS='-O2 -fcommon' \
    KCFLAGS='-w -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=int-conversion' \
    -j"${JOBS:-4}" zImage modules

cp "$out/arch/arm/boot/zImage" "$dist_dir/zImage-rtd288o"
cp "$out/arch/arm/boot/Image" "$dist_dir/Image-rtd288o"
cp "$out/drivers/rtk_kdriver/rtk_venusfb.ko" "$dist_dir/rtk_venusfb.ko"
printf 'Kernel and venusfb copied to %s\n' "$dist_dir"
