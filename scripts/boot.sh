#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"
for name in TV_HOST TV_STOCK_HOST_KEY_ALIAS TV_STOCK_USER SSH_KEY; do require_var "$name"; done
exec ssh -i "$SSH_KEY" -o BatchMode=yes -o "HostKeyAlias=$TV_STOCK_HOST_KEY_ALIAS" \
    "$TV_STOCK_USER@$TV_HOST" 'python -u /tmp/lg-linux-tv/tv_kexec.py --execute-once'
