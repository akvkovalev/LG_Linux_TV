#!/bin/sh
set -eu
. "$(dirname "$0")/lib.sh"
need python3
need dtc
python3 "$repo_dir/tools/make_boot_bundle.py"
