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
- Supports Ogg Vorbis conversion for most audio/video formats using integrated [FFMPEG](https://www.ffmpeg.org/).
- Fully customizable shortcuts
- Customizable game styles and noteskins
- Chart conversions between .sm/.ssc, .dwi, and .osu formats
- Multiplatform support (Windows, Linux, and MacOS)
- Preview window to examine the behavior of stops/fakes/speed mod changes

## Supported formats

Audio formats:
- Ogg Vorbis (.ogg)
- MPEG-1 Audio Layer III (.mp3)
- Waveform Audio File Format (.wav)
The program can automatically convert other formats into one of these three.

Simfile formats:

- StepMania/ITG (.sm)
- StepMania 5 (.ssc)
- Dance With Intensity (.dwi)
- osu! (.osu)
Loading, saving, and conversion are all supported.

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
Custom game styles are also supported.

# About this project

## Support and Contributing to the project

When you have questions about ArrowVortex and its functionality, make a discussion topic!

If you would like to report bugs or suggest simple features to add, please use the issue templates to do so. Make sure an issue for your problem doesn't exist before you create a new one.
When the changes you would like to add are of a larger scope (e.g. adding a new subsystem to the editor or redesigning a popup window), make a discussion instead.

Pull requests to add features and fix bugs are always welcomed; however, make sure that you understand the feature you are trying to implement and that you have considered all the potential ramifications of your change before submitting one (otherwise it may be denied). 
Please reach out to @uvcat7 with any questions. After a contribution and upon request, @uvcat7 will add you to the repository as a collaborator.

## Building ArrowVortex

The project uses CMake for building and vcpkg for package management. If building on Windows, Visual Studio is recommended, but it is not required.

Currently, builds support Windows, Linux, and MacOS, but since the project is SDL3-based it can be ported to a platform of your choosing if you'd like.

See the [build details](BUILDING.md) for more information.

## License

ArrowVortex is provided under the GPLv3 license, or at your option, any later version.

The original author provided the volunteer developers with an archive of the source code, which did not include a formal license. The original author explicitly requested that any new code developed by the volunteer developers be released under a license which prevents the code from being used in closed source software. The volunteer developers, as a separate party, have acted in good faith to comply with this request and bear no liability for the licensing status of the original code as provided to them.

For licensing info on the dependent projects and a list of contributors, see the [CREDITS file](CREDITS).
