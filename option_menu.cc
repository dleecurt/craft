/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from option_menu.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes;
 *              new features (see README), including zoom
 * Exact changes: git log -- option_menu.cc option_menu.hc
 */

#include "screens.h"
#include "savegame.h"
#include "scroller.h"
#include "ppm.h"
#include "dial.h"
#include "buttons.h"
#include "option_menu.h"
#include "zoom.h"
#include "sound.h"
#include "getline.h"
#include "lobby.h"
#include "win.h"
#include "cmap.h"
#include "craft.h"
#include "xstring.h"

#define w_dx 400
#define w_dy 360 /* room under the pages for the network line */
#define flag_dx 100
#define flag_dy 30
#define flag_x 300

#define m_main 0
#define m_details 1
#define m_player 2
#define m_robot 3
#define m_cancel 4
#define m_quit 5
#define m_inactive 6

cmap *p[40];
bool is_p[40];
win *w;
int act_menu;
int menu_type[10];
int num_menus;

int act_speed;
int r_power;
int r_agress;
bool was_seed;
button *b_restore = NULL; /* "restore" under the menu column */
button *b_host = NULL;    /* "host" and "join": network play */
button *b_join = NULL;
bool menu_hosting = false; /* "host" is on: Play opens the lobby */
bool menu_join = false;    /* "join" was pressed */

static void show_network_line()
{
   w->set_color(gray80);
   w->fill(0, 300, flag_x, w_dy - 300);
   w->set_color(menu_hosting ? red : gray50);
   w->write(12, 322,
            menu_hosting ? "Hosting: play opens the lobby." : "host: others join your game");
   w->write(12, 342, menu_hosting ? "(host again: play alone)" : "join: play in someone's game");
}

void show_flags()
{
   { // prepare
      w->set_color(gray80);
      w->fill(flag_x, 0, w_dx, w_dy);
   }
   for (int i = 0; i < num_menus; i++)
   { // show_menu_topic
      int m_no;

      if (i == act_menu)
      { // show_active
         m_no = menu_type[i] + 10;
      }
      else
      { // show_passive
         m_no = menu_type[i] + 20;
      }
      { // show_p
         if (!is_p[m_no])
         { // load_p
            char name[128];

            sprintf(name, "%s/men%d", craft_data("hcraft"), m_no);
            p[m_no] = new cmap(w, name);
            is_p[m_no] = true;
         }
         p[m_no]->show(flag_x, i * flag_dy);
      }
   }
   if (b_restore != NULL)
      b_restore->press(false); /* redraw it over the cleared column */
   if (b_host != NULL)
   {
      b_host->press(menu_hosting);
      b_join->press(false);
      show_network_line();
   }
   w->tick();
}

void set_robot(int pno)
{
   strcpy(host[pno], "-");
   switch (i_random(0, 9))
   {
   case 0:
      strcpy(name[pno], "Sven");
      break;
   case 1:
      strcpy(name[pno], "Thor");
      break;
   case 2:
      strcpy(name[pno], "Haegedin");
      break;
   case 3:
      strcpy(name[pno], "Roland");
      break;
   case 4:
      strcpy(name[pno], "Lars");
      break;
   case 5:
      strcpy(name[pno], "Frederik");
      break;
   case 6:
      strcpy(name[pno], "Oehr");
      break;
   case 7:
      strcpy(name[pno], "Lunda");
      break;
   case 8:
      strcpy(name[pno], "Arnhold");
      break;
   case 9:
      strcpy(name[pno], "Eric");
      break;
   }
}

bool any_flag_request()
{
   int x;
   int y;
   int b;

   if (b_restore != NULL && b_restore->eval())
   { // handle_restore
      char chosen[128];
      bool ok;

      b_restore->press(true);
      ok = choose_saved_game(chosen);
      b_restore->press(false);
      if (ok)
      { // leave_the_menu_as_if_play_was_pressed
         snprintf(restore_name, sizeof(restore_name), "%s", chosen);
         restoring = true;
         for (int i = 0; i < num_menus; i++)
            if (menu_type[i] == m_quit)
               act_menu = i;
         return true;
      }
   }

   if (b_host != NULL && b_host->eval())
   { // toggle_hosting
      menu_hosting = !menu_hosting && host_notice();
      b_host->press(menu_hosting);
      show_network_line();
      w->tick();
   }
   if (b_join != NULL && b_join->eval())
   { // leave_the_menu_to_join
      menu_join = true;
      for (int i = 0; i < num_menus; i++)
         if (menu_type[i] == m_quit)
            act_menu = i;
      return true;
   }

   if (w->is_mouse(x, y, b) && (x >= flag_x && y <= num_menus * flag_dy))
   { // handle_flag_request
      int xb;
      int yb;
      int prev_menu = act_menu;

      w->mouse(x, y, xb, yb, b);
      if (b == button1press)
      {
         act_menu = yb / flag_dy;
         sfx_ui(sfx_click);
         { // perhaps_type_change
            if ((act_menu - 2) > 0
                && (menu_type[act_menu] == m_player || menu_type[act_menu] == m_robot
                    || menu_type[act_menu] == m_inactive))
            { // handle_change
               int b = xb - flag_x;

               if (b > 66 && prev_menu == act_menu)
               { // set_to_next
                  switch (menu_type[act_menu])
                  {
                  case m_player:
                  { // set_to_robot
                     active[(act_menu - 2)] = true;
                     menu_type[act_menu] = m_robot;
                     set_robot((act_menu - 2));
                  };
                  break;
                  case m_robot:
                  { // set_to_inactive
                     if ((act_menu - 2) < 2)
                     { // set_to_player
                        active[(act_menu - 2)] = true;
                        menu_type[act_menu] = m_player;
                        strcpy(host[(act_menu - 2)], "");
                        sprintf(name[(act_menu - 2)], "player %d", (act_menu - 2));
                     }
                     else
                     { // set_inactive
                        active[(act_menu - 2)] = false;
                        menu_type[act_menu] = m_inactive;
                     }
                  };
                  break;
                  case m_inactive:
                  { // set_to_player
                     active[(act_menu - 2)] = true;
                     menu_type[act_menu] = m_player;
                     strcpy(host[(act_menu - 2)], "");
                     sprintf(name[(act_menu - 2)], "player %d", (act_menu - 2));
                  };
                  break;
                  }
               }
            }
         }
         return true;
      }
      else
         return false;
   }
   else
      return false;
}

void main_menu(int /* act_menu */, int &tic)
{
   dial *d_is_self;
   dial *d_is_suny;
   dial *d_water_world;
   craft_getline *seed;
   char seed_s[128];

   {    // init
      { // show_pic
         if (!is_p[m_main])
         { // load_p
            w->set_cursor(150);
            w->tick();
            p[m_main] = new cmap(w, craft_data("hcraft/m1"));
            is_p[m_main] = true;
            w->set_cursor(2);
            w->tick();
         }
         p[m_main]->show(0, 0);
      }
      { // show_buttons
         d_is_self = new dial(w, "", 0, 200, 100, is_self);
         d_is_suny = new dial(w, "", 0, 200, 140, is_suny);
         d_water_world = new dial(w, "", 0, 200, 180, is_water_world);
         strcpy(seed_s, "");
         seed = new craft_getline("", w, seed_s, 200, 250, 90, 24);
      }
   }
   { // session
      w->mark_mouse();
      w->tick();
      while (!any_flag_request())
      {
         w->scratch_mouse();
         w->mark_mouse();
         w->tick();
         { // handle_cmds
            seed->eval();
            d_is_self->eval(is_self);
            d_is_suny->eval(is_suny);
            d_water_world->eval(is_water_world);
         }
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
      if (strlen(seed_s) > 0)
      { // set_seed
         if (atoi(seed_s) > 0)
            tic = atoi(seed_s);
         else
         { // get_alpha_seed
            char strlist[1024];

            strcpy(strlist, "abcdefghijklmnopqrstuvwxyz");
            strcat(strlist, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
            strcat(strlist, " ,.;_-=+!@#$%^&*()[]{}<>");
            tic = 0;
            for (int i = 0; i < (int)strlen(seed_s); i++)
            {
               tic += strpos(strlist, seed_s[i]) % 123124;
            }
         }
         was_seed = true;
      }
   }
   { // finish
      delete (d_is_self);
      delete (d_is_suny);
      delete (d_water_world);
   }
}

void player_menu(int act_menu, int &tic)
{
   craft_getline *g_host;
   craft_getline *g_name;

   {    // init
      { // show_pic
         if (!is_p[m_player])
         { // load_p
            w->set_cursor(150);
            w->tick();
            p[m_player] = new cmap(w, craft_data("hcraft/m4"));
            is_p[m_player] = true;
            w->set_cursor(2);
            w->tick();
         }
         p[m_player]->show(0, 0);
      }
      { // show_buttons
         g_host = new craft_getline("", w, host[(act_menu - 2)], 60, 160, 180, 20);
         g_name = new craft_getline("", w, name[(act_menu - 2)], 60, 230, 180, 20);
      }
   }
   { // session
      w->mark_mouse();
      w->tick();
      while (!any_flag_request())
      {
         w->scratch_mouse();
         w->mark_mouse();
         w->tick();
         { // handle_cmds
            g_host->get();
            g_name->get();
            speed[(act_menu - 2)] = 1;
         }
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
   }
   { // finish
      delete (g_host);
      delete (g_name);
   }
}

/* "pace NN steps/s" between the pace buttons on the Details page */
void show_pace()
{
   char text[64];

   snprintf(text, sizeof(text), " pace %3d steps/s ", game_pace);
   w->set_background(gray80);
   w->set_color(gray80);
   w->fill(92, 268, 140, 26);
   w->set_color(black);
   w->write(98, 286, text);
   w->tick();
}

/* "zoom auto (1.25x)" between the zoom buttons on the Details page */
void show_zoom()
{
   char text[64];

   snprintf(text, sizeof(text), " zoom %-12s", zoom_label());
   w->set_background(gray80);
   w->set_color(gray80);
   w->fill(92, 14, 140, 26);
   w->set_color(black);
   w->write(98, 32, text);
   w->tick();
}

/* "music 70%" or "effects 80%" between its - and + buttons */
#define vol_y 44
static const int vol_label_x[2] = { 26, 156 };
static const int vol_label_dx[2] = { 78, 98 };

void show_volume(int channel)
{
   w->set_background(gray80);
   w->set_color(gray80);
   w->fill(vol_label_x[channel], vol_y, vol_label_dx[channel], 26);
   w->set_color(black);
   w->write(vol_label_x[channel] + 4, vol_y + 18, sound_label(channel));
   w->tick();
}

void details_menu(int /* act_menu */, double &g_speed, int &tic)
{
   dial *d_min_land_dx;
   dial *d_min_land_dy;
   dial *d_max_land_dx;
   dial *d_max_land_dy;
   scroller *s_act_speed;
   button *b_pace_down;
   button *b_pace_up;
   button *b_zoom_down;
   button *b_zoom_up;
   button *b_vol_down[2];
   button *b_vol_up[2];

   {    // init
      { // show_pic
         if (!is_p[m_details])
         { // load_p
            w->set_cursor(150);
            w->tick();
            p[m_details] = new cmap(w, craft_data("hcraft/m2"));
            is_p[m_details] = true;
            w->set_cursor(2);
            w->tick();
         }
         p[m_details]->show(0, 0);
      }
      { // show_buttons
         s_act_speed = new scroller("", w, 60, 200, 220, 0, 1, 5, 0, act_speed, 1, 1);
         d_min_land_dx
             = new dial(w, "", 0, 200, 70, 50, min_rland_dx, 400, 10, false, 40, false, 50, false);
         d_max_land_dx
             = new dial(w, "", 0, 200, 90, 80, max_rland_dx, 400, 10, false, 40, false, 50, false);
         d_min_land_dy
             = new dial(w, "", 0, 200, 110, 50, min_rland_dy, 400, 10, false, 40, false, 50, false);
         d_max_land_dy
             = new dial(w, "", 0, 200, 130, 80, max_rland_dy, 400, 10, false, 40, false, 50, false);
         b_pace_down = new button(w, " - ", 60, 268);
         b_pace_up = new button(w, " + ", 236, 268);
         show_pace();
         b_zoom_down = new button(w, " - ", 60, 14);
         b_zoom_up = new button(w, " + ", 236, 14);
         show_zoom();
         for (int c = 0; c < 2; c++)
         {
            b_vol_down[c] = new button(w, "-", vol_label_x[c] - 22, vol_y);
            b_vol_up[c] = new button(w, "+", vol_label_x[c] + vol_label_dx[c] + 2, vol_y);
            show_volume(c);
         }
      }
   }
   { // session
      w->mark_mouse();
      w->tick();
      while (!any_flag_request())
      {
         w->scratch_mouse();
         w->mark_mouse();
         w->tick();
         { // handle_cmds
            if (d_min_land_dx->eval(min_rland_dx))
            {
               min_rland_dx = i_min(min_rland_dx, max_rland_dx);
               d_min_land_dx->set(min_rland_dx);
            }
            if (d_min_land_dy->eval(min_rland_dy))
            {
               min_rland_dy = i_min(min_rland_dy, max_rland_dy);
               d_min_land_dy->set(min_rland_dy);
            }
            if (d_max_land_dx->eval(max_rland_dx))
            {
               min_rland_dx = i_min(min_rland_dx, max_rland_dx);
               d_min_land_dx->set(min_rland_dx);
            }
            if (d_max_land_dy->eval(max_rland_dy))
            {
               min_rland_dy = i_min(min_rland_dy, max_rland_dy);
               d_min_land_dy->set(min_rland_dy);
            }
            s_act_speed->eval(act_speed);
            if (b_pace_down->eval())
            {
               step_game_pace(-1);
               show_pace();
            }
            if (b_pace_up->eval())
            {
               step_game_pace(1);
               show_pace();
            }
            if (b_zoom_down->eval())
            {
               win::rezoom_all(zoom_step(-1));
               show_zoom();
            }
            if (b_zoom_up->eval())
            {
               win::rezoom_all(zoom_step(1));
               show_zoom();
            }
            for (int c = 0; c < 2; c++)
            {
               if (b_vol_down[c]->eval())
               {
                  sound_step_volume(c, -1);
                  show_volume(c);
               }
               if (b_vol_up[c]->eval())
               {
                  sound_step_volume(c, 1);
                  show_volume(c);
               }
            }
            switch (act_speed)
            {
            case 1:
               g_speed = 1.0 / 5.0;
               break;
            case 2:
               g_speed = 1.0 / 4.0;
               break;
            case 3:
               g_speed = 1.0 / 2.0;
               break;
            case 4:
               g_speed = 1.0 / 1.0;
               break;
            case 5:
               g_speed = 1.0 / 0.5;
               break;
            }
         }
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
   }
   { // finish
      delete (d_min_land_dx);
      delete (d_min_land_dy);
      delete (d_max_land_dx);
      delete (d_max_land_dy);
      delete (s_act_speed);
      delete (b_pace_down);
      delete (b_pace_up);
      delete (b_zoom_down);
      delete (b_zoom_up);
      for (int c = 0; c < 2; c++)
      {
         delete (b_vol_down[c]);
         delete (b_vol_up[c]);
      }
   }
}

void robo_menu(int /* act_menu */, int &tic)
{
   scroller *s_robo_power;

   {    // init
      { // show_pic
         if (!is_p[m_robot])
         { // load_p
            w->set_cursor(150);
            w->tick();
            p[m_robot] = new cmap(w, craft_data("hcraft/m3"));
            is_p[m_robot] = true;
            w->set_cursor(2);
            w->tick();
         }
         p[m_robot]->show(0, 0);
      }
      { // show_buttons
         s_robo_power = new scroller("", w, 40, 180, 200, 0, 1, 5, 0, r_power, 1, 1);
      }
   }
   { // session
      w->mark_mouse();
      w->tick();
      while (!any_flag_request())
      {
         w->scratch_mouse();
         w->mark_mouse();
         w->tick();
         { // handle_cmds
            s_robo_power->eval(r_power);
         }
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
   }
   { // finish
      delete (s_robo_power);
   }
}

void inactive_menu(int /* act_menu */, int &tic)
{
   { // init
      if (!is_p[m_inactive])
      { // load_p
         w->set_cursor(150);
         w->tick();
         p[m_inactive] = new cmap(w, craft_data("hcraft/m5"));
         is_p[m_inactive] = true;
         w->set_cursor(2);
         w->tick();
      }
      p[m_inactive]->show(0, 0);
   }
   { // session
      w->mark_mouse();
      w->tick();
      while (!any_flag_request())
      {
         w->scratch_mouse();
         w->mark_mouse();
         w->tick();
         { // handle_cmds
         }
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
   }
}

/* Shown before?  Then keep what was set (back from the lobby). */
static bool menu_was_shown = false;

void option_menu(double &speed, int &tic)
{
   bool quit;

   {    // init
      { // open_w
         w = new win("craft_options", "", 10, 10, w_dx, w_dy);
      }
      { // init_p
         /* pictures belong to a window: none kept from a previous menu */
         for (int i = 0; i < 40; i++)
            is_p[i] = false;
      }
      { // init_menus
         act_menu = 0;
         quit = false;
         menu_type[0] = m_main;
         menu_type[1] = m_details;
         menu_type[2] = m_player;
         menu_type[3] = m_robot;
         menu_type[4] = m_inactive;
         menu_type[5] = m_inactive;
         menu_type[6] = m_cancel;
         menu_type[7] = m_quit;
         num_menus = 8;
         b_restore = new button(w, "restore", flag_x + 18, num_menus * flag_dy + 22);
         b_host = new button(w, "host", flag_x + 8, num_menus * flag_dy + 64);
         b_join = new button(w, "join", flag_x + 54, num_menus * flag_dy + 64);
         menu_join = false;
         show_flags();
      }
      if (!menu_was_shown)
      { // default_params
         active[0] = true;
         active[1] = true;
         active[2] = false;
         active[3] = false;
         is_self = true;
         is_suny = true;
         is_water_world = true;
         act_speed = 4;
         q_step = 1;
         r_power = 3;
         r_agress = 3;
         min_rland_dx = 60;
         min_rland_dy = 60;
         max_rland_dx = 80;
         max_rland_dy = 80;
         strcpy(host[0], "");
         strcpy(host[1], "");
         strcpy(host[2], "");
         strcpy(host[3], "");
         strcpy(name[0], "player 0");
         strcpy(name[1], "player 1");
         strcpy(name[2], "player 2");
         strcpy(name[3], "player 3");
         was_seed = false;
         set_robot(1);
         menu_was_shown = true;
      }
   }
   while (!quit)
   {
      { // session
         switch (menu_type[act_menu])
         {
         case m_main:
            main_menu(act_menu, tic);
            break;
         case m_details:
            details_menu(act_menu, speed, tic);
            break;
         case m_player:
            player_menu(act_menu, tic);
            break;
         case m_robot:
            robo_menu(act_menu, tic);
            break;
         case m_inactive:
            inactive_menu(act_menu, tic);
            break;
         case m_cancel:
            show_credits();
            exit(0);
            break;
         case m_quit:
            quit = true;
            break;
         }
         show_flags();
         if (!was_seed)
            tic = (tic + 1) % 123124;
      }
   }
   { // finish
      delete (b_restore);
      delete (b_host);
      delete (b_join);
      b_restore = b_host = b_join = NULL;
      for (int i = 0; i < 40; i++)
         if (is_p[i])
         {
            delete (p[i]);
            is_p[i] = false;
         }
      delete (w);
      switch (act_speed)
      {
      case 5:
         speed = 1.0 / 5.0;
         break;
      case 4:
         speed = 1.0 / 4.0;
         break;
      case 3:
         speed = 1.0 / 2.0;
         break;
      case 2:
         speed = 1.0 / 1.0;
         break;
      case 1:
         speed = 1.0 / 0.5;
         break;
      }
      switch (r_power)
      {
      case 1:
         robo_power = 1;
         break;
      case 2:
         robo_power = 1.25;
         break;
      case 3:
         robo_power = 1.5;
         break;
      case 4:
         robo_power = 3;
         break;
      case 5:
         robo_power = 8;
         break;
      }
      switch (r_agress)
      {
      case 5:
         robo_agress = 0.1;
         break;
      case 4:
         robo_agress = 0.5;
         break;
      case 3:
         robo_agress = 1.0;
         break;
      case 2:
         robo_agress = 1.75;
         break;
      case 1:
         robo_agress = 2;
         break;
      }
   }
}
