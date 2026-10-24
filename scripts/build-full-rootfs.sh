#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var ALPINE_MINIROOTFS
require_var ALPINE_MINIROOTFS_SHA256
require_var AUTHORIZED_KEY
for name in TV_HOST TV_NETMASK_CIDR TV_GATEWAY; do require_var "$name"; done
need qemu-arm-static
need chroot
need sha256sum

actual=$(sha256sum "$ALPINE_MINIROOTFS" | awk '{print $1}')
[ "$actual" = "$ALPINE_MINIROOTFS_SHA256" ] || die "Alpine minirootfs checksum mismatch"

root="$build_dir/full-rootfs"
rm -rf "$root"
mkdir -p "$root"
tar -xzf "$ALPINE_MINIROOTFS" -C "$root"
cp "$(command -v qemu-arm-static)" "$root/usr/bin/qemu-arm-static"
cp /etc/resolv.conf "$root/etc/resolv.conf"

if ! chroot "$root" /bin/true 2>/dev/null; then
    die "ARM binfmt is not active; install qemu-user-static/binfmt-support and enable qemu-arm"
fi

chroot "$root" /sbin/apk add --no-cache \
    alpine-base openssh xorg-server xf86-video-fbdev openbox xterm \
    font-dejavu kbd build-base linux-headers

install -D -m 0600 "$AUTHORIZED_KEY" "$root/root/.ssh/authorized_keys"
install -D -m 0644 "$repo_dir/rootfs/sshd_config.lg-tv" "$root/etc/ssh/sshd_config.lg-tv"
cp -a "$repo_dir/rootfs/overlay/." "$root/"
install -D -m 0644 "$dist_dir/gdma-trigger.ko" "$root/opt/lg-linux-tv/modules/gdma-trigger.ko"
install -D -m 0644 "$dist_dir/rtk_venusfb.ko" "$root/opt/lg-linux-tv/modules/rtk_venusfb.ko"
install -D -m 0644 "$repo_dir/rootfs/fix-fb-ioctl.c" "$root/tmp/fix-fb-ioctl.c"

chroot "$root" /usr/bin/cc -shared -fPIC -O2 -ldl \
    /tmp/fix-fb-ioctl.c -o /usr/local/lib/libfixfbioctl.so
rm -f "$root/tmp/fix-fb-ioctl.c"
chroot "$root" /sbin/apk del build-base linux-headers

python3 - "$repo_dir/rootfs/full-init.in" "$root/init-alpine" <<'PY'
import os, pathlib, sys
src = pathlib.Path(sys.argv[1]).read_text()
for key in ('TV_HOST','TV_NETMASK_CIDR','TV_GATEWAY'):
    src = src.replace('@'+key+'@', os.environ[key])
path = pathlib.Path(sys.argv[2]); path.write_text(src); path.chmod(0o755)
PY

rm -f "$root/usr/bin/qemu-arm-static"
rm -f "$root/etc/ssh/ssh_host_"*
tar --numeric-owner --owner=0 --group=0 --sort=name --mtime='@0' \
    -czf "$dist_dir/alpine-full-armv7.tar.gz" -C "$root" .
sha256sum "$dist_dir/alpine-full-armv7.tar.gz"
