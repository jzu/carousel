# carousel

_Yet Another Carousel Software_

carousel presents pictures one at a time at a fixed interval, like many other such slideshow applications. There's a special feature, however: you can switch to different series of pictures, which won't automatically change to the next one, allowing you to manually browse these series.

The software was a written for a live show my friend and I put together. He's [Richard Bellia](https://richardbellia.com/), a photographer of all things rock'n'roll, and I'm a guitarist. Occasionally the organiser of his photo exhibitions will be willing to include the show. He'll talk for 5 minutes, then I'll play a part, rinse &amp; repeat. During the show, a projector displays his photographs on a screen behind us at a 6-second interval. But for a particular subject, like his late friend [Crazy White Sean](https://www.crazywhitesean.biz), he'll want related pictures he took of him. Sometimes it's possible, sometimes it's not. With this application, using a laptop connected via wifi to a Rapsberry Pi controlling the HDMI projector, he'll be able to control the streams of pictures from the comfort of his own reading desk.

## Usage

`./carousel [delay]`

At launch, the first picture is displayed in full screen. Hitting "0" on the keyboard, either local or remote, starts rolling, displaying pictures contained in subdirectory `./0/`. Then, hitting "1" switches to bank 1 (pictures in `./1/`), and you can browse through images by using the left and right arrows; same for banks 2, 3, etc. Up to 9 subdirectories are available. After the last picture, the first one is used, and vice versa. If no pictures are to be found there, the key is inactive. Switching back to the main series of pictures by hitting "0" resumes the automation. PgUp increments the picture index by 50 by default (with rollover), PgDn decrements (stops at zero), e.g. in the case of resuming at about the middle of the set after an interruption. Spacebar pauses/unpauses the main series. Hitting `Esc`&nbsp;`q` exits.

A simple numerical argument can be provided, which is the delay between pictures for the main series. It overrides the default duration (6 seconds).

An optional file containing lists of filenames, called `pictures.lst`, can be used to specify the order in which the photographs will be displayed.

Image files can be "normalized", i.e. set to the same horizontal size and correct orientation, prior to running the show. Original files are copied to `/tmp/0/`, `/tmp/1/`, etc. The `normalize.sh` script is then run from the destination directory, where corresponding subdirectories `0/`, `1/`, etc. are created, containing the files processed using ImageMagick's `convert` tool with options ensuring the highest possible fidelity. Corrupted files are detected in the process.

## Filename List

carousel attempts to read a file named `pictures.lst` either in the current directory, or in `~/.carousel`, or in `~/.config/carousel`, in that order. This file simply contains a list of filenames which are present in the series subdirectories, allowing to organize their contents at will. These directories are suffixed with a colon (:), just like the output of the command `ls -1 0 1 2 3 ` (that's dash-one). Filenames beginning with a dot (.) or a space (&nbsp;) are ignored. If a picture is missing, a black screen will be drawn instead. If the file list is missing in all locations, the directories will be read in the order given by `readdir(2)`. Almost no check is done apart from stripping whitespaces and the like, and non compliant contents will wreak havoc. Please act responsibly.

Simple example of `pictures.lst`:

```
0:
TheFirstPicture.jpg
ASecondPicture.jpg
AnotherPicture.jpg

1:
AlternateSeries1.jpg
AlternateSeries2.jpg
```

The corresponding tree structure could look like this, using `ls -1 ./?/*`. 

```
./0/AnotherPicture.jpg
./0/ASecondPicture.jpg
./0/TheFirstPicture.jpg
./1/AlternateSeries1.jpg
./1/AlternateSeries2.jpg
./1/AlternateSeries3.jpg
```

You see that the order is different in `./0/`, and `AlternateSeries3.jpg` is present in `./1/` but won't be used. The order in which pictures appear without a filename list would be different still, due to the sequential way the `readdir` syscall scrolls through the directory itself.

## Client Software

The laptop connects to a wifi access point on the Raspberry Pi, and runs a small program transmitting keystrokes to the carousel application running on the Raspberry where they are interpreted as SDL2 events. The current picture is also displayed in the background (the root window), thanks to a simple shell script which checks whether a new one is active and downloads it from an Apache server running on the Raspberry. You don't want a Gnome environment for this, because it hijacks the background image, so you want a simple window manager (I use Sawfish but any run-of-the-mill WM will do). Just to be totally secure, another script pings the Raspberry and attempts to reconnect should the connection be cut for whatever reason. The `remote.sh` wrapper script is launched at startup. Logs are written in `/var/tmp/` mainly for wifi debugging purposes.

## Vaguely Technical Stuff

This software has been written in C with SDL2 libs (`libsdl2-2.0-0`) on GNU/Linux. I suppose it could work more or less as is on other platforms. You will need the developement packages (`libsdl2-dev` on Debian). Compiling is simply a matter of

`gcc carousel.c -g -o carousel -lSDL2 -lSDL2_image -lpthread -Wall`

The full screen mode is `SDL_WINDOW_FULLSCREEN`, which seems to only activate the main screen, i.e. the standard screen on a laptop. `SDL_WINDOW_FULLSCREEN_DESKTOP` doesn't seem to behave better. I used to fake the full screen with a screen-sized window without borders, but it didn't work as intended on multiscreen setups among other shenanigans.

Speaking of shenanigans, have a look at [this thread on the Raspberry Pi forums](https://forums.raspberrypi.com/viewtopic.php?p=2266620). The Pi Zero refused to connect to the PI 5 as a wifi access point, and so would the laptop to the Pi 5's wifi USB antenna. I don't know the reason, but changing a capital letter to lowercase in the SSID solved the issue.
