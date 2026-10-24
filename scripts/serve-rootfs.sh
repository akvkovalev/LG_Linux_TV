#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
need python3
[ -f "$dist_dir/alpine-full-armv7.tar.gz" ] || die "build the full rootfs first"
cd "$dist_dir"
exec python3 -m http.server "${ROOTFS_HTTP_PORT:-18080}" --bind 0.0.0.0
