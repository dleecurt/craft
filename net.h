/*
 * Network play: lockstep over a byte stream.
 *
 * Every computer plays the whole game.  Each sends only its own
 * player's orders, to the host; the host collects every player's orders
 * for a step and sends them all out together (STEP).  No computer runs
 * a step before it holds that step's STEP, so all of them run the same
 * steps with the same orders and stay in the same state.  Orders given
 * at step n run at step n + net_delay.
 *
 * Every net_check_every steps each guest sends a hash of its game state
 * (CHECK); the host's game counts as correct, and a guest whose hash
 * differs is dropped.
 *
 * The host listens on 127.0.0.1 only.  Guests reach it through SSH
 * (phase 4); until then, on the same computer, they connect directly.
 */

#ifndef net_h
#define net_h

#include <stdint.h>
#include <zlib.h>

#include "bool.h"
#include "craft_def.h"

#define net_default_port 47800
#define net_check_every 40

enum
{
   net_off,
   net_host,
   net_guest
};

extern int net_role;  /* net_off, net_host or net_guest */
extern int net_delay; /* input delay in steps */
extern int net_seat;  /* this computer's player number */

/* One player's orders for one step. */
struct net_orders
{
   int n;
   int cmd[max_orders];
   int id[max_orders];
   int x[max_orders];
   int y[max_orders];
};

/*---- host ------------------------------------------------------------*/

/* Start listening for guests (on 127.0.0.1 only) and make a join code. */
bool net_host_listen(int port);
const char *net_join_code();

/* The lobby: take in guests who arrive, turn away the ones that do not
   fit.  Returns how many guests are waiting (at most max_guests), with
   their names in the order they came. */
int net_lobby_poll(int max_guests, char names[][128]);

/* Tell the waiting guests what the lobby looks like. */
void net_lobby_send(const char lines[][128], int num_lines, const char summary[]);

/* Play: the first num waiting guests take seats seat_of[0..num-1]; any
   others are sent away. */
void net_lobby_seat(const int seat_of[], int num);

/* The host gave up hosting: send everyone away (with why, if not NULL). */
void net_lobby_close(const char why[]);

/* The game is set up: send every guest the starting state. */
bool net_host_start(int first_step);

/* Host only: pause and pace are the host's; guests are told. */
void net_send_pause(bool on);
void net_send_pace(int pace);

/*---- guest -----------------------------------------------------------*/

enum
{
   net_lobby_waiting, /* nothing new */
   net_lobby_changed, /* new seat lines */
   net_lobby_started, /* the game starts: state is ready to read */
   net_lobby_ended    /* turned away or disconnected: why says why */
};

/* Connect to a host: addr "user@host" goes through ssh (ssh -W, or the
   command in $CRAFT_SSH); a plain "host" or "host:port" connects
   directly (same computer, or a tunnel made by hand). */
bool net_guest_connect(const char addr[], int port, const char code[], const char name[]);

/* Wait up to ms for the host; see the enum above. */
int net_guest_lobby_poll(int ms, gzFile &state, char lines[][128], int &num_lines,
                         char summary[256], char why[256]);

/* Leave the lobby. */
void net_guest_cancel();

/* Connect and wait for the game to start, with no screen (--join on the
   command line without the join window).  NULL on failure. */
gzFile net_join(const char addr[], int port, const char code[], const char name[]);

/*---- both, during play -----------------------------------------------*/

/* This computer's orders for a step (net_delay steps ahead). */
void net_submit(int step, const net_orders &mine);

/* Every player's orders for this step, if they have arrived. */
bool net_take_step(int step, net_orders all[max_players]);

/* After running a step: send or compare state hashes. */
void net_after_step(int step);

/* Read and write whatever the network has; wait up to ms for news. */
void net_poll(int ms);

/* Is player p's side played on another computer that has gone?
   (Their side then falls, as if they gave up.) */
bool net_seat_left(int p);

/* Status for the screen: who we are waiting for, how long, why the game
   ended.  NULL if nothing to say. */
const char *net_status();

/* A message for this player (guest: "Paused by Sven"); NULL if none.
   Each message is returned once. */
const char *net_message();

/* The game is over for us (gave up, game over, quitting). */
void net_leave(const char reason[]);

/* True when a guest has lost the host (the game is over). */
bool net_host_gone();

/* True when the host dropped us because our game went out of sync. */
bool net_dropped_out_of_sync();

#endif
