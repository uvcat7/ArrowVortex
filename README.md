# ArrowVortex

ArrowVortex is a cross-platform simfile editor for macOS, Linux, and Windows. It
can create or edit stepfiles for StepMania, ITG, osu!, and other games which
support DDR-style and/or PIU-style panel layouts.

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

# Platforms

- macOS 12.0 or later on Apple Silicon and Intel (Universal 2 DMG)
- Linux x86_64 (portable tarball built on Ubuntu 22.04)
- Windows x64

The desktop ports use SDL3 for lifecycle, windowing, input, dialogs, clipboard,
URLs, drag-and-drop, and audio while retaining ArrowVortex's OpenGL renderer and
simfile data model.

# About this project

## Support and Contributing to the project

When you have questions about ArrowVortex and its functionality, make a discussion topic!

If you would like to report bugs or suggest simple features to add, please use the issue templates to do so. Make sure an issue for your problem doesn't exist before you create a new one.
When the changes you would like to add are of a larger scope (e.g. adding a new subsystem to the editor or redesigning a popup window), make a discussion instead.

Pull requests to add features and fix bugs are always welcomed. Please reach out to @uvcat7 with any questions. After a contribution, @uvcat7 will add you to the repository as a collaborator.

## Building ArrowVortex

The project uses CMake presets and a pinned vcpkg manifest. See the
[build details](BUILDING.md) for native instructions, tests, package validation,
and release-signing configuration.

## License

ArrowVortex is provided under the GPLv3 license, or at your option, any later version.

The original author provided the volunteer developers with an archive of the source code, which did not include a formal license. The original author explicitly requested that any new code developed by the volunteer developers be released under a license which prevents the code from being used in closed source software. The volunteer developers, as a separate party, have acted in good faith to comply with this request and bear no liability for the licensing status of the original code as provided to them.

For licensing info on the dependent projects and a list of contributors, see the [CREDITS file](CREDITS).
