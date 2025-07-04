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
- Support for custom snaps
- Non-destructive paste option (Shift+Ctrl+V)
- New visual sync feature, inspired by DDreamStudio (sub-beat placement)
- Wider range and finer granularity in zooming
- Replaced the fixed Mini options with a slider

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

In order to compile ArrowVortex on your own PC, Visual Studio is required. It is recommended to use Visual Studio 2022 with the "Desktop development for C++" components installed.

Simply open `build/VisualStudio/ArrowVortex.sln` in Visual Studio, and build the project.

## License

ArrowVortex is provided under the GPLv3 license, or at your option, any later version.

The original author provided the volunteer developers with an archive of the source code, which did not include a formal license. The original author explicitly requested that any new code developed by the volunteer developers be released under a license which prevents the code from being used in closed source software. The volunteer developers, as a separate party, have acted in good faith to comply with this request and bear no liability for the licensing status of the original code as provided to them.

Code from the following projects is included in this repository:
- [freetype](https://freetype.org/license.html), using the FreeType License, which is compatible with GPLv3
   - See  `lib/freetype/` for more info.
- [libogg & libvorbis](https://gitlab.xiph.org/xiph), using its BSD-style license, which is compatible with GPLv3
   - See  `lib/libvorbis/license` for more info.
- [libmad](https://www.underbit.com/products/mad/), using the GPL license (libmad allows for GPLv2 or any later version)
   - See  `lib/freetype/` for more info.
- [lua](https://www.lua.org/license.html), using its GPLv3-compatible permissive license
   - See  `lib/lua/` for more info.
- Code from [liir.c](https://www.exstrom.com/journal/sigproc/dsigproc.html) by Exstrom Labratories, which is GPL, is present in `src/Editor/Butterworth.cpp`
- Code from [Takuya OOURA's General Purpose FFT Package](https://www.kurims.kyoto-u.ac.jp/~ooura/fft.html), which is released freely, is present in `src/Editor/FFT.cpp` 
- Code from the [aubio](https://github.com/aubio/aubio) library, which is also GPLv3, is present in `src/Editor/Aubio.h` and `src/Editor/FindOnsets.cpp`


## Contributors

Original author:
- Bram 'Fietsemaker' van de Wetering

The following people helped prepare the initial public release of the open sourced ArrowVortex:
- @uvcat7
- @sukibaby
- @Psycast
- @DeltaEpsilon7787
- @DolphinChips
