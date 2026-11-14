/*
 * The screens around a network game.  See lobby.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lobby.h"
#include "buttons.h"
#include "craft.h"
#include "getline.h"
#include "masks.h"
#include "net.h"
#include "win.h"
#include "xtimer.h"

#define lobby_dx 460
#define lobby_dy 330

namespace
{

enum seat_kind
{
   seat_host,
   seat_open,
   seat_computer,
   seat_closed
};

const char *kind_name[] = { "host", "Open", "Computer", "Closed" };

/* A window with a frame and a title, in the style of the game's
   dialogs. */
win *open_box(const char title[], int dx, int dy)
{
   win *w = new win("craft_network", "", 120, 80, dx, dy);

   w->set_background(win_default_c("tell_background"));
   w->set_color(win_default_c("tell_background"));
   w->fill(0, 0, dx, dy);
   frame(w);
   w->set_color(black);
   w->write(16, 24, title);
   w->tick();
   return w;
}

void clear_line(win *w, int y, int dx)
{
   w->set_color(win_default_c("tell_background"));
   w->fill(8, y - 14, dx - 16, 18);
}

void say(win *w, int y, int dx, int color, const char text[])
{
   clear_line(w, y, dx);
   w->set_color(color);
   w->write(16, y, text);
}

/* Run one turn of a window's event loop. */
void turn(win *w)
{
   w->mark_mouse();
   w->tick();
}

void turn_done(win *w)
{
   w->scratch_mouse();
}

} // namespace

/*----------------------------------------------------------------------*/
/* The host                                                             */
/*----------------------------------------------------------------------*/

bool host_notice()
{
   static const char *lines[] = {
      "You host the game for up to three guests.",
      "",
      "You control the game: only you can pause it",
      "or change its pace.",
      "",
      "If you quit, or your computer or its connection",
      "stops, the game ends for every player.",
      "",
      "Craft is made for players on the same local",
      "network; over the internet it lags.",
   };
   int n = (int)(sizeof(lines) / sizeof(lines[0]));
   int dy = 70 + n * 18 + 50;
   win *w = open_box("Host a network game", 400, dy);
   button *b_ok;
   button *b_cancel;
   bool answer = false;

   w->set_color(black);
   for (int i = 0; i < n; i++)
      w->write(16, 56 + i * 18, lines[i]);
   b_ok = new button(w, "host", 240, dy - 40);
   b_cancel = new button(w, "cancel", 310, dy - 40);
   for (;;)
   {
      turn(w);
      if (b_ok->eval())
      {
         answer = true;
         break;
      }
      if (b_cancel->eval())
         break;
      turn_done(w);
      delay(30);
   }
   delete b_ok;
   delete b_cancel;
   delete w;
   return answer;
}

static void lobby_summary(char out[256])
{
   snprintf(out, 256, "Map %d to %d squares, %s, %s, pace %d/s", min_rland_dx, max_rland_dx,
            is_water_world ? "islands" : "no islands", is_suny ? "fog of war" : "no fog",
            game_pace);
}

bool host_lobby(int port, bool fixed_seats)
{
   int kind[max_players];
   char seat_name[max_players][128];
   char guests[8][128];
   int num_guests = 0;
   int last_guests = -1;
   char hostname[128] = "";
   win *w;
   button *b_seat[max_players];
   button *b_play;
   button *b_cancel;
   bool started = false;
   bool changed = true;
   char problem[200] = "";

   if (!net_host_listen(port))
   {
      char text[100];

      snprintf(text, sizeof(text), "Cannot host: port %d is in use", port);
      ack(text);
      return false;
   }
   gethostname(hostname, sizeof(hostname) - 1);
   { // seats_from_the_menu
      bool mine = false;

      for (int i = 0; i < max_players; i++)
      {
         snprintf(seat_name[i], 128, "%.120s", name[i]);
         if (!active[i])
            kind[i] = seat_closed;
         else if (strcmp(host[i], "-") == 0)
            kind[i] = seat_computer;
         else if (!mine)
         {
            kind[i] = seat_host;
            mine = true;
         }
         else
            kind[i] = seat_open;
      }
      if (!mine)
      { /* nobody to play here: the host takes the first seat */
         if (kind[0] == seat_computer)
            for (int i = 1; i < max_players; i++)
               if (kind[i] == seat_closed)
               {
                  kind[i] = seat_computer;
                  memcpy(seat_name[i], seat_name[0], 128);
                  break;
               }
         kind[0] = seat_host;
         snprintf(seat_name[0], 128, "player 0");
      }
   }
   w = open_box("Hosting a network game", lobby_dx, lobby_dy);
   { // how_to_join
      char text[200];

      w->set_color(black);
      w->write(16, 50, "Guests join from the start menu (join), or with");
      snprintf(text, sizeof(text), "   craft --join craft@%s --code %s", hostname, net_join_code());
      w->write(16, 68, text);
      snprintf(text, sizeof(text), "Join code:  %s", net_join_code());
      w->set_color(red);
      w->write(16, 90, text);
   }
   for (int i = 0; i < max_players; i++)
      b_seat[i] = kind[i] == seat_host || fixed_seats
                      ? NULL
                      : new button(w, "change", lobby_dx - 90, 112 + i * 26);
   b_play = new button(w, "play", lobby_dx - 150, lobby_dy - 40);
   b_cancel = new button(w, "cancel", lobby_dx - 90, lobby_dy - 40);

   for (;;)
   {
      int open = 0;
      int computers = 0;
      int people = 0;

      turn(w);
      { // guests
         int max_guests = 0;

         for (int i = 0; i < max_players; i++)
            if (kind[i] == seat_open)
               max_guests++;
         num_guests = net_lobby_poll(max_guests, guests);
         if (num_guests != last_guests)
            changed = true;
         last_guests = num_guests;
      }
      for (int i = 0; i < max_players; i++)
         if (b_seat[i] != NULL && b_seat[i]->eval())
         { /* Open -> Computer -> Closed -> Open */
            kind[i] = kind[i] == seat_open       ? seat_computer
                      : kind[i] == seat_computer ? seat_closed
                                                 : seat_open;
            changed = true;
         }
      for (int i = 0; i < max_players; i++)
      {
         open += kind[i] == seat_open;
         computers += kind[i] == seat_computer;
         people += kind[i] == seat_host || kind[i] == seat_open;
      }
      if (changed)
      { // show_seats_and_tell_the_guests
         char lines[max_players][128];
         char summary[256];
         int g = 0;

         for (int i = 0; i < max_players; i++)
         {
            char text[160];

            switch (kind[i])
            {
            case seat_host:
               snprintf(lines[i], 128, "%d  %.60s (host)", i + 1, seat_name[i]);
               snprintf(text, sizeof(text), "%d  %.60s (you)", i + 1, seat_name[i]);
               break;
            case seat_open:
               if (g < num_guests)
                  snprintf(lines[i], 128, "%d  %.60s", i + 1, guests[g++]);
               else
                  snprintf(lines[i], 128, "%d  Open: waiting for a guest", i + 1);
               snprintf(text, sizeof(text), "%s", lines[i]);
               break;
            default:
               snprintf(lines[i], 128, "%d  %s", i + 1, kind_name[kind[i]]);
               snprintf(text, sizeof(text), "%s", lines[i]);
               break;
            }
            say(w, 126 + i * 26, lobby_dx - 100, kind[i] == seat_closed ? gray50 : black, text);
         }
         lobby_summary(summary);
         say(w, 240, lobby_dx, gray50, summary);
         net_lobby_send(lines, max_players, summary);
         changed = false;
      }
      { // can_we_start
         if (computers > 1)
            snprintf(problem, sizeof(problem), "At most one seat can be a computer player.");
         else if (open == 0)
            snprintf(problem, sizeof(problem), "Open a seat for a guest (change).");
         else if (people + computers < 2)
            snprintf(problem, sizeof(problem), "A game needs two players or more.");
         else if (num_guests < open)
            snprintf(problem, sizeof(problem), "Waiting for %d more guest%s.", open - num_guests,
                     open - num_guests == 1 ? "" : "s");
         else
            snprintf(problem, sizeof(problem), "Everyone is here: press play.");
         say(w, 264, lobby_dx, num_guests < open && computers <= 1 ? black : red, problem);
      }
      if (b_cancel->eval())
         break;
      if (b_play->eval() && computers <= 1 && open > 0 && num_guests >= open)
      {
         started = true;
         break;
      }
      turn_done(w);
      delay(30);
   }

   if (started)
   { // set_up_the_seats
      /* New games use seats 1, 2, ... with no gaps (the game expects
         that); a restored game keeps its seats. */
      int order[max_players];
      int num = 0;
      int seat_of[max_players];
      int g = 0;

      if (fixed_seats)
         for (int i = 0; i < max_players; i++)
            order[num++] = i;
      else
      {
         for (int i = 0; i < max_players; i++)
            if (kind[i] == seat_host)
               order[num++] = i;
         for (int i = 0; i < max_players; i++)
            if (kind[i] == seat_open)
               order[num++] = i;
         for (int i = 0; i < max_players; i++)
            if (kind[i] == seat_computer)
               order[num++] = i;
         for (int i = 0; i < max_players; i++)
            if (kind[i] == seat_closed)
               order[num++] = i;
      }
      {
         int k_kind[max_players];
         char k_name[max_players][128];

         for (int p = 0; p < max_players; p++)
         {
            k_kind[p] = kind[order[p]];
            memcpy(k_name[p], seat_name[order[p]], 128);
         }
         for (int p = 0; p < max_players; p++)
         {
            active[p] = k_kind[p] != seat_closed;
            switch (k_kind[p])
            {
            case seat_host:
               strcpy(host[p], "");
               net_seat = p;
               snprintf(name[p], sizeof(name[p]), "%.120s", k_name[p]);
               break;
            case seat_open:
               strcpy(host[p], "@");
               snprintf(name[p], sizeof(name[p]), "%.60s", guests[g]);
               seat_of[g++] = p;
               break;
            case seat_computer:
               strcpy(host[p], "-");
               snprintf(name[p], sizeof(name[p]), "%.120s", k_name[p]);
               break;
            default:
               break;
            }
         }
      }
      net_lobby_seat(seat_of, g);
   }
   else
      net_lobby_close("The host stopped hosting");
   for (int i = 0; i < max_players; i++)
      delete b_seat[i];
   delete b_play;
   delete b_cancel;
   delete w;
   return started;
}

/*----------------------------------------------------------------------*/
/* The guest                                                            */
/*----------------------------------------------------------------------*/

gzFile join_window(int port, const char addr[], const char code[], const char my_name[], bool go)
{
   static char f_addr[128];
   static char f_code[128];
   static char f_name[128];
   win *w;
   gzFile state = NULL;
   bool done = false;

   snprintf(f_addr, sizeof(f_addr), "%s", addr);
   snprintf(f_code, sizeof(f_code), "%s", code);
   snprintf(f_name, sizeof(f_name), "%s", my_name);
   w = open_box("Join a network game", lobby_dx, lobby_dy);
   while (!done)
   { // the_form
      craft_getline *g_addr;
      craft_getline *g_code;
      craft_getline *g_name;
      button *b_join;
      button *b_cancel;
      bool join = false;

      clear_line(w, 300, lobby_dx); /* the form covers the lobby view */
      w->set_color(win_default_c("tell_background"));
      w->fill(8, 34, lobby_dx - 16, lobby_dy - 42);
      w->set_color(black);
      w->write(16, 56, "Host (craft@host name, for SSH):");
      w->write(16, 116, "Join code:");
      w->write(16, 176, "Your name:");
      g_addr = new craft_getline("", w, f_addr, 16, 66, 300, 22);
      g_code = new craft_getline("", w, f_code, 16, 126, 160, 22);
      g_name = new craft_getline("", w, f_name, 16, 186, 160, 22);
      b_join = new button(w, "join", lobby_dx - 150, lobby_dy - 40);
      b_cancel = new button(w, "cancel", lobby_dx - 90, lobby_dy - 40);
      for (; !go;)
      {
         turn(w);
         g_addr->get();
         g_code->get();
         g_name->get();
         if (b_cancel->eval())
         {
            done = true;
            break;
         }
         if (b_join->eval())
         {
            if (f_addr[0] == 0 || f_code[0] == 0 || f_name[0] == 0)
               say(w, 236, lobby_dx, red, "Fill in all three, please.");
            else
            {
               join = true;
               break;
            }
         }
         turn_done(w);
         delay(30);
      }
      delete g_addr;
      delete g_code;
      delete g_name;
      delete b_join;
      delete b_cancel;
      if (go)
      {
         go = false;
         join = true;
      }
      if (!join)
         break;

      { // connect_and_wait
         char why[256] = "";
         char lines[8][128];
         int num_lines = 0;
         char summary[256];
         button *b_leave;
         bool redraw = true;

         w->set_color(win_default_c("tell_background"));
         w->fill(8, 34, lobby_dx - 16, lobby_dy - 42);
         say(w, 56, lobby_dx, black,
             strchr(f_addr, '@') != NULL ? "Connecting through SSH (a passphrase is asked in"
                                         : "Connecting...");
         if (strchr(f_addr, '@') != NULL)
            say(w, 74, lobby_dx, black, "the terminal Craft was started from)...");
         w->tick();
         if (!net_guest_connect(f_addr, port, f_code, f_name))
         {
            ack((char *)(net_status() != NULL ? net_status() : "Cannot connect"));
            continue; /* back to the form */
         }
         b_leave = new button(w, "leave", lobby_dx - 90, lobby_dy - 40);
         for (;;)
         {
            int r;

            turn(w);
            r = net_guest_lobby_poll(20, state, lines, num_lines, summary, why);
            if (r == net_lobby_started)
            {
               done = true;
               break;
            }
            if (r == net_lobby_ended)
               break;
            if (r == net_lobby_changed || redraw)
            {
               w->set_color(win_default_c("tell_background"));
               w->fill(8, 34, lobby_dx - 16, lobby_dy - 82);
               say(w, 56, lobby_dx, black,
                   num_lines > 0 ? "In the lobby. The players:"
                                 : "Connected; waiting for the host to let us in...");
               for (int i = 0; i < num_lines; i++)
                  say(w, 86 + i * 24, lobby_dx, black, lines[i]);
               say(w, 200, lobby_dx, gray50, summary);
               if (num_lines > 0)
                  say(w, 240, lobby_dx, black, "Waiting for the host to start the game.");
               redraw = false;
            }
            if (b_leave->eval())
            {
               net_guest_cancel();
               why[0] = 0;
               break;
            }
            turn_done(w);
         }
         delete b_leave;
         if (!done && why[0] != 0)
            ack(why);
      }
   }
   delete w;
   return state;
}
