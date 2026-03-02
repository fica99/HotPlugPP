#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec pwsh -NoLogo -NoProfile -File "$SCRIPT_DIR/setup-agent-worktrees.ps1" "$@"
