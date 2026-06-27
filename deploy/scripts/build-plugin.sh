#!/usr/bin/env bash
#
# Build the Linux plugin binary inside the deploy/Dockerfile build target.

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"
cd "$RepoRoot"

CC=clang CXX=clang++ "$RepoRoot/scripts/build.sh" linux-steamrt-release
