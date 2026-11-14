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

You need a C++ compiler, the Xlib, XRender and zlib development files,
autoconf and automake.
On Debian or Ubuntu:

    sudo apt install build-essential autoconf automake libx11-dev libxrender-dev zlib1g-dev

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

### Zoom

The game screen is 1016×740 pixels, which is small on today's screens.
Craft can enlarge every window by 1.25×, 1.5×, 2× or 3×. Whole-number
steps keep the pixels sharp; the others are smoothed. Nothing else
changes: the game draws at its original size and the X server (XRender)
enlarges the picture.

By default the zoom is **auto**: the largest step that fits the screen
(1.25× on 1920×1080, 1.5× on 2560×1440, 2× on 4K). To change it, use
the **-** and **+** buttons next to *zoom* on the start menu's
**Details** page. The menu resizes at once, and your choice is kept in
`~/.config/craft/settings` (or `$XDG_CONFIG_HOME/craft/settings`). Going
below 1× or above 3× returns to auto.

For one run only, use `craft -scale N` with any value from 1 to 4, for
example `craft -scale 1` for the original size.

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

## Sound and music

Craft plays sound effects and music through PipeWire, PulseAudio or
ALSA. The audio library (miniaudio, with stb_vorbis for Ogg) is
bundled in `third_party/`, so nothing extra needs installing. Without a
sound device the game runs silently.

**Effects** are heard where they happen: loudest on your screen,
fainter further away, and panned left or right. Nothing is heard from
places hidden in the fog, so sound gives nothing away. You hear felling,
mining, digging and building; swords, bows, catapults and explosions;
deaths and falling buildings; and, for your own side, a horn when a man
is trained or a building is finished, a war horn when your men are
attacked, and a knock when you give an order.

The effects are original sounds made by `sfxgen` (`sfxgen.cc`), one
recipe per effect; `make sounds` rebuilds `sounds/*.wav`. Any of them
can be replaced by another WAV file of the same name.

**Music** goes in `music/` (installed with the game) or in
`~/.local/share/craft/music/` (your own; used first). Ogg Vorbis, MP3,
FLAC and WAV all work:

| File name | When it plays |
|---|---|
| `menu.*` | splash, start menu and credits, looping |
| `win.*` / `lose.*` | once, when a game is won or lost |
| anything else | shuffled one after another during a game |

**Controls**: music and effects volume on the start menu's **Details**
page (kept in `~/.config/craft/settings`), and **Shift+M** to mute or
unmute during a game (plain **m** is Move). `CRAFT_SOUND=off` turns
sound off completely.

For testing, `CRAFT_SOUND_DEBUG=1` logs every effect the game asks for
and whether it was played, and `CRAFT_SOUND_RECORD=file.wav` writes
what would have been heard to a file instead of the speakers.

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

### Game pace

The game runs at 40 steps per second by default, about as fast as the
original ran on 1997 hardware. Change it in the start menu's **Details**
page with the **-** and **+** buttons next to *pace*, or during play by
pressing **+** (or **=**) and **-** with the mouse over the map. The
current pace is shown above the **save** button. Choices: 10, 20, 30,
40, 50, 60, 80 and 100 steps per second.

(The **speed** slider on the Details page is different: it changes how
many steps each unit needs per action.)

The pace stays the same all game, however many units there are. A step
takes well under a millisecond on a current machine. To check it, run
`CRAFT_PROFILE=1 craft`: every 200 steps it prints how long the steps
took, how many objects are alive, and the length of the X event queue
(which should stay at a few events). `CRAFT_PROFILE=fast` also drops
the pause between steps, to fast-forward a game for testing.

### Network play

Up to four people play one game, each on their own computer, on the
same local network. One computer hosts; the others join through SSH.
Every computer runs the whole game in lockstep: each sends only its own
player's orders, the host passes everyone's orders around, and every 40
steps the guests check that their game still matches the host's. Both
computers need the same build of Craft.

**Setting up the host (once).** Guests reach the host's game through
SSH, all with one restricted account, `craft`, that can open the game's
port on the host and nothing else: no shell, no files. As root:

    craft-host-setup apply                 # the account and sshd settings
    craft-host-setup add-key guest.pub     # once for each guest's public key
    craft-host-setup check                 # what is in place

(`craft-host-setup print` shows the settings to apply by hand
instead.) Guests need an SSH key (`ssh-keygen -t ed25519`) and to send
the host their `~/.ssh/id_ed25519.pub`.

**Hosting.** On the start menu press **host** (a notice explains that
the host controls the game, and that the game ends for everyone if the
host leaves), set up the map as usual, and press **Play**. The lobby
shows four seats: yours, and three that are **Open** (for a guest),
**Computer** (at most one) or **Closed**; **change** cycles them. It also
shows the join code. When every Open seat has a guest, press **play**.
`craft --host` starts with **host** already pressed.

**Joining.** On the start menu press **join**, enter `craft@HOSTNAME`,
the join code and your name, and press **join**. Craft starts SSH in
the background (if your key has a passphrase, SSH asks for it in the
terminal Craft was started from) and shows the lobby until the host
starts the game. Or from the command line:

    craft --join craft@HOSTNAME --code FJORD-4821 --name Lee

A plain `--join HOSTNAME` (no `user@`) connects without SSH, which only
works on the host itself or through a tunnel you made
(`ssh -N -L 47800:localhost:47800 user@host`).

**During play** only the host can pause or change the pace; guests are
told. Orders take effect 4 steps (a tenth of a second) after they are
given, on every computer at once; if the host often has to wait for a
guest's orders, the network is slower than that and the delay grows,
up to 12 steps.

If a guest's game stops answering, everyone waits, with "Waiting for
NAME" and a countdown; after 15 seconds that guest is dropped and their
side falls. A guest who quits leaves at once, even while the game is
waiting, and their side falls. If the host quits, or the host's
computer cannot be reached for about 15 seconds, the game ends for
everyone. If the host's own side falls, the host watches the rest of
the game.

Every 40 steps the guests check that their game still matches the
host's. A guest whose game does not is dropped ("out of sync"), and
keeps its recording as `out-of-sync.craftreplay` next to
`last.craftreplay`: please report such a game with that file and the
host's `last.craftreplay`.

### Messages

When the game turns an order down it says why in the strip above the
talk field at the bottom left: *Need gold*, *Need wood*, *Too many men*
(the 70-man limit, which also stops new buildings), *Map is full* (every
one of the game's 400 object slots is in use) or *No room for it* (a
building has finished training a unit but there is no free square
within reach; the unit comes out as soon as there is).

### Replays

Every game is recorded to `~/.local/share/craft/replays/last.craftreplay`
(the next game replaces it; `CRAFT_REPLAY=off` turns recording off, and
`craft --record FILE` records to a file of your choice). A recording
holds the game as it was at the start plus every order given, so it is
small, a few dozen kilobytes.

`craft --replay FILE` plays a recording back. Every 200 steps it checks
that the game is in exactly the state it was in when recorded, and at
the end it prints whether the whole game *matched* and a hash of the
final state. Add `CRAFT_PROFILE=fast` to run through it quickly:

    CRAFT_PROFILE=fast craft --replay ~/.local/share/craft/replays/last.craftreplay

`--no-view` plays a recording back without opening any window (no X
display needed), as fast as the computer can; `make check` does this
with the recordings in `tests/`.

Network play (in the works) depends on this: every computer runs the
whole game from the same orders, so the game must play out exactly the
same way everywhere. If a replay ever diverges, run it again with
`CRAFT_REPLAY_DUMP=/some/dir` set (both when recording and when
playing back) and compare the `rec-*` and `play-*` files there with
`diff` to see which unit went its own way.

If Craft crashes, or freezes (one step of the game taking more than 10
seconds), it says so, prints where in the code it was, and keeps the
recording up to that point; `--replay` then plays the game to there and
on past it, to see whether it happens again. A recording that simply
stops (the game was killed) plays on past its end the same way. Please
send both when you report a crash.

Ctrl-C (or `kill`) ends a game cleanly, so the recording is complete.
Pressing it twice quits at once.

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
- `hcraft/`, `buttons/`, `sounds/`, `music/`, `.windefaults.params`:
  game data. `sfxgen.cc` makes the sounds.
- `third_party/`: the bundled audio libraries (miniaudio, stb_vorbis),
  built by `audio_lib.c`.
- `doc/`: the original web pages (`craft.html` is the manual,
  `craftcc.html` the old build notes) and the original `README`.

The code was originally written in `.hc` files with "refinements": named
blocks defined after a function body and expanded by a preprocessor
called `hcc`. It has been expanded once into ordinary C++. Comments like
`{ // perform_enter` mark where each refinement used to be.

## History

- **1990s**: written by Uwe Beyer at GMD. Version 3.5,
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
- windows can be zoomed for large screens (XRender), and repaint
  themselves after being covered
- new: splash and credits screens, a pause button, saving and
  restoring games, an adjustable game pace, and sound effects and music
  (the original only had an unused `/dev/audio` stub)
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
