/*
 * The screens around a network game: the host's notice and lobby, and
 * the guest's join window and lobby.
 */

#ifndef lobby_h
#define lobby_h

#include <zlib.h>

#include "bool.h"

/* Before hosting: what hosting means.  True to go ahead. */
bool host_notice();

/* The host's lobby.  Seats come from active[] and host[] (the start
   menu, or a restored game); with fixed_seats (a restored game) they
   cannot be changed.  Waits for the guests; on Play sets active[],
   host[], name[] and net_seat for the game and seats the guests.  False
   if the host cancels. */
bool host_lobby(int port, bool fixed_seats);

/* The guest's join window and lobby: address, join code and name (the
   given values fill the fields).  Returns the host's game, ready to
   read, or NULL if the guest gives up.  With go, connect at once, as if
   join was pressed. */
gzFile join_window(int port, const char addr[], const char code[], const char name[], bool go);

#endif
