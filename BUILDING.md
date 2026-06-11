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
cmake -Bbuild -S. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-linux -DPRESET_NAME=linux-release
cmake --build build
```
The resulting binary will be located at `out\install\{windows-debug | windows-release}\bin\ArrowVortex.exe`

To install ArrowVortex to a folder run:
```pwsh
cmake --install build --prefix "<your_folder_here>"
```

### With Visual Studio
1. Run `vcpkg integrate install` in Developer PowerShell. This will integrate vcpkg with your installation of Visual Studio and needs to be run once.
2. Open root folder of this project in Visual Studio

## Linux

Prerequsites:
* Needed packages from package manager
* CMake
* vcpkg (per https://lindevs.com/install-vcpkg-on-ubuntu). Note you will need to give your user read/write access to the install folder with sudo.
* `VCPKG_ROOT` environment variable pointing at vcpkg installation folder (env VCPKG_ROOT={path}) OR install it to /opt/vcpkg.

### Build Packages
For Ubuntu/Debian install:
```
sudo apt-get install build-essential git \
pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
libaudio-dev libjack-dev libsndio-dev libusb-1.0-0-dev libx11-dev libxext-dev \
libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libwayland-dev \
libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev fcitx-libs-dev \
libasound2-dev libdecor-0-dev python3-venv libltdl-dev autoconf autoconf-archive \
automake libtool libx11-dev libxft-dev libxext-dev libwayland-dev libxkbcommon-dev \
libegl1-mesa-dev libibus-1.0-dev clang-tidy clang-format zenity
```

For Fedora install:
```sudo dnf install gcc git-core make cmake \
alsa-lib-devel fribidi-devel pulseaudio-libs-devel pipewire-devel \
libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXfixes-devel \
libXi-devel libXScrnSaver-devel libXtst-devel dbus-devel ibus-devel \
systemd-devel mesa-libGL-devel libxkbcommon-devel mesa-libGLES-devel \
mesa-libEGL-devel vulkan-devel wayland-devel wayland-protocols-devel \
libdrm-devel mesa-libgbm-devel libusb1-devel libdecor-devel \
pipewire-jack-audio-connection-kit-devel libthai-devel liburing-devel zlib-ng-compat-static 
```

You may be missing more packages depending on your distribution and what default packages it comes with. However, missing packages should be flagged by vcpkg or caused a build command to fail.

### With CMake CLI
In the terminal at the root folder of this project run:
```
cmake -Bbuild -S. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-linux -DPRESET_NAME={linux-debug | linux-release}
cmake --build build
```
The resulting binary will be located at `out\install\{linux-debug | linux-release}\bin\ArrowVortex`

To install ArrowVortex to a folder run:
```
cmake --install build --prefix "{your_folder_here}"
```

### With Visual Studio
1. Use the connection manager to connect to your Linux installation (preferably WSL) over SSH: https://learn.microsoft.com/en-us/cpp/linux/connect-to-your-remote-linux-computer?view=msvc-170#connect-to-wsl
2. Install vcpkg on the remote system to /opt/vcpkg, or modify CMakePresets.json/create a user preset.
3. Build on Windows to the Linux installation. WSL can run the GUI application.

## Troubleshooting
### No SDL3 video or audio device
One of the dependent development packages SDL needs is missing. Delete your cached vcpkg packages so they are force rebuilt, then rerun `vcpkg integrate install` and the build process.
### On WSL, the window border is sometimes missing
This is a WSL bug and cannot be fixed. Restarting your PC tends to work temporarily.
### On WSL, the audio is choppy
This can't be fixed without desyncing the audio between the host and client, as WSL uses pulseaudio to send the sound to Windows over a virtual network audio port.