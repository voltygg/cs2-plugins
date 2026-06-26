#!/usr/bin/env bash
#
# Build the Linux plugin binary inside the deploy/Dockerfile build target.

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"
cd "$RepoRoot"

ProtoRoot="vendor/cs2-kit/vendor/hl2sdk-cs2"
Protoc="$ProtoRoot/devtools/bin/linux/protoc"

# Only the protos cs2-kit/Protobuf.ambuild declares as required; the full
# wildcard pulls in base_gcmessages.proto -> steammessages.proto (unneeded).
"$Protoc" \
    --proto_path="$ProtoRoot/common" \
    --proto_path="$ProtoRoot/thirdparty/protobuf-3.21.8/src" \
    --cpp_out="$ProtoRoot/public" \
    "$ProtoRoot/common/network_connection.proto" \
    "$ProtoRoot/common/networkbasetypes.proto" \
    "$ProtoRoot/common/engine_gcmessages.proto"

"$Protoc" \
    --proto_path="$ProtoRoot/common" \
    --proto_path="$ProtoRoot/game/shared" \
    --proto_path="$ProtoRoot/thirdparty/protobuf-3.21.8/src" \
    --cpp_out="$ProtoRoot/game/shared" \
    "$ProtoRoot/game/shared/usermessages.proto" \
    "$ProtoRoot/game/shared/usercmd.proto" \
    "$ProtoRoot/game/shared/gameevents.proto"

mkdir -p objdir
cd objdir
CC=clang CXX=clang++ python3 ../configure.py
ambuild
