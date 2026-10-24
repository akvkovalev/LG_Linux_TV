#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var ALPINE_MINIROOTFS
require_var ALPINE_MINIROOTFS_SHA256
for name in TV_HOST TV_NETMASK_CIDR TV_GATEWAY NETCONSOLE_HOST NETCONSOLE_PORT ROOTFS_URL; do require_var "$name"; done
need python3
need sha256sum

actual=$(sha256sum "$ALPINE_MINIROOTFS" | awk '{print $1}')
[ "$actual" = "$ALPINE_MINIROOTFS_SHA256" ] || die "Alpine minirootfs checksum mismatch"

root="$build_dir/loader-rootfs"
rm -rf "$root"
mkdir -p "$root"
tar -xzf "$ALPINE_MINIROOTFS" -C "$root"
mkdir -p "$root/dev" "$root/proc" "$root/sys" "$root/run" "$root/tmp"
install -m 0755 "$dist_dir/init-stage1" "$root/init"

python3 - "$repo_dir/rootfs/loader-init.in" "$root/init-alpine" <<'PY'
import os, pathlib, sys
src = pathlib.Path(sys.argv[1]).read_text()
for key in ('TV_HOST','TV_NETMASK_CIDR','TV_GATEWAY','NETCONSOLE_HOST','NETCONSOLE_PORT','ROOTFS_URL'):
    src = src.replace('@'+key+'@', os.environ[key])
path = pathlib.Path(sys.argv[2]); path.write_text(src); path.chmod(0o755)
PY

python3 "$repo_dir/tools/build_initramfs.py" "$root" "$dist_dir/alpine-loader.cpio.gz"
