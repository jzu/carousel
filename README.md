# carousel

_Yet Another Carousel Software_

carousel allows to present pictures one at a time on a fixed delay, like many other such applications. There's a special feature, however: you can switch to a different series of pictures, which won't automatically change to the next one, allowing you to manually browse this series.

The reason for this software to exist is a show a friend and me have set up together. He's [Richard Bellia](https://richardbellia.com/), a photographer of all things rock'n'roll, and I'm a guitarist. Occasionally, an exhibition with his photographs will be willing to arrange a show. He'll talk for 5 minutes, then I'll play a part, rinse &amp; repeat. During the show, a projector displays his photographs on a screen behind us at a 6-second interval. But for a particular subject, like his late friend [Crazy White Sean](https://www.crazywhitesean.biz), he'll want related pictures he took of him. Sometimes it's possible, sometimes it's not. With this application, using a keyboard or perhaps a Stream Deck, and some [USB/IP](https://usbip.sourceforge.net/) magic between two Rapsberry Pis, he'll be able to control the streams of pictures from the comfort of his own reading desk.

## Usage

`./carousel [number]`

At launch, the first picture is displayed in full screen. Hitting "0" starts rolling, displaying pictures contained in subdirectory `./0/`. Then, hitting "1" switches to bank 1 (pictures in `./1/`), and you can browse through images by using the left and right arrows; same for banks 2, 3, etc. After the last picture, the first one is used, and vice versa. If no pictures are to be found there, the key is inactive. Switching back to the main series of pictures by hitting "0" resumes the automation. Hitting "q" or Escape exits.

A simple numerical argument can be provided, which is the delay between pictures for the main series. It overrides the default duration (6 seconds).

An optional file containing lists of filenames, called `pictures.lst`, can be used to specify the order in which the photographs will be displayed.

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

The corresponding tree structure could look like this, using `ls -1 ./?/*`. You see that the order is different in `./0/`, and `AlternateSeries3.jpg` is present in `./1/` but won't be used. The order in which pictures appear without a filename list would be different still, due to the sequential way the `readdir` syscall scrolls through the directory itself.

```
./0/AnotherPicture.jpg
./0/ASecondPicture.jpg
./0/TheFirstPicture.jpg
./1/AlternateSeries1.jpg
./1/AlternateSeries2.jpg
./1/AlternateSeries3.jpg
```

## Vaguely Technical Stuff

This software has been written in C with SDL2 libs (`libsdl2-2.0-0`) on GNU/Linux. I suppose it could work more or less as is on other platforms. You will need the developement packages (`libsdl2-dev` on Debian). Version 2.0.10 on Ubuntu 20.04 doesn't make the cut because it lacks `SDL_GetTicks64()`, and delay consistency between pictures is flaky to say the least – version 2.30 acts as intended. Compiling is simply a matter of

`gcc carousel.c -g -o carousel -lSDL2 -lSDL2_image`

Up to 6 subdirectories are available by default. In case more would be needed, just change `#define DELAY 6` in the source. 
