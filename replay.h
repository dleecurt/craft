/*
 * Replays: every game records the orders players give, step by step,
 * so the game can be played back exactly.  Playing a replay back and
 * comparing state hashes is how we check that the game is deterministic,
 * which network play (lockstep) depends on.
 *
 * File: gzip, "CRAFTRPL" + version, a full snapshot of the game at the
 * first step (the same format as a saved game), then records:
 *   ordr  step player n  ids[n] cmds[n] xs[n] ys[n]
 *   hash  step hash                   (every replay_hash_every steps)
 *   end   step hash                   (the game ended here)
 */

#ifndef replay_h
#define replay_h

#include <stdint.h>

#include "bool.h"

#define replay_hash_every 200

/* Set by main() before the game starts. */
extern bool replay_playing;

/* Default recording file, ~/.local/share/craft/replays/last.craftreplay
   (or under $XDG_DATA_HOME). */
const char *replay_default_path();

/* Recording.  Start after the game is set up, before the first step. */
bool replay_record_start(const char path[]);
void replay_record_orders(int step, int player, int n, const int id[], const int cmd[],
                          const int x[], const int y[]);
void replay_record_close(); /* safe to call more than once */

/* The file being recorded to ("" if none). */
const char *replay_recording_path();

/* The game crashed: save what was recorded (no final hash). */
void replay_record_crashed();

/* Playback.  replay_open reads the header and restores the snapshot's
   globals; call replay_open_objects after init_craft() to restore the
   units and players. */
bool replay_open(const char path[]);
bool replay_open_objects();

/* The orders recorded for this player at this step (num 0 if none).
   Ends the program when the recording ends. */
void replay_take(int step, int player, int &num, int cmd[], int id[], int x[], int y[]);

/* The game ended during playback: report and end the program. */
void replay_playback_end(int step);

/* Call after each step (ticker already advanced): records or checks the
   state hash. */
void replay_after_step(int step);

#endif
