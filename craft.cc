/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from craft.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes;
 *              new features (see README), including zoom
 * Exact changes: git log -- craft.cc craft.hc
 */

#include "win.h"
#include "cmap.h"
#include "masks.h"
#include "player.h"
#include "craft_def.h"
#include "object_handler.h"
#include "land.h"
#include "xstring.h"
#include "setupedit.h"
#include "dial.h"
#include "scroller.h"
#include "buttons.h"
#include <signal.h>

#include "ship.h"
#include "replay.h"
#include "net.h"
#include "option_menu.h"
#include "xtimer.h"
#include "screens.h"
#include "zoom.h"
#include "sound.h"
#include <time.h>
#include "savegame.h"

#define min_cycle_time 50 /* pause loop poll interval, ms */

double robo_power;
double robo_agress;

int overall_money;

int q_step;

int min_rland_dx;
int min_rland_dy;
int max_rland_dx;
int max_rland_dy;

int speed_stag;
int speed_worker;
int speed_trader;
int speed_knight;
int speed_pawn;
int speed_scout;
int speed_archer;
int speed_home;
int speed_farm;
int speed_farm_exec;
int speed_zombi;
int speed_schrott;
int speed_swim;
int speed_water;
int speed_hit;
int speed_building_zombi;
int speed_archer_hit;
int speed_cata;
int speed_cata_load;
int speed_explosion;
int speed_doktor;
int speed_doktor_hit;
int speed_general;
int speed_trap;
int speed_arrow;
int speed_stone;
int speed_ship1;
int speed_ship2;
int speed_ship_zombi;

char land_name[128];

int ilid[max_land_dx][max_land_dy];
int landscape[max_land_dx][max_land_dy];
int landhight[max_land_dx][max_land_dy];
int landpic[max_land_dx][max_land_dy];
int landoverlay[max_land_dx][max_land_dy][2];
int landscape_dx;
int landscape_dy;
int landscale;
int olandscale;

int unit[max_land_dx][max_land_dy];
int upic[max_land_dx][max_land_dy][8];

int color_player[max_cols];
int player_color[max_players];
bool paused[max_players];
double p_speed[max_players];
char p_name[max_players][128];
int num_paused;

int ticker;

/* Game steps per second. The original slept for 0.5 ms per step (a units
   bug), which on 1997 kernels meant roughly 10 ms, so it ran at about
   30-60 steps per second on the hardware of the time. */
int game_pace = 40;

static const int game_paces[] = { 10, 20, 30, 40, 50, 60, 80, 100 };
#define num_game_paces (int)(sizeof(game_paces) / sizeof(game_paces[0]))

int step_game_pace(int direction)
{
   int i = 0;

   while (i < num_game_paces - 1 && game_paces[i] < game_pace)
      i++;
   i = i_bound(0, i + direction, num_game_paces - 1);
   game_pace = game_paces[i];
   return game_pace;
}

bool is_self;
bool is_suny;
bool is_water_world;

land_prop land_properties[max_land_types];

object_handler *objects;

player *players[max_players];
int num_humans;
int num_players;
int active[max_players];

char host[max_players][128];
char name[max_players][128];
double speed[max_players];

/*--- main ------------------------------------------------------------------*/

void dump()
{
   { // print_landscape
      printf("--- landscape\n");
      printf("dx %d dy %d\n", landscape_dx, landscape_dy);
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
            printf("%4d ", landscape[x][y]);
         printf("\n");
      }
   }
   { // print_landhight
      printf("--- landhight\n");
      printf("dx %d dy %d\n", landscape_dx, landscape_dy);
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
            printf("%4d ", landhight[x][y]);
         printf("\n");
      }
   }
   { // print_landpic
      printf("--- landpic\n");
      printf("dx %d dy %d\n", landscape_dx, landscape_dy);
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
            printf("%4d ", landpic[x][y]);
         printf("\n");
      }
   }
   { // print_units
      printf("--- unit\n");
      printf("dx %d dy %d\n", landscape_dx, landscape_dy);
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
            printf("%4d ", unit[x][y]);
         printf("\n");
      }
   }
   { // print_upic
      printf("--- upic\n");
      printf("dx %d dy %d\n", landscape_dx, landscape_dy);
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
         {
            for (int i = 0; i < 8; i++)
               printf("%4d ", upic[x][y][i]);
            printf(",");
         }
         printf("\n");
      }
   }
   { // print_objs
      for (int i = 0; i < max_objects; i++)
         printf("%d %d %d %d %d %d %d %d %d\n", i, objects->type[i], objects->is_free[i],
                objects->color[i], objects->x[i], objects->y[i], objects->wx[i], objects->wy[i],
                objects->pic[i]);
   }
}

void handle_abnormal_termination()
{
   printf("\nabnormal termination occurred ***\n");
   /*
   dump   ();
*/
   exit(0);
}

void handle_bus_error()
{
   printf("\nbus error occurred ***\n");
   /*
   dump   ();
*/
   exit(0);
}

void handle_seg_fault()
{
   printf("\nsegmentation fault occurred ***\n");
   /*
   dump   ();
*/
   exit(0);
}

void install_abnormal_termination_handler()
{
   /*
   set_interrupt (SIGINT,  handle_abnormal_termination);
   set_interrupt (SIGBUS,  handle_bus_error);
   set_interrupt (SIGSEGV, handle_seg_fault);
   set_interrupt (SIGABRT, handle_abnormal_termination);
*/
}

/*--- main ------------------------------------------------------------------*/

double pdist(int x1, int y1, int x2, int y2)
{
   int xx = x1 - x2;
   int yy = y1 - y2;

   return sqrt(xx * xx + yy * yy);
}

void init_speed(double factor)
{
   speed_stag = i_max(1, xspeed_stag);
   speed_worker = i_max(1, (int)((double)xspeed_worker * factor));
   speed_trader = i_max(1, (int)((double)xspeed_trader * factor));
   speed_knight = i_max(1, (int)((double)xspeed_knight * factor));
   speed_pawn = i_max(1, (int)((double)xspeed_pawn * factor));
   speed_scout = i_max(1, (int)((double)xspeed_scout * factor));
   speed_archer = i_max(1, (int)((double)xspeed_archer * factor));
   speed_home = i_max(1, (int)((double)xspeed_home * factor));
   speed_farm = i_max(1, (int)((double)xspeed_farm * factor));
   speed_farm_exec = i_max(1, speed_farm - (int)(200.0 * factor));
   speed_zombi = i_max(1, (int)((double)xspeed_zombi * factor));
   speed_schrott = i_max(1, (int)((double)xspeed_schrott * factor));
   speed_swim = i_max(1, (int)((double)xspeed_swim * factor));
   speed_water = i_max(1, (int)((double)xspeed_water * factor));
   speed_hit = i_max(1, (int)((double)xspeed_hit * factor));
   speed_building_zombi = i_max(1, (int)((double)xspeed_building_zombi * factor));
   speed_archer_hit = i_max(1, (int)((double)xspeed_archer_hit * factor));
   speed_cata = i_max(1, (int)((double)xspeed_cata * factor));
   speed_cata_load = i_max(1, (int)((double)xspeed_cata_load * factor));
   speed_explosion = i_max(1, (int)((double)xspeed_explosion * factor));
   speed_doktor = i_max(1, (int)((double)xspeed_doktor * factor));
   speed_doktor_hit = i_max(1, (int)((double)xspeed_doktor_hit * factor));
   speed_general = i_max(1, (int)((double)xspeed_general * factor));
   speed_trap = i_max(1, (int)((double)xspeed_trap * factor));
   speed_arrow = i_max(1, (int)((double)xspeed_arrow * factor));
   speed_stone = i_max(1, (int)((double)xspeed_stone * factor));
   speed_ship1 = i_max(1, (int)((double)xspeed_ship1 * factor));
   speed_ship2 = i_max(1, (int)((double)xspeed_ship2 * factor));
   speed_ship_zombi = i_max(1, (int)((double)xspeed_ship_zombi * factor));
}

void init_craft(char host[max_players][128], char name[max_players][128],
                double /* speed */[max_players])

{
   install_abnormal_termination_handler();
   { // store_params
      num_players = 0;
      num_humans = 0;
      for (int i = 0; i < max_players; i++)
      { // cnt_player
         if (active[i])
         {
            num_players++;
            if (!(strcmp(host[i], "-") == 0))
               num_humans++;
         }
      }
      num_paused = 0;
   }
   { // init_ticker
      ticker = 0;
   }
   { // load_troups
      objects = new object_handler();
   }
   { // init_land
      load_land_props();
      if (restoring)
         ; /* the landscape was loaded by restore_game_globals() */
      else if (is_water_world)
         rnd_water_land();
      else
         rnd_land();
   }
   { // init_players
      int i;

      olandscale = 1;
      for (i = 0; i < max_players; i++)
      {
         paused[i] = false;
         strcpy(p_name[i], name[i]);
         p_speed[i] = 1;
      }
      for (i = 0; i < max_players; i++)
         if (active[i])
         { // gen_player
            if (i == 0)
               players[i] = new player(i, name[i], host[i], red, p_speed[i],
                                       strcmp(host[i], "-") == 0, strcmp(host[i], "@") != 0);
            if (i == 1)
               players[i] = new player(i, name[i], host[i], blue, p_speed[i],
                                       strcmp(host[i], "-") == 0, strcmp(host[i], "@") != 0);
            if (i == 2)
               players[i] = new player(i, name[i], host[i], yellow, p_speed[i],
                                       strcmp(host[i], "-") == 0, strcmp(host[i], "@") != 0);
            if (i == 3)
               players[i] = new player(i, name[i], host[i], cyan, p_speed[i],
                                       strcmp(host[i], "-") == 0, strcmp(host[i], "@") != 0);
         }
      for (i = 0; i < max_players; i++)
         if (active[i])
            players[i]->initial_display();
      color_player[red] = 0;
      color_player[blue] = 1;
      color_player[yellow] = 2;
      color_player[cyan] = 3;
      player_color[0] = red;
      player_color[1] = blue;
      player_color[2] = yellow;
      player_color[3] = cyan;
   }
}

void finish_craft()
{
   for (int i = 0; i < max_players; i++)
      if (active[i])
         delete (players[i]);
   delete (objects);
}

/* A free 2x2 dry spot for a starting unit at (xx, yy). */
static bool start_spot_ok(int xx, int yy)
{
   return 1 <= xx && xx <= landscape_dx - 3 && 1 <= yy && yy <= landscape_dy - 3
          && unit[xx][yy] == none && unit[xx + 1][yy] == none && unit[xx + 1][yy + 1] == none
          && landscape[xx][yy] != land_water && landscape[xx + 1][yy] != land_water
          && landscape[xx + 1][yy + 1] != land_water && landscape[xx][yy] != land_sea
          && landscape[xx + 1][yy] != land_sea && landscape[xx + 1][yy + 1] != land_sea;
}

/* The valid start spot nearest (cx, cy); leaves (xx, yy) unchanged if
   there is none at all. */
static void nearest_start_spot(int cx, int cy, int &xx, int &yy)
{
   long best = -1;

   for (int x = 1; x <= landscape_dx - 3; x++)
      for (int y = 1; y <= landscape_dy - 3; y++)
         if (start_spot_ok(x, y))
         {
            long d = (long)(x - cx) * (x - cx) + (long)(y - cy) * (y - cy);

            if (best < 0 || d < best)
            {
               best = d;
               xx = x;
               yy = y;
            }
         }
}

void set_initial_objects()
{
   int color = blue;
   int il_locked[15];

   { // init
      for (int i = 0; i < 15; i++)
         il_locked[i] = false;
   }
   if (active[0])
   {
      color = blue;
      { // gen
         int xx;
         int yy;

         { // get_xx_yy
            if (is_water_world)
            { // get_island_xx_yy
               int il;

               { // get_il
                  il = i_random(0, land_num_il() - 4 - 1);
               }
               while (il_locked[il])
               {
                  { // get_il
                     il = i_random(0, land_num_il() - 4 - 1);
                  }
               }
               xx = land_x_il(il);
               yy = land_y_il(il);
               il_locked[il] = true;
               { // march
                  int ox = xx;
                  int oy = yy;
                  bool il_ok;

                  { // check_il
                     il_ok = start_spot_ok(xx, yy);
                  }
                  int steps = 0;

                  while (!il_ok)
                  {
                     if (++steps > 20000)
                     { // give_up_walking_and_search
                        /* The random walk can loop forever on an island with no free
                           2x2 dry spot it can reach; take the valid spot nearest the
                           island centre instead. */
                        nearest_start_spot(ox, oy, xx, yy);
                        break;
                     }
                     { // perhaps_backstep
                        if (is_water(10000, xx, yy))
                        {
                           xx = ox;
                           yy = oy;
                        }
                     }
                     { // step
                        ox = xx;
                        oy = yy;
                        xx = i_bound(1, xx + i_random(-2, 2), landscape_dx - 3);
                        yy = i_bound(1, yy + i_random(-2, 2), landscape_dy - 3);
                     }
                     { // check_il
                        il_ok = start_spot_ok(xx, yy);
                     }
                  }
                  { // set_to_grass
                     for (int xa = i_max(0, xx - 2); xa < i_min(landscape_dx, xx + 3); xa++)
                        for (int ya = i_max(0, yy - 2); ya < i_min(landscape_dy, yy + 3); ya++)
                           if (landhight[xa][ya] > 0 && unit[xa][ya] == none)
                           {
                              landscape[xa][ya] = land_grass;
                              landpic[xa][ya] = land_grass + land_profile(xa, ya, 1);
                              ;
                              objects->refresh(xa, ya);
                           }
                     il_ok = true;
                  }
               }
            }
            else
            { // get_land_xx_yy
               bool ok;

               { // rnd
                  int s;

                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                  { // check_ok
                     ok = (!is_water_world && landscape[xx][yy] == land_grass
                           && landscape[xx + 1][yy] == land_grass
                           && landscape[xx + 1][yy + 1] == land_grass)
                          || (is_water_world && is_water(10000, xx, yy));
                     for (int i = 0; i < max_objects; i++)
                        if (!objects->is_free[i] && objects->type[i] == object_knight)
                           ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                  }
               }
               while (!ok)
               {
                  { // rnd
                     int s;

                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                     { // check_ok
                        ok = (!is_water_world && landscape[xx][yy] == land_grass
                              && landscape[xx + 1][yy] == land_grass
                              && landscape[xx + 1][yy + 1] == land_grass)
                             || (is_water_world && is_water(10000, xx, yy));
                        for (int i = 0; i < max_objects; i++)
                           if (!objects->is_free[i] && objects->type[i] == object_knight)
                              ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                     }
                  }
               }
            }
         }
         { // gen_on_land
            objects->create_worker(xx, yy, color, none);
            objects->create_worker(xx + 1, yy, color, none);
            objects->create_knight(xx + 1, yy + 1, color, none,
                                   is_self && !players[color_player[color]]->is_robot);
            players[color_player[color]]->focus(xx, yy);
         }
      };
   };
   if (active[1])
   {
      color = red;
      { // gen
         int xx;
         int yy;

         { // get_xx_yy
            if (is_water_world)
            { // get_island_xx_yy
               int il;

               { // get_il
                  il = i_random(0, land_num_il() - 4 - 1);
               }
               while (il_locked[il])
               {
                  { // get_il
                     il = i_random(0, land_num_il() - 4 - 1);
                  }
               }
               xx = land_x_il(il);
               yy = land_y_il(il);
               il_locked[il] = true;
               { // march
                  int ox = xx;
                  int oy = yy;
                  bool il_ok;

                  { // check_il
                     il_ok = start_spot_ok(xx, yy);
                  }
                  int steps = 0;

                  while (!il_ok)
                  {
                     if (++steps > 20000)
                     { // give_up_walking_and_search
                        /* The random walk can loop forever on an island with no free
                           2x2 dry spot it can reach; take the valid spot nearest the
                           island centre instead. */
                        nearest_start_spot(ox, oy, xx, yy);
                        break;
                     }
                     { // perhaps_backstep
                        if (is_water(10000, xx, yy))
                        {
                           xx = ox;
                           yy = oy;
                        }
                     }
                     { // step
                        ox = xx;
                        oy = yy;
                        xx = i_bound(1, xx + i_random(-2, 2), landscape_dx - 3);
                        yy = i_bound(1, yy + i_random(-2, 2), landscape_dy - 3);
                     }
                     { // check_il
                        il_ok = start_spot_ok(xx, yy);
                     }
                  }
                  { // set_to_grass
                     for (int xa = i_max(0, xx - 2); xa < i_min(landscape_dx, xx + 3); xa++)
                        for (int ya = i_max(0, yy - 2); ya < i_min(landscape_dy, yy + 3); ya++)
                           if (landhight[xa][ya] > 0 && unit[xa][ya] == none)
                           {
                              landscape[xa][ya] = land_grass;
                              landpic[xa][ya] = land_grass + land_profile(xa, ya, 1);
                              ;
                              objects->refresh(xa, ya);
                           }
                     il_ok = true;
                  }
               }
            }
            else
            { // get_land_xx_yy
               bool ok;

               { // rnd
                  int s;

                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                  { // check_ok
                     ok = (!is_water_world && landscape[xx][yy] == land_grass
                           && landscape[xx + 1][yy] == land_grass
                           && landscape[xx + 1][yy + 1] == land_grass)
                          || (is_water_world && is_water(10000, xx, yy));
                     for (int i = 0; i < max_objects; i++)
                        if (!objects->is_free[i] && objects->type[i] == object_knight)
                           ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                  }
               }
               while (!ok)
               {
                  { // rnd
                     int s;

                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                     { // check_ok
                        ok = (!is_water_world && landscape[xx][yy] == land_grass
                              && landscape[xx + 1][yy] == land_grass
                              && landscape[xx + 1][yy + 1] == land_grass)
                             || (is_water_world && is_water(10000, xx, yy));
                        for (int i = 0; i < max_objects; i++)
                           if (!objects->is_free[i] && objects->type[i] == object_knight)
                              ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                     }
                  }
               }
            }
         }
         { // gen_on_land
            objects->create_worker(xx, yy, color, none);
            objects->create_worker(xx + 1, yy, color, none);
            objects->create_knight(xx + 1, yy + 1, color, none,
                                   is_self && !players[color_player[color]]->is_robot);
            players[color_player[color]]->focus(xx, yy);
         }
      };
   };
   if (active[2])
   {
      color = yellow;
      { // gen
         int xx;
         int yy;

         { // get_xx_yy
            if (is_water_world)
            { // get_island_xx_yy
               int il;

               { // get_il
                  il = i_random(0, land_num_il() - 4 - 1);
               }
               while (il_locked[il])
               {
                  { // get_il
                     il = i_random(0, land_num_il() - 4 - 1);
                  }
               }
               xx = land_x_il(il);
               yy = land_y_il(il);
               il_locked[il] = true;
               { // march
                  int ox = xx;
                  int oy = yy;
                  bool il_ok;

                  { // check_il
                     il_ok = start_spot_ok(xx, yy);
                  }
                  int steps = 0;

                  while (!il_ok)
                  {
                     if (++steps > 20000)
                     { // give_up_walking_and_search
                        /* The random walk can loop forever on an island with no free
                           2x2 dry spot it can reach; take the valid spot nearest the
                           island centre instead. */
                        nearest_start_spot(ox, oy, xx, yy);
                        break;
                     }
                     { // perhaps_backstep
                        if (is_water(10000, xx, yy))
                        {
                           xx = ox;
                           yy = oy;
                        }
                     }
                     { // step
                        ox = xx;
                        oy = yy;
                        xx = i_bound(1, xx + i_random(-2, 2), landscape_dx - 3);
                        yy = i_bound(1, yy + i_random(-2, 2), landscape_dy - 3);
                     }
                     { // check_il
                        il_ok = start_spot_ok(xx, yy);
                     }
                  }
                  { // set_to_grass
                     for (int xa = i_max(0, xx - 2); xa < i_min(landscape_dx, xx + 3); xa++)
                        for (int ya = i_max(0, yy - 2); ya < i_min(landscape_dy, yy + 3); ya++)
                           if (landhight[xa][ya] > 0 && unit[xa][ya] == none)
                           {
                              landscape[xa][ya] = land_grass;
                              landpic[xa][ya] = land_grass + land_profile(xa, ya, 1);
                              ;
                              objects->refresh(xa, ya);
                           }
                     il_ok = true;
                  }
               }
            }
            else
            { // get_land_xx_yy
               bool ok;

               { // rnd
                  int s;

                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                  { // check_ok
                     ok = (!is_water_world && landscape[xx][yy] == land_grass
                           && landscape[xx + 1][yy] == land_grass
                           && landscape[xx + 1][yy + 1] == land_grass)
                          || (is_water_world && is_water(10000, xx, yy));
                     for (int i = 0; i < max_objects; i++)
                        if (!objects->is_free[i] && objects->type[i] == object_knight)
                           ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                  }
               }
               while (!ok)
               {
                  { // rnd
                     int s;

                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                     { // check_ok
                        ok = (!is_water_world && landscape[xx][yy] == land_grass
                              && landscape[xx + 1][yy] == land_grass
                              && landscape[xx + 1][yy + 1] == land_grass)
                             || (is_water_world && is_water(10000, xx, yy));
                        for (int i = 0; i < max_objects; i++)
                           if (!objects->is_free[i] && objects->type[i] == object_knight)
                              ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                     }
                  }
               }
            }
         }
         { // gen_on_land
            objects->create_worker(xx, yy, color, none);
            objects->create_worker(xx + 1, yy, color, none);
            objects->create_knight(xx + 1, yy + 1, color, none,
                                   is_self && !players[color_player[color]]->is_robot);
            players[color_player[color]]->focus(xx, yy);
         }
      };
   };
   if (active[3])
   {
      color = cyan;
      { // gen
         int xx;
         int yy;

         { // get_xx_yy
            if (is_water_world)
            { // get_island_xx_yy
               int il;

               { // get_il
                  il = i_random(0, land_num_il() - 4 - 1);
               }
               while (il_locked[il])
               {
                  { // get_il
                     il = i_random(0, land_num_il() - 4 - 1);
                  }
               }
               xx = land_x_il(il);
               yy = land_y_il(il);
               il_locked[il] = true;
               { // march
                  int ox = xx;
                  int oy = yy;
                  bool il_ok;

                  { // check_il
                     il_ok = start_spot_ok(xx, yy);
                  }
                  int steps = 0;

                  while (!il_ok)
                  {
                     if (++steps > 20000)
                     { // give_up_walking_and_search
                        /* The random walk can loop forever on an island with no free
                           2x2 dry spot it can reach; take the valid spot nearest the
                           island centre instead. */
                        nearest_start_spot(ox, oy, xx, yy);
                        break;
                     }
                     { // perhaps_backstep
                        if (is_water(10000, xx, yy))
                        {
                           xx = ox;
                           yy = oy;
                        }
                     }
                     { // step
                        ox = xx;
                        oy = yy;
                        xx = i_bound(1, xx + i_random(-2, 2), landscape_dx - 3);
                        yy = i_bound(1, yy + i_random(-2, 2), landscape_dy - 3);
                     }
                     { // check_il
                        il_ok = start_spot_ok(xx, yy);
                     }
                  }
                  { // set_to_grass
                     for (int xa = i_max(0, xx - 2); xa < i_min(landscape_dx, xx + 3); xa++)
                        for (int ya = i_max(0, yy - 2); ya < i_min(landscape_dy, yy + 3); ya++)
                           if (landhight[xa][ya] > 0 && unit[xa][ya] == none)
                           {
                              landscape[xa][ya] = land_grass;
                              landpic[xa][ya] = land_grass + land_profile(xa, ya, 1);
                              ;
                              objects->refresh(xa, ya);
                           }
                     il_ok = true;
                  }
               }
            }
            else
            { // get_land_xx_yy
               bool ok;

               { // rnd
                  int s;

                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                  if (i_random(0, 100) > 50)
                     s = 1;
                  else
                     s = -1;
                  yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                  { // check_ok
                     ok = (!is_water_world && landscape[xx][yy] == land_grass
                           && landscape[xx + 1][yy] == land_grass
                           && landscape[xx + 1][yy + 1] == land_grass)
                          || (is_water_world && is_water(10000, xx, yy));
                     for (int i = 0; i < max_objects; i++)
                        if (!objects->is_free[i] && objects->type[i] == object_knight)
                           ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                  }
               }
               while (!ok)
               {
                  { // rnd
                     int s;

                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     xx = landscape_dx / 2 + i_random(10, landscape_dx / 2 - 5) * s;
                     if (i_random(0, 100) > 50)
                        s = 1;
                     else
                        s = -1;
                     yy = landscape_dy / 2 + i_random(10, landscape_dy / 2 - 5) * s;
                     { // check_ok
                        ok = (!is_water_world && landscape[xx][yy] == land_grass
                              && landscape[xx + 1][yy] == land_grass
                              && landscape[xx + 1][yy + 1] == land_grass)
                             || (is_water_world && is_water(10000, xx, yy));
                        for (int i = 0; i < max_objects; i++)
                           if (!objects->is_free[i] && objects->type[i] == object_knight)
                              ok &= (pdist(xx, yy, objects->x[i], objects->y[i]) > 25);
                     }
                  }
               }
            }
         }
         { // gen_on_land
            objects->create_worker(xx, yy, color, none);
            objects->create_worker(xx + 1, yy, color, none);
            objects->create_knight(xx + 1, yy + 1, color, none,
                                   is_self && !players[color_player[color]]->is_robot);
            players[color_player[color]]->focus(xx, yy);
         }
      };
   };
}

/*----------------------------------------------------------------------*/
/* Step timing, printed to stderr when CRAFT_PROFILE is set: every 200  */
/* steps, the average and longest time spent per step on player input   */
/* and drawing, and on the units, with the number of live objects.      */
/* CRAFT_PROFILE=fast also skips the sleep between steps.               */
/*----------------------------------------------------------------------*/

/* Milliseconds on a clock that never jumps (unlike the time of day). */
static double now_ms()
{
   struct timespec ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int prof_mode = -1; /* -1 unknown, 0 off, 1 on, 2 on and fast */
static double prof_cmds = 0;
static double prof_exec = 0;
static double prof_max = 0;
static int prof_steps = 0;
static long prof_total = 0;

/* Run steps back to back, with no pacing: CRAFT_PROFILE=fast, or a
   replay played back without a view. */
static bool unpaced = false;

static int profiling()
{
   if (prof_mode < 0)
   {
      const char *e = getenv("CRAFT_PROFILE");

      prof_mode = e == NULL ? 0 : strcmp(e, "fast") == 0 ? 2 : 1;
   }
   return prof_mode;
}

static void prof_step(double t_cmds, double t_exec, double t_all)
{
   prof_cmds += t_cmds;
   prof_exec += t_exec;
   if (t_all > prof_max)
      prof_max = t_all;
   prof_steps++;
   prof_total++;
   if (prof_steps == 200)
   {
      int alive = 0;

      for (int i = 0; i < max_objects; i++)
         if (!objects->is_free[i])
            alive++;
      int queued = 0;

      for (int i = 0; i < win::num_wins; i++)
         if (win::all_wins[i]->mydisplay != NULL)
            queued = i_max(queued, XQLength(win::all_wins[i]->mydisplay));
      fprintf(stderr,
              "craft: step %6ld  objects %4d  input+draw %6.2f ms  units %6.2f ms  max %6.1f ms"
              "  X queue %d\n",
              prof_total, alive, prof_cmds / prof_steps, prof_exec / prof_steps, prof_max, queued);
      prof_cmds = prof_exec = prof_max = 0;
      prof_steps = 0;
   }
}

static void usage(FILE *f)
{
   fprintf(f,
           "usage: craft [-scale N] [--record FILE | --replay FILE [--no-view]]\n"
              "             [--host | --join HOST --code CODE [--name NAME]] [--port N]\n"
           "  -scale N        zoom the game N times (1 to 4, e.g. 1.5) for this run only;\n"
           "                  without it, the zoom set on the start menu's Details page\n"
           "                  is used (by default chosen from the screen size)\n"
           "  --record FILE   record the game to FILE (by default every game is\n"
           "                  recorded to ~/.local/share/craft/replays/last.craftreplay;\n"
           "                  CRAFT_REPLAY=off turns that off)\n"
           "  --replay FILE   play a recorded game back, check that it plays out\n"
           "                  exactly as recorded, and print the result\n"
           "  --no-view       with --replay: play it back without any window\n"
           "                  (no X display needed)\n"
           "  --host          host a network game: every person seat on the start\n"
           "                  menu after your own is played by a guest\n"
           "  --port N        port to host on or join (default %d)\n"
           "  --join HOST     join the game hosted at HOST (or HOST:PORT)\n"
           "  --code CODE     the join code the host was given\n"
           "  --name NAME     your name in the game (default: your login)\n",
           net_default_port);
}

/* Ctrl-C or kill: finish the current step and end the game cleanly, so
   the replay is complete.  A second signal ends the program at once. */
static volatile sig_atomic_t stop_requested = 0;

static void on_stop_signal(int sig)
{
   if (stop_requested)
   {
      signal(sig, SIG_DFL);
      raise(sig);
   }
   stop_requested = 1;
}

int main(int argc, char **argv)
{
   double act_speed;
   const char *record_path = NULL;
   const char *replay_path = NULL;
   bool no_view = false;
   bool hosting = false;
   int port = net_default_port;
   const char *join_addr = NULL;
   const char *join_code = "";
   const char *join_name = getenv("USER") != NULL ? getenv("USER") : "guest";
   gzFile join_state = NULL;

   for (int i = 1; i < argc; i++)
   { // read_arguments
      if ((strcmp(argv[i], "-scale") == 0 || strcmp(argv[i], "--scale") == 0) && i + 1 < argc)
      {
         if (!zoom_override(atof(argv[++i])))
         {
            fprintf(stderr, "craft: -scale must be between 1 and 4\n");
            exit(2);
         }
      }
      else if (strncmp(argv[i], "--scale=", 8) == 0)
      {
         if (!zoom_override(atof(argv[i] + 8)))
         {
            fprintf(stderr, "craft: -scale must be between 1 and 4\n");
            exit(2);
         }
      }
      else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc)
         record_path = argv[++i];
      else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc)
         replay_path = argv[++i];
      else if (strcmp(argv[i], "--no-view") == 0)
         no_view = true;
      else if (strcmp(argv[i], "--host") == 0)
         hosting = true;
      else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
         port = atoi(argv[++i]);
      else if (strcmp(argv[i], "--join") == 0 && i + 1 < argc)
         join_addr = argv[++i];
      else if (strcmp(argv[i], "--code") == 0 && i + 1 < argc)
         join_code = argv[++i];
      else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc)
         join_name = argv[++i];
      else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
      {
         usage(stdout);
         exit(0);
      }
      else
      {
         usage(stderr);
         exit(2);
      }
   }

   { // start_message
      printf("CRAFT Version 3.5 (http://set.gmd.de/~hua) started\n");
   }
   { // init_sound
      sound_open();
      win::idle_hook = sound_tick;
      music_menu();
   }
   { // init_players
      int tic = 0;

      if (join_addr != NULL)
      { // join_a_network_game
         char addr[300];

         if (strchr(join_addr, ':') == NULL && port != net_default_port)
            snprintf(addr, sizeof(addr), "%s:%d", join_addr, port);
         else
            snprintf(addr, sizeof(addr), "%s", join_addr);
         join_state = net_join(addr, join_code, join_name);
         if (join_state == NULL || !restore_game_globals_from(join_state))
         {
            fprintf(stderr, "craft: could not join the game\n");
            exit(1);
         }
         restoring = true;
         /* Only our own side has a screen here. */
         for (int i = 0; i < max_players; i++)
            if (active[i] && strcmp(host[i], "-") != 0)
               strcpy(host[i], i == net_seat ? "" : "@");
      }
      else if (replay_path != NULL)
      { // open_replay
         if (!replay_open(replay_path))
            exit(1);
         restoring = true;
         /* Show every human side on this display, or (--no-view) none:
            "@" marks a person whose screen is elsewhere. */
         for (int i = 0; i < max_players; i++)
            if (active[i] && strcmp(host[i], "-") != 0)
               strcpy(host[i], no_view ? "@" : "");
         unpaced = no_view;
      }
      else
      {
         show_splash();
         { // get_params
            for (int i = 0; i < max_players; i++)
               active[i] = false;
            option_menu(act_speed, tic);
         }
         if (hosting)
         { // wait_for_guests
            int seats[max_players];
            int num_seats = 0;
            int robots = 0;
            bool mine = false;

            for (int i = 0; i < max_players; i++)
               if (active[i] && strcmp(host[i], "-") == 0)
                  robots++;
               else if (active[i] && !mine)
               {
                  mine = true;
                  net_seat = i;
                  strcpy(host[i], "");
               }
               else if (active[i])
                  seats[num_seats++] = i;
            if (robots > 1)
            {
               fprintf(stderr, "craft: a network game can have at most one computer player\n");
               exit(2);
            }
            if (!mine || num_seats == 0)
            {
               fprintf(stderr, "craft: to host, set up two or more people on the start menu "
                               "(you, then one seat per guest)\n");
               exit(2);
            }
            if (getenv("CRAFT_NET_DELAY") != NULL)
               net_delay = i_bound(1, atoi(getenv("CRAFT_NET_DELAY")), 12);
            if (!net_host_wait(port, seats, num_seats, name))
               exit(1);
            for (int k = 0; k < num_seats; k++)
               strcpy(host[seats[k]], "@");
         }
      }
      if (replay_path != NULL)
         printf("playing back replay \"%s\"\n", replay_path);
      else if (join_state != NULL)
         printf("joined the game as player %d\n", net_seat + 1);
      else if (restoring)
      { // load_options_and_land
         if (!restore_game_globals(restore_name))
         {
            fprintf(stderr, "craft: could not restore \"%s\"\n", restore_name);
            exit(1);
         }
         /* A game saved during network play: everyone plays here now. */
         for (int i = 0; i < max_players; i++)
            if (strcmp(host[i], "@") == 0)
               strcpy(host[i], "");
      }
      else
      {
         d_randomize(tic);
         init_speed(act_speed);
      }
      init_craft(host, name, speed);
      if (replay_path != NULL || join_state != NULL)
         ;
      else if (restoring)
         printf("restoring game \"%s\"\n", restore_name);
      else
         printf("starting game with seed %d\n", tic);
   }
   if (join_state != NULL)
   { // load_hosts_game
      if (!restore_game_objects_from(join_state))
      {
         fprintf(stderr, "craft: the host's game arrived damaged\n");
         exit(1);
      }
      gzclose(join_state);
      players[net_seat]->rebuild_sight();
   }
   else if (replay_path != NULL)
   { // load_replay_objects
      if (!replay_open_objects())
      {
         fprintf(stderr, "craft: the replay \"%s\" is damaged\n", replay_path);
         exit(1);
      }
   }
   else if (restoring)
   { // load_objects_and_players
      if (!restore_game_objects())
      {
         fprintf(stderr, "craft: the saved game \"%s\" is damaged\n", restore_name);
         exit(1);
      }
   }
   else
      set_initial_objects();
   if (replay_path == NULL)
   { // start_recording
      const char *e = getenv("CRAFT_REPLAY");

      if (record_path != NULL)
         replay_record_start(record_path);
      else if (e == NULL || strcmp(e, "off") != 0)
         replay_record_start(replay_default_path());
   }
   if (net_role == net_host && !net_host_start(ticker))
      exit(1);
   music_game();
   { // exec_craft
      bool quit = false;
      int cnt;
      int h = 0;

      { // set_cnt
         cnt = num_players;
      }
      signal(SIGINT, on_stop_signal);
      signal(SIGTERM, on_stop_signal);
      net_orders net_step[max_players]; /* network game: this step's orders */
      net_orders net_mine;              /* our orders not yet sent */
      int net_submitted = -1;           /* last step whose orders went out */

      net_mine.n = 0;
      while (!quit && !stop_requested)
      { // cycle
         int suspend = -1;
         double prof_t0 = profiling() ? now_ms() : 0;

         if (net_role == net_guest && !active[net_seat])
         { /* our side is out of the game: leave the others to it */
            quit = true;
            break;
         }
         if (net_role != net_off)
         { // network_lockstep
            /* Orders given now run net_delay steps later, on every
               computer at once; a step runs only when everyone's orders
               for it are here. */
            if (active[net_seat] && players[net_seat]->has_view)
            { // gather_my_orders
               net_orders o;
               bool gives_up = false;

               players[net_seat]->get_cmds(gives_up, o.n, o.cmd, o.id, o.x, o.y);
               if (gives_up && o.n < max_orders)
               {
                  o.cmd[o.n] = cmd_give_up;
                  o.id[o.n] = none;
                  o.x[o.n] = 0;
                  o.y[o.n] = 0;
                  o.n++;
               }
               for (int k = 0; k < o.n && net_mine.n < max_orders; k++, net_mine.n++)
               {
                  net_mine.cmd[net_mine.n] = o.cmd[k];
                  net_mine.id[net_mine.n] = o.id[k];
                  net_mine.x[net_mine.n] = o.x[k];
                  net_mine.y[net_mine.n] = o.y[k];
               }
            }
            if (net_submitted != ticker)
            {
               net_submit(ticker + net_delay, net_mine);
               net_mine.n = 0;
               net_submitted = ticker;
            }
            { // show_network_news
               const char *m = net_message();

               if (m != NULL && active[net_seat] && players[net_seat]->has_view)
                  players[net_seat]->write(m);
            }
            if (net_host_gone())
            { // the_game_is_over
               char msg[300];

               snprintf(msg, sizeof(msg), "%s",
                        net_status() != NULL ? net_status() : "The host has left the game");
               if (active[net_seat] && players[net_seat]->has_view && !stop_requested)
               { // tell_and_wait
                  win *w;
                  button *b;

                  tell(host[net_seat], players[net_seat]->w_land, w, msg);
                  b = new button(w, "OK", 120, 80);
                  while (!stop_requested)
                  {
                     w->mark_mouse();
                     w->tick();
                     if (b->eval())
                        break;
                     w->scratch_mouse();
                     delay(min_cycle_time);
                  }
                  delete b;
                  delete w;
               }
               quit = true;
               break;
            }
            if (!net_take_step(ticker, net_step))
            { // wait_for_the_others
               const char *st = net_status();

               if (st != NULL && active[net_seat] && players[net_seat]->has_view)
                  players[net_seat]->write(st);
               net_poll(4);
               continue;
            }
         }

         { // read_cmds
            for (int pi = 0; pi < max_players; pi++)
               if (active[pi] && !players[pi]->is_robot)
               { // read_player_cmd
                  int cmd[max_orders];
                  int x[max_orders];
                  int y[max_orders];
                  int id[max_orders];
                  int num = 0;
                  int suspend = -1;
                  bool is_suspend = false;

                  { // collect_orders
                     /* From this screen's panel, if the player sits here;
                        otherwise (and in a replay) from elsewhere. */
                     if (net_role != net_off)
                     { /* agreed by every computer for this step */
                        num = net_step[pi].n;
                        for (int k = 0; k < num; k++)
                        {
                           cmd[k] = net_step[pi].cmd[k];
                           id[k] = net_step[pi].id[k];
                           x[k] = net_step[pi].x[k];
                           y[k] = net_step[pi].y[k];
                        }
                     }
                     else if (players[pi]->has_view)
                        players[pi]->get_cmds(is_suspend, num, cmd, id, x, y);
                     if (is_suspend && num < max_orders)
                     {
                        cmd[num] = cmd_give_up;
                        id[num] = none;
                        x[num] = 0;
                        y[num] = 0;
                        num++;
                     }
                     if (replay_playing) /* the recording decides, not the viewer */
                        replay_take(ticker, pi, num, cmd, id, x, y);
                     else
                        replay_record_orders(ticker, pi, num, id, cmd, x, y);
                  }
                  { // perhaps_exec_player_cmd
                     bool any = false;

                     is_suspend = false;
                     for (int p = 0; p < num; p++)
                        if (cmd[p] == cmd_give_up)
                           is_suspend = true;
                        else
                        { // exec_player_cmd
                           objects->new_order(id[p], cmd[p], x[p], y[p]);
                           any = true;
                        }
                     if (any && players[pi]->has_view)
                        sfx_for(sfx_ack, player_color[pi]);
                  }
                  if (is_suspend)
                  {
                     suspend = pi;
                     { // check_over
                        int only = -1;

                        for (int i = 0; i < max_players; i++)
                           if (active[i]
                               && (players[i]->num_mans <= 0 || suspend == i
                                   || players[i]->master_dead))
                           { // handle_game_over
                              char msg[640];
                              win *w[max_players];
                              button *b[max_players] = {};
                              bool m_active[max_players];

                              if (players[i]->master_dead)
                                 snprintf(msg, sizeof(msg), "%s haes been killed", name[i]);
                              else if (players[i]->num_mans <= 0)
                                 snprintf(msg, sizeof(msg), "%s lost all his man", name[i]);
                              else if (suspend == i)
                                 snprintf(msg, sizeof(msg), "%s gives up", name[i]);
                              { // tell_to_players
                                 for (int j = 0; j < max_players; j++)
                                 {
                                    m_active[j] = false;
                                    if (active[j] && players[j]->has_view && !replay_playing)
                                    { // tell_to_player
                                       tell(host[j], players[j]->w_land, w[j], msg);
                                       b[j] = new button(w[j], "OK", 120, 80);
                                       m_active[j] = true;
                                    }
                                 }
                              }
                              { // ack_from_players
                                 bool any_active = true;

                                 while (any_active && !stop_requested)
                                 { // check_buttons
                                    any_active = false;
                                    for (int j = 0; j < max_players; j++)
                                       if (m_active[j])
                                       { // ack_from_player
                                          any_active = true;
                                          w[j]->mark_mouse();
                                          w[j]->tick();
                                          if (b[j]->eval())
                                          {
                                             delete (b[j]);
                                             delete (w[j]);
                                             m_active[j] = false;
                                          }
                                          else
                                             w[j]->scratch_mouse();
                                       }
                                 }
                              }
                              if (!players[i]->is_robot)
                                 num_humans--;
                              cnt--;
                              quit = (cnt <= 1 || num_humans < 1);
                              for (int o = 0; o < max_players; o++)
                                 if (active[o] && o != i && !players[o]->is_robot)
                                    only = o;
                              if (quit && only != -1)
                              {
                                 sprintf(msg, "%s wins", name[only]);
                                 music_game_over(true);
                                 { // tell_to_players
                                    for (int j = 0; j < max_players; j++)
                                    {
                                       m_active[j] = false;
                                       if (active[j] && players[j]->has_view && !replay_playing)
                                       { // tell_to_player
                                          tell(host[j], players[j]->w_land, w[j], msg);
                                          b[j] = new button(w[j], "OK", 120, 80);
                                          m_active[j] = true;
                                       }
                                    }
                                 }
                                 { // ack_from_players
                                    bool any_active = true;

                                    while (any_active && !stop_requested)
                                    { // check_buttons
                                       any_active = false;
                                       for (int j = 0; j < max_players; j++)
                                          if (m_active[j])
                                          { // ack_from_player
                                             any_active = true;
                                             w[j]->mark_mouse();
                                             w[j]->tick();
                                             if (b[j]->eval())
                                             {
                                                delete (b[j]);
                                                delete (w[j]);
                                                m_active[j] = false;
                                             }
                                             else
                                                w[j]->scratch_mouse();
                                          }
                                    }
                                 }
                              }
                              else if (quit && only == -1)
                              {
                                 sprintf(msg, "The computer wins, haha...");
                                 music_game_over(false);
                                 { // tell_to_players
                                    for (int j = 0; j < max_players; j++)
                                    {
                                       m_active[j] = false;
                                       if (active[j] && players[j]->has_view && !replay_playing)
                                       { // tell_to_player
                                          tell(host[j], players[j]->w_land, w[j], msg);
                                          b[j] = new button(w[j], "OK", 120, 80);
                                          m_active[j] = true;
                                       }
                                    }
                                 }
                                 { // ack_from_players
                                    bool any_active = true;

                                    while (any_active && !stop_requested)
                                    { // check_buttons
                                       any_active = false;
                                       for (int j = 0; j < max_players; j++)
                                          if (m_active[j])
                                          { // ack_from_player
                                             any_active = true;
                                             w[j]->mark_mouse();
                                             w[j]->tick();
                                             if (b[j]->eval())
                                             {
                                                delete (b[j]);
                                                delete (w[j]);
                                                m_active[j] = false;
                                             }
                                             else
                                                w[j]->scratch_mouse();
                                          }
                                    }
                                 }
                              }
                              players[i]->deactivate();
                              active[i] = false;
                           }
                     }
                  }
               }
         }
         if (h % 3 == 0)
         { // check_pause
            bool was = false;

            if (num_paused > 0)
               net_send_pause(true);
            while (num_paused > 0)
            {
               { // pause_a_bit
                  net_poll(0);
                  was = true;
                  for (int i = 0; i < max_players; i++)
                     if (active[i] && players[i]->has_view)
                        players[i]->handle_pause();
                  delay(min_cycle_time); /* don't spin a CPU core while paused */
               }
            }
            if (was)
               net_send_pause(false);
            if (was)
            { // pause_a_bit
               was = true;
               for (int i = 0; i < max_players; i++)
                  if (active[i] && players[i]->has_view)
                     players[i]->handle_pause();
            }
         }
         double t_cmds_done = profiling() ? now_ms() : 0;
         { // exec_objects
            for (int i = 0; i < 1; i++)
            {
               objects->exec();
               ticker++;
               replay_after_step(ticker);
               net_after_step(ticker);
            }
         }
         double t_exec_done = profiling() ? now_ms() : 0;
         { // check_over
            int only = -1;

            for (int i = 0; i < max_players; i++)
               if (active[i]
                   && (players[i]->num_mans <= 0 || suspend == i || players[i]->master_dead))
               { // handle_game_over
                  char msg[640];
                  win *w[max_players];
                  button *b[max_players] = {};
                  bool m_active[max_players];

                  if (players[i]->master_dead)
                     snprintf(msg, sizeof(msg), "%s haes been killed", name[i]);
                  else if (players[i]->num_mans <= 0)
                     snprintf(msg, sizeof(msg), "%s lost all his man", name[i]);
                  else if (suspend == i)
                     snprintf(msg, sizeof(msg), "%s gives up", name[i]);
                  { // tell_to_players
                     for (int j = 0; j < max_players; j++)
                     {
                        m_active[j] = false;
                        if (active[j] && players[j]->has_view && !replay_playing)
                        { // tell_to_player
                           tell(host[j], players[j]->w_land, w[j], msg);
                           b[j] = new button(w[j], "OK", 120, 80);
                           m_active[j] = true;
                        }
                     }
                  }
                  { // ack_from_players
                     bool any_active = true;

                     while (any_active && !stop_requested)
                     { // check_buttons
                        any_active = false;
                        for (int j = 0; j < max_players; j++)
                           if (m_active[j])
                           { // ack_from_player
                              any_active = true;
                              w[j]->mark_mouse();
                              w[j]->tick();
                              if (b[j]->eval())
                              {
                                 delete (b[j]);
                                 delete (w[j]);
                                 m_active[j] = false;
                              }
                              else
                                 w[j]->scratch_mouse();
                           }
                     }
                  }
                  if (!players[i]->is_robot)
                     num_humans--;
                  cnt--;
                  quit = (cnt <= 1 || num_humans < 1);
                  for (int o = 0; o < max_players; o++)
                     if (active[o] && o != i && !players[o]->is_robot)
                        only = o;
                  if (quit && only != -1)
                  {
                     sprintf(msg, "%s wins", name[only]);
                     music_game_over(true);
                     { // tell_to_players
                        for (int j = 0; j < max_players; j++)
                        {
                           m_active[j] = false;
                           if (active[j] && players[j]->has_view && !replay_playing)
                           { // tell_to_player
                              tell(host[j], players[j]->w_land, w[j], msg);
                              b[j] = new button(w[j], "OK", 120, 80);
                              m_active[j] = true;
                           }
                        }
                     }
                     { // ack_from_players
                        bool any_active = true;

                        while (any_active && !stop_requested)
                        { // check_buttons
                           any_active = false;
                           for (int j = 0; j < max_players; j++)
                              if (m_active[j])
                              { // ack_from_player
                                 any_active = true;
                                 w[j]->mark_mouse();
                                 w[j]->tick();
                                 if (b[j]->eval())
                                 {
                                    delete (b[j]);
                                    delete (w[j]);
                                    m_active[j] = false;
                                 }
                                 else
                                    w[j]->scratch_mouse();
                              }
                        }
                     }
                  }
                  else if (quit && only == -1)
                  {
                     sprintf(msg, "The computer wins, haha...");
                     music_game_over(false);
                     { // tell_to_players
                        for (int j = 0; j < max_players; j++)
                        {
                           m_active[j] = false;
                           if (active[j] && players[j]->has_view && !replay_playing)
                           { // tell_to_player
                              tell(host[j], players[j]->w_land, w[j], msg);
                              b[j] = new button(w[j], "OK", 120, 80);
                              m_active[j] = true;
                           }
                        }
                     }
                     { // ack_from_players
                        bool any_active = true;

                        while (any_active && !stop_requested)
                        { // check_buttons
                           any_active = false;
                           for (int j = 0; j < max_players; j++)
                              if (m_active[j])
                              { // ack_from_player
                                 any_active = true;
                                 w[j]->mark_mouse();
                                 w[j]->tick();
                                 if (b[j]->eval())
                                 {
                                    delete (b[j]);
                                    delete (w[j]);
                                    m_active[j] = false;
                                 }
                                 else
                                    w[j]->scratch_mouse();
                              }
                        }
                     }
                  }
                  players[i]->deactivate();
                  active[i] = false;
               }
         }
         if (profiling())
            prof_step(t_cmds_done - prof_t0, t_exec_done - t_cmds_done, now_ms() - prof_t0);
         { // perhaps_delay
            /* Keep a fixed schedule: each step has its own slot, 1/pace
               seconds after the last, so the pace stays exact however
               long a step takes (as long as it fits). After a long stop
               (pause, a dialog) start a new schedule instead of racing
               to catch up. */
            static double next_step = 0;
            double period = 1000.0 / game_pace;
            double now = now_ms();

            if (next_step == 0 || now - next_step > 4 * period)
               next_step = now;
            next_step += period;
            if (next_step > now && profiling() != 2 && !unpaced)
            {
               struct timespec ts;
               double wait = next_step - now;

               ts.tv_sec = (time_t)(wait / 1000);
               ts.tv_nsec = (long)((wait - ts.tv_sec * 1000.0) * 1e6);
               nanosleep(&ts, NULL);
            }
         }
      }
   }
   net_leave(stop_requested ? "quit" : "game over");
   replay_record_close();
   replay_playback_end(ticker);
   finish_craft();
   if (!stop_requested)
      show_credits();
}
