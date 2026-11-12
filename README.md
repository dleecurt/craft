# CRAFT: The Vicious Vikings

A real-time strategy game for X11, in the spirit of Warcraft, Dune II and
Command & Conquer. Written by Uwe Beyer ("hua") and team at GMD (the German
National Research Center for Information Technology). The last original
release was version 3.5 in 1997.

> You are a Viking, and, one fine day, after drinking a bit too much beer,
> you fall out with the leader of your settlement ...

You and two drunken companions found a new nation, and they elect you
king. Build farms, mines, camps and docks, feed up to 70 citizens, train
knights, archers, catapults and ships, trade wood and gold, and conquer
the computer players' islands. The full original manual is in
[`doc/craft.html`](doc/craft.html).

This fork brings the 1990s source back to life on modern Linux. The code
is now plain C++ with an Autotools build, and the game can be installed
system-wide.

## Building

You need a C++ compiler, the Xlib and zlib development files, autoconf and
automake.
On Debian or Ubuntu:

    sudo apt install build-essential autoconf automake libx11-dev zlib1g-dev

Then:

    ./autogen.sh
    ./configure
    make
    sudo make install     # optional

Useful `configure` options:

| Option | Effect |
|---|---|
| `--prefix=DIR` | install under `DIR` (default `/usr/local`) |
| `--disable-tools` | build only the game, not the `craft-*` art tools |
| `--disable-warnings` | drop the default `-Wall -Wextra` (the code builds warning-free with gcc and clang) |

## Running

    craft

Pick the options in the start menu and press **Play**. You can also run
`./craft` straight from the build folder without installing.

Craft needs its data files: `hcraft/` (pictures), `buttons/` (icons) and
`.windefaults.params` (colours and window settings). It looks for them in
this order:

1. the directory named by the `CRAFT_DATADIR` environment variable
2. the current directory, if it contains the data (a source checkout)
3. the install location, `PREFIX/share/craft`

Craft keeps an off-screen copy of every window and repaints from it, so
windows redraw correctly after being covered or minimised, with or
without X backing store. (The original relied on backing store, which
most X servers now leave off.)

## Splash and credits screens

Craft opens with a splash screen and closes with a credits screen. Both
go away on a mouse click or key press, or by themselves after a few
seconds. The splash shows the picture `hcraft/splash.cmap` if it is
installed; without it, the splash shows only the title.

The splash picture is the public-domain (CC0) "Viking warrior" from
<https://freesvg.org/viking-warrior> (OpenClipart). To make or replace
it from a PNG (ImageMagick and the bundled art tools):

    convert viking-warrior.png -resize 300x300 -background '#cccccc' \
            -flatten -compress none splash.ppm
    craft-ppm2cmap splash.ppm hcraft/splash.cmap

`craft-ppm2cmap` needs a running X display, and the PPM must be the
plain (ASCII, `P3`) kind, which `-compress none` produces.

## Playing

The start menu sets up the world (with or without a king, local sight,
islands, and a seed to repeat a landscape) and the four players. Each
player button cycles between **Human**, **Computer** and inactive
(`- - -`); since version 3.5 more than one computer player can take part.
The full rules are in [`doc/craft.html`](doc/craft.html).

### Saving and restoring

Press **save** in the status panel (above **pause**) to save the running
game. Give it a name (the date and time is suggested) and press Return.
When you quit, the game asks whether to save first.

To continue a saved game, press **restore** under **Play** in the start
menu and pick it from the list (newest first).

Saves are kept in `~/.local/share/craft/saves/` (or
`$XDG_DATA_HOME/craft/saves/`) as `NAME.craftsave` files. They hold the
whole game: the landscape, every unit and building, all players and the
computer players' plans, so a restored game carries on exactly where it
was saved. Games with several human players can be saved too; restoring
one reopens the windows on the same X displays. A save only loads in the
same version of Craft that wrote it (the game says so if it doesn't).

### Pausing

To pause, press **pause** next to **quit** in the status panel (or the
button with your name in the pause panel, as in the original). The game
stops for all players until whoever paused presses it again. Quit is not
available while the game is paused.

New in 3.5, from the author's release notes:

- several computer players in one game;
- timer-driven scheduling, which uses less CPU and shares time more
  fairly between players;
- more plausible battles between ships;
- **talking**: type a message into the small text bar at the bottom left
  of the control panel, then give one of your units the *talk* command
  to show it. It only appears if nothing is in the way at the unit's
  upper right.

The author also noted that version 3.5 fixed many bugs from 3.05,
including several segmentation faults.

## Art tools

The original developers' tools for making the game's pictures. They work
on Craft's own `.cmap` picture format (with `.xbm` masks), usually on a
numbered series `NAME.N.cmap`.

| Program | Usage | Purpose |
|---|---|---|
| `craft-cmedit` | `craft-cmedit [NAME]` | interactive picture editor |
| `craft-ppm2cmap` | `craft-ppm2cmap PPM CMAP` | convert a PPM image to a cmap |
| `craft-compress` | `craft-compress -c\|-x FILE` | Craft's own file compression |
| `craft-mirror` | `craft-mirror NAME FROM TO` | make the turned and mirrored direction frames of pictures FROM..TO |
| `craft-split` | `craft-split ORIG DEST FROM TO INCR DX DY` | cut pictures into tiles |
| `craft-recolor` | `craft-recolor NAME LIST COLOR` | make player-colour versions of the pictures listed in LIST |
| `craft-shade` | `craft-shade NAME N AMOUNT DD` | make 15 edge-shaded variants of tile N |
| `craft-shade1` | `craft-shade1 NAME N DD` | older variant of `craft-shade` |
| `craft-medit` | `craft-medit REGIONS ORIG DEST` | morph editor: define matching points and triangles |
| `craft-morph` | `craft-morph ORIG DEST BACKGROUND REGIONS STEPS` | render a morph between two pictures |

`craft-compress` and `craft-morph` call the netpbm programs `pcxtoppm`,
`ppmtopcx` and `ppmquant` (`sudo apt install netpbm`).

## Source layout

- `*.cc`, `*.h`: the game and its window/picture library. `craft.cc` is
  the game's `main`; the tools' `main`s are `cmedit.cc`, `hcomp.cc`,
  `hmedit.cc`, `hmorph.cc`, `mirror.cc`, `ppm2cmap.cc`, `recolor.cc`,
  `shade.cc`, `shade1.cc` and `split.cc`.
- `hcraft/`, `buttons/`, `.windefaults.params`: game data.
- `doc/`: the original web pages (`craft.html` is the manual,
  `craftcc.html` the old build notes) and the original `README`.

The code was originally written in `.hc` files with "refinements": named
blocks defined after a function body and expanded by a preprocessor
called `hcc`. It has been expanded once into ordinary C++. Comments like
`{ // perform_enter` mark where each refinement used to be.

## History

- **1990s**: written by Uwe Beyer at GMD (borneo.gmd.de). Version 3.5,
  27 October 1997, distributed as `craftcc35.tar.Z`.
- **1999 to 2009**: packaged in Debian (until Etch) and in SuSE.
- **Quipyowert2/craft**: imported the 3.5 source to GitHub and fixed many
  modern-compiler problems.
- **dleecurt/craft** (this fork): build and crash fixes for current gcc,
  conversion to plain C++, Autotools build, `make install`, cleanup.

More: [libregamewiki](https://libregamewiki.org/CRAFT:_The_Vicious_Vikings),
[SourceForge](https://sourceforge.net/projects/craft-vikings/).

## Differences from the Standard Version

This fork is a modified version of CRAFT 3.5 (`craftcc35.tar.Z`). Every
changed file starts with a notice saying how and when it changed, and
the full history is in git. In summary:

- the source is plain C++ instead of `.hc` files for the `hcc`
  preprocessor; the `hcc` tool and the `install` script are gone
- it builds with Autoconf/Automake and installs with `make install`;
  data is found through `CRAFT_DATADIR` or the install location instead
  of only the current directory
- fixes for current compilers, all compiler warnings, and a number of
  bugs (crashes, uninitialised data, buffer overflows); see the git log
- the art tools are installed as `craft-cmedit`, `craft-mirror` and so
  on, so they don't clash with standard programs such as `split`
- test programs, unused modules, editor backups and two third-party
  images were removed; the original web pages are in `doc/`

The unmodified Standard Version is the first import in this
repository's history (commit `593e4f6`), originally from
`ibiblio.org/pub/linux/games/strategy/craftcc35.tar.Z`.

## License

CRAFT is under the **Artistic License** (version 1.0), as stated by its
author on the 3.5 source page. See [`LICENSE`](LICENSE), and
[`LICENSE-NOTES.md`](LICENSE-NOTES.md) for the evidence and the older,
conflicting statements.
