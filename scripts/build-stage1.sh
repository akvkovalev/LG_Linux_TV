#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
require_var CROSS_COMPILE
require_var TV_HOST
require_var TV_MAC
require_var NETCONSOLE_HOST
require_var NETCONSOLE_HOST_MAC
[ "${TV_NETMASK_CIDR:-}" = 24 ] || die "stage-1 currently supports only a /24 network"

defs_for_ip() {
    prefix=$1 value=$2
    oldifs=$IFS; IFS=.; set -- $value; IFS=$oldifs
    [ "$#" -eq 4 ] || die "invalid IPv4 address: $value"
    printf -- '-D%s_IP_A=%s -D%s_IP_B=%s -D%s_IP_C=%s -D%s_IP_D=%s ' \
        "$prefix" "$1" "$prefix" "$2" "$prefix" "$3" "$prefix" "$4"
}
defs_for_mac() {
    prefix=$1 value=$2
    oldifs=$IFS; IFS=:; set -- $value; IFS=$oldifs
    [ "$#" -eq 6 ] || die "invalid MAC address: $value"
    i=0
    for byte in "$@"; do printf -- '-D%s_MAC_%s=0x%s ' "$prefix" "$i" "$byte"; i=$((i + 1)); done
}

defs="$(defs_for_ip TV "$TV_HOST")$(defs_for_ip NETCONSOLE "$NETCONSOLE_HOST")"
defs="$defs$(defs_for_mac TV "$TV_MAC")$(defs_for_mac NETCONSOLE "$NETCONSOLE_HOST_MAC")"
defs="$defs -DNETCONSOLE_PORT=${NETCONSOLE_PORT:-6666}"

# shellcheck disable=SC2086
"${CROSS_COMPILE}gcc" -Os -marm -march=armv7-a -mfloat-abi=soft \
    -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
    -fno-unwind-tables -fno-asynchronous-unwind-tables \
    -fno-tree-loop-distribute-patterns -nostdlib -static -no-pie \
    -DCODEX_CHILD_DELAY=5 -DCODEX_HOST_ALIVE=0 $defs \
    -Wl,-e,_start -Wl,--build-id=none \
    "$repo_dir/kernel/init-stage1.c" -o "$dist_dir/init-stage1"

printf 'Stage-1 PID 1 copied to %s\n' "$dist_dir"
