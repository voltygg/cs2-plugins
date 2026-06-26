#!/usr/bin/env bash
#
# Build the Linux plugin binary inside the deploy/Dockerfile build target.

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"
cd "$RepoRoot"

ProtoRoot="vendor/cs2-kit/vendor/hl2sdk-cs2"
Protoc="$ProtoRoot/devtools/bin/linux/protoc"

"$Protoc" \
    --proto_path="$ProtoRoot/common" \
    --proto_path="$ProtoRoot/thirdparty/protobuf-3.21.8/src" \
    --cpp_out="$ProtoRoot/public" \
    "$ProtoRoot"/common/*.proto

"$Protoc" \
    --proto_path="$ProtoRoot/common" \
    --proto_path="$ProtoRoot/game/shared" \
    --proto_path="$ProtoRoot/thirdparty/protobuf-3.21.8/src" \
    --cpp_out="$ProtoRoot/game/shared" \
    "$ProtoRoot"/game/shared/*.proto

mkdir -p objdir
cd objdir
CC=clang CXX=clang++ python3 ../configure.py
ambuild
