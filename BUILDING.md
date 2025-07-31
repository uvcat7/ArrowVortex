# Building

## Windows

Prerequsites:
* Installation of Visual Studio Build Tools
* CMake (available with VS Build Tools)
* vcpkg (available with VS Build Tools)
* `VCPKG_ROOT` environment variable pointing at vcpkg installation folder

### With CMake CLI
With Developer PowerShell open at root folder of this project run:
```pwsh
cmake -Bbuild -S. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build build
```
Resulting binary will be located at `build/src/System/Debug/ArrowVortex.exe`

To install ArrowVortex to a folder run:
```pwsh
cmake --install build --prefix "<your_folder_here>"
```

### With Visual Studio
1. Run `vcpkg integrate install` in Developer PowerShell. This will integrate vcpkg with your installation of Visual Studio and needs to be run once.
2. Open root folder of this project in Visual Studio


## Troubleshooting
### CMake can't find freetype
CMake failed to integrate with vcpkg. Remove your build directory (either `build/` or `out/`) and follow instructions for your system.
