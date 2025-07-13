# ArrowVortex

ArrowVortex is a simfile editor for Windows. It can be used to create or edit stepfiles for various rhythm games, such as StepMania, ITG, osu!, and other games which support DDR-style and/or PIU-style panel layouts.

This is a continuation of the original project by Bram 'Fietsemaker' van de Wetering. He has graciously allowed for the open sourcing of this code so that development can continue.

See the [About this project](#about-this-project) section below for more information about this repository.

## Features

Some of the features that ArrowVortex has to offer include:

- Automatic BPM/offset detection
- Automatic stream generation
- Automatic difficulty estimation
- Full editing history with undo/redo
- Dancing Bot: plays your charts using two virtual feet
- Powerful editing tools: copy/paste/mirror/expand/compress/etc.
- Scrollable minimap with chart preview, for easy navigation
- Supports Ogg Vorbis conversion for MP3/WAV files
   - requires any version of `oggenc2.exe`, available from [RareWares](https://www.rarewares.org/ogg-oggenc.php)
- Fully customizable shortcuts
- Customizable game styles and noteskins

The following features are exclusive to the newer versions of ArrowVortex provided here:

- Support for Fake and Lift note types
  - New "Convert" menu options for converting to Fakes or Lifts
- Timestamps can be copied and pasted
- Complete support for custom snaps (copy-pasting, saving, editing)
- Non-destructive paste option (Shift+Ctrl+V)
- New visual sync feature, inspired by DDreamStudio (sub-beat placement)
- Wider range and finer granularity in zooming
- Fixed Mini options have been replaced with a slider
- Ability to show Stepmania-style preview during playback
- Auto-jump to next snap option

## Supported formats

Audio formats:
- Ogg Vorbis (.ogg)
- MPEG-1 Audio Layer III (.mp3)
- Waveform Audio File Format (.wav)

Simfile formats:

- StepMania/ITG (.sm)
- StepMania 5 (.ssc)
- Dance With Intensity (.dwi)
- osu! (.osu)

Game styles:

- Dance Single
- Dance Double
- Dance Couple
- Dance Routine
- Dance Solo
- Pump Single
- Pump Halfdouble
- Pump Double
- Pump Couple

## About this project

Unfortunately, the source code for the 2017-02-25 release of ArrowVortex has been lost to time. The only surviving archive of source code was a development snapshot from 2016. A small group of volunteer developers put a lot of time and effort into restoring the full functionality of the 2017 release of ArrowVortex.

Some new features were introduced as well when preparing this release, and some bugs present in the 2017 version have been fixed as well.

## Building ArrowVortex

### Windows:

In order to compile ArrowVortex on your own PC, Visual Studio is required. It is recommended to use Visual Studio 2022 with the "Desktop development for C++" components installed.

Simply open `build/VisualStudio/ArrowVortex.sln` in Visual Studio, and build the project.

### Linux:

Compiling for Linux requires that you have `vcpkg`, `powershell`, `cmake`, and most importantly `w64-mingw32`.

You may find `w64-mingw32` for your Linux distribution at:
* AUR, as `mingw-w64-tools`
* Debian/Ubuntu, as `g++-mingw-w64-x86-64` and `gcc-mingw-w64-x86-64`
* Fedora, as `mingw64-gcc-c++`

You may find installation info of `vcpkg` at [vcpkg's getting-started documentation](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash).

You may find binaries of `powershell` at [PowerShell's repository](https://github.com/PowerShell/PowerShell) or the AUR.

You will likely already have `cmake`, but if needed, you will find it in your package manager.

Once you have those set up, run `export VCPKG_ROOT=[where your vcpkg directory is]`.

This command is needed to define vcpkg's root directory and use it when building with cmake.

Now you may run this: `cmake -B build -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`

This command specifies that you are using a MinGW environment that is statically linked and targets 64-bit, and for CMake to use vcpkg's buildsystem.

This command also requires powershell as it is used for vcpkg's copy tools, and will fail to compile if it isn't installed.

Now enter your build directory with `cd build`.

Finally, build ArrowVortex with `cmake --build .`.

## License

ArrowVortex is provided under the GPLv3 license, or at your option, any later version.

The original author provided the volunteer developers with an archive of the source code, which did not include a formal license. The original author explicitly requested that any new code developed by the volunteer developers be released under a license which prevents the code from being used in closed source software. The volunteer developers, as a separate party, have acted in good faith to comply with this request and bear no liability for the licensing status of the original code as provided to them.

For licensing info on the dependent projects and a list of contributors, see the CREDITS file.
