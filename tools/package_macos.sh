#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 ArrowVortex.app output-directory" >&2
    exit 2
fi

app=$1
output_dir=$2
mkdir -p "$output_dir"

identity=${MACOS_SIGN_IDENTITY:-}
if [ -n "$identity" ]; then
    find "$app/Contents" -type f -print | while IFS= read -r file_path; do
        if file "$file_path" | grep -q 'Mach-O'; then
            codesign --force --options runtime --timestamp --sign "$identity" "$file_path"
        fi
    done
    codesign --force --options runtime --timestamp --sign "$identity" "$app"
else
    codesign --force --deep --sign - "$app"
fi

codesign --verify --deep --strict --verbose=2 "$app"

find "$app/Contents" -type f -print | while IFS= read -r file_path; do
    if file "$file_path" | grep -q 'Mach-O'; then
        dependencies=$(otool -L "$file_path")
        if echo "$dependencies" | grep -E '/(opt/homebrew|usr/local|vcpkg|out/build|_work)/'; then
            echo "non-portable dependency in $file_path" >&2
            exit 1
        fi
    fi
done

dmg="$output_dir/ArrowVortex-macOS-Universal2.dmg"
hdiutil create -quiet -volname ArrowVortex -srcfolder "$app" -ov -format UDZO "$dmg"
if [ -n "$identity" ]; then
    codesign --force --timestamp --sign "$identity" "$dmg"
fi

if [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && \
   [ -n "${APPLE_APP_PASSWORD:-}" ]; then
    xcrun notarytool submit "$dmg" --apple-id "$APPLE_ID" \
        --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
    xcrun stapler staple "$dmg"
    xcrun stapler validate "$dmg"
    spctl --assess --type open --context context:primary-signature -v "$dmg"
fi

shasum -a 256 "$dmg" > "$dmg.sha256"
