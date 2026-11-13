#ifndef savegame_h
#define savegame_h

#include <zlib.h>
#include "bool.h"

/*----------------------------------------------------------------------*/
/* Saving and restoring games                                           */
/*                                                                      */
/* Saves are gzip-compressed files NAME.craftsave in                    */
/* $XDG_DATA_HOME/craft/saves (default ~/.local/share/craft/saves).     */
/*                                                                      */
/* Restoring happens in two steps around the normal start-up:           */
/*   restore_game_globals()  before init_craft(): options and landscape */
/*   init_craft()            creates players and windows (no new land)  */
/*   restore_game_objects()  objects, ships, players, computer players  */
/*----------------------------------------------------------------------*/

#define max_saves 20

/* Set by the start menu when the player picked a game to restore. */
extern bool restoring;
extern char restore_name[128];

/* Directory holding the saves (created if needed). */
const char *saves_dir();

/* mkdir -p */
void make_dirs(const char path[]);

/* Save the running game. Returns false and fills error on failure. */
bool save_game(const char save_name[], char error[256]);

/* List saves, newest first: names to pass to restore, and labels with
   the date for showing. Returns the number found (at most max). */
int list_saves(char names[][128], char labels[][160], int max);

/* Restore, step 1 and step 2 (see above). On failure they print the
   reason and return false. */
bool restore_game_globals(const char save_name[]);
bool restore_game_objects();

/* For replays: the same snapshot written into, or read from, a file the
   caller has open, which stays open for what follows the snapshot. */
bool save_game_into(gzFile f);
bool restore_game_globals_from(gzFile f); /* reads past the snapshot's options */
bool restore_game_objects_from(gzFile f); /* the rest; f stays open */

/* Ask for a name and save; reports the result in a dialog. Used by the
   in-game Save button and when quitting. */
void save_game_dialog();

/* Let the player pick a saved game. Returns false if there are none or
   the player cancels. */
bool choose_saved_game(char save_name[128]);

#endif
