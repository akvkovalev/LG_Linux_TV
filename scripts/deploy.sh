#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"
for name in TV_HOST TV_STOCK_HOST_KEY_ALIAS TV_STOCK_USER SSH_KEY; do require_var "$name"; done

bundle="$dist_dir/boot"
[[ -f "$bundle/stage-manifest.json" ]] || die "build the boot bundle first"
ssh_opts=(-i "$SSH_KEY" -o BatchMode=yes -o "HostKeyAlias=$TV_STOCK_HOST_KEY_ALIAS")
target="$TV_STOCK_USER@$TV_HOST"

ssh "${ssh_opts[@]}" "$target" 'rm -rf /tmp/lg-linux-tv && mkdir -p /tmp/lg-linux-tv'
scp -O "${ssh_opts[@]}" \
    "$bundle/uImage-rtd288o" "$bundle/rtd288o.dtb" \
    "$bundle/alpine-loader.cpio.gz" "$bundle/kexec-arm" \
    "$bundle/stage-manifest.json" "$repo_dir/tools/tv_kexec.py" \
    "$repo_dir/tools/monitor_state.py" "$target:/tmp/lg-linux-tv/"
ssh "${ssh_opts[@]}" "$target" 'python /tmp/lg-linux-tv/tv_kexec.py --check'
echo 'Load/unload check passed. Run scripts/boot.sh to enter Alpine.'
