#!/usr/bin/env bash

# Shared build helpers. Canonical copy lives in the cs2-kit submodule (single source
# of truth); this just sources it. Callers must set RepoRoot before run_tool.

_lib_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_cs2_kit_common="$_lib_dir/../../vendor/cs2-kit/scripts/lib/common.sh"

if [[ ! -f "$_cs2_kit_common" ]]; then
    echo "ERROR: cs2-kit helpers not found at $_cs2_kit_common" >&2
    echo "Initialize submodules: git submodule update --init --recursive" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "$_cs2_kit_common"
