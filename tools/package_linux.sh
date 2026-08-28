#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 install-prefix output-directory" >&2
    exit 2
fi

prefix=$1
output_dir=$2
package_root="$output_dir/ArrowVortex-linux-x86_64"
archive="$output_dir/ArrowVortex-linux-x86_64.tar.gz"

rm -rf "$package_root"
mkdir -p "$package_root"
cp -R "$prefix/bin/." "$package_root/"
cp arrowvortex.desktop "$package_root/ArrowVortex.desktop"
cp LICENSE CREDITS "$package_root/"
cp docs/linux-launch.txt "$package_root/README.txt"

if ldd "$package_root/ArrowVortex" | grep -E 'not found|out/build|_work'; then
    echo "Linux package has unresolved or build-tree dependencies" >&2
    exit 1
fi

tar -C "$output_dir" -czf "$archive" "$(basename "$package_root")"
sha256sum "$archive" > "$archive.sha256"
