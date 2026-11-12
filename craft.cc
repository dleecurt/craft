/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from craft.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
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
#include "ship.h"
#include "option_menu.h"
#include "xtimer.h"
#include "screens.h"
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
               players[i]
                   = new player(i, name[i], host[i], red, p_speed[i], (strcmp(host[i], "-") == 0));
            if (i == 1)
               players[i]
                   = new player(i, name[i], host[i], blue, p_speed[i], (strcmp(host[i], "-") == 0));
            if (i == 2)
               players[i] = new player(i, name[i], host[i], yellow, p_speed[i],
                                       (strcmp(host[i], "-") == 0));
            if (i == 3)
               players[i]
                   = new player(i, name[i], host[i], cyan, p_speed[i], (strcmp(host[i], "-") == 0));
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

int main()
{
   double act_speed;

   { // start_message
      printf("CRAFT Version 3.5 (http://set.gmd.de/~hua) started\n");
   }
   { // init_players
      int tic;

      show_splash();
      { // get_params
         for (int i = 0; i < max_players; i++)
            active[i] = false;
         option_menu(act_speed, tic);
      }
      if (restoring)
      { // load_options_and_land
         if (!restore_game_globals(restore_name))
         {
            fprintf(stderr, "craft: could not restore \"%s\"\n", restore_name);
            exit(1);
         }
      }
      else
      {
         d_randomize(tic);
         init_speed(act_speed);
      }
      init_craft(host, name, speed);
      if (restoring)
         printf("restoring game \"%s\"\n", restore_name);
      else
         printf("starting game with seed %d\n", tic);
   }
   if (restoring)
   { // load_objects_and_players
      if (!restore_game_objects())
      {
         fprintf(stderr, "craft: the saved game \"%s\" is damaged\n", restore_name);
         exit(1);
      }
   }
   else
      set_initial_objects();
   { // exec_craft
      bool quit = false;
      int cnt;
      int h = 0;

      { // set_cnt
         cnt = num_players;
      }
      while (!quit)
      { // cycle
         int suspend = -1;
         double cycle_start_time = x_sys_time();

         { // read_cmds
            for (int pi = 0; pi < max_players; pi++)
               if (active[pi] && !players[pi]->is_robot)
               { // read_player_cmd
                  int cmd[max_marked];
                  int x[max_marked];
                  int y[max_marked];
                  int id[max_marked];
                  int num;
                  int suspend = -1;
                  bool is_suspend = false;

                  players[pi]->get_cmds(is_suspend, num, cmd, id, x, y);
                  { // perhaps_exec_player_cmd
                     for (int p = 0; p < num; p++)
                     { // exec_player_cmd
                        objects->new_order(id[p], cmd[p], x[p], y[p]);
                     }
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
                                    if (active[j] && !players[j]->is_robot)
                                    { // tell_to_player
                                       tell(host[j], players[j]->w_land, w[j], msg);
                                       b[j] = new button(w[j], "OK", 120, 80);
                                       m_active[j] = true;
                                    }
                                 }
                              }
                              { // ack_from_players
                                 bool any_active = true;

                                 while (any_active)
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
                                 { // tell_to_players
                                    for (int j = 0; j < max_players; j++)
                                    {
                                       m_active[j] = false;
                                       if (active[j] && !players[j]->is_robot)
                                       { // tell_to_player
                                          tell(host[j], players[j]->w_land, w[j], msg);
                                          b[j] = new button(w[j], "OK", 120, 80);
                                          m_active[j] = true;
                                       }
                                    }
                                 }
                                 { // ack_from_players
                                    bool any_active = true;

                                    while (any_active)
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
                                 { // tell_to_players
                                    for (int j = 0; j < max_players; j++)
                                    {
                                       m_active[j] = false;
                                       if (active[j] && !players[j]->is_robot)
                                       { // tell_to_player
                                          tell(host[j], players[j]->w_land, w[j], msg);
                                          b[j] = new button(w[j], "OK", 120, 80);
                                          m_active[j] = true;
                                       }
                                    }
                                 }
                                 { // ack_from_players
                                    bool any_active = true;

                                    while (any_active)
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

            while (num_paused > 0)
            {
               { // pause_a_bit
                  was = true;
                  for (int i = 0; i < max_players; i++)
                     if (active[i] && !players[i]->is_robot)
                        players[i]->handle_pause();
                  delay(min_cycle_time); /* don't spin a CPU core while paused */
               }
            }
            if (was)
            { // pause_a_bit
               was = true;
               for (int i = 0; i < max_players; i++)
                  if (active[i] && !players[i]->is_robot)
                     players[i]->handle_pause();
            }
         }
         { // exec_objects
            for (int i = 0; i < 1; i++)
            {
               objects->exec();
               ticker++;
            }
         }
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
                        if (active[j] && !players[j]->is_robot)
                        { // tell_to_player
                           tell(host[j], players[j]->w_land, w[j], msg);
                           b[j] = new button(w[j], "OK", 120, 80);
                           m_active[j] = true;
                        }
                     }
                  }
                  { // ack_from_players
                     bool any_active = true;

                     while (any_active)
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
                     { // tell_to_players
                        for (int j = 0; j < max_players; j++)
                        {
                           m_active[j] = false;
                           if (active[j] && !players[j]->is_robot)
                           { // tell_to_player
                              tell(host[j], players[j]->w_land, w[j], msg);
                              b[j] = new button(w[j], "OK", 120, 80);
                              m_active[j] = true;
                           }
                        }
                     }
                     { // ack_from_players
                        bool any_active = true;

                        while (any_active)
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
                     { // tell_to_players
                        for (int j = 0; j < max_players; j++)
                        {
                           m_active[j] = false;
                           if (active[j] && !players[j]->is_robot)
                           { // tell_to_player
                              tell(host[j], players[j]->w_land, w[j], msg);
                              b[j] = new button(w[j], "OK", 120, 80);
                              m_active[j] = true;
                           }
                        }
                     }
                     { // ack_from_players
                        bool any_active = true;

                        while (any_active)
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
         { // perhaps_delay
            int rest_dt = (int)(1000 / game_pace - (x_sys_time() - cycle_start_time));

            if (rest_dt > 0)
            {
               delay(rest_dt);
            }
         }
      }
   }
   finish_craft();
   show_credits();
}
