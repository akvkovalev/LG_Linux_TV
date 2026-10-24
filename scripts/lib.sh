#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ -f "$repo_dir/.env" ]; then
    # shellcheck disable=SC1091
    set -a
    . "$repo_dir/.env"
    set +a
fi

die() { printf 'error: %s\n' "$*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }
require_var() { eval "value=\${$1:-}"; [ -n "$value" ] || die "set $1 in .env"; }

build_dir=${BUILD_DIR:-$repo_dir/build}
dist_dir=${DIST_DIR:-$repo_dir/dist}
mkdir -p "$build_dir" "$dist_dir"
