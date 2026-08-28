# Building ArrowVortex

ArrowVortex uses CMake 3.28+, Ninja, and the pinned vcpkg manifest in
`vcpkg.json`. Set `VCPKG_ROOT` to a vcpkg checkout before using a release
preset.

## macOS

The supported deployment target is macOS 12.0. Release builds use static
vcpkg triplets so the app bundle does not depend on Homebrew or build-tree
paths.

```sh
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --parallel
```

Use `macos-x64-release` on an Intel Mac. For local Apple Silicon development
with Homebrew dependencies, use `macos-local-debug`. The app is emitted under
`out/build/<preset>/src/System/ArrowVortex.app`.

The CI workflow builds both native architectures on matching GitHub-hosted
runners, merges Mach-O files with `lipo`, validates both slices, signs the app,
and creates a DMG. Tagged builds use Developer ID and notarization only when
the protected release environment provides the documented secrets.

## Linux

Install the SDL build prerequisites listed in `.github/workflows/release.yml`,
then run:

```sh
cmake --preset linux-release
cmake --build --preset linux-release --parallel
ctest --test-dir out/build/linux-release --output-on-failure
cmake --install out/build/linux-release --prefix out/install/linux
tools/package_linux.sh out/install/linux out/packages
```

Ubuntu 22.04 is the release baseline. The supported Linux release artifact is
the x86_64 tarball produced by the packaging script.

## Windows

From Developer PowerShell with Ninja and vcpkg available:

```pwsh
cmake --preset windows-release
cmake --build --preset windows-release --parallel
ctest --test-dir out/build/windows-release --output-on-failure
```

## Tests

`--smoke-test [fixture]` initializes SDL video, OpenGL 2.1, audio, the editor,
and optional simfile input, renders 120 frames, and exits. CTest also performs
load-save-reload checks for SM, SSC, osu!, and DWI fixtures, including BPM
changes, repeated stops at BPM boundaries, warps, holds, and Unicode metadata.

## Release secrets

Store these only in GitHub's protected `release` environment:

- `MACOS_CERTIFICATE` (base64-encoded Developer ID Application PKCS#12)
- `MACOS_CERTIFICATE_PASSWORD`
- `MACOS_SIGN_IDENTITY`
- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_PASSWORD` (app-specific password)

Without them, CI still produces ad-hoc-signed Universal 2 artifacts. A tagged
notarized release is intentionally blocked until all signing values exist.
