#!/bin/bash
set -e

# This script generates protobuf files for the HL2SDK
cd "$(dirname "$0")/.."

# Source environment variables
source env.sh

if [ -z "$HL2SDKCS2" ]; then
    echo "Error: HL2SDKCS2 environment variable not set"
    exit 1
fi

PROTOC="$HL2SDKCS2/devtools/bin/protoc"
PROTO_PATH_COMMON="$HL2SDKCS2/common"
PROTO_PATH_GOOGLE="$HL2SDKCS2/thirdparty/protobuf-3.21.8/src"
OUTPUT_DIR="$HL2SDKCS2/public"

if [ ! -f "$PROTOC" ]; then
    echo "Error: protoc not found at $PROTOC"
    exit 1
fi

echo "Generating protobuf files for hl2sdk-cs2..."
echo "SDK path: $HL2SDKCS2"

# Generate common proto files
echo "Generating common/*.proto..."
"$PROTOC" \
    --proto_path="$PROTO_PATH_COMMON" \
    --proto_path="$PROTO_PATH_GOOGLE" \
    --cpp_out="$OUTPUT_DIR" \
    "$PROTO_PATH_COMMON"/*.proto

echo "Done! Generated files are in $OUTPUT_DIR"
