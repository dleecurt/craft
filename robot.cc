/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from robot.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- robot.cc robot.hc
 */

#include "robot.h"
#include "building.h"
#include "craft_def.h"
#include "craft.h"
#include "player.h"
#include "dir.h"
#include "errorhandling.h"
#include "field.h"
#include "ship.h"
#include "xtimer.h"

#define worker_limit 4

void wcheck(int &d, int x0, int y0, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
      d = i_max(d, i_abs(landhight[x][y] - landhight[x0][y0]));
}

int wdh(int x, int y)
{
   int r = 0;

   wcheck(r, x, y, x - 1, y);
   wcheck(r, x, y, x + 1, y);
   wcheck(r, x, y, x, y - 1);
   wcheck(r, x, y, x, y + 1);
   return r;
}

void xwcheck(int &d, int x0, int y0, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
      d = i_max(d, landhight[x][y] - landhight[x0][y0]);
}

int xwdh(int x, int y)
{
   int r = 0;

   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
   {
      xwcheck(r, x, y, x - 1, y);
      xwcheck(r, x, y, x + 1, y);
      xwcheck(r, x, y, x, y - 1);
      xwcheck(r, x, y, x, y + 1);
   }
   return r;
}

robot::robot(int my_color, int my_pno)
{
   { // store_params
      cnt = 0;
      color = my_color;
      pno = my_pno;
      o = 0;
      h_mine[0] = -1;
      h_mine[1] = -1;
      h_cnt[0] = -1;
      h_cnt[1] = -1;
   }
   { // init_status
      num_workers = 0;
      num_pawns = 0;
      num_lumber_jacks = 0;
      num_miners = 0;
      num_building_sites = 0;
      num_attack = 0;
      num_hasar = 0;
      num_ships = 0;
      num_knights = 0;
      num_archers = 0;
      num_traders = 0;
      num_catas = 0;
      num_doktors = 0;
      num_running = 0;
      should_construct = false;
      should_to_ship = false;
      just_finished = -1;
      num_heapies_required = 0;
      num_heap_run = 0;
      num_trapies_required = 0;
      num_trap_run = 0;
      tnr = 0;
      need_a_healer = -1;
      need_a_captain = -1;
      last_ilf_update = -2000;
      x_market = 0;
      y_market = 0;
      is_first = true;
      for (int i = 0; i < max_objects; i++)
      {
         doing[i] = doing_nothing;
      }
   }
   { // init_f
      f = new field();
      f_init = false;
      pf = new field();
      ppf = new field();
      pf->set_zero();
      ppf->set_zero();
      x_forti = 0;
      y_forti = 0;
      urgent = -1;
      urgent_1 = -1;
      ilf = NULL; /* was left uninitialised on land worlds */
      if (is_water_world)
         ilf = new ilfield();
   }
   { // get_tps
      ntp = 0;
      for (int x = 0; x < landscape_dx && ntp < 30; x++)
         for (int y = 0; y < landscape_dy && ntp < 30; y++)
            if (landscape[x][y] == land_t_wood || landscape[x][y] == land_t_gold)
            {
               xtp[ntp] = x;
               ytp[ntp] = y;
               ntp++;
            }
   }
}

robot::~robot()
{
   if (ilf)
   {
      delete ilf;
   }
   delete ppf;
   delete pf;
   delete f;
}

int other_home(int h)
{
   if (h == 0)
      return 1;
   return 0;
}

void robot::eval()
{
   int exec_cnt = 0;
   bool worker_set_idle = false;

   if ((ticker % 15) == 0)
   { // perform_a_bit
      if ((ticker % 200) == 0 || !f_init)
      {    // get_f
         { // get_ff
            f->init_man(red);
            f->expand(1000, red);
            if ((f->a_x[0] != -1 && f->f[f->a_x[0]][f->a_y[0]] < 89)
                && (num_knights + num_archers + num_catas + num_pawns) > 2)
            { // start_an_attack
               num_hasar
                   = i_random(1, i_min(5, (num_knights + num_archers + num_catas + num_pawns) / 2));
            }
         }
         if (!f_init)
         { // get_ilf
            if (is_water_world)
               ilf->update(color);
         }
         f_init = true;
      }
      if ((ticker % 60) == 0)
      { // get_ilf
         if (is_water_world)
            ilf->update(color);
      }
      /* up to 200 tries for one action (this also stopped after 50 ms,
         which made the computer player depend on the machine's speed) */
      for (int ct = 0; ct < 200 && exec_cnt < 1; ct++)
      {
         { // cycle
            cnt++;
            objects->fresh_pot = true;
            if (o == 0)
            { // init
               n_workers = 0;
               n_pawns = 0;
               n_lumber_jacks = 0;
               n_miners = 0;
               n_building_sites = 0;
               n_ships = 0;
               n_knights = 0;
               n_archers = 0;
               n_traders = 0;
               n_catas = 0;
               n_doktors = 0;
               n_attack = 0;
               n_running = 0;
               worker_set_idle = false;
               { // handle_heap_runs
                  for (int j = 0; j < num_heap_run; j++)
                  { // check_heap
                     int cc = heap_run_worker[j];

                     if ((objects->cmd[cc][0] != cmd_heap && objects->cmd[cc][1] != cmd_heap
                          && objects->cmd[cc][2] != cmd_heap && objects->cmd[cc][0] != cmd_dig
                          && objects->cmd[cc][1] != cmd_dig && objects->cmd[cc][2] != cmd_dig))
                     { // pop_heap
                        for (int l = j; l < num_heap_run - 1; l++)
                        {
                           heap_run_x[l] = heap_run_x[l + 1];
                           heap_run_y[l] = heap_run_y[l + 1];
                           heap_run_worker[l] = heap_run_worker[l + 1];
                        }
                        num_heap_run--;
                        j--;
                     }
                  }
               }
               { // handle_trap_runs
                  for (int j = 0; j < num_trap_run; j++)
                  { // check_trap
                     int cc = trap_run_worker[j];

                     if ((objects->cmd[cc][0] != cmd_dig && objects->cmd[cc][1] != cmd_dig
                          && objects->cmd[cc][2] != cmd_dig && objects->cmd[cc][0] != cmd_dig_trap
                          && objects->cmd[cc][1] != cmd_dig_trap
                          && objects->cmd[cc][2] != cmd_dig_trap))
                     { // pop_trap
                        for (int l = j; l < num_trap_run - 1; l++)
                        {
                           trap_run_x[l] = trap_run_x[l + 1];
                           trap_run_y[l] = trap_run_y[l + 1];
                           trap_run_worker[l] = trap_run_worker[l + 1];
                        }
                        num_trap_run--;
                        j--;
                     }
                  }
               }
               { // handle_just_finished
                  if (just_finished != -1 && !is_water_world && !objects->is_free[just_finished]
                      && objects->type[just_finished] != object_home
                      && objects->type[just_finished] != object_market)
                  { // gen_new_traps
                     for (int i = 0; i < 5; i++)
                     { // try_trap
                        int xt = i_bound(0, objects->x[just_finished] + i_random(-10, 10),
                                         landscape_dx - 1);
                        int yt = i_bound(0, objects->y[just_finished] + i_random(-10, 10),
                                         landscape_dy - 1);

                        if (landscape[xt][yt] == land_grass && num_trapies_required < 20)
                        {
                           trap_x[num_trapies_required] = xt;
                           trap_y[num_trapies_required] = yt;
                           num_trapies_required++;
                        }
                     }
                  }
                  just_finished = -1;
               }
            }
            { // exec_normal
               if (!objects->is_free[o] && objects->color[o] == color)
               { // exec_object
                  exec_cnt++;
                  if (objects->cmd[o][0] == cmd_idle)
                  { // gen_new_order
                     switch (objects->type[o])
                     {
                     case object_worker:
                     { // cmd_worker
                        if (objects->on_ship[o] == -1)
                        { // gen_a_new_worker_cmd
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (need_a_captain != -1 && crew_worker > 0
                                   && ilf->f[objects->x[o]][objects->y[o]] == captain_isle
                                   && ilf->num_worker[ilf->f[objects->x[o]][objects->y[o]]]
                                          > ilf->num_homes[ilf->f[objects->x[o]][objects->y[o]]]
                                                * 2))
                           {
                              worker_to_ship(o);
                           };
                           if ((objects->cmd[o][0] == cmd_idle) && (num_heapies_required > 0))
                           {
                              { // gen_heapie
                                 num_heapies_required--;
                                 if (can_walk_to(5, o, heap_x[num_heapies_required],
                                                 heap_y[num_heapies_required], 1, false))
                                 { // handle_heap_request
                                    switch (landscape[heap_x[num_heapies_required]]
                                                     [heap_y[num_heapies_required]])
                                    {
                                    case land_water:
                                    { // cross_river
                                       int dxx;
                                       int dyy;
                                       int length;

                                       { // look_for_river
                                          int dxb = heap_x[num_heapies_required] - objects->x[o];
                                          int dyb = heap_y[num_heapies_required] - objects->y[o];

                                          length = INT_MAX;
                                          if (dyb > 0)
                                          { // check_down
                                             int l = 0;
                                             int x;
                                             int y;

                                             x = heap_x[num_heapies_required];
                                             for (y = heap_y[num_heapies_required];
                                                  y < landscape_dy
                                                  && (landscape[x][y] == land_water);
                                                  y++)
                                                l++;
                                             if (l < length)
                                             {
                                                length = l;
                                                dxx = 0;
                                                dyy = 1;
                                             }
                                          }
                                          else
                                          { // check_up
                                             int l = 0;
                                             int x;
                                             int y;

                                             x = heap_x[num_heapies_required];
                                             for (y = heap_y[num_heapies_required];
                                                  y > 0 && (landscape[x][y] == land_water); y--)
                                                l++;
                                             if (l < length)
                                             {
                                                length = l;
                                                dxx = 0;
                                                dyy = -1;
                                             }
                                          }
                                          if (dxb > 0)
                                          { // check_right
                                             int l = 0;
                                             int x;
                                             int y;

                                             y = heap_y[num_heapies_required];
                                             for (x = heap_x[num_heapies_required];
                                                  x < landscape_dx
                                                  && (landscape[x][y] == land_water);
                                                  x++)
                                                l++;
                                             if (l < length)
                                             {
                                                length = l;
                                                dxx = 1;
                                                dyy = 0;
                                             }
                                          }
                                          else
                                          { // check_left
                                             int l = 0;
                                             int x;
                                             int y;

                                             y = heap_y[num_heapies_required];
                                             for (x = heap_x[num_heapies_required];
                                                  x > 0 && (landscape[x][y] == land_water); x--)
                                                l++;
                                             if (l < length)
                                             {
                                                length = l;
                                                dxx = -1;
                                                dyy = 0;
                                             }
                                          }
                                       }
                                       { // start_bridge
                                          for (int i = length - 1; i >= 0; i--)
                                          {
                                             objects->push_order(
                                                 o, cmd_heap,
                                                 heap_x[num_heapies_required] + i * dxx,
                                                 heap_y[num_heapies_required] + i * dyy);
                                          }
                                          objects->harvest_type[o] = harvest_heap;
                                       }
                                    };
                                    break;
                                    case land_wall:
                                    { // heap_at_wall
                                       bool no_archer;

                                       { // check_no_archer
                                          no_archer = true;
                                          for (int i = 0; i < max_objects && no_archer; i++)
                                          { // check_obj
                                             int xmi = i_max(0, heap_x[num_heapies_required]
                                                                    - scope_cata);
                                             int xma
                                                 = i_min(landscape_dx - 1,
                                                         heap_x[num_heapies_required] + scope_cata);
                                             int ymi = i_max(0, heap_y[num_heapies_required]
                                                                    - scope_cata);
                                             int yma
                                                 = i_min(landscape_dy - 1,
                                                         heap_y[num_heapies_required] + scope_cata);

                                             for (int xx = xmi; xx <= xma && no_archer; xx++)
                                                for (int yy = ymi; yy <= yma && no_archer; yy++)
                                                { // check_fa
                                                   int u = unit[xx][yy];

                                                   if (u != none && objects->color[u] != color
                                                       && (objects->type[u] == object_archer
                                                           || objects->type[u] == object_cata))
                                                   {
                                                      no_archer = false;
                                                      break;
                                                   }
                                                }
                                          }
                                       }
                                       if (no_archer)
                                       { // do_heap_at_wall
                                          objects->push_order(o, cmd_dig,
                                                              heap_x[num_heapies_required],
                                                              heap_y[num_heapies_required]);
                                          objects->harvest_type[o] = harvest_dig;
                                       }
                                    };
                                    break;
                                    }
                                    heap_run_x[num_heap_run] = heap_x[num_heapies_required];
                                    heap_run_y[num_heap_run] = heap_y[num_heapies_required];
                                    heap_run_worker[num_heap_run] = o;
                                    num_heap_run++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls
                                       + players[pno]->town_hall_in_progress
                                   < 2))
                           {
                              { // gen_town_hall
                                 int x;
                                 int y;
                                 bool any_gold;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 clean_mid(o);
                                 { // get_th_gold_place
                                    int best_f = INT_MAX;
                                    double best_dist = DBL_MAX;
                                    ;
                                    int best_x;
                                    int best_y;

                                    any_gold = false;
                                    for (int p = 0; p < max_objects; p++)
                                    {
                                       int x = objects->x[p];
                                       int y = objects->y[p];

                                       if ((!objects->is_free[p] && objects->type[p] == object_mine)
                                           && ((tnr >= 2 && f->f[x][y] < best_f)
                                               || (tnr < 2
                                                   && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                          < best_dist))
                                           && p != h_mine[0] && p != h_mine[1] && pf->f[x][y] == 0
                                           && can_walk_to(14, o, x, y, 1))
                                       { // grab_mine
                                          best_x = x;
                                          best_y = y;
                                          best_dist = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                          any_gold = true;
                                       }
                                    }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 cx = x;
                                 cy = y;
                                 if (any_gold && unit[cx][cy] != -1)
                                 { // try_to_built_a_home
                                    bool any_place;

                                    { // get_t_place
                                       int best_x;
                                       int best_y;
                                       double best_dist = DBL_MAX;

                                       any_place = false;
                                       /* Every square within 15 of the centre, nearest valid one wins. This
                                          used to try random squares until 100 ms had passed: about two
                                          million tries now, a stall of one tenth of a second, and a result
                                          that depended on the machine's speed. */
                                       for (x = i_max(0, cx - 15);
                                            x <= i_min(landscape_dx - 1, cx + 15); x++)
                                          for (y = i_max(0, cy - 15);
                                               y <= i_min(landscape_dy - 1, cy + 15); y++)
                                          {    // try_t_field
                                             { // check_t_field
                                                if (robo_can_built(o, x, y)
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           < best_dist
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           > 0
                                                    && sqrt(((cx - x) * (cx - x)
                                                             + (cy - y) * (cy - y)))
                                                           >= 2
                                                    && pf->f[x][y] == 0
                                                    && can_walk_to(10, o, x, y, 1))
                                                {
                                                   best_dist = ((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y));
                                                   best_x = x;
                                                   best_y = y;
                                                   any_place = true;
                                                }
                                             }
                                          }
                                       /*
      for (x = xmin; x < xmax; x+= 1)
        for (y = ymin; y < ymax; y+= 1)
          check_t_field;
*/
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_home, x, y);
                                       doing[o] = doing_nothing;
                                       tnr++;
                                       set_mid(o, x, y, unit[cx][cy]);
                                    }
                                 }
                                 else if (players[pno]->num_town_halls > 0)
                                 { // gen_harvester
                                    if ((objects->home_id[o] >= max_objects
                                         || objects->home_id[o] == none
                                         || objects->type[objects->home_id[o]] != object_home
                                         || objects->color[objects->home_id[o]] != color
                                         || sqrt((objects->x[o] - objects->x[objects->home_id[o]])
                                                     * (objects->x[o]
                                                        - objects->x[objects->home_id[o]])
                                                 + (objects->y[o] - objects->y[objects->home_id[o]])
                                                       * (objects->y[o]
                                                          - objects->y[objects->home_id[o]]))
                                                > 10))
                                    { // get_new_home
                                       if (h_mine[0] != -1 && h_mine[1] != -1)
                                       { // choose_best
                                          if (i_random(0, 100) > 50
                                              && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                          }
                                          else if (can_walk_to(12, o, h_x[1], h_y[1], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                          }
                                       }
                                       else if (h_mine[0] != -1
                                                && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                       }
                                       else if (h_mine[1] != -1
                                                && can_walk_to(12, o, h_x[1], h_y[1], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                       }
                                    }
                                    else
                                    { // start_harvest
                                       if ((num_miners > 0
                                            && (double)num_lumber_jacks / (double)num_miners < 0.4)
                                           || (num_traders > 3 && num_miners > 3))
                                       { // do_harvest_wood
                                          int x;
                                          int y;
                                          bool any_wood;

                                          { // get_wood_place
                                             double best_dist = DBL_MAX;
                                             int best_x;
                                             int best_y;
                                             int cx = objects->x[o];
                                             int cy = objects->y[o];

                                             any_wood = false;
                                             for (int xw = i_max(0, cx - 15);
                                                  xw <= i_min(landscape_dx, cx + 15); xw += 2)
                                                for (int yw = i_max(0, cy - 15);
                                                     yw <= i_min(landscape_dy, cy + 15); yw += 2)
                                                   if ((land_properties[landscape[xw][yw]]
                                                            .is_forest)
                                                       && ((cx - xw) * (cx - xw)
                                                           + (cy - yw) * (cy - yw))
                                                              < best_dist
                                                       && can_walk_to(13, o, xw, yw, 1))
                                                   { // grab_wood
                                                      best_x = xw;
                                                      best_y = yw;
                                                      best_dist = ((cx - xw) * (cx - xw)
                                                                   + (cy - yw) * (cy - yw));
                                                      any_wood = true;
                                                   }
                                             x = best_x;
                                             y = best_y;
                                          }
                                          if (any_wood)
                                             objects->new_order(o, cmd_harvest, x, y);
                                          else
                                          { // walk_somewhere
                                             int xx = objects->x[o];
                                             int yy = objects->y[o];
                                             int x = i_bound(0, xx + i_random(-10, 10),
                                                             landscape_dx - 1);
                                             int y = i_bound(0, yy + i_random(-10, 10),
                                                             landscape_dy - 1);

                                             if (can_walk_to(8, o, x, y, 0))
                                             {
                                                if (i_random(1, 100) > 10)
                                                   objects->new_order(o, cmd_move_to, x, y);
                                                else
                                                   objects->new_order(o, cmd_dig_trap, x, y);
                                             }
                                          }
                                       }
                                       else
                                       { // do_harvest_gold
                                          int x;
                                          int y;
                                          bool any_gold;
                                          int cx = objects->x[o];
                                          int cy = objects->y[o];

                                          { // get_gold_place
                                             double best_dist = DBL_MAX;
                                             int best_x;
                                             int best_y;

                                             any_gold = false;
                                             for (int p = 0; p < max_objects; p++)
                                             {
                                                int x = objects->x[p];
                                                int y = objects->y[p];

                                                if ((!objects->is_free[p]
                                                     && objects->type[p] == object_mine)
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           < best_dist
                                                    && can_walk_to(15, o, x, y, 1))
                                                { // grab_mine
                                                   best_x = x;
                                                   best_y = y;
                                                   best_dist = ((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y));
                                                   any_gold = true;
                                                }
                                             }
                                             x = best_x;
                                             y = best_y;
                                          }
                                          if (any_gold)
                                             objects->new_order(o, cmd_harvest, x, y);
                                          else
                                          { // walk_somewhere
                                             int xx = objects->x[o];
                                             int yy = objects->y[o];
                                             int x = i_bound(0, xx + i_random(-10, 10),
                                                             landscape_dx - 1);
                                             int y = i_bound(0, yy + i_random(-10, 10),
                                                             landscape_dy - 1);

                                             if (can_walk_to(8, o, x, y, 0))
                                             {
                                                if (i_random(1, 100) > 10)
                                                   objects->new_order(o, cmd_move_to, x, y);
                                                else
                                                   objects->new_order(o, cmd_dig_trap, x, y);
                                             }
                                          }
                                       }
                                    }
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                   && (players[pno]->num_farms < players[pno]->num_mans / 20 + 1
                                       || players[pno]->food < 400)
                                   && players[pno]->money > price_farm
                                   && players[pno]->wood > wood_farm))
                           {
                              { // gen_farm
                                 int x;
                                 int y;
                                 bool any_place;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 { // get_th_place
                                    int best_x;
                                    int best_y;
                                    double best_pf = DBL_MAX;
                                    double best_dist = DBL_MAX;
                                    int best_f = INT_MAX;

                                    any_place = false;
                                    for (x = i_max(5, cx - 15);
                                         x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                       for (y = i_max(5, cy - 15);
                                            y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                       { // check_th_field
                                          if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                              && (((cx - x) * (cx - x) + (cy - y) * (cy - y)) >= 3
                                                  && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         < best_dist
                                                  && f->f[x][y] <= best_f)
                                              && can_walk_to(9, o, x, y, 1))
                                          {
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             best_pf = pf->f[x][y];
                                             best_x = x;
                                             best_y = y;
                                             any_place = true;
                                          }
                                       }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 if (any_place)
                                 {
                                    objects->new_order(o, cmd_built_farm, x, y);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (is_water_world && players[pno]->num_docks < 1
                                   && num_building_sites == 0))
                           {
                              { // gen_docks
                                 int xd = objects->x[o];
                                 int yd = objects->y[o];

                                 if (look_for_dock_place(xd, yd))
                                 {
                                    objects->new_order(o, cmd_built_docks, xd, yd);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && players[pno]->num_mills > 0
                                   && players[pno]->num_camps > 0 && players[pno]->num_unis > 0
                                   && num_building_sites == 0 && players[pno]->num_markets < 1
                                   && players[pno]->market_in_progress == 0
                                   && players[pno]->money > price_market
                                   && players[pno]->wood > wood_market))
                           {
                              { // gen_market
                                 int x = 0;
                                 int y = 0;
                                 bool any_place = false;

                                 { // get_gold_tp
                                    int best_pot = INT_MAX;

                                    for (int i = 0; i < ntp; i++)
                                       if (landscape[xtp[i]][ytp[i]] == land_t_gold
                                           && f->f[xtp[i]][ytp[i]] < best_pot)
                                       { // grab_gold_place
                                          best_pot = f->f[xtp[i]][ytp[i]];
                                          gold_tp = i;
                                       }
                                 }
                                 { // get_wood_tp
                                    int best_pot = INT_MAX;

                                    for (int i = 0; i < ntp; i++)
                                       if (landscape[xtp[i]][ytp[i]] == land_t_wood
                                           && f->f[xtp[i]][ytp[i]] < best_pot)
                                       { // grab_wood_place
                                          best_pot = f->f[xtp[i]][ytp[i]];
                                          wood_tp = i;
                                       }
                                 }
                                 { // get_market_place
                                    int cx = xtp[gold_tp];
                                    int cy = ytp[gold_tp];

                                    if (can_walk_to(7, o, cx, cy, 1))
                                    { // get_t_place
                                       int best_x;
                                       int best_y;
                                       double best_dist = DBL_MAX;

                                       any_place = false;
                                       /* Every square within 15 of the centre, nearest valid one wins. This
                                          used to try random squares until 100 ms had passed: about two
                                          million tries now, a stall of one tenth of a second, and a result
                                          that depended on the machine's speed. */
                                       for (x = i_max(0, cx - 15);
                                            x <= i_min(landscape_dx - 1, cx + 15); x++)
                                          for (y = i_max(0, cy - 15);
                                               y <= i_min(landscape_dy - 1, cy + 15); y++)
                                          {    // try_t_field
                                             { // check_t_field
                                                if (robo_can_built(o, x, y)
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           < best_dist
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           > 0
                                                    && sqrt(((cx - x) * (cx - x)
                                                             + (cy - y) * (cy - y)))
                                                           >= 2
                                                    && pf->f[x][y] == 0
                                                    && can_walk_to(10, o, x, y, 1))
                                                {
                                                   best_dist = ((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y));
                                                   best_x = x;
                                                   best_y = y;
                                                   any_place = true;
                                                }
                                             }
                                          }
                                       /*
      for (x = xmin; x < xmax; x+= 1)
        for (y = ymin; y < ymax; y+= 1)
          check_t_field;
*/
                                       x = best_x;
                                       y = best_y;
                                    }
                                 }
                                 if (any_place)
                                    objects->new_order(o, cmd_built_market, x, y);
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                   && (is_water_world
                                       || players[pno]->num_unis <= players[pno]->num_smiths
                                       || players[pno]->num_unis <= players[pno]->num_mills)
                                   && players[pno]->num_camps > 0 && players[pno]->num_unis < 2
                                   && players[pno]->money > price_uni
                                   && players[pno]->wood > wood_uni))
                           {
                              { // gen_uni
                                 int x;
                                 int y;
                                 bool any_place;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 { // get_th_place
                                    int best_x;
                                    int best_y;
                                    double best_pf = DBL_MAX;
                                    double best_dist = DBL_MAX;
                                    int best_f = INT_MAX;

                                    any_place = false;
                                    for (x = i_max(5, cx - 15);
                                         x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                       for (y = i_max(5, cy - 15);
                                            y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                       { // check_th_field
                                          if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                              && (((cx - x) * (cx - x) + (cy - y) * (cy - y)) >= 3
                                                  && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         < best_dist
                                                  && f->f[x][y] <= best_f)
                                              && can_walk_to(9, o, x, y, 1))
                                          {
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             best_pf = pf->f[x][y];
                                             best_x = x;
                                             best_y = y;
                                             any_place = true;
                                          }
                                       }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 if (any_place)
                                 {
                                    objects->new_order(o, cmd_built_uni, x, y);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                   && players[pno]->num_camps <= players[pno]->num_mills
                                   && ((players[pno]->num_camps < 2 && !is_water_world)
                                       || (players[pno]->num_camps < 1 && is_water_world))
                                   && players[pno]->money > price_camp
                                   && players[pno]->wood > wood_camp))
                           {
                              { // gen_camp
                                 int x;
                                 int y;
                                 bool any_place;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 { // get_th_place
                                    int best_x;
                                    int best_y;
                                    double best_pf = DBL_MAX;
                                    double best_dist = DBL_MAX;
                                    int best_f = INT_MAX;

                                    any_place = false;
                                    for (x = i_max(5, cx - 15);
                                         x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                       for (y = i_max(5, cy - 15);
                                            y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                       { // check_th_field
                                          if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                              && (((cx - x) * (cx - x) + (cy - y) * (cy - y)) >= 3
                                                  && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         < best_dist
                                                  && f->f[x][y] <= best_f)
                                              && can_walk_to(9, o, x, y, 1))
                                          {
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             best_pf = pf->f[x][y];
                                             best_x = x;
                                             best_y = y;
                                             any_place = true;
                                          }
                                       }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 if (any_place)
                                 {
                                    objects->new_order(o, cmd_built_camp, x, y);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                   && players[pno]->num_mills <= players[pno]->num_unis
                                   && players[pno]->num_mills < 2
                                   && players[pno]->money > price_mill
                                   && players[pno]->wood > wood_mill))
                           {
                              { // gen_mill
                                 int x;
                                 int y;
                                 bool any_place;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 { // get_th_place
                                    int best_x;
                                    int best_y;
                                    double best_pf = DBL_MAX;
                                    double best_dist = DBL_MAX;
                                    int best_f = INT_MAX;

                                    any_place = false;
                                    for (x = i_max(5, cx - 15);
                                         x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                       for (y = i_max(5, cy - 15);
                                            y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                       { // check_th_field
                                          if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                              && (((cx - x) * (cx - x) + (cy - y) * (cy - y)) >= 3
                                                  && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         < best_dist
                                                  && f->f[x][y] <= best_f)
                                              && can_walk_to(9, o, x, y, 1))
                                          {
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             best_pf = pf->f[x][y];
                                             best_x = x;
                                             best_y = y;
                                             any_place = true;
                                          }
                                       }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 if (any_place)
                                 {
                                    objects->new_order(o, cmd_built_mill, x, y);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                   && players[pno]->num_mills > 0 && players[pno]->num_camps > 0
                                   && players[pno]->num_unis > 0 && players[pno]->num_smiths < 2
                                   && players[pno]->money > price_smith
                                   && players[pno]->wood > wood_smith))
                           {
                              { // gen_smith
                                 int x;
                                 int y;
                                 bool any_place;
                                 int cx = objects->x[o];
                                 int cy = objects->y[o];

                                 { // get_th_place
                                    int best_x;
                                    int best_y;
                                    double best_pf = DBL_MAX;
                                    double best_dist = DBL_MAX;
                                    int best_f = INT_MAX;

                                    any_place = false;
                                    for (x = i_max(5, cx - 15);
                                         x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                       for (y = i_max(5, cy - 15);
                                            y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                       { // check_th_field
                                          if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                              && (((cx - x) * (cx - x) + (cy - y) * (cy - y)) >= 3
                                                  && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         < best_dist
                                                  && f->f[x][y] <= best_f)
                                              && can_walk_to(9, o, x, y, 1))
                                          {
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             best_pf = pf->f[x][y];
                                             best_x = x;
                                             best_y = y;
                                             any_place = true;
                                          }
                                       }
                                    x = best_x;
                                    y = best_y;
                                 }
                                 if (any_place)
                                 {
                                    objects->new_order(o, cmd_built_smith, x, y);
                                    doing[o] = doing_nothing;
                                    num_building_sites++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (num_trapies_required > 0 && num_trap_run < 1))
                           {
                              { // gen_trap
                                 num_trapies_required--;
                                 if (can_walk_to(6, o, trap_x[num_trapies_required],
                                                 trap_y[num_trapies_required], 1, false))
                                 {    // handle_trap_request
                                    { // trap_at_wall
                                       bool no_archer;

                                       { // check_no_archer
                                          no_archer = true;
                                          for (int i = 0; i < max_objects && no_archer; i++)
                                          { // check_obj
                                             int xmi = i_max(0, heap_x[num_heapies_required]
                                                                    - scope_cata);
                                             int xma
                                                 = i_min(landscape_dx - 1,
                                                         heap_x[num_heapies_required] + scope_cata);
                                             int ymi = i_max(0, heap_y[num_heapies_required]
                                                                    - scope_cata);
                                             int yma
                                                 = i_min(landscape_dy - 1,
                                                         heap_y[num_heapies_required] + scope_cata);

                                             for (int xx = xmi; xx <= xma && no_archer; xx++)
                                                for (int yy = ymi; yy <= yma && no_archer; yy++)
                                                { // check_fa
                                                   int u = unit[xx][yy];

                                                   if (u != none && objects->color[u] != color
                                                       && (objects->type[u] == object_archer
                                                           || objects->type[u] == object_cata))
                                                   {
                                                      no_archer = false;
                                                      break;
                                                   }
                                                }
                                          }
                                       }
                                       if (no_archer)
                                       { // do_trap_at_wall
                                          objects->push_order(o, cmd_dig_trap,
                                                              trap_x[num_trapies_required],
                                                              trap_y[num_trapies_required]);
                                       }
                                    }
                                    trap_run_x[num_trap_run] = trap_x[num_trapies_required];
                                    trap_run_y[num_trap_run] = trap_y[num_trapies_required];
                                    trap_run_worker[num_trap_run] = o;
                                    num_trap_run++;
                                 }
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_markets > 0
                                   && players[pno]->money > price_trader
                                   && players[pno]->wood > wood_trader && num_traders < 3
                                   && num_miners > 2 && num_lumber_jacks > 2))
                           {
                              { // gen_trader
                                 int u = unit[x_market][y_market];

                                 if (u != none && objects->color[u] == color
                                     && objects->type[u] == object_market
                                     && can_walk_to(4, o, x_market, y_market, 1))
                                    objects->new_order(o, cmd_upgrade, 0, 0);
                              }
                           };
                           if ((objects->cmd[o][0] == cmd_idle) && need_a_captain > 0)
                           {
                              move_to_ship(o);
                           };
                           if ((objects->cmd[o][0] == cmd_idle)
                               && (players[pno]->num_town_halls > 0))
                           {
                              { // gen_harvester
                                 if ((objects->home_id[o] >= max_objects
                                      || objects->home_id[o] == none
                                      || objects->type[objects->home_id[o]] != object_home
                                      || objects->color[objects->home_id[o]] != color
                                      || sqrt(
                                             (objects->x[o] - objects->x[objects->home_id[o]])
                                                 * (objects->x[o] - objects->x[objects->home_id[o]])
                                             + (objects->y[o] - objects->y[objects->home_id[o]])
                                                   * (objects->y[o]
                                                      - objects->y[objects->home_id[o]]))
                                             > 10))
                                 { // get_new_home
                                    if (h_mine[0] != -1 && h_mine[1] != -1)
                                    { // choose_best
                                       if (i_random(0, 100) > 50
                                           && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                       }
                                       else if (can_walk_to(12, o, h_x[1], h_y[1], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                       }
                                    }
                                    else if (h_mine[0] != -1
                                             && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                    {
                                       objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                    }
                                    else if (h_mine[1] != -1
                                             && can_walk_to(12, o, h_x[1], h_y[1], 1))
                                    {
                                       objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                    }
                                 }
                                 else
                                 { // start_harvest
                                    if ((num_miners > 0
                                         && (double)num_lumber_jacks / (double)num_miners < 0.4)
                                        || (num_traders > 3 && num_miners > 3))
                                    { // do_harvest_wood
                                       int x;
                                       int y;
                                       bool any_wood;

                                       { // get_wood_place
                                          double best_dist = DBL_MAX;
                                          int best_x;
                                          int best_y;
                                          int cx = objects->x[o];
                                          int cy = objects->y[o];

                                          any_wood = false;
                                          for (int xw = i_max(0, cx - 15);
                                               xw <= i_min(landscape_dx, cx + 15); xw += 2)
                                             for (int yw = i_max(0, cy - 15);
                                                  yw <= i_min(landscape_dy, cy + 15); yw += 2)
                                                if ((land_properties[landscape[xw][yw]].is_forest)
                                                    && ((cx - xw) * (cx - xw)
                                                        + (cy - yw) * (cy - yw))
                                                           < best_dist
                                                    && can_walk_to(13, o, xw, yw, 1))
                                                { // grab_wood
                                                   best_x = xw;
                                                   best_y = yw;
                                                   best_dist = ((cx - xw) * (cx - xw)
                                                                + (cy - yw) * (cy - yw));
                                                   any_wood = true;
                                                }
                                          x = best_x;
                                          y = best_y;
                                       }
                                       if (any_wood)
                                          objects->new_order(o, cmd_harvest, x, y);
                                       else
                                       { // walk_somewhere
                                          int xx = objects->x[o];
                                          int yy = objects->y[o];
                                          int x = i_bound(0, xx + i_random(-10, 10),
                                                          landscape_dx - 1);
                                          int y = i_bound(0, yy + i_random(-10, 10),
                                                          landscape_dy - 1);

                                          if (can_walk_to(8, o, x, y, 0))
                                          {
                                             if (i_random(1, 100) > 10)
                                                objects->new_order(o, cmd_move_to, x, y);
                                             else
                                                objects->new_order(o, cmd_dig_trap, x, y);
                                          }
                                       }
                                    }
                                    else
                                    { // do_harvest_gold
                                       int x;
                                       int y;
                                       bool any_gold;
                                       int cx = objects->x[o];
                                       int cy = objects->y[o];

                                       { // get_gold_place
                                          double best_dist = DBL_MAX;
                                          int best_x;
                                          int best_y;

                                          any_gold = false;
                                          for (int p = 0; p < max_objects; p++)
                                          {
                                             int x = objects->x[p];
                                             int y = objects->y[p];

                                             if ((!objects->is_free[p]
                                                  && objects->type[p] == object_mine)
                                                 && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                        < best_dist
                                                 && can_walk_to(15, o, x, y, 1))
                                             { // grab_mine
                                                best_x = x;
                                                best_y = y;
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                any_gold = true;
                                             }
                                          }
                                          x = best_x;
                                          y = best_y;
                                       }
                                       if (any_gold)
                                          objects->new_order(o, cmd_harvest, x, y);
                                       else
                                       { // walk_somewhere
                                          int xx = objects->x[o];
                                          int yy = objects->y[o];
                                          int x = i_bound(0, xx + i_random(-10, 10),
                                                          landscape_dx - 1);
                                          int y = i_bound(0, yy + i_random(-10, 10),
                                                          landscape_dy - 1);

                                          if (can_walk_to(8, o, x, y, 0))
                                          {
                                             if (i_random(1, 100) > 10)
                                                objects->new_order(o, cmd_move_to, x, y);
                                             else
                                                objects->new_order(o, cmd_dig_trap, x, y);
                                          }
                                       }
                                    }
                                 }
                              }
                           };
                           if (objects->cmd[o][0] == cmd_idle)
                           {
                              objects->new_order(o, cmd_wait, i_random(100, 300), 0);
                           }
                        }
                     };
                     break;
                     case object_market:
                     { // handle_market
                        x_market = objects->x[o];
                        y_market = objects->y[o];
                        objects->money_limit[o] = 0;
                        objects->wood_limit[o] = 700;
                     };
                     break;
                     case object_home:
                     { // handle_home
                        int h = home_id(o);
                        int other = other_home(h);

                        if (h_mine[other] == -1 || h_cnt[h] <= h_cnt[other])
                        { // perform_home_action
                           int il = 0;

                           home_id(o); // aborts with "home error" if o is not a home

                           if (is_water_world)
                              il = isle(objects->x[o], objects->y[o]);
                           if (num_pawns < players[pno]->num_town_halls
                               && f->f[objects->x[o]][objects->y[o]] > 90
                               && players[pno]->money > price_pawn)
                           {
                              objects->new_order(o, cmd_train_pawn, 0, 0);
                              h_cnt[home_id(o)]++;
                           }
                           else if (need_another_worker(il))
                           {
                              objects->new_order(o, cmd_train_worker, 0, 0);
                              h_cnt[home_id(o)]++;
                           }
                        }
                        { // check_over
                           int hid = home_id(o);
                           int m = h_mine[hid];

                           if (m == -1 || objects->is_free[m] || objects->type[m] != object_mine
                               || objects->money[m] <= 0)
                           { // burn_town_hall
                              objects->health[o] = 0;
                           }
                        }
                        if (!should_construct && num_building_sites == 0
                            && ((players[pno]->num_camps < 2 && players[pno]->money > price_camp)
                                || (players[pno]->num_mills < 2 && players[pno]->money > price_mill)
                                || (players[pno]->num_farms < 2 && players[pno]->money > price_mill)
                                || (players[pno]->num_smiths < 2
                                    && players[pno]->money > price_smith)
                                || (is_water_world && players[pno]->num_docks < 1
                                    && players[pno]->money > price_docks)
                                || (!is_water_world && players[pno]->num_markets < 1
                                    && players[pno]->money > price_market)
                                || (players[pno]->num_unis < 2 && players[pno]->money > price_uni)))
                        {
                           should_construct = true;
                        }
                     };
                     break;
                     case object_mill:
                        handle_mill();
                        break;
                     case object_smith:
                        handle_smith();
                        break;
                     case object_uni:
                        handle_uni();
                        break;
                     case object_camp:
                        handle_camp();
                        break;
                     case object_docks:
                        handle_docks();
                        break;
                     case object_pawn:
                        handle_pawn();
                        break;
                     case object_trader:
                        handle_trader();
                        break;
                     case object_knight:
                     { // handle_knight
                        if (objects->health[o] > 90)
                           objects->idle_attack_r[o] = 10;
                        else
                           objects->idle_attack_r[o] = 3;
                        move_a_bit();
                     };
                     break;
                     case object_archer:
                        handle_archer();
                        break;
                     case object_cata:
                        handle_cata();
                        break;
                     case object_doktor:
                     { // handle_doktor
                        if (objects->on_ship[o] == -1)
                        { // exec_dok
                           int ht;

                           objects->idle_attack_r[o] = 3;
                           ht = objects->harvest_type[o];
                           if (ht != -1
                               && (objects->is_free[ht] || objects->color[ht] != color
                                   || objects->type[ht] != object_cata))
                              objects->harvest_type[o] = -1;
                           if (need_a_healer != -1 && objects->harvest_type[o] == -1)
                           {
                              objects->harvest_type[o] = need_a_healer;
                              objects->harvest_type[need_a_healer] = o;
                              need_a_healer = -1;
                           }
                           if (objects->harvest_type[o] != -1)
                           { // try_to_move_to_patient
                              int p = objects->harvest_type[o];

                              if (dist(objects->x[o], objects->y[o], objects->x[p], objects->y[p])
                                      >= 2
                                  && objects->cmd[o][0] == cmd_idle)
                                 objects->new_order(o, cmd_heal, objects->x[p], objects->y[p]);
                           }
                           else
                              move_a_bit();
                        }
                     };
                     break;
                     case object_ship1:
                     {    // handle_ship
                        { // idle_captain
                           if (objects->s[o]->num_man > 0)
                              objects->new_order(objects->s[o]->unit[0], cmd_wait, 300, 0);
                           else
                           {
                              need_a_captain = o;
                              captain_isle = isle(objects->x[o], objects->y[o]);
                              crew_fighter = 1;
                           }
                        }
                        switch (objects->harvest_type[o])
                        {
                        case 0:
                        {    // get_dest
                           { // get_ilf
                              if (is_water_world)
                                 ilf->update(color);
                           }
                           { // get_a_goal_isle
                              int di = isle(objects->x[o], objects->y[o]);

                              { // get_goal
                                 bool any_goal = false;

                                 { // look_for_nice
                                    int best_size = 0;
                                    int xg;
                                    int yg;

                                    for (int i = 0; i < ilf->num_il; i++)
                                       if (ilf->num_worker[i] == 0 && ilf->others[i] == 0 && i != di
                                           && ilf->size[i] > best_size
                                           && (ilf->num_mines[i] > 0
                                               || (ilf->num_trade[i] > 0
                                                   && players[pno]->num_markets < 1))
                                           && i_random(1, 100) > 5
                                           && isle_goal(o, s_dest[o], xg, yg)
                                           && can_walk_to(1, o, xg, yg, 0))
                                       {
                                          s_dest[o] = i;
                                          s_crew_worker[o] = 1;
                                          s_crew_fighter[o] = 2;
                                          s_crew_healer[o] = 0;
                                          any_goal = true;
                                          best_size = ilf->size[i];
                                       }
                                 }
                                 if (!any_goal)
                                 { // look_for_medium
                                    int xg;
                                    int yg;

                                    for (int i = 0; i < ilf->num_il; i++)
                                       if (ilf->others[i] != 0 && ilf->own[i] != 0 && i != di
                                           && i_random(1, 100) > 5
                                           && isle_goal(o, s_dest[o], xg, yg)
                                           && can_walk_to(2, o, xg, yg, 0))
                                       {
                                          s_dest[o] = i;
                                          s_crew_worker[o] = 0;
                                          s_crew_fighter[o] = 10;
                                          s_crew_healer[o] = 2;
                                          any_goal = true;
                                          break;
                                       }
                                 }
                                 if (!any_goal)
                                 { // look_for_brutal
                                    int xg;
                                    int yg;

                                    for (int i = 0; i < ilf->num_il; i++)
                                       if (ilf->others[i] != 0 && ilf->own[i] == 0 && i != di
                                           && i_random(1, 100) > 5
                                           && isle_goal(o, s_dest[o], xg, yg)
                                           && can_walk_to(3, o, xg, yg, 0))
                                       {
                                          s_dest[o] = i;
                                          s_crew_worker[o] = 0;
                                          s_crew_fighter[o] = 10;
                                          s_crew_healer[o] = 2;
                                          any_goal = true;
                                          break;
                                       }
                                 }
                              }
                              objects->harvest_type[o] = 1;
                              urgent = o;
                              urgent_1 = o;
                           }
                        };
                        break;
                        case 1:
                        { // collect_man
                           int n_worker;
                           int n_fighter;
                           int n_healer;

                           objects->s[o]->get_crew(n_worker, n_fighter, n_healer);
                           if (objects->s[o]->num_man != s_last_cnt[o])
                           {
                              s_last_enter[o] = objects->s[o]->num_man;
                              s_last_cnt[o] = ticker;
                           }
                           if ((objects->s[o]->num_man < 10
                                && (n_worker < s_crew_worker[o] || n_fighter < s_crew_fighter[o]
                                    || n_healer < s_crew_healer[o])))
                           { // call_for_man
                              int di = isle(objects->x[o], objects->y[o]);

                              { // get_ilf
                                 if (is_water_world)
                                    ilf->update(color);
                              }
                              crew_worker = i_bound(
                                  0, (s_crew_worker[o] - n_worker),
                                  i_max(0, ilf->num_worker[di] - ilf->num_homes[di] * 2 - 2));
                              crew_fighter = i_bound(0, (s_crew_fighter[o] - n_fighter),
                                                     i_max(0, ilf->num_fighter[di] - 2));
                              crew_healer = i_max(0, s_crew_healer[o] - n_healer);
                              captain_isle = di;
                              need_a_captain = o;
                              if (crew_worker > 0 && !should_to_ship && num_building_sites == 0)
                              {
                                 should_to_ship = true;
                              }
                              for (int i = 0; i < objects->s[o]->num_man; i++)
                                 objects->new_order(objects->s[o]->unit[i], cmd_wait, 300, 300);
                              objects->new_order(o, cmd_wait, 40, 40);
                              if (!(ilf->others[di] == 0
                                    && (crew_worker > 0 || crew_fighter > 0 || crew_healer > 0))
                                  || i_random(1, 100) > 90)
                              {    // change_isle
                                 { // get_ilf
                                    if (is_water_world)
                                       ilf->update(color);
                                 }
                                 s_dest[o] = di;
                                 { // look_for_man
                                    for (int i = 0; i < ilf->num_il; i++)
                                       if (ilf->others[i] == 0 && i != di
                                           && (ilf->num_worker[i] - ilf->num_homes[i] * 2
                                                       - (s_crew_worker[o] - n_worker)
                                                   > 1
                                               || ilf->num_fighter[i]
                                                          - (s_crew_fighter[o] - n_fighter)
                                                      > 2))
                                       {
                                          s_dest[o] = i;
                                          urgent = o;
                                          urgent_1 = o;
                                          break;
                                       }
                                 }
                                 { // start_sail
                                    int xg;
                                    int yg;

                                    if (need_a_captain == o)
                                       need_a_captain = -1;
                                    if (enter_goal(o, xg, yg))
                                    {
                                       objects->new_order(o, cmd_enter, xg, yg);
                                       objects->delay[o] = 0;
                                       objects->harvest_type[o] = 0;
                                    }
                                    else if (isle_goal(o, s_dest[o], xg, yg))
                                    {
                                       objects->new_order(o, cmd_sail, xg, yg);
                                       objects->delay[o] = 0;
                                       objects->harvest_type[o] = 2;
                                    }
                                    else
                                       objects->harvest_type[o] = 0;
                                 }
                              }
                           }
                           else
                           { // start_sail
                              int xg;
                              int yg;

                              if (need_a_captain == o)
                                 need_a_captain = -1;
                              if (enter_goal(o, xg, yg))
                              {
                                 objects->new_order(o, cmd_enter, xg, yg);
                                 objects->delay[o] = 0;
                                 objects->harvest_type[o] = 0;
                              }
                              else if (isle_goal(o, s_dest[o], xg, yg))
                              {
                                 objects->new_order(o, cmd_sail, xg, yg);
                                 objects->delay[o] = 0;
                                 objects->harvest_type[o] = 2;
                              }
                              else
                                 objects->harvest_type[o] = 0;
                           }
                        };
                        break;
                        case 2:
                        { // unload_man
                           if (need_a_captain == o)
                              need_a_captain = -1;
                           if (objects->s[o]->num_man > 1 && i_random(1, 100) > 5)
                           { // start_unload
                              int m = 1;

                              for (int x = i_max(0, objects->x[o] - 8);
                                   x < i_min(landscape_dx - 1, objects->x[o] + 10)
                                   && m < objects->s[o]->num_man;
                                   x++)
                                 for (int y = i_max(0, objects->y[o] - 8);
                                      y < i_min(landscape_dy - 1, objects->y[o] + 10)
                                      && m < objects->s[o]->num_man;
                                      y++)
                                 { // check_unload_field
                                    if (land_properties[landscape[x][y]].walk_possible
                                        && unit[x][y] == none
                                        && dist(x, y, objects->x[o], objects->y[o]) > 4
                                        && ilf->f[x][y] == s_dest[o]
                                        && can_walk_to(0, objects->s[o]->unit[m], x, y, 0))
                                    {
                                       objects->new_order(objects->s[o]->unit[m], cmd_move_to, x,
                                                          y);
                                       m++;
                                    }
                                 }
                              if (m == 1)
                              {
                                 objects->harvest_type[o] = 0;
                                 urgent = o;
                                 urgent_1 = o;
                              }
                           }
                           else
                           {
                              objects->harvest_type[o] = 0;
                           }
                           urgent = o;
                           urgent_1 = o;
                        };
                        break;
                        }
                     };
                     break;
                     }
                  }
                  else if ((objects->type[o] == object_worker
                            && !((objects->cmd[o][0] == cmd_harvest
                                  || objects->cmd[o][1] == cmd_harvest
                                  || objects->cmd[o][2] == cmd_harvest)
                                 && objects->harvest_type[o] == harvest_built)
                            && (i_random(1, 100) > 70) && objects->on_ship[o] == -1
                            && !worker_set_idle
                            && ((players[pno]->num_markets > 0 && players[pno]->money > price_trader
                                 && players[pno]->wood > wood_trader && num_traders < 3
                                 && num_miners > 2 && num_lumber_jacks > 2)
                                || should_construct
                                || (should_to_ship
                                    && ilf->f[objects->x[o]][objects->y[o]] == captain_isle)
                                || num_heapies_required > 0
                                || (num_trapies_required > 0 && num_workers > 6
                                    && players[pno]->money > price_trap
                                    && players[pno]->wood > wood_trap))))
                  { // idle_a_worker
                     objects->new_order(o, cmd_idle, 0, 0);
                     worker_set_idle = true;
                     should_construct = false;
                     should_to_ship = false;
                  }
                  else
                  { // check_health_or_tp
                     exec_cnt--;
                     if (objects->cmd[o][0] != cmd_attack && objects->cmd[o][1] != cmd_attack
                         && objects->cmd[o][2] != cmd_attack)
                        n_running++;
                     if (objects->type[o] == object_trader
                         && ((objects->cmd[o][0] == cmd_trade || objects->cmd[o][1] == cmd_trade
                              || objects->cmd[o][2] == cmd_trade)
                             && (objects->gx[o][0] != xtp[gold_tp]
                                 || objects->gy[o][0] != ytp[gold_tp])
                             && (objects->gx[o][0] != xtp[wood_tp]
                                 || objects->gy[o][0] != ytp[wood_tp])))
                        objects->new_order(o, cmd_idle, 0, 0);
                     if (objects->health[o] < 100
                         && (objects->type[o] == object_pawn || objects->type[o] == object_knight
                             || objects->type[o] == object_archer || objects->type[o] == object_cata
                             || objects->type[o] == object_doktor)
                         && objects->cmd[o][0] != cmd_move_to && objects->cmd[o][1] != cmd_move_to
                         && objects->cmd[o][2] != cmd_move_to)
                     {
                        objects->idle_attack_r[o] = 2;
                        move_a_bit();
                        exec_cnt++;
                     }
                     if (objects->type[o] == object_cata
                         && i_abs(pf->f[objects->x[o]][objects->y[o]]) > 3)
                        objects->new_order(o, cmd_idle, 0, 0);
                  }
                  objects->atta_x[o] = none;
                  { // count
                     switch (objects->type[o])
                     {
                     case object_worker:
                     { // cnt_worker
                        n_workers++;
                        switch (objects->harvest_type[o])
                        {
                        case harvest_wood:
                           n_lumber_jacks++;
                           break;
                        case harvest_gold:
                           n_miners++;
                           break;
                        }
                     };
                     break;
                     case object_pawn:
                        n_pawns++;
                        break;
                     case object_building_site:
                        n_building_sites++;
                        break;
                     case object_site_docks:
                        n_building_sites++;
                        break;
                     case object_ship1:
                        n_ships++;
                        break;
                     case object_knight:
                        n_knights++;
                        break;
                     case object_archer:
                        n_archers++;
                        break;
                     case object_trader:
                        n_traders++;
                        break;
                     case object_cata:
                        n_catas++;
                        break;
                     case object_doktor:
                        n_doktors++;
                        break;
                     }
                     if (doing[o] == doing_attack)
                        n_attack++;
                  }
               }
            }
            if (urgent != -1)
            { // exec_urgent
               int old_o = o;

               o = urgent;
               { // exec_normal
                  if (!objects->is_free[o] && objects->color[o] == color)
                  { // exec_object
                     exec_cnt++;
                     if (objects->cmd[o][0] == cmd_idle)
                     { // gen_new_order
                        switch (objects->type[o])
                        {
                        case object_worker:
                        { // cmd_worker
                           if (objects->on_ship[o] == -1)
                           { // gen_a_new_worker_cmd
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (need_a_captain != -1 && crew_worker > 0
                                      && ilf->f[objects->x[o]][objects->y[o]] == captain_isle
                                      && ilf->num_worker[ilf->f[objects->x[o]][objects->y[o]]]
                                             > ilf->num_homes[ilf->f[objects->x[o]][objects->y[o]]]
                                                   * 2))
                              {
                                 worker_to_ship(o);
                              };
                              if ((objects->cmd[o][0] == cmd_idle) && (num_heapies_required > 0))
                              {
                                 { // gen_heapie
                                    num_heapies_required--;
                                    if (can_walk_to(5, o, heap_x[num_heapies_required],
                                                    heap_y[num_heapies_required], 1, false))
                                    { // handle_heap_request
                                       switch (landscape[heap_x[num_heapies_required]]
                                                        [heap_y[num_heapies_required]])
                                       {
                                       case land_water:
                                       { // cross_river
                                          int dxx;
                                          int dyy;
                                          int length;

                                          { // look_for_river
                                             int dxb = heap_x[num_heapies_required] - objects->x[o];
                                             int dyb = heap_y[num_heapies_required] - objects->y[o];

                                             length = INT_MAX;
                                             if (dyb > 0)
                                             { // check_down
                                                int l = 0;
                                                int x;
                                                int y;

                                                x = heap_x[num_heapies_required];
                                                for (y = heap_y[num_heapies_required];
                                                     y < landscape_dy
                                                     && (landscape[x][y] == land_water);
                                                     y++)
                                                   l++;
                                                if (l < length)
                                                {
                                                   length = l;
                                                   dxx = 0;
                                                   dyy = 1;
                                                }
                                             }
                                             else
                                             { // check_up
                                                int l = 0;
                                                int x;
                                                int y;

                                                x = heap_x[num_heapies_required];
                                                for (y = heap_y[num_heapies_required];
                                                     y > 0 && (landscape[x][y] == land_water); y--)
                                                   l++;
                                                if (l < length)
                                                {
                                                   length = l;
                                                   dxx = 0;
                                                   dyy = -1;
                                                }
                                             }
                                             if (dxb > 0)
                                             { // check_right
                                                int l = 0;
                                                int x;
                                                int y;

                                                y = heap_y[num_heapies_required];
                                                for (x = heap_x[num_heapies_required];
                                                     x < landscape_dx
                                                     && (landscape[x][y] == land_water);
                                                     x++)
                                                   l++;
                                                if (l < length)
                                                {
                                                   length = l;
                                                   dxx = 1;
                                                   dyy = 0;
                                                }
                                             }
                                             else
                                             { // check_left
                                                int l = 0;
                                                int x;
                                                int y;

                                                y = heap_y[num_heapies_required];
                                                for (x = heap_x[num_heapies_required];
                                                     x > 0 && (landscape[x][y] == land_water); x--)
                                                   l++;
                                                if (l < length)
                                                {
                                                   length = l;
                                                   dxx = -1;
                                                   dyy = 0;
                                                }
                                             }
                                          }
                                          { // start_bridge
                                             for (int i = length - 1; i >= 0; i--)
                                             {
                                                objects->push_order(
                                                    o, cmd_heap,
                                                    heap_x[num_heapies_required] + i * dxx,
                                                    heap_y[num_heapies_required] + i * dyy);
                                             }
                                             objects->harvest_type[o] = harvest_heap;
                                          }
                                       };
                                       break;
                                       case land_wall:
                                       { // heap_at_wall
                                          bool no_archer;

                                          { // check_no_archer
                                             no_archer = true;
                                             for (int i = 0; i < max_objects && no_archer; i++)
                                             { // check_obj
                                                int xmi = i_max(0, heap_x[num_heapies_required]
                                                                       - scope_cata);
                                                int xma = i_min(landscape_dx - 1,
                                                                heap_x[num_heapies_required]
                                                                    + scope_cata);
                                                int ymi = i_max(0, heap_y[num_heapies_required]
                                                                       - scope_cata);
                                                int yma = i_min(landscape_dy - 1,
                                                                heap_y[num_heapies_required]
                                                                    + scope_cata);

                                                for (int xx = xmi; xx <= xma && no_archer; xx++)
                                                   for (int yy = ymi; yy <= yma && no_archer; yy++)
                                                   { // check_fa
                                                      int u = unit[xx][yy];

                                                      if (u != none && objects->color[u] != color
                                                          && (objects->type[u] == object_archer
                                                              || objects->type[u] == object_cata))
                                                      {
                                                         no_archer = false;
                                                         break;
                                                      }
                                                   }
                                             }
                                          }
                                          if (no_archer)
                                          { // do_heap_at_wall
                                             objects->push_order(o, cmd_dig,
                                                                 heap_x[num_heapies_required],
                                                                 heap_y[num_heapies_required]);
                                             objects->harvest_type[o] = harvest_dig;
                                          }
                                       };
                                       break;
                                       }
                                       heap_run_x[num_heap_run] = heap_x[num_heapies_required];
                                       heap_run_y[num_heap_run] = heap_y[num_heapies_required];
                                       heap_run_worker[num_heap_run] = o;
                                       num_heap_run++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls
                                          + players[pno]->town_hall_in_progress
                                      < 2))
                              {
                                 { // gen_town_hall
                                    int x;
                                    int y;
                                    bool any_gold;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    clean_mid(o);
                                    { // get_th_gold_place
                                       int best_f = INT_MAX;
                                       double best_dist = DBL_MAX;
                                       ;
                                       int best_x;
                                       int best_y;

                                       any_gold = false;
                                       for (int p = 0; p < max_objects; p++)
                                       {
                                          int x = objects->x[p];
                                          int y = objects->y[p];

                                          if ((!objects->is_free[p]
                                               && objects->type[p] == object_mine)
                                              && ((tnr >= 2 && f->f[x][y] < best_f)
                                                  || (tnr < 2
                                                      && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                             < best_dist))
                                              && p != h_mine[0] && p != h_mine[1]
                                              && pf->f[x][y] == 0 && can_walk_to(14, o, x, y, 1))
                                          { // grab_mine
                                             best_x = x;
                                             best_y = y;
                                             best_dist
                                                 = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                             any_gold = true;
                                          }
                                       }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    cx = x;
                                    cy = y;
                                    if (any_gold && unit[cx][cy] != -1)
                                    { // try_to_built_a_home
                                       bool any_place;

                                       { // get_t_place
                                          int best_x;
                                          int best_y;
                                          double best_dist = DBL_MAX;

                                          any_place = false;
                                          /* Every square within 15 of the centre, nearest valid one wins. This
                                             used to try random squares until 100 ms had passed: about two
                                             million tries now, a stall of one tenth of a second, and a result
                                             that depended on the machine's speed. */
                                          for (x = i_max(0, cx - 15);
                                               x <= i_min(landscape_dx - 1, cx + 15); x++)
                                             for (y = i_max(0, cy - 15);
                                                  y <= i_min(landscape_dy - 1, cy + 15); y++)
                                             {    // try_t_field
                                                { // check_t_field
                                                   if (robo_can_built(o, x, y)
                                                       && ((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y))
                                                              < best_dist
                                                       && ((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y))
                                                              > 0
                                                       && sqrt(((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y)))
                                                              >= 2
                                                       && pf->f[x][y] == 0
                                                       && can_walk_to(10, o, x, y, 1))
                                                   {
                                                      best_dist = ((cx - x) * (cx - x)
                                                                   + (cy - y) * (cy - y));
                                                      best_x = x;
                                                      best_y = y;
                                                      any_place = true;
                                                   }
                                                }
                                             }
                                          /*
      for (x = xmin; x < xmax; x+= 1)
        for (y = ymin; y < ymax; y+= 1)
          check_t_field;
*/
                                          x = best_x;
                                          y = best_y;
                                       }
                                       if (any_place)
                                       {
                                          objects->new_order(o, cmd_built_home, x, y);
                                          doing[o] = doing_nothing;
                                          tnr++;
                                          set_mid(o, x, y, unit[cx][cy]);
                                       }
                                    }
                                    else if (players[pno]->num_town_halls > 0)
                                    { // gen_harvester
                                       if ((objects->home_id[o] >= max_objects
                                            || objects->home_id[o] == none
                                            || objects->type[objects->home_id[o]] != object_home
                                            || objects->color[objects->home_id[o]] != color
                                            || sqrt(
                                                   (objects->x[o] - objects->x[objects->home_id[o]])
                                                       * (objects->x[o]
                                                          - objects->x[objects->home_id[o]])
                                                   + (objects->y[o]
                                                      - objects->y[objects->home_id[o]])
                                                         * (objects->y[o]
                                                            - objects->y[objects->home_id[o]]))
                                                   > 10))
                                       { // get_new_home
                                          if (h_mine[0] != -1 && h_mine[1] != -1)
                                          { // choose_best
                                             if (i_random(0, 100) > 50
                                                 && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                             {
                                                objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                             }
                                             else if (can_walk_to(12, o, h_x[1], h_y[1], 1))
                                             {
                                                objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                             }
                                          }
                                          else if (h_mine[0] != -1
                                                   && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                          }
                                          else if (h_mine[1] != -1
                                                   && can_walk_to(12, o, h_x[1], h_y[1], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                          }
                                       }
                                       else
                                       { // start_harvest
                                          if ((num_miners > 0
                                               && (double)num_lumber_jacks / (double)num_miners
                                                      < 0.4)
                                              || (num_traders > 3 && num_miners > 3))
                                          { // do_harvest_wood
                                             int x;
                                             int y;
                                             bool any_wood;

                                             { // get_wood_place
                                                double best_dist = DBL_MAX;
                                                int best_x;
                                                int best_y;
                                                int cx = objects->x[o];
                                                int cy = objects->y[o];

                                                any_wood = false;
                                                for (int xw = i_max(0, cx - 15);
                                                     xw <= i_min(landscape_dx, cx + 15); xw += 2)
                                                   for (int yw = i_max(0, cy - 15);
                                                        yw <= i_min(landscape_dy, cy + 15); yw += 2)
                                                      if ((land_properties[landscape[xw][yw]]
                                                               .is_forest)
                                                          && ((cx - xw) * (cx - xw)
                                                              + (cy - yw) * (cy - yw))
                                                                 < best_dist
                                                          && can_walk_to(13, o, xw, yw, 1))
                                                      { // grab_wood
                                                         best_x = xw;
                                                         best_y = yw;
                                                         best_dist = ((cx - xw) * (cx - xw)
                                                                      + (cy - yw) * (cy - yw));
                                                         any_wood = true;
                                                      }
                                                x = best_x;
                                                y = best_y;
                                             }
                                             if (any_wood)
                                                objects->new_order(o, cmd_harvest, x, y);
                                             else
                                             { // walk_somewhere
                                                int xx = objects->x[o];
                                                int yy = objects->y[o];
                                                int x = i_bound(0, xx + i_random(-10, 10),
                                                                landscape_dx - 1);
                                                int y = i_bound(0, yy + i_random(-10, 10),
                                                                landscape_dy - 1);

                                                if (can_walk_to(8, o, x, y, 0))
                                                {
                                                   if (i_random(1, 100) > 10)
                                                      objects->new_order(o, cmd_move_to, x, y);
                                                   else
                                                      objects->new_order(o, cmd_dig_trap, x, y);
                                                }
                                             }
                                          }
                                          else
                                          { // do_harvest_gold
                                             int x;
                                             int y;
                                             bool any_gold;
                                             int cx = objects->x[o];
                                             int cy = objects->y[o];

                                             { // get_gold_place
                                                double best_dist = DBL_MAX;
                                                int best_x;
                                                int best_y;

                                                any_gold = false;
                                                for (int p = 0; p < max_objects; p++)
                                                {
                                                   int x = objects->x[p];
                                                   int y = objects->y[p];

                                                   if ((!objects->is_free[p]
                                                        && objects->type[p] == object_mine)
                                                       && ((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y))
                                                              < best_dist
                                                       && can_walk_to(15, o, x, y, 1))
                                                   { // grab_mine
                                                      best_x = x;
                                                      best_y = y;
                                                      best_dist = ((cx - x) * (cx - x)
                                                                   + (cy - y) * (cy - y));
                                                      any_gold = true;
                                                   }
                                                }
                                                x = best_x;
                                                y = best_y;
                                             }
                                             if (any_gold)
                                                objects->new_order(o, cmd_harvest, x, y);
                                             else
                                             { // walk_somewhere
                                                int xx = objects->x[o];
                                                int yy = objects->y[o];
                                                int x = i_bound(0, xx + i_random(-10, 10),
                                                                landscape_dx - 1);
                                                int y = i_bound(0, yy + i_random(-10, 10),
                                                                landscape_dy - 1);

                                                if (can_walk_to(8, o, x, y, 0))
                                                {
                                                   if (i_random(1, 100) > 10)
                                                      objects->new_order(o, cmd_move_to, x, y);
                                                   else
                                                      objects->new_order(o, cmd_dig_trap, x, y);
                                                }
                                             }
                                          }
                                       }
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                      && (players[pno]->num_farms < players[pno]->num_mans / 20 + 1
                                          || players[pno]->food < 400)
                                      && players[pno]->money > price_farm
                                      && players[pno]->wood > wood_farm))
                              {
                                 { // gen_farm
                                    int x;
                                    int y;
                                    bool any_place;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    { // get_th_place
                                       int best_x;
                                       int best_y;
                                       double best_pf = DBL_MAX;
                                       double best_dist = DBL_MAX;
                                       int best_f = INT_MAX;

                                       any_place = false;
                                       for (x = i_max(5, cx - 15);
                                            x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                          for (y = i_max(5, cy - 15);
                                               y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                          { // check_th_field
                                             if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                                 && (((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         >= 3
                                                     && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                            < best_dist
                                                     && f->f[x][y] <= best_f)
                                                 && can_walk_to(9, o, x, y, 1))
                                             {
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                best_pf = pf->f[x][y];
                                                best_x = x;
                                                best_y = y;
                                                any_place = true;
                                             }
                                          }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_farm, x, y);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (is_water_world && players[pno]->num_docks < 1
                                      && num_building_sites == 0))
                              {
                                 { // gen_docks
                                    int xd = objects->x[o];
                                    int yd = objects->y[o];

                                    if (look_for_dock_place(xd, yd))
                                    {
                                       objects->new_order(o, cmd_built_docks, xd, yd);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0
                                      && players[pno]->num_mills > 0 && players[pno]->num_camps > 0
                                      && players[pno]->num_unis > 0 && num_building_sites == 0
                                      && players[pno]->num_markets < 1
                                      && players[pno]->market_in_progress == 0
                                      && players[pno]->money > price_market
                                      && players[pno]->wood > wood_market))
                              {
                                 { // gen_market
                                    int x = 0;
                                    int y = 0;
                                    bool any_place = false;

                                    { // get_gold_tp
                                       int best_pot = INT_MAX;

                                       for (int i = 0; i < ntp; i++)
                                          if (landscape[xtp[i]][ytp[i]] == land_t_gold
                                              && f->f[xtp[i]][ytp[i]] < best_pot)
                                          { // grab_gold_place
                                             best_pot = f->f[xtp[i]][ytp[i]];
                                             gold_tp = i;
                                          }
                                    }
                                    { // get_wood_tp
                                       int best_pot = INT_MAX;

                                       for (int i = 0; i < ntp; i++)
                                          if (landscape[xtp[i]][ytp[i]] == land_t_wood
                                              && f->f[xtp[i]][ytp[i]] < best_pot)
                                          { // grab_wood_place
                                             best_pot = f->f[xtp[i]][ytp[i]];
                                             wood_tp = i;
                                          }
                                    }
                                    { // get_market_place
                                       int cx = xtp[gold_tp];
                                       int cy = ytp[gold_tp];

                                       if (can_walk_to(7, o, cx, cy, 1))
                                       { // get_t_place
                                          int best_x;
                                          int best_y;
                                          double best_dist = DBL_MAX;

                                          any_place = false;
                                          /* Every square within 15 of the centre, nearest valid one wins. This
                                             used to try random squares until 100 ms had passed: about two
                                             million tries now, a stall of one tenth of a second, and a result
                                             that depended on the machine's speed. */
                                          for (x = i_max(0, cx - 15);
                                               x <= i_min(landscape_dx - 1, cx + 15); x++)
                                             for (y = i_max(0, cy - 15);
                                                  y <= i_min(landscape_dy - 1, cy + 15); y++)
                                             {    // try_t_field
                                                { // check_t_field
                                                   if (robo_can_built(o, x, y)
                                                       && ((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y))
                                                              < best_dist
                                                       && ((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y))
                                                              > 0
                                                       && sqrt(((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y)))
                                                              >= 2
                                                       && pf->f[x][y] == 0
                                                       && can_walk_to(10, o, x, y, 1))
                                                   {
                                                      best_dist = ((cx - x) * (cx - x)
                                                                   + (cy - y) * (cy - y));
                                                      best_x = x;
                                                      best_y = y;
                                                      any_place = true;
                                                   }
                                                }
                                             }
                                          /*
      for (x = xmin; x < xmax; x+= 1)
        for (y = ymin; y < ymax; y+= 1)
          check_t_field;
*/
                                          x = best_x;
                                          y = best_y;
                                       }
                                    }
                                    if (any_place)
                                       objects->new_order(o, cmd_built_market, x, y);
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                      && (is_water_world
                                          || players[pno]->num_unis <= players[pno]->num_smiths
                                          || players[pno]->num_unis <= players[pno]->num_mills)
                                      && players[pno]->num_camps > 0 && players[pno]->num_unis < 2
                                      && players[pno]->money > price_uni
                                      && players[pno]->wood > wood_uni))
                              {
                                 { // gen_uni
                                    int x;
                                    int y;
                                    bool any_place;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    { // get_th_place
                                       int best_x;
                                       int best_y;
                                       double best_pf = DBL_MAX;
                                       double best_dist = DBL_MAX;
                                       int best_f = INT_MAX;

                                       any_place = false;
                                       for (x = i_max(5, cx - 15);
                                            x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                          for (y = i_max(5, cy - 15);
                                               y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                          { // check_th_field
                                             if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                                 && (((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         >= 3
                                                     && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                            < best_dist
                                                     && f->f[x][y] <= best_f)
                                                 && can_walk_to(9, o, x, y, 1))
                                             {
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                best_pf = pf->f[x][y];
                                                best_x = x;
                                                best_y = y;
                                                any_place = true;
                                             }
                                          }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_uni, x, y);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                      && players[pno]->num_camps <= players[pno]->num_mills
                                      && ((players[pno]->num_camps < 2 && !is_water_world)
                                          || (players[pno]->num_camps < 1 && is_water_world))
                                      && players[pno]->money > price_camp
                                      && players[pno]->wood > wood_camp))
                              {
                                 { // gen_camp
                                    int x;
                                    int y;
                                    bool any_place;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    { // get_th_place
                                       int best_x;
                                       int best_y;
                                       double best_pf = DBL_MAX;
                                       double best_dist = DBL_MAX;
                                       int best_f = INT_MAX;

                                       any_place = false;
                                       for (x = i_max(5, cx - 15);
                                            x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                          for (y = i_max(5, cy - 15);
                                               y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                          { // check_th_field
                                             if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                                 && (((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         >= 3
                                                     && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                            < best_dist
                                                     && f->f[x][y] <= best_f)
                                                 && can_walk_to(9, o, x, y, 1))
                                             {
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                best_pf = pf->f[x][y];
                                                best_x = x;
                                                best_y = y;
                                                any_place = true;
                                             }
                                          }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_camp, x, y);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                      && players[pno]->num_mills <= players[pno]->num_unis
                                      && players[pno]->num_mills < 2
                                      && players[pno]->money > price_mill
                                      && players[pno]->wood > wood_mill))
                              {
                                 { // gen_mill
                                    int x;
                                    int y;
                                    bool any_place;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    { // get_th_place
                                       int best_x;
                                       int best_y;
                                       double best_pf = DBL_MAX;
                                       double best_dist = DBL_MAX;
                                       int best_f = INT_MAX;

                                       any_place = false;
                                       for (x = i_max(5, cx - 15);
                                            x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                          for (y = i_max(5, cy - 15);
                                               y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                          { // check_th_field
                                             if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                                 && (((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         >= 3
                                                     && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                            < best_dist
                                                     && f->f[x][y] <= best_f)
                                                 && can_walk_to(9, o, x, y, 1))
                                             {
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                best_pf = pf->f[x][y];
                                                best_x = x;
                                                best_y = y;
                                                any_place = true;
                                             }
                                          }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_mill, x, y);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0 && num_building_sites == 0
                                      && players[pno]->num_mills > 0 && players[pno]->num_camps > 0
                                      && players[pno]->num_unis > 0 && players[pno]->num_smiths < 2
                                      && players[pno]->money > price_smith
                                      && players[pno]->wood > wood_smith))
                              {
                                 { // gen_smith
                                    int x;
                                    int y;
                                    bool any_place;
                                    int cx = objects->x[o];
                                    int cy = objects->y[o];

                                    { // get_th_place
                                       int best_x;
                                       int best_y;
                                       double best_pf = DBL_MAX;
                                       double best_dist = DBL_MAX;
                                       int best_f = INT_MAX;

                                       any_place = false;
                                       for (x = i_max(5, cx - 15);
                                            x < i_min(landscape_dx - 5, cx + 15); x += 2)
                                          for (y = i_max(5, cy - 15);
                                               y < i_min(landscape_dy - 5, cy + 15); y += 2)
                                          { // check_th_field
                                             if (robo_can_built(o, x, y) && pf->f[x][y] < best_pf
                                                 && (((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                         >= 3
                                                     && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                            < best_dist
                                                     && f->f[x][y] <= best_f)
                                                 && can_walk_to(9, o, x, y, 1))
                                             {
                                                best_dist
                                                    = ((cx - x) * (cx - x) + (cy - y) * (cy - y));
                                                best_pf = pf->f[x][y];
                                                best_x = x;
                                                best_y = y;
                                                any_place = true;
                                             }
                                          }
                                       x = best_x;
                                       y = best_y;
                                    }
                                    if (any_place)
                                    {
                                       objects->new_order(o, cmd_built_smith, x, y);
                                       doing[o] = doing_nothing;
                                       num_building_sites++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (num_trapies_required > 0 && num_trap_run < 1))
                              {
                                 { // gen_trap
                                    num_trapies_required--;
                                    if (can_walk_to(6, o, trap_x[num_trapies_required],
                                                    trap_y[num_trapies_required], 1, false))
                                    {    // handle_trap_request
                                       { // trap_at_wall
                                          bool no_archer;

                                          { // check_no_archer
                                             no_archer = true;
                                             for (int i = 0; i < max_objects && no_archer; i++)
                                             { // check_obj
                                                int xmi = i_max(0, heap_x[num_heapies_required]
                                                                       - scope_cata);
                                                int xma = i_min(landscape_dx - 1,
                                                                heap_x[num_heapies_required]
                                                                    + scope_cata);
                                                int ymi = i_max(0, heap_y[num_heapies_required]
                                                                       - scope_cata);
                                                int yma = i_min(landscape_dy - 1,
                                                                heap_y[num_heapies_required]
                                                                    + scope_cata);

                                                for (int xx = xmi; xx <= xma && no_archer; xx++)
                                                   for (int yy = ymi; yy <= yma && no_archer; yy++)
                                                   { // check_fa
                                                      int u = unit[xx][yy];

                                                      if (u != none && objects->color[u] != color
                                                          && (objects->type[u] == object_archer
                                                              || objects->type[u] == object_cata))
                                                      {
                                                         no_archer = false;
                                                         break;
                                                      }
                                                   }
                                             }
                                          }
                                          if (no_archer)
                                          { // do_trap_at_wall
                                             objects->push_order(o, cmd_dig_trap,
                                                                 trap_x[num_trapies_required],
                                                                 trap_y[num_trapies_required]);
                                          }
                                       }
                                       trap_run_x[num_trap_run] = trap_x[num_trapies_required];
                                       trap_run_y[num_trap_run] = trap_y[num_trapies_required];
                                       trap_run_worker[num_trap_run] = o;
                                       num_trap_run++;
                                    }
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_markets > 0
                                      && players[pno]->money > price_trader
                                      && players[pno]->wood > wood_trader && num_traders < 3
                                      && num_miners > 2 && num_lumber_jacks > 2))
                              {
                                 { // gen_trader
                                    int u = unit[x_market][y_market];

                                    if (u != none && objects->color[u] == color
                                        && objects->type[u] == object_market
                                        && can_walk_to(4, o, x_market, y_market, 1))
                                       objects->new_order(o, cmd_upgrade, 0, 0);
                                 }
                              };
                              if ((objects->cmd[o][0] == cmd_idle) && need_a_captain > 0)
                              {
                                 move_to_ship(o);
                              };
                              if ((objects->cmd[o][0] == cmd_idle)
                                  && (players[pno]->num_town_halls > 0))
                              {
                                 { // gen_harvester
                                    if ((objects->home_id[o] >= max_objects
                                         || objects->home_id[o] == none
                                         || objects->type[objects->home_id[o]] != object_home
                                         || objects->color[objects->home_id[o]] != color
                                         || sqrt((objects->x[o] - objects->x[objects->home_id[o]])
                                                     * (objects->x[o]
                                                        - objects->x[objects->home_id[o]])
                                                 + (objects->y[o] - objects->y[objects->home_id[o]])
                                                       * (objects->y[o]
                                                          - objects->y[objects->home_id[o]]))
                                                > 10))
                                    { // get_new_home
                                       if (h_mine[0] != -1 && h_mine[1] != -1)
                                       { // choose_best
                                          if (i_random(0, 100) > 50
                                              && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                          }
                                          else if (can_walk_to(12, o, h_x[1], h_y[1], 1))
                                          {
                                             objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                          }
                                       }
                                       else if (h_mine[0] != -1
                                                && can_walk_to(11, o, h_x[0], h_y[0], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[0], h_y[0]);
                                       }
                                       else if (h_mine[1] != -1
                                                && can_walk_to(12, o, h_x[1], h_y[1], 1))
                                       {
                                          objects->new_order(o, cmd_move_to, h_x[1], h_y[1]);
                                       }
                                    }
                                    else
                                    { // start_harvest
                                       if ((num_miners > 0
                                            && (double)num_lumber_jacks / (double)num_miners < 0.4)
                                           || (num_traders > 3 && num_miners > 3))
                                       { // do_harvest_wood
                                          int x;
                                          int y;
                                          bool any_wood;

                                          { // get_wood_place
                                             double best_dist = DBL_MAX;
                                             int best_x;
                                             int best_y;
                                             int cx = objects->x[o];
                                             int cy = objects->y[o];

                                             any_wood = false;
                                             for (int xw = i_max(0, cx - 15);
                                                  xw <= i_min(landscape_dx, cx + 15); xw += 2)
                                                for (int yw = i_max(0, cy - 15);
                                                     yw <= i_min(landscape_dy, cy + 15); yw += 2)
                                                   if ((land_properties[landscape[xw][yw]]
                                                            .is_forest)
                                                       && ((cx - xw) * (cx - xw)
                                                           + (cy - yw) * (cy - yw))
                                                              < best_dist
                                                       && can_walk_to(13, o, xw, yw, 1))
                                                   { // grab_wood
                                                      best_x = xw;
                                                      best_y = yw;
                                                      best_dist = ((cx - xw) * (cx - xw)
                                                                   + (cy - yw) * (cy - yw));
                                                      any_wood = true;
                                                   }
                                             x = best_x;
                                             y = best_y;
                                          }
                                          if (any_wood)
                                             objects->new_order(o, cmd_harvest, x, y);
                                          else
                                          { // walk_somewhere
                                             int xx = objects->x[o];
                                             int yy = objects->y[o];
                                             int x = i_bound(0, xx + i_random(-10, 10),
                                                             landscape_dx - 1);
                                             int y = i_bound(0, yy + i_random(-10, 10),
                                                             landscape_dy - 1);

                                             if (can_walk_to(8, o, x, y, 0))
                                             {
                                                if (i_random(1, 100) > 10)
                                                   objects->new_order(o, cmd_move_to, x, y);
                                                else
                                                   objects->new_order(o, cmd_dig_trap, x, y);
                                             }
                                          }
                                       }
                                       else
                                       { // do_harvest_gold
                                          int x;
                                          int y;
                                          bool any_gold;
                                          int cx = objects->x[o];
                                          int cy = objects->y[o];

                                          { // get_gold_place
                                             double best_dist = DBL_MAX;
                                             int best_x;
                                             int best_y;

                                             any_gold = false;
                                             for (int p = 0; p < max_objects; p++)
                                             {
                                                int x = objects->x[p];
                                                int y = objects->y[p];

                                                if ((!objects->is_free[p]
                                                     && objects->type[p] == object_mine)
                                                    && ((cx - x) * (cx - x) + (cy - y) * (cy - y))
                                                           < best_dist
                                                    && can_walk_to(15, o, x, y, 1))
                                                { // grab_mine
                                                   best_x = x;
                                                   best_y = y;
                                                   best_dist = ((cx - x) * (cx - x)
                                                                + (cy - y) * (cy - y));
                                                   any_gold = true;
                                                }
                                             }
                                             x = best_x;
                                             y = best_y;
                                          }
                                          if (any_gold)
                                             objects->new_order(o, cmd_harvest, x, y);
                                          else
                                          { // walk_somewhere
                                             int xx = objects->x[o];
                                             int yy = objects->y[o];
                                             int x = i_bound(0, xx + i_random(-10, 10),
                                                             landscape_dx - 1);
                                             int y = i_bound(0, yy + i_random(-10, 10),
                                                             landscape_dy - 1);

                                             if (can_walk_to(8, o, x, y, 0))
                                             {
                                                if (i_random(1, 100) > 10)
                                                   objects->new_order(o, cmd_move_to, x, y);
                                                else
                                                   objects->new_order(o, cmd_dig_trap, x, y);
                                             }
                                          }
                                       }
                                    }
                                 }
                              };
                              if (objects->cmd[o][0] == cmd_idle)
                              {
                                 objects->new_order(o, cmd_wait, i_random(100, 300), 0);
                              }
                           }
                        };
                        break;
                        case object_market:
                        { // handle_market
                           x_market = objects->x[o];
                           y_market = objects->y[o];
                           objects->money_limit[o] = 0;
                           objects->wood_limit[o] = 700;
                        };
                        break;
                        case object_home:
                        { // handle_home
                           int h = home_id(o);
                           int other = other_home(h);

                           if (h_mine[other] == -1 || h_cnt[h] <= h_cnt[other])
                           { // perform_home_action
                              int il = 0;

                              home_id(o); // aborts with "home error" if o is not a home

                              if (is_water_world)
                                 il = isle(objects->x[o], objects->y[o]);
                              if (num_pawns < players[pno]->num_town_halls
                                  && f->f[objects->x[o]][objects->y[o]] > 90
                                  && players[pno]->money > price_pawn)
                              {
                                 objects->new_order(o, cmd_train_pawn, 0, 0);
                                 h_cnt[home_id(o)]++;
                              }
                              else if (need_another_worker(il))
                              {
                                 objects->new_order(o, cmd_train_worker, 0, 0);
                                 h_cnt[home_id(o)]++;
                              }
                           }
                           { // check_over
                              int hid = home_id(o);
                              int m = h_mine[hid];

                              if (m == -1 || objects->is_free[m] || objects->type[m] != object_mine
                                  || objects->money[m] <= 0)
                              { // burn_town_hall
                                 objects->health[o] = 0;
                              }
                           }
                           if (!should_construct && num_building_sites == 0
                               && ((players[pno]->num_camps < 2 && players[pno]->money > price_camp)
                                   || (players[pno]->num_mills < 2
                                       && players[pno]->money > price_mill)
                                   || (players[pno]->num_farms < 2
                                       && players[pno]->money > price_mill)
                                   || (players[pno]->num_smiths < 2
                                       && players[pno]->money > price_smith)
                                   || (is_water_world && players[pno]->num_docks < 1
                                       && players[pno]->money > price_docks)
                                   || (!is_water_world && players[pno]->num_markets < 1
                                       && players[pno]->money > price_market)
                                   || (players[pno]->num_unis < 2
                                       && players[pno]->money > price_uni)))
                           {
                              should_construct = true;
                           }
                        };
                        break;
                        case object_mill:
                           handle_mill();
                           break;
                        case object_smith:
                           handle_smith();
                           break;
                        case object_uni:
                           handle_uni();
                           break;
                        case object_camp:
                           handle_camp();
                           break;
                        case object_docks:
                           handle_docks();
                           break;
                        case object_pawn:
                           handle_pawn();
                           break;
                        case object_trader:
                           handle_trader();
                           break;
                        case object_knight:
                        { // handle_knight
                           if (objects->health[o] > 90)
                              objects->idle_attack_r[o] = 10;
                           else
                              objects->idle_attack_r[o] = 3;
                           move_a_bit();
                        };
                        break;
                        case object_archer:
                           handle_archer();
                           break;
                        case object_cata:
                           handle_cata();
                           break;
                        case object_doktor:
                        { // handle_doktor
                           if (objects->on_ship[o] == -1)
                           { // exec_dok
                              int ht;

                              objects->idle_attack_r[o] = 3;
                              ht = objects->harvest_type[o];
                              if (ht != -1
                                  && (objects->is_free[ht] || objects->color[ht] != color
                                      || objects->type[ht] != object_cata))
                                 objects->harvest_type[o] = -1;
                              if (need_a_healer != -1 && objects->harvest_type[o] == -1)
                              {
                                 objects->harvest_type[o] = need_a_healer;
                                 objects->harvest_type[need_a_healer] = o;
                                 need_a_healer = -1;
                              }
                              if (objects->harvest_type[o] != -1)
                              { // try_to_move_to_patient
                                 int p = objects->harvest_type[o];

                                 if (dist(objects->x[o], objects->y[o], objects->x[p],
                                          objects->y[p])
                                         >= 2
                                     && objects->cmd[o][0] == cmd_idle)
                                    objects->new_order(o, cmd_heal, objects->x[p], objects->y[p]);
                              }
                              else
                                 move_a_bit();
                           }
                        };
                        break;
                        case object_ship1:
                        {    // handle_ship
                           { // idle_captain
                              if (objects->s[o]->num_man > 0)
                                 objects->new_order(objects->s[o]->unit[0], cmd_wait, 300, 0);
                              else
                              {
                                 need_a_captain = o;
                                 captain_isle = isle(objects->x[o], objects->y[o]);
                                 crew_fighter = 1;
                              }
                           }
                           switch (objects->harvest_type[o])
                           {
                           case 0:
                           {    // get_dest
                              { // get_ilf
                                 if (is_water_world)
                                    ilf->update(color);
                              }
                              { // get_a_goal_isle
                                 int di = isle(objects->x[o], objects->y[o]);

                                 { // get_goal
                                    bool any_goal = false;

                                    { // look_for_nice
                                       int best_size = 0;
                                       int xg;
                                       int yg;

                                       for (int i = 0; i < ilf->num_il; i++)
                                          if (ilf->num_worker[i] == 0 && ilf->others[i] == 0
                                              && i != di && ilf->size[i] > best_size
                                              && (ilf->num_mines[i] > 0
                                                  || (ilf->num_trade[i] > 0
                                                      && players[pno]->num_markets < 1))
                                              && i_random(1, 100) > 5
                                              && isle_goal(o, s_dest[o], xg, yg)
                                              && can_walk_to(1, o, xg, yg, 0))
                                          {
                                             s_dest[o] = i;
                                             s_crew_worker[o] = 1;
                                             s_crew_fighter[o] = 2;
                                             s_crew_healer[o] = 0;
                                             any_goal = true;
                                             best_size = ilf->size[i];
                                          }
                                    }
                                    if (!any_goal)
                                    { // look_for_medium
                                       int xg;
                                       int yg;

                                       for (int i = 0; i < ilf->num_il; i++)
                                          if (ilf->others[i] != 0 && ilf->own[i] != 0 && i != di
                                              && i_random(1, 100) > 5
                                              && isle_goal(o, s_dest[o], xg, yg)
                                              && can_walk_to(2, o, xg, yg, 0))
                                          {
                                             s_dest[o] = i;
                                             s_crew_worker[o] = 0;
                                             s_crew_fighter[o] = 10;
                                             s_crew_healer[o] = 2;
                                             any_goal = true;
                                             break;
                                          }
                                    }
                                    if (!any_goal)
                                    { // look_for_brutal
                                       int xg;
                                       int yg;

                                       for (int i = 0; i < ilf->num_il; i++)
                                          if (ilf->others[i] != 0 && ilf->own[i] == 0 && i != di
                                              && i_random(1, 100) > 5
                                              && isle_goal(o, s_dest[o], xg, yg)
                                              && can_walk_to(3, o, xg, yg, 0))
                                          {
                                             s_dest[o] = i;
                                             s_crew_worker[o] = 0;
                                             s_crew_fighter[o] = 10;
                                             s_crew_healer[o] = 2;
                                             any_goal = true;
                                             break;
                                          }
                                    }
                                 }
                                 objects->harvest_type[o] = 1;
                                 urgent = o;
                                 urgent_1 = o;
                              }
                           };
                           break;
                           case 1:
                           { // collect_man
                              int n_worker;
                              int n_fighter;
                              int n_healer;

                              objects->s[o]->get_crew(n_worker, n_fighter, n_healer);
                              if (objects->s[o]->num_man != s_last_cnt[o])
                              {
                                 s_last_enter[o] = objects->s[o]->num_man;
                                 s_last_cnt[o] = ticker;
                              }
                              if ((objects->s[o]->num_man < 10
                                   && (n_worker < s_crew_worker[o] || n_fighter < s_crew_fighter[o]
                                       || n_healer < s_crew_healer[o])))
                              { // call_for_man
                                 int di = isle(objects->x[o], objects->y[o]);

                                 { // get_ilf
                                    if (is_water_world)
                                       ilf->update(color);
                                 }
                                 crew_worker = i_bound(
                                     0, (s_crew_worker[o] - n_worker),
                                     i_max(0, ilf->num_worker[di] - ilf->num_homes[di] * 2 - 2));
                                 crew_fighter = i_bound(0, (s_crew_fighter[o] - n_fighter),
                                                        i_max(0, ilf->num_fighter[di] - 2));
                                 crew_healer = i_max(0, s_crew_healer[o] - n_healer);
                                 captain_isle = di;
                                 need_a_captain = o;
                                 if (crew_worker > 0 && !should_to_ship && num_building_sites == 0)
                                 {
                                    should_to_ship = true;
                                 }
                                 for (int i = 0; i < objects->s[o]->num_man; i++)
                                    objects->new_order(objects->s[o]->unit[i], cmd_wait, 300, 300);
                                 objects->new_order(o, cmd_wait, 40, 40);
                                 if (!(ilf->others[di] == 0
                                       && (crew_worker > 0 || crew_fighter > 0 || crew_healer > 0))
                                     || i_random(1, 100) > 90)
                                 {    // change_isle
                                    { // get_ilf
                                       if (is_water_world)
                                          ilf->update(color);
                                    }
                                    s_dest[o] = di;
                                    { // look_for_man
                                       for (int i = 0; i < ilf->num_il; i++)
                                          if (ilf->others[i] == 0 && i != di
                                              && (ilf->num_worker[i] - ilf->num_homes[i] * 2
                                                          - (s_crew_worker[o] - n_worker)
                                                      > 1
                                                  || ilf->num_fighter[i]
                                                             - (s_crew_fighter[o] - n_fighter)
                                                         > 2))
                                          {
                                             s_dest[o] = i;
                                             urgent = o;
                                             urgent_1 = o;
                                             break;
                                          }
                                    }
                                    { // start_sail
                                       int xg;
                                       int yg;

                                       if (need_a_captain == o)
                                          need_a_captain = -1;
                                       if (enter_goal(o, xg, yg))
                                       {
                                          objects->new_order(o, cmd_enter, xg, yg);
                                          objects->delay[o] = 0;
                                          objects->harvest_type[o] = 0;
                                       }
                                       else if (isle_goal(o, s_dest[o], xg, yg))
                                       {
                                          objects->new_order(o, cmd_sail, xg, yg);
                                          objects->delay[o] = 0;
                                          objects->harvest_type[o] = 2;
                                       }
                                       else
                                          objects->harvest_type[o] = 0;
                                    }
                                 }
                              }
                              else
                              { // start_sail
                                 int xg;
                                 int yg;

                                 if (need_a_captain == o)
                                    need_a_captain = -1;
                                 if (enter_goal(o, xg, yg))
                                 {
                                    objects->new_order(o, cmd_enter, xg, yg);
                                    objects->delay[o] = 0;
                                    objects->harvest_type[o] = 0;
                                 }
                                 else if (isle_goal(o, s_dest[o], xg, yg))
                                 {
                                    objects->new_order(o, cmd_sail, xg, yg);
                                    objects->delay[o] = 0;
                                    objects->harvest_type[o] = 2;
                                 }
                                 else
                                    objects->harvest_type[o] = 0;
                              }
                           };
                           break;
                           case 2:
                           { // unload_man
                              if (need_a_captain == o)
                                 need_a_captain = -1;
                              if (objects->s[o]->num_man > 1 && i_random(1, 100) > 5)
                              { // start_unload
                                 int m = 1;

                                 for (int x = i_max(0, objects->x[o] - 8);
                                      x < i_min(landscape_dx - 1, objects->x[o] + 10)
                                      && m < objects->s[o]->num_man;
                                      x++)
                                    for (int y = i_max(0, objects->y[o] - 8);
                                         y < i_min(landscape_dy - 1, objects->y[o] + 10)
                                         && m < objects->s[o]->num_man;
                                         y++)
                                    { // check_unload_field
                                       if (land_properties[landscape[x][y]].walk_possible
                                           && unit[x][y] == none
                                           && dist(x, y, objects->x[o], objects->y[o]) > 4
                                           && ilf->f[x][y] == s_dest[o]
                                           && can_walk_to(0, objects->s[o]->unit[m], x, y, 0))
                                       {
                                          objects->new_order(objects->s[o]->unit[m], cmd_move_to, x,
                                                             y);
                                          m++;
                                       }
                                    }
                                 if (m == 1)
                                 {
                                    objects->harvest_type[o] = 0;
                                    urgent = o;
                                    urgent_1 = o;
                                 }
                              }
                              else
                              {
                                 objects->harvest_type[o] = 0;
                              }
                              urgent = o;
                              urgent_1 = o;
                           };
                           break;
                           }
                        };
                        break;
                        }
                     }
                     else if ((objects->type[o] == object_worker
                               && !((objects->cmd[o][0] == cmd_harvest
                                     || objects->cmd[o][1] == cmd_harvest
                                     || objects->cmd[o][2] == cmd_harvest)
                                    && objects->harvest_type[o] == harvest_built)
                               && (i_random(1, 100) > 70) && objects->on_ship[o] == -1
                               && !worker_set_idle
                               && ((players[pno]->num_markets > 0
                                    && players[pno]->money > price_trader
                                    && players[pno]->wood > wood_trader && num_traders < 3
                                    && num_miners > 2 && num_lumber_jacks > 2)
                                   || should_construct
                                   || (should_to_ship
                                       && ilf->f[objects->x[o]][objects->y[o]] == captain_isle)
                                   || num_heapies_required > 0
                                   || (num_trapies_required > 0 && num_workers > 6
                                       && players[pno]->money > price_trap
                                       && players[pno]->wood > wood_trap))))
                     { // idle_a_worker
                        objects->new_order(o, cmd_idle, 0, 0);
                        worker_set_idle = true;
                        should_construct = false;
                        should_to_ship = false;
                     }
                     else
                     { // check_health_or_tp
                        exec_cnt--;
                        if (objects->cmd[o][0] != cmd_attack && objects->cmd[o][1] != cmd_attack
                            && objects->cmd[o][2] != cmd_attack)
                           n_running++;
                        if (objects->type[o] == object_trader
                            && ((objects->cmd[o][0] == cmd_trade || objects->cmd[o][1] == cmd_trade
                                 || objects->cmd[o][2] == cmd_trade)
                                && (objects->gx[o][0] != xtp[gold_tp]
                                    || objects->gy[o][0] != ytp[gold_tp])
                                && (objects->gx[o][0] != xtp[wood_tp]
                                    || objects->gy[o][0] != ytp[wood_tp])))
                           objects->new_order(o, cmd_idle, 0, 0);
                        if (objects->health[o] < 100
                            && (objects->type[o] == object_pawn || objects->type[o] == object_knight
                                || objects->type[o] == object_archer
                                || objects->type[o] == object_cata
                                || objects->type[o] == object_doktor)
                            && objects->cmd[o][0] != cmd_move_to
                            && objects->cmd[o][1] != cmd_move_to
                            && objects->cmd[o][2] != cmd_move_to)
                        {
                           objects->idle_attack_r[o] = 2;
                           move_a_bit();
                           exec_cnt++;
                        }
                        if (objects->type[o] == object_cata
                            && i_abs(pf->f[objects->x[o]][objects->y[o]]) > 3)
                           objects->new_order(o, cmd_idle, 0, 0);
                     }
                     objects->atta_x[o] = none;
                     { // count
                        switch (objects->type[o])
                        {
                        case object_worker:
                        { // cnt_worker
                           n_workers++;
                           switch (objects->harvest_type[o])
                           {
                           case harvest_wood:
                              n_lumber_jacks++;
                              break;
                           case harvest_gold:
                              n_miners++;
                              break;
                           }
                        };
                        break;
                        case object_pawn:
                           n_pawns++;
                           break;
                        case object_building_site:
                           n_building_sites++;
                           break;
                        case object_site_docks:
                           n_building_sites++;
                           break;
                        case object_ship1:
                           n_ships++;
                           break;
                        case object_knight:
                           n_knights++;
                           break;
                        case object_archer:
                           n_archers++;
                           break;
                        case object_trader:
                           n_traders++;
                           break;
                        case object_cata:
                           n_catas++;
                           break;
                        case object_doktor:
                           n_doktors++;
                           break;
                        }
                        if (doing[o] == doing_attack)
                           n_attack++;
                     }
                  }
               }
               o = old_o;
               urgent = urgent_1;
               urgent_1 = -1;
            }
            o++;
            if (o == max_objects)
            {
               { // status
                  num_workers = n_workers;
                  num_pawns = n_pawns;
                  num_lumber_jacks = n_lumber_jacks;
                  num_miners = n_miners;
                  num_building_sites = n_building_sites;
                  num_ships = n_ships;
                  num_knights = n_knights;
                  num_archers = n_archers;
                  num_traders = n_traders;
                  num_catas = n_catas;
                  num_doktors = n_doktors;
                  num_attack = n_attack;
                  num_running = n_running;
                  is_first = false;
               }
               o = 0;
            }
            objects->fresh_pot = true;
         }
      }
   }
   { // check_forti
      if (objects->num_hd > 0)
      { // handle_hd
         int x_forti;
         int y_forti;

         objects->num_hd--;
         x_forti = objects->hd_x[objects->num_hd];
         y_forti = objects->hd_y[objects->num_hd];
         check_f(x_forti, y_forti, 1, 0);
         check_f(x_forti, y_forti, -1, 0);
         check_f(x_forti, y_forti, 0, 1);
         check_f(x_forti, y_forti, 0, -1);
      }
      else
      {    // handle_normal_forti
         { // incr_forti_pos
            x_forti++;
            if (x_forti == landscape_dx)
            {
               x_forti = 0;
               y_forti++;
            }
            if (y_forti == landscape_dy)
               y_forti = 0;
         }
         if (pf->f[x_forti][y_forti] < 0)
         {
            pf->f[x_forti][y_forti]++;
         }
         check_f(x_forti, y_forti, 1, 0);
         check_f(x_forti, y_forti, -1, 0);
         check_f(x_forti, y_forti, 0, 1);
         check_f(x_forti, y_forti, 0, -1);
      }
   }
}

bool robot::need_another_worker(int il)
{
   return (
       ((players[pno]->num_camps > 0 && num_workers < worker_limit * players[pno]->num_town_halls)
        || (players[pno]->num_camps < 1
            && num_workers < worker_limit / 2 * players[pno]->num_town_halls))
       /* As written, money is only checked in the water-world case. */
       || ((is_water_world && (ilf->num_worker[il] < ilf->num_homes[il] * 2 + 3))
           && players[pno]->money > price_worker));
}

void robot::worker_to_ship(int o)
{
   if (can_walk_to(16, o, objects->x[need_a_captain], objects->y[need_a_captain], 0))
   {
      crew_worker--;
      move_to_ship(o);
      to_ship = true;
   }
}

void robot::move_to_ship(int o)
{
   objects->new_order(o, cmd_move_to, objects->x[need_a_captain], objects->y[need_a_captain]);
   objects->delay[0] = 0;
   if (crew_worker + crew_fighter + crew_healer <= 0)
      need_a_captain = -1;
}

void robot::move_a_bit()
{
   to_ship = false;
   { // perhaps_to_ship
      if (need_a_captain > 0 && objects->on_ship[o] == -1)
      { // check_type_to_ship
         switch (objects->type[o])
         {
         case object_worker:
            if (crew_worker > 0)
               worker_to_ship(o);
            break;
         case object_pawn:
         case object_knight:
         case object_archer:
         case object_cata:
            if (crew_fighter > 0)
            { // fighter_to_ship
               if (can_walk_to(18, o, objects->x[need_a_captain], objects->y[need_a_captain], 0))
               {
                  crew_fighter--;
                  move_to_ship(o);
                  to_ship = true;
               }
            };
            break;
         case object_doktor:
            if (crew_healer > 0)
            { // healer_to_ship
               if (can_walk_to(17, o, objects->x[need_a_captain], objects->y[need_a_captain], 0))
               {
                  crew_healer--;
                  move_to_ship(o);
                  to_ship = true;
               }
            };
            break;
         }
      }
   }
   if (!to_ship)
   { // handle_other
      int xx = objects->x[o];
      int yy = objects->y[o];
      int pot;
      int x;
      int y;

      { // get_x_y
         if ((o % 3) == 0)
         {
            x = f->b_x[1];
            y = f->b_y[1];
         }
         else
         {
            x = f->b_x[0];
            y = f->b_y[0];
         }
         if (x == -1)
         {
            x = f->b_x[0];
            y = f->b_y[0];
         }
      }
      { // get_pot
         double d;

         { // get_base_pot
            switch (objects->type[o])
            {
            case object_pawn:
               pot = 2;
               break;
            case object_knight:
               pot = 2;
               break;
            case object_archer:
               pot = 2;
               break;
            case object_cata:
               pot = 1;
               break;
            case object_doktor:
               pot = 1;
               break;
            default:
               pot = 0;
               break;
            }
         }
         d = (double)players[pno]->num_mans / (double)players[0]->num_mans;
         { // get_approach
            d = (double)(num_knights + num_archers + num_catas + num_pawns)
                / (double)players[0]->num_mans;
            if (d < 0.8)
               pot += (int)((double)(f->max_p - f->f[x][y]) * 0.2 * robo_agress);
            else if (d < 1.2)
               pot += (int)((double)(f->max_p - f->f[x][y]) * 0.3 * robo_agress);
            else if (d < 1.5)
               pot += (int)((double)(f->max_p - f->f[x][y]) * 0.5 * robo_agress);
            else
               pot += (int)((double)(f->max_p - f->f[x][y]));
         }
      }
      if (objects->atta_x[o] != -1)
      { // handle_bad_boy
         int d = 1;

         if (objects->type[o] == object_archer || objects->type[o] == object_cata)
            d = scope_archer;
         if (!can_walk_to(19, o, objects->atta_x[o], objects->atta_y[o], d))
         { // handle_stayaway
            int dxx = objects->atta_x[o] - objects->x[o];
            int dyy = objects->atta_y[o] - objects->y[o];

            if (i_abs(dxx) > i_abs(dyy))
               dyy = 0;
            else
               dxx = 0;
            dxx = i_sign(dxx);
            dyy = i_sign(dyy);
            pf->add_stayaway(objects->atta_x[o], objects->atta_y[o], 9, -dxx, -dyy);
         }
      }
      if (((x != -1 && f->f[xx][yy] != f->f[x][y] + pot)
           || (objects->type[o] != object_worker && pf->f[objects->x[o]][objects->y[o]] > 0))
          && f->b_x[0] != -1)
      { // change_place
         int d;

         { // get_walk_goal
            if (objects->type[o] != object_doktor && f->a_x[0] != -1 && objects->health[o] > 90
                && num_hasar - num_attack > 0)
            {
               { // yeah
                  if (f->a_x[1] != -1 && f->f[f->a_x[1]][f->a_y[1]] < 89 && i_random(1, 100) > 60)
                  {
                     x = f->a_x[1];
                     y = f->a_y[1];
                     d = 1;
                     num_hasar--;
                  }
                  else
                  {
                     x = f->a_x[0];
                     y = f->a_y[0];
                     d = 1;
                     num_hasar--;
                  }
               }
            }
            else
            {
               f->climb(x, y, pot);
               d = 0;
            }
         }
         if (objects->type[o] != object_worker && pf->f[x][y] != 0)
            pf->down(x, y);
         if ((unit[x][y] != none && unit[x][y] != o)
             || (objects->type[o] != object_worker && pf->f[x][y] != 0))
            f->nbest(x, y, pf);
         last_x = x;
         last_y = y;
         if (can_walk_to(20, o, x, y, d)
             && !(objects->type[o] != object_worker && pf->f[x][y] != 0))
         { // move_to_new_goal
            objects->new_order(o, cmd_move_to, x, y);
            if (d == 0)
               doing[o] = doing_nothing;
            else
               doing[o] = doing_attack;
         }
         else
         { // handle_goal_unreachable
            if (objects->type[o] != object_worker && pf->f[objects->x[o]][objects->y[o]] > 0)
            { // try_to_escape
               int x = objects->x[o];
               int y = objects->y[o];

               if (pf->f[x][y] != 0)
               {
                  pf->down(x, y);
                  if (can_walk_to(21, o, x, y, d))
                  {
                     { // move_to_new_goal
                        objects->new_order(o, cmd_move_to, x, y);
                        if (d == 0)
                           doing[o] = doing_nothing;
                        else
                           doing[o] = doing_attack;
                     }
                  }
               }
               else
               { // handle_stayaway
                  int dxx = objects->atta_x[o] - objects->x[o];
                  int dyy = objects->atta_y[o] - objects->y[o];

                  if (i_abs(dxx) > i_abs(dyy))
                     dyy = 0;
                  else
                     dxx = 0;
                  dxx = i_sign(dxx);
                  dyy = i_sign(dyy);
                  pf->add_stayaway(objects->atta_x[o], objects->atta_y[o], 9, -dxx, -dyy);
               }
            }
            else if (objects->type[o] == object_cata)
            { // handle_cata_from_inside
               bool any_wall = false;
               int xw;
               int yw;

               if (i_abs(pf->f[objects->x[o]][objects->y[o]]) < 4)
               { // check_any_if_inside_wall
                  int xo = objects->x[o];
                  int yo = objects->y[o];

                  if (!any_wall)
                     check_any_wall(last_x, last_y, xo, yo, xw, yw, any_wall);
               }
               if (any_wall)
               { // attack_wall
                  objects->new_order(o, cmd_attack, xw, yw);
               }
               else
                  objects->delay_wait(o);
            }
            else
               objects->delay_wait(o);
         }
         objects->atta_x[o] = -1;
      }
      /*
      if (objects->cmd [o][0] == cmd_idle)
         perhaps_to_ship;
*/
   }
}

void robot::handle_camp()
{
   int il = 0;

   if (is_water_world)
      il = isle(objects->x[o], objects->y[o]);
   if (players[pno]->money > price_knight && !need_another_worker(il) && objects->support[o] > 0)
   {
      objects->new_order(o, cmd_train_knight, 0, 0);
   }
}

void robot::handle_docks()
{
   if (players[pno]->money > price_ship1 && players[pno]->wood > wood_ship1
       && players[pno]->num_unis > 0 && num_ships < players[pno]->num_mans / 15)
   {
      objects->new_order(o, cmd_built_ship, 0, 0);
   }
}

void robot::handle_smith()
{
   int il = 0;

   if (is_water_world)
      il = isle(objects->x[o], objects->y[o]);
   if (players[pno]->money > price_cata && !need_another_worker(il) && objects->support[o] > 0)
   {
      objects->new_order(o, cmd_train_cata, 0, 0);
   }
}

void robot::handle_uni()
{
   int il = 0;

   if (is_water_world)
      il = isle(objects->x[o], objects->y[o]);
   if (players[pno]->money > price_doktor && !need_another_worker(il) && objects->support[o] > 0)
   {
      objects->new_order(o, cmd_train_doktor, 0, 0);
   }
}

void robot::handle_mill()
{
   int il = 0;

   if (is_water_world)
      il = isle(objects->x[o], objects->y[o]);
   if (players[pno]->money > price_archer && !need_another_worker(il) && objects->support[o] > 0)
   {
      objects->new_order(o, cmd_train_archer, 0, 0);
   }
}

void robot::handle_trader()
{
   if (i_random(1, 100) > 70)
      objects->new_order(o, cmd_trade, xtp[wood_tp], ytp[wood_tp]);
   else
      objects->new_order(o, cmd_trade, xtp[gold_tp], ytp[gold_tp]);
}

void robot::handle_pawn()
{
   if (objects->health[o] > 90)
      objects->idle_attack_r[o] = 10;
   else
      objects->idle_attack_r[o] = 3;
   move_a_bit();
}

void robot::handle_archer()
{
   move_a_bit();
}

void robot::handle_cata()
{
   bool any_wall = false;
   int xw;
   int yw;

   { // look_for_a_healer
      int ht = objects->harvest_type[o];

      if (ht == -1 || objects->type[ht] != object_doktor || objects->color[ht] != color
          || objects->harvest_type[ht] != o)
         need_a_healer = o;
   }
   if (i_abs(pf->f[objects->x[o]][objects->y[o]]) < 4)
   { // check_any_if_wall
      int xo = objects->x[o];
      int yo = objects->y[o];

      check_any_wall(xo, yo, xo, yo, xw, yw, any_wall);
   }
   if (any_wall)
   { // attack_wall
      objects->new_order(o, cmd_attack, xw, yw);
   }
   else
      move_a_bit();
}

bool robot::can_walk_to(int /* reason */, int id, int x, int y, int dist, bool force_it)

{
   bool res;

   objects->gx[id][0] = x;
   objects->gy[id][0] = y;
   res = objects->man_plan_path(id, dist, true);
   if (!res && force_it && num_heapies_required < 3 && num_heap_run < 5)
   { // start_heap
      bool allready_in_work;
      bool sea_around;

      { // check_allready_in_work
         int j;

         allready_in_work = false;
         for (j = 0; j < num_heap_run; j++)
         {
            if (heap_run_x[j] == objects->x_no_way && heap_run_y[j] == objects->y_no_way)
            {
               allready_in_work = true;
               break;
            }
         }
         for (j = 0; j < num_heapies_required; j++)
         {
            if (heap_x[j] == objects->x_no_way && heap_y[j] == objects->y_no_way)
            {
               allready_in_work = true;
               break;
            }
         }
      }
      { // check_sea_around
         int xmi = i_max(0, objects->x_no_way - 1);
         int xma = i_min(landscape_dx - 1, objects->x_no_way + 2);
         int ymi = i_max(0, objects->y_no_way - 1);
         int yma = i_min(landscape_dy - 1, objects->y_no_way + 2);

         sea_around = false;
         for (int xx = xmi; xx < xma; xx++)
            for (int yy = ymi; yy < yma; yy++)
               if (landscape[xx][yy] == land_sea)
               {
                  sea_around = true;
                  break;
               }
      }
      if ((landscape[objects->x_no_way][objects->y_no_way] == land_water
           || landscape[objects->x_no_way][objects->y_no_way] == land_wall)
          && !sea_around && !allready_in_work)
      { // start_new_order
         heap_x[num_heapies_required] = objects->x_no_way;
         heap_y[num_heapies_required] = objects->y_no_way;
         num_heapies_required++;
      }
   }
   return res;
}

void check_dh(int &d, int h, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
      d = i_max(d, h - landhight[x][y]);
}

int robot::dh(int x, int y)
{
   int h = landhight[x][y];
   int dd;

   dd = 0;
   check_dh(dd, h, x + 1, y);
   check_dh(dd, h, x, y + 1);
   check_dh(dd, h, x - 1, y);
   check_dh(dd, h, x, y - 1);
   return dd;
}

void check_dhabs(int &d, int h, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
      d = i_max(d, i_abs(h - landhight[x][y]));
}

int dhabs(int x, int y)
{
   int h = landhight[x][y];
   int dd;

   dd = 0;
   check_dhabs(dd, h, x + 1, y);
   check_dhabs(dd, h, x, y + 1);
   check_dhabs(dd, h, x - 1, y);
   check_dhabs(dd, h, x, y - 1);
   return dd;
}

void robot::clean_mid(int o)
{
   for (int i = 0; i < 2; i++)
      if (h_mine[i] != -1)
      { // check_mine
         int m = unit[h_x[i]][h_y[i]];

         if ((m != none
              && (objects->is_free[m]
                  || !(objects->type[m] == object_home
                       || (objects->type[m] == object_building_site
                           && objects->dir[m] == object_home))
                  || objects->color[m] != objects->color[o]))
             || m == none)
         {
            h_mine[i] = -1;
            h_cnt[i] = 0;
         }
      }
}

void robot::set_mid(int /* o */, int hx, int hy, int m)
{
   for (int i = 0; i < 2; i++)
      if (h_mine[i] == -1)
      {
         h_mine[i] = m;
         h_x[i] = hx;
         h_y[i] = hy;
         h_cnt[i] = 0;
         break;
      }
}

int robot::home_id(int o)
{
   for (int i = 0; i < 2; i++)
      if (h_mine[i] != -1 && unit[h_x[i]][h_y[i]] == o)
         return i;
   errorstop(1, "robot", "home error");
   return 0;
}

bool robot::robo_can_built(int o, int xx, int yy)
{
   if (objects->can_built(o, xx, yy, cmd_built_home))
   { // check_size
      for (int x = i_max(0, xx - 1); x < i_min(landscape_dx, xx + 3); x++)
         for (int y = i_max(0, yy - 1); y < i_min(landscape_dy, yy + 3); y++)
            if (unit[x][y] != none && objects->type[unit[x][y]] != object_mine
                && is_building(objects->type[unit[x][y]]))
               return false;
      return true;
   }
   return false;
}

void robot::check_f(int x, int y, int dx, int dy)
{
   int xx = x + dx;
   int yy = y + dy;
   int d = direction(dx, dy);

   if (0 <= xx && xx < landscape_dx && 0 <= yy && yy < landscape_dy)
   { // perform_check
      if (b_on(ppf->f[x][y], d))
      { // handle_perhaps_off
         if (landhight[xx][yy] - landhight[x][y] < 2)
         {
            pf->sub_forti(xx, yy, 9, -dx, -dy);
            ppf->f[x][y] = b_reset(ppf->f[x][y], d);
         }
      }
      { // handle_perhaps_on
         if (landhight[xx][yy] - landhight[x][y] >= 2
             || (is_water_world && landscape[x][y] == land_sea))
         {
            pf->add_forti(xx, yy, 9, -dx, -dy, 0);
            ppf->f[x][y] = b_set(ppf->f[x][y], d);
         }
      }
   }
}

void robot::check_any_wall(int x0, int y0, int xm, int ym, int &xw, int &yw, bool &any_wall)
{
   int xmi = i_max(0, x0 - (scope_cata + 4));
   int xma = i_min(landscape_dx - 1, x0 + (scope_cata + 4));
   int ymi = i_max(0, y0 - (scope_cata + 4));
   int yma = i_min(landscape_dy - 1, y0 + (scope_cata + 4));
   double best_d = DBL_MAX;

   any_wall = false;
   for (int xx = xmi; xx <= xma; xx++)
      for (int yy = ymi; yy <= yma; yy++)
      {
         if (landscape[xx][yy] == land_wall && dist(xx, yy, xm, ym) < best_d)
         {
            any_wall = true;
            xw = xx;
            yw = yy;
            best_d = dist(xx, yy, xm, ym);
         }
      }
}

bool robot::look_for_dock_place(int &x, int &y, double dx, double dy)
{
   double xx = x;
   double yy = y;

   { // search_water
      while (landscape[(int)xx][(int)yy] != land_water
             && (0 < (int)xx && (int)xx < landscape_dx - 3 && 0 < (int)yy
                 && (int)yy < landscape_dy - 3))
      {
         xx += dx;
         yy += dy;
      }
   }
   if ((0 < (int)xx && (int)xx < landscape_dx - 3 && 0 < (int)yy && (int)yy < landscape_dy - 3))
   { // look_for_a_dock_place
      for (int xa = (int)xx - (3 * (int)d_sign(dx)); xa != (int)xx; xa += (int)d_sign(dx))
         for (int ya = (int)yy - (3 * (int)d_sign(dy)); ya != (int)yy; ya += (int)d_sign(dy))
            if (objects->can_built(0, xa, ya, cmd_built_docks, true))
            {
               x = xa;
               y = ya;
               return true;
            }
      return false;
   }
   else
      return false;
}

bool robot::look_for_dock_place(int &x, int &y)
{
   point d = new_point(landscape_dx / 2, landscape_dy / 2, 0);
   point z = new_point(x, y, 0);
   vector v = set_length(new_vector(new_point(x, y, 0), d), 1);

   d = new_point(x, y, 0) + v;
   for (int a = 0; a <= 180; a += 20)
   { // check_dir
      point np;
      vector dd;

      np = rotate(d, new_line(z, z + new_vector(0, 0, 1)), a);
      dd = new_vector(z, np);
      if (look_for_dock_place(x, y, dd.dx, dd.dy))
         return true;
      np = rotate(d, new_line(z, z + new_vector(0, 0, 1)), -a);
      dd = new_vector(z, np);
      if (look_for_dock_place(x, y, dd.dx, dd.dy))
         return true;
   }
   return false;
}

bool robot::enter_goal(int o, int &xg, int &yg)
{
   int n_worker;
   int n_fighter;
   int n_healer;

   objects->s[o]->get_crew(n_worker, n_fighter, n_healer);
   if (n_fighter > 2)
   { // look_for_a_nice_goal
      int xx = 0;
      int yy = 0;
      double best_dist = DBL_MAX;
      int xo = objects->x[o];
      int yo = objects->y[o];

      for (int i = 0; i < max_objects; i++)
      { // check_it
         int xe = objects->x[i];
         int ye = objects->y[i];

         if (!objects->is_free[i] && objects->color[i] != objects->color[o]
             && objects->type[i] == object_ship1 && dist(xo, yo, xe, ye) < best_dist
             && can_walk_to(45, o, xe, ye, 3, false))
         { // grab_it
            xx = xe;
            yy = ye;
            best_dist = dist(xo, yo, xe, ye);
         }
      }
      if (best_dist != DBL_MAX)
      {
         xg = xx;
         yg = yy;
         return true;
      }
      return false;
   }
   return false;
}

bool robot::isle_goal(int o, int isle, int &xg, int &yg)
{
   bool reachable = false;
   double dx;
   double dy;
   double xx;
   double yy;

   xx = ilf->x_il[isle];
   yy = ilf->y_il[isle];
   dx = (double)i_random(-4, 4) / 4.0;
   dy = (double)i_random(-4, 4) / 4.0;
   if (dx == 0 && dy == 0)
   {
      dx = 1;
   }
   { // search_cost
      while ((0 <= (int)xx && (int)xx < landscape_dx && 0 <= (int)yy && (int)yy < landscape_dy)
             && (!(landscape[(int)xx][(int)yy] == land_water
                   || landscape[(int)xx][(int)yy] == land_sea)
                 || !(reachable = can_walk_to(22, o, (int)xx, (int)yy, 0))))
      {
         { // incr
            int xa = (int)xx;
            int ya = (int)yy;

            while ((int)xx == xa && (int)yy == ya)
            {
               xx += dx;
               yy += dy;
            }
         }
      }
   }
   xg = (int)xx;
   yg = (int)yy;
   return reachable;
}

int robot::isle(int x, int y)
{
   double best_dist = DBL_MAX;
   int il = 0;

   for (int xx = i_max(0, objects->x[o] - 6); xx < i_min(landscape_dx - 1, objects->x[o] + 7); xx++)
      for (int yy = i_max(0, objects->y[o] - 6); yy < i_min(landscape_dy - 1, objects->y[o] + 7);
           yy++)
         if (ilf->f[xx][yy] >= 0 && sqrt((xx - x) * (xx - x) + (yy - y) * (yy - y)) < best_dist)
         {
            best_dist = sqrt((xx - x) * (xx - x) + (yy - y) * (yy - y));
            il = ilf->f[xx][yy];
         }
   return il;
}
