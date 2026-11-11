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

## License

Different sources state different licenses. See
[`LICENSE-NOTES.md`](LICENSE-NOTES.md).
