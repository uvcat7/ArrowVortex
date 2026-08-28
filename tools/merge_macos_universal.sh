#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 arm64.app x86_64.app output.app" >&2
    exit 2
fi

arm_app=$1
intel_app=$2
output_app=$3

test -d "$arm_app/Contents"
test -d "$intel_app/Contents"
rm -rf "$output_app"
ditto "$arm_app" "$output_app"

find "$arm_app/Contents" -type f -print | while IFS= read -r arm_file; do
    if file "$arm_file" | grep -q 'Mach-O'; then
        relative=${arm_file#"$arm_app"/}
        intel_file="$intel_app/$relative"
        output_file="$output_app/$relative"
        test -f "$intel_file"
        lipo -create "$arm_file" "$intel_file" -output "$output_file"
        architectures=$(lipo -archs "$output_file")
        echo "$architectures" | grep -qw arm64
        echo "$architectures" | grep -qw x86_64
    fi
done

codesign --force --deep --sign - "$output_app"
codesign --verify --deep --strict --verbose=2 "$output_app"
