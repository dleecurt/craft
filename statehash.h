/*
 * Whole-game state hash, used to check that two runs of the game (a
 * replay and its recording, or two computers playing in lockstep)
 * agree on everything that affects play.
 */

#ifndef statehash_h
#define statehash_h

#include <stdint.h>
#include <stdio.h>

/* 64-bit FNV-1a hash over the game state: units and their orders,
   ships, the map, each player's economy, the random number generator
   and the step counter.  Display-only state (selections, cursors,
   windows) and pathfinding scratch space are left out, so two players
   looking at different parts of the map still get the same hash. */
uint64_t game_state_hash();

/* The same, also writing a readable breakdown to out (for finding where
   two runs part ways). */
uint64_t game_state_hash_dump(FILE *out);

#endif
