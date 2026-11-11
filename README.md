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

You need a C++ compiler, the Xlib development files, autoconf and automake.
On Debian or Ubuntu:

    sudo apt install build-essential autoconf automake libx11-dev

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

If game windows don't redraw after another window covers them, your X
server has *backing store* turned off: Craft asks X to keep window
contents for it (the original author noted this). With Xorg it can usually be
enabled with `Option "BackingStore" "true"` in the `Device` section of
`xorg.conf`, or by running a compositing
window manager.

## Playing

The start menu sets up the world (with or without a king, local sight,
islands, and a seed to repeat a landscape) and the four players. Each
player button cycles between **Human**, **Computer** and inactive
(`- - -`); since version 3.5 more than one computer player can take part.
The full rules are in [`doc/craft.html`](doc/craft.html).

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
