**Note:** stripped down version of [this page](https://github.com/ppy/osu-wiki/blob/master/wiki/osu!_File_Formats/Osu_(file_format)/en.md)

## Structure

The first line of the file specifies the file format version. For example, `osu file format v14` is the latest version.

The following content is separated into sections, indicated by section titles in square brackets.

| Section | Description | Content type |
| :-- | :-- | :-- |
| `[General]` | General information about the beatmap | `key: value` pairs |
| `[Editor]` | Saved settings for the beatmap editor | `key: value` pairs |
| `[Metadata]` | [Information](/wiki/Beatmap_Editor/Song_Setup#song-and-map-metadata) used to identify the beatmap | `key:value` pairs |
| `[Difficulty]` | [Difficulty settings](/wiki/Beatmap_Editor/Song_Setup#difficulty) | `key:value` pairs |
| `[Events]` | Beatmap and storyboard graphic events | Comma-separated lists |
| `[TimingPoints]` | Timing and control points | Comma-separated lists |
| `[Colours]` | Combo and skin colours | `key : value` pairs |
| `[HitObjects]` | Hit objects | Comma-separated lists |

## General

| Option | Value type | Description | Default value |
| :-- | :-- | :-- | :-- |
| `AudioFilename` | String | Location of the audio file relative to the current folder |  |
| `AudioLeadIn` | Integer | Milliseconds of silence before the audio starts playing | 0 |
| `AudioHash` | String | *Deprecated* |  |
| `PreviewTime` | Integer | Time in milliseconds when the audio preview should start | -1 |
| `Countdown` | Integer | Speed of the countdown before the first hit object (`0` = no countdown, `1` = normal, `2` = half, `3` = double) | 1 |
| `SampleSet` | String | Sample set that will be used if timing points do not override it (`Normal`, `Soft`, `Drum`) | Normal |
| `StackLeniency` | Decimal | Multiplier for the threshold in time where hit objects placed close together stack (0–1) | 0.7 |
| `Mode` | Integer | Game mode (`0` = osu!, `1` = osu!taiko, `2` = osu!catch, `3` = osu!mania) | 0 |
| `LetterboxInBreaks` | 0 or 1 | Whether or not breaks have a letterboxing effect | 0 |
| `StoryFireInFront` | 0 or 1 | *Deprecated* | 1 |
| `UseSkinSprites` | 0 or 1 | Whether or not the storyboard can use the user's skin images | 0 |
| `AlwaysShowPlayfield` | 0 or 1 | *Deprecated* | 0 |
| `OverlayPosition` | String | Draw order of hit circle overlays compared to hit numbers (`NoChange` = use skin setting, `Below` = draw overlays under numbers, `Above` = draw overlays on top of numbers) | NoChange |
| `SkinPreference` | String | Preferred skin to use during gameplay |  |
| `EpilepsyWarning` | 0 or 1 | Whether or not a warning about flashing colours should be shown at the beginning of the map | 0 |
| `CountdownOffset` | Integer | Time in beats that the countdown starts before the first hit object | 0 |
| `SpecialStyle` | 0 or 1 | Whether or not the "N+1" style key layout is used for osu!mania | 0 |
| `WidescreenStoryboard` | 0 or 1 | Whether or not the storyboard allows widescreen viewing | 0 |
| `SamplesMatchPlaybackRate` | 0 or 1 | Whether or not sound samples will change rate when playing with speed-changing mods | 0 |

## Metadata

| Option | Value type | Description |
| :-- | :-- | :-- |
| `Title` | String | Romanised song title |
| `TitleUnicode` | String | Song title |
| `Artist` | String | Romanised song artist |
| `ArtistUnicode` | String | Song artist |
| `Creator` | String | Beatmap creator |
| `Version` | String | Difficulty name |
| `Source` | String | Original media the song was produced for |
| `Tags` | Space-separated list of strings | Search terms |
| `BeatmapID` | Integer | Difficulty ID |
| `BeatmapSetID` | Integer | Beatmap ID |

## Difficulty

| Option | Value type | Description |
| :-- | :-- | :-- |
| `HPDrainRate` | Decimal | HP setting (0–10) |
| `CircleSize` | Decimal | CS setting (0–10) |
| `OverallDifficulty` | Decimal | OD setting (0–10) |
| `ApproachRate` | Decimal | AR setting (0–10) |
| `SliderMultiplier` | Decimal | Base slider velocity in hundreds of [osu! pixels](/wiki/osupixel) per beat |
| `SliderTickRate` | Decimal | Amount of slider ticks per beat |

## Events

*Event syntax:* `eventType,startTime,eventParams`

- **`eventType` (String or Integer):** Type of the event. Some events may be referred to by either a name or a number.
- **`startTime` (Integer):** Start time of the event, in milliseconds from the beginning of the beatmap's audio. For events that do not use a start time, the default is `0`.
- **`eventParams` (Comma-separated list):** Extra parameters specific to the event's type.

### Backgrounds

*Background syntax:* `0,0,filename,xOffset,yOffset`

- **`filename` (String):** Location of the background image relative to the beatmap directory. Double quotes are usually included surrounding the filename, but they are not required.
- **`xOffset` (Integer)** and **`yOffset` (Integer):** Offset in [osu! pixels](/wiki/osupixel) from the centre of the screen. For example, an offset of `50,100` would have the background shown 50 osu! pixels to the right and 100 osu! pixels down from the centre of the screen. If the offset is `0,0`, writing it is optional.

## Timing points

Each timing point influences a specified portion of the map, commonly called a "timing section". The `.osu` file format requires these to be sorted in chronological order.

*Timing point syntax:* `time,beatLength,meter,sampleSet,sampleIndex,volume,uninherited,effects`

- **`time` (Integer):** Start time of the timing section, in milliseconds from the beginning of the beatmap's audio. The end of the timing section is the next timing point's time (or never, if this is the last timing point).
- **`beatLength` (Decimal):** This property has two meanings:
  - For uninherited timing points, the duration of a beat, in milliseconds.
  - For inherited timing points, a negative inverse slider velocity multiplier, as a percentage. For example, `-50` would make all sliders in this timing section twice as fast as `SliderMultiplier`.
- **`meter` (Integer):** Amount of beats in a measure. Inherited timing points ignore this property.
- **`sampleSet` (Integer):** Default sample set for hit objects (0 = beatmap default, 1 = normal, 2 = soft, 3 = drum).
- **`sampleIndex` (Integer):** Custom sample index for hit objects. `0` indicates osu!'s default hitsounds.
- **`volume` (Integer):** Volume percentage for hit objects.
- **`uninherited` (0 or 1):** Whether or not the timing point is uninherited.
- **`effects` (Integer):** Bit flags that give the timing point extra effects. See [the effects section](#effects).

### Examples

```
10000,333.33,4,0,0,100,1,1
12000,-25,4,3,0,100,0,1
```

The first timing point at 10 seconds is uninherited, and sets:

- BPM to 180 (`1 / 333.33 * 1000 * 60`)
- Time signature to 4/4
- Sample set to the beatmap default
- Sample index to osu!'s default hitsounds
- Volume to 100%
- Kiai time

The second timing point at 12 seconds is inherited, changing the slider velocity to 4x and the sample set to drum.

## Hit objects

*Hit object syntax:* `x,y,time,type,hitSound,objectParams,hitSample`

- **`x` (Integer)** and **`y` (Integer):** Position in [osu! pixels](/wiki/osupixel) of the object.
- **`time` (Integer):** Time when the object is to be hit, in milliseconds from the beginning of the beatmap's audio.
- **`type` (Integer):** Bit flags indicating the type of the object. See [the type section](#type).
- **`hitSound` (Integer):** Bit flags indicating the hitsound applied to the object. See [the hitsounds section](#hitsounds).
- **`objectParams` (Comma-separated list):** Extra parameters specific to the object's type.
- **`hitSample` (Colon-separated list):** Information about which samples are played when the object is hit. It is closely related to `hitSound`; see [the hitsounds section](#hitsounds). If it is not written, it defaults to `0:0:0:0:`.

### Type

Hit object types are stored in an 8-bit integer where each bit is a flag with special meaning. The base hit object type is given by bits 0, 1, 3, and 7 (from least to most significant):

- 0: Hit circle
- 1: Slider
- 3: Spinner
- 7: osu!mania hold

The remaining bits are used for distinguishing new combos and optionally skipping combo colours (commonly called "colour hax"):

- 2: New combo
- 4–6: A 3-bit integer specifying how many combo colours to skip, if this object starts a new combo.

### Holds (osu!mania only)

*Hold syntax:* `x,y,time,type,hitSound,endTime:hitSample`

- **`endTime` (Integer):** End time of the hold, in milliseconds from the beginning of the beatmap's audio.
- `x` determines the index of the column that the hold will be in. It is computed by `floor(x * columnCount / 512)` and clamped between `0` and `columnCount - 1`.
- `y` does not affect holds. It defaults to the centre of the playfield, `192`.