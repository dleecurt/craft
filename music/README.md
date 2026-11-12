# Music

Put the game's music in this folder. Craft plays Ogg Vorbis (`.ogg`),
MP3 (`.mp3`), FLAC (`.flac`) and WAV (`.wav`) files.

| File name | When it plays |
|---|---|
| `menu.*` | On the splash screen, the start menu and the credits, looping |
| `win.*` | Once, when you win a game |
| `lose.*` | Once, when the computer wins |
| anything else | Shuffled, one after another, during a game |

For example: `menu.ogg`, `win.ogg`, `lose.ogg`, `01-longships.ogg`,
`02-the-fjord.ogg`. Any of the files may be left out; the game is simply
quiet at that point.

Players can add or replace music without reinstalling by putting files
in `~/.local/share/craft/music/` (or `$XDG_DATA_HOME/craft/music/`).
Files there are used first, so a `menu.ogg` there replaces this one.

Ogg Vorbis is the best choice: small files, good quality and no licence
questions. Music and effects volumes are on the start menu's Details
page; Shift+M mutes everything during a game.
