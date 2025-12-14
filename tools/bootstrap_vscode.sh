#!/usr/bin/env sh
set -e

# TOOLSDIR = <repo_root>/tools (absolute path)
TOOLSDIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
# REPO = parent of tools
REPO="$(CDPATH= cd -- "$TOOLSDIR/.." && pwd)"

python3 "$TOOLSDIR/init_env.py" --os linux --config "$TOOLSDIR/env_configure.json" --repo-root "$REPO" "$@"
