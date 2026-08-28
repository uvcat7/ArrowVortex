#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 input.png output.icns" >&2
    exit 2
fi

source_png=$1
output_icns=$2

mkdir -p "$(dirname "$output_icns")"
# sips writes a multi-representation ICNS directly and remains available on
# every supported macOS/Xcode runner. This also avoids iconutil rejecting
# generated iconsets on newer SDKs.
sips -s format icns "$source_png" --out "$output_icns" >/dev/null
