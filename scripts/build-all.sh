#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$here/apply-kernel-patch.sh"
"$here/build-kernel.sh"
"$here/build-gdma-trigger.sh"
"$here/build-gdma-show.sh"
"$here/build-stage1.sh"
"$here/build-loader-initramfs.sh"
"$here/build-kexec.sh"
sudo -E "$here/build-full-rootfs.sh"
"$here/build-boot-bundle.sh"
