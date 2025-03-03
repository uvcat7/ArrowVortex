# DANCE WITH INTENSITY v2.50.00
© SimWolf, 2003
Excerpt from the documentation on the (now defunct) website.

## Steps

DWI uses step-files that are similar to the ".MSD" file format. However, there are new additions and some tags are treated slightly differently, so the extension was changed to avoid confusion. DWI files with these new additions will not work properly in other simulators.

Step-patterns are defined in the same way as .MSD files - use the numeric keypad as a reference for most patterns:

```
  7=U+L     8=U      9=U+R

  4=L                6=R

  1=D+L     2=D      3=D+R

    (U+D = A and L+R = B)
```

A '0' indicates no step. Each character defaults to one 1/8 of a beat. Surround a series of characters with the following brackets to change the rate at which the steps come:

```
   (...)  = 1/16 steps
   [...]  = 1/24 steps
   {...}  = 1/64 steps
   `...'  = 1/192 steps
```

6-panel (Solo) mode uses additional characters:

```
   -\---- = C
   ----/- = D

   L\---- = E
   -\D--- = F
   -\-U-- = G
   -\---R = H

   L---/- = I
   --D-/- = J
   ---U/- = K
   ----/R = L

   -\--/- = M
```

To do more than 2 panels at a time, you can join codes together with the "<..>" object, and they will all count as the same beat. So, to do a jump that involves Left, Right, Up-Left, and Up-Right, you could do:

```
   -\--/- = M
   L----R = B
   ======
   L\--/R = <MB>  (or <LE>, <IH>, <46M>, etc.)
```

MSD files from other simulators will work with DWI, with a change in the 'GAP' value being the only change usually necessary. DWI calculates the 'GAP' value differently than other simulators that use the MSD format.

DWI does not support the BMS file format. There is a utility available that can convert any BMS file into DWI format. Each song only requires one DWI file for all of its steps, so if you are converting BMS files please remember that all the different difficulties of step patterns will be contained in the same DWI file.

## Hold Arrows

In the DWI file format a hold arrow is signified with the ! symbol. The string 8!8 will begin an 'up' hold arrow, and the arrow will be released the next time the program encounters an 'up' arrow: by itself or combined with another arrow (7, 8, 9, A, etc.) The characters 7!4 would show both 'up' and 'left' arrows but only the left arrow would be held. The format could best be described as "show!hold".

## Tags

These tags should be in every DWI file:

`#TITLE:...;` title of the song.

`#ARTIST:...;` artist of the song.

`#GAP:...;` number of milliseconds that pass before the program starts counting beats. Used to  sync the steps to the music.

`#BPM:...;` BPM of the music.

Additionally, the following tags can be given:

`#DISPLAYTITLE:...;` provides an alternate version of the song name that can also include special characters.

`#DISPLAYARTIST:...;` provides an alternate version of the artist name that can also include special characters.
 
*Special Characters are denoted by giving filenames in curly-brackets.*
eg. `#DISPLAYTITLE:The {kanji.png} Song;`
 
*The extra character files should be 50 pixels high and be black-and-white. The baseline for the font should be 34 pixels from the top.*
 
`#DISPLAYBPM:...;` tells DWI to display the BPM on the song select screen in a user-defined way. Options can be:

* `'*'`    - BPM cycles randomly
* `'a'`    - BPM stays set at 'a' value (no cycling)
* `'a..b'` - BPM cycles between 'a' and 'b' values|

`#FILE:...;` path to the music file to play (eg. "/music/mysongs/abc.mp3")

*(NB: if the file is not found, a .wav or .mp3 file in the same folder as the DWI file is used)*

`#MD5:...;` an MD5 string for the music file. Helps ensure that same music file is used on all systems.

`#FREEZE:...;` a value of the format "BBB=sss". Indicates that at 'beat' "BBB", the motion of the arrows should stop for "sss" milliseconds. Turn on beat-display in the System menu to help determine what values to use. Multiple freezes can be given by separating them with commas.

`#CHANGEBPM:...;` a value of the format "BBB=nnn". Indicates that at 'beat' "BBB", the speed of the arrows will change to reflect a new BPM of "nnn". Multiple BPM changes can be given by separating them with commas.

`#STATUS:...;` can be `"NEW"` or `"NORMAL"`. Changes the display of songs on the song-select screen.

`#GENRE:...;` a genre to assign to the song if "sort by Genre" is selected in the System Options. Multiple Genres can be given by separating them with commas.

`#CDTITLE:...;` points to a small graphic file (64x40) that will display in the song selection screen in the bottom right of the background, showing which CD the song is from. The colour of the pixel in the upper-left will be made transparent.

`#SAMPLESTART:...;` the time in the music file that the preview music should start at the song-select screen. Can be given in Milliseconds (eg. 5230), Seconds (eg. 5.23), or minutes (eg. 0:05.23). Prefix the number with a "+" to factor in the GAP value.

`#SAMPLELENGTH:...;` how long to play the preview music for at the song-select screen. Can be in milliseconds, seconds, or minutes.

`#RANDSEED:x;` provide a number that will influence what AVIs DWI picks and their order. Will be the same animation each time if AVI filenames and count doesn't change (default is random each time).

`#RANDSTART:x;` tells DWI what beat to start the animations on. Default is 32.

`#RANDFOLDER:...;` tells DWI to look in another folder when choosing AVIs, allowing 'themed' folders.

`#RANDLIST:...;` a list of comma-separated filenames to use in the folder.
Each pattern of steps for different modes have the same basic format:

```
#SINGLE:BASIC:X:...;
 ^      ^     ^ ^
 |      |     | + step patterns.  In doubles, the left pad's steps are given first, 
 |      |     |   then the right pad's, separated by a colon (:).
 |      |     |
 |      |     + difficulty rating.  Should be 1 or higher.
 |      |
 |      + Difficulty.  Can be one of "BASIC", "ANOTHER", "MANIAC", or "SMANIAC"
 |
 + Style.  Can be one of "SINGLE", "DOUBLE", "COUPLE", or "SOLO".  "COUPLE" is 
   Battle-mode steps.
```

Comments can be used by using `"//"`. Everything after this on the same line in the file will be ignored.

## Background Animations, Movies, and Visualizations

DWI allows for background animations using a special script within the step-file. A script consists of static images, animated images, and/or an AVI movie. Using the script, you can create a variety of layered effects. A sample animation is described below:

```
#BACKGROUND:
  M:MOVIE:.\movies\sfx.avi STARTAT:-1.0 LAYER:0;
  V:VIS:.\Vis\somevis.svp LAYER:0;
  E:FILE:.\anim\equalizer.png ANIMATE:10,33 POSITION:-33,0 SPACING:40,40 LAYER:1;
  D:FILE:.\anim\dancer-m1.png ANIMATE:24,66 SIZE:2 MULT:0,0.5,1 SPACING:30,30 LAYER:1;
  X:LAYER:1 OFF;
  SCRIPT:M.......................
         ................E...............
         D...............X,V...............E...............D...............
         X,M...............E...............D...............X,V...............
         E...............D...............X,M...............E...............
         D...............X,V...............E...............D...............
         X,M...............E...............D...............X;
#END;
```

The first part of the "BACKGROUND" definition defines the effects. Each effect is attributed to a letter or number ("a-z", "A-Z", "1-9"). The format for defining an effect is:

The first four tags cannot be used together...

`FILE:` path to a file. Either a still image, or an animation (multiple frames of animation are stacked *vertically* in the image).

`MOVIE:` path to a standard Windows AVI file. Note that the movie won't play unless you have the right codecs installed in Windows. Movies are currently stretched to fill the whole screen.

`VIS:` path to a Sonique Visualization plug-in (SVP). Visualizations are currently stretched to fill the whole screen and are always put on Layer 0, and no other commands will affect it.

`OFF` turns off a layer, effectively making it invisible. Will not turn off layer 0.
 
`LAYER:l` the layer to use (required):
* `0` - base layer. (Image/Movie is **always** tiled and SPACING is ignored).
* `1` - overlay layer
* `2` - overlay layer
* `3` - overlay layer
 
`STARTAT:t` number of seconds into the AVI file to start at. If negative, the movie will wait that many seconds before playing. Can be decimal (eg. 1.3 = 1300ms).

`MULT:r,g,b` Red, Green, and Blue pixels in the image/movie are tinted by the given amounts. This way the same image/movie can be used multiple times across DWI files and have different colours.

`ANIMATE:f,n1,n2,...nF` indicates that the FILE contains multiple frames of animation. "f" is the number of frames of animation. Each following value is the number of milliseconds each frame of animation is displayed. If not enough time-values are given, the last given value is used. Ignored for MOVIE type.

`MOVE:x,y` the image/movie is moved by the given number of pixels every millisecond.

`SPACING:x,y` images are always tiled if they don't fill up the screen. This tag allows you to add some spacing between the images by the given number of pixels horizontally and vertically (Layer > 0)

`SIZE:s` multiplies the image size by 's' in both directions. Must be a whole number.

`KEEPPOS` normally when a new effect is turned on, its position is reset. This tag keeps the layer where it is, useful for keeping images moving smoothly.

`KEEPTIME` (MOVIEs only) normally when a movie starts, it starts from the beginning or the time given in the "STARTAT" tag. Adding this tag will keep it playing so when that layer is enabled again, it will have kept going.

Following the effect definition, comes the actual animation script. This is a sequence of characters in a similar way as the step-patterns are given - each character normally is 1/8 of a beat, though brackets can be used to change the time-values. In this way, background animations can be syched to the steps. It is suggested that the steps for "SINGLE:BASIC" are copied to after the "SCRIPT:" tag, and then the effects be set. In this way one knows that the script will match the same length of the music.

The animations take effect as that point in the song is reached. Multiple effects can occur at the same time if they are separated with a comma. A period (.) means no new effect should take place at that point. A zero (0) turns off all effects and returns the background to the original graphic.