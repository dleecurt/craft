/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from land.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- land.cc land.hc
 */

#include "land.h"
#include "errorhandling.h"
#include "craft_def.h"
#include "craft.h"
#include "xfile.h"
#include "win.h"
#include "player.h"

int xx_il[15][2000];
int yy_il[15][2000];

int num_il;
int x_il[15];
int y_il[15];
int ss[15];

void trace_il()
{
   for (int i = 0; i < num_il; i++)
      for (int s = 0; s < ss[i]; s++)
         players[0]->show_int(xx_il[i][s], yy_il[i][s], i);
}

int land_num_il()
{
   return num_il;
}

int land_x_il(int i)
{
   return x_il[i];
}

int land_y_il(int i)
{
   return y_il[i];
}

void new_land(char name[], int dx, int dy)
{
   { // gen_landscape
      FILE *f;

      f_open(f, complete(name, ".land"), "w");
      fprintf(f, "%d %d %d\n", 1, dx, dy);
      for (int y = 0; y < dy; y++)
      {
         for (int x = 0; x < dx; x++)
            fprintf(f, "%d ", land_grass);
         fprintf(f, "\n");
      }
      fclose(f);
   }
   { // gen_landhight
      FILE *f;

      f_open(f, complete(name, ".lhight"), "w");
      for (int y = 0; y < dy; y++)
      {
         for (int x = 0; x < dx; x++)
            fprintf(f, "%d ", 1);
         fprintf(f, "\n");
      }
      fclose(f);
   }
   { // gen_units
      FILE *f;

      f_open(f, complete(name, ".units"), "w");
      fprintf(f, "%d ", 0);
      fclose(f);
   }
}

void init_land_units()
{
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
      {
         unit[x][y] = none;
         for (int k = 0; k < 8; k++)
         {
            upic[x][y][k] = none;
         }
      }
}

void load_units(char name[])
{
   FILE *f;
   int num_units;

   { // open_f
      f_open(f, complete(name, ".units"), "r");
   }
   { // get_size
      read_check(fscanf(f, "%d", &num_units), 1, "land file");
   }
   { // read_units
      for (int i = 0; i < num_units; i++)
      { // read_unit
         int type;
         int x;
         int y;
         int color;
         int money;

         read_check(fscanf(f, "%d %d %d %d %d", &type, &x, &y, &color, &money), 5, "land file");
         if (type == object_mine)
         {
            objects->create_building(x, y, type, money, 0, none);
         }
         if (type == object_home)
         {
            objects->create_building(x, y, type, 0, 0, none);
         }
      }
   }
   fclose(f);
}

void load_land(char name[])
{
   FILE *f;
   FILE *f1;

   { // open_f
      f_open(f, complete(name, ".land"), "r");
      f_open(f1, complete(name, ".lhight"), "r");
   }
   { // get_size
      int d;

      read_check(fscanf(f, "%d %d %d", &d, &landscape_dx, &landscape_dy), 3, "land file");
   }
   { // read_landscape
      for (int y = 0; y < landscape_dy; y++)
         for (int x = 0; x < landscape_dx; x++)
         {
            read_check(fscanf(f, "%d", &landscape[x][y]), 1, "land file");
            read_check(fscanf(f1, "%d", &landhight[x][y]), 1, "land file");
         }
   }
   fclose(f);
   fclose(f1);
   { // adjust_pics
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         { // check_field
            landpic[x][y] = landscape[x][y];
            landoverlay[x][y][0] = none;
            landoverlay[x][y][1] = none;
            if (land_properties[landscape[x][y]].with_hl)
            { // apply_hl
               landpic[x][y] += land_profile(x, y, 1);
            }
         }
   }
}

void heap(int xx, int yy, int h, bool up)
{
   if (0 <= xx && xx < landscape_dx && 0 <= yy && yy < landscape_dy)
   { // perform_heap
      int x;
      int y;

      if (((up && landhight[xx][yy] < h) || (!up && landhight[xx][yy] > h))
          && (h > 0 || unit[xx][yy] == none))
      {
         landhight[xx][yy] = h;
      }
      if (h >= -1)
      { // set_ilid
         int best_dist = INT_MAX;
         int id;

         for (x = i_max(0, xx - 3); x < i_min(landscape_dx - 1, xx + 4); x++)
            for (y = i_max(0, yy - 3); y < i_min(landscape_dy - 1, yy + 4); y++)
               if (ilid[x][y] != -1 && landhight[x][y] > 0
                   && ((xx - x) * (xx - x) + (yy - y) * (yy - y)) < best_dist)
               {
                  id = ilid[x][y];
                  best_dist = ((xx - x) * (xx - x) + (yy - y) * (yy - y));
               }
         if (best_dist != INT_MAX)
            ilid[xx][yy] = id;
      }
      for (x = i_max(0, xx - 1); x < i_min(landscape_dx, xx + 2); x++)
         for (y = i_max(0, yy - 1); y < i_min(landscape_dy, yy + 2); y++)
            if (x != xx || y != yy)
            { // check_field
               if (h - landhight[x][y] > 1 && up)
                  heap(x, y, h - 1, up);
               else if (h - landhight[x][y] < -1 && !up)
                  heap(x, y, h + 1, up);
            }
   }
}

int rnd_grass_pic()
{
   return land_grass;
}

void rnd_land()
{
   { // rnd_size
      landscape_dx = i_random(min_rland_dx, max_rland_dx);
      landscape_dy = i_random(min_rland_dy, max_rland_dy);
      if (landscape_dx > 120 || landscape_dy > 120)
         landscale = 1;
      else
         landscale = 2;
   }
   { // init_land
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            landhight[x][y] = 1;
            landscape[x][y] = land_grass;
            landpic[x][y] = rnd_grass_pic();
            landoverlay[x][y][0] = none;
            landoverlay[x][y][1] = none;
         }
      init_land_units();
   }
   { // rnd_hight
      int minh;
      int maxh;

      if (landscale == 2)
      {
         minh = 4;
         maxh = 24;
      }
      else
      {
         minh = 16;
         maxh = 100;
      };
      for (int hi = i_random(minh, maxh); hi > 0; hi--)
      {
         { // gen_hill
            int h = i_random(1, 5);
            int x;
            int y;
            int dxx = i_random(6 - h, 12 - h);
            int dyy = i_random(6 - h, 12 - h);

            { // rnd_xy
               x = i_random(1, landscape_dx - 2);
               y = i_random(1, landscape_dy - 2);
            }
            for (int xx = i_max(0, x - dxx); xx < i_min(landscape_dx - 1, x + dxx); xx++)
               for (int yy = i_max(0, y - dyy); yy < i_min(landscape_dy - 1, y + dyy); yy++)
               {
                  heap(xx, yy, h, true);
               }
         }
      }
   }
   { // rnd_rivers
      int num_r;
      int dd;
      int bb;

      if (landscale == 2)
      {
         num_r = 2;
         dd = 20;
         bb = 4;
      }
      else
      {
         num_r = 5;
         dd = 20;
         bb = 20;
      }
      for (int r = i_random(0, num_r); r > 0; r--)
      { // gen_one_river
         if (i_random(1, 100) > 50)
         { // lr_river
            int x = 0;
            int y = i_random(dd, landscape_dy - dd);
            int dx = 1;
            int dy = 0;
            int b = i_random(2, bb);

            { // river
               while (x != landscape_dx && y != landscape_dy)
               {
                  {    // rstep
                     { // rset
                        if (dx == 0)
                           for (int a = y - b / 2; a < y + b / 2; a++)
                              heap(x, a, 0, false);
                        else
                           for (int a = y - b / 2; a < y + b / 2; a++)
                              heap(x, a, 0, false);
                     }
                     x += dx;
                     y += dy;
                     if (dx != 0 && i_random(1, 100) > 60)
                        y = i_bound(6, y + i_random(-1, 1), landscape_dy - 6);
                     if (dy != 0 && i_random(1, 100) > 60)
                        x = i_bound(6, x + i_random(-1, 1), landscape_dx - 6);
                     if (i_random(1, 100) > 60)
                        b = i_bound(2, b + i_random(-1, 1), 4);
                  }
               }
            }
         }
         else
         { // up_river
            int y = 0;
            int x = i_random(dd, landscape_dx - dd);
            int dx = 0;
            int dy = 1;
            int b = i_random(2, bb);

            { // river
               while (x != landscape_dx && y != landscape_dy)
               {
                  {    // rstep
                     { // rset
                        if (dx == 0)
                           for (int a = y - b / 2; a < y + b / 2; a++)
                              heap(x, a, 0, false);
                        else
                           for (int a = y - b / 2; a < y + b / 2; a++)
                              heap(x, a, 0, false);
                     }
                     x += dx;
                     y += dy;
                     if (dx != 0 && i_random(1, 100) > 60)
                        y = i_bound(6, y + i_random(-1, 1), landscape_dy - 6);
                     if (dy != 0 && i_random(1, 100) > 60)
                        x = i_bound(6, x + i_random(-1, 1), landscape_dx - 6);
                     if (i_random(1, 100) > 60)
                        b = i_bound(2, b + i_random(-1, 1), 4);
                  }
               }
            }
         }
      }
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            if (landhight[x][y] <= 0)
               landscape[x][y] = land_water;
            if (landhight[x][y] <= -3)
               landscape[x][y] = land_sea;
            landpic[x][y] = landscape[x][y] + land_profile(x, y, 1);
         }
   }
   { // rnd_mines
      int nn;

      { // get_nn
         if (landscale == 2)
            nn = 4;
         else
            nn = 6;
      }
      for (int xm = 1; xm < nn; xm++)
         for (int ym = 1; ym < nn; ym++)
         {
            { // set_mine
               int x;
               int y;
               int m;

               { // rnd_mxy
                  x = i_random(landscape_dx / nn * xm - 3, landscape_dx / nn * xm + 3);
                  y = i_random(landscape_dy / nn * ym - 3, landscape_dy / nn * ym + 3);
               }
               if (landhight[x][y] < 1)
                  for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
                     for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
                     {
                        heap(xx, yy, 1, true);
                        if (xx != x || yy != y)
                           landscape[xx][yy] = land_grass;
                     }
               for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
                  for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
                  {
                     if (landhight[xx][yy] > 0)
                        landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
                  }
               m = 8000;
               objects->create_building(x, y, object_mine, m, 11, none);
            }
         }
   }
   { // rnd_trade_points
      int num_t_wood = 2;
      int num_t_gold = 2;
      int xt;
      int yt;

      xt = landscape_dx / 2;
      yt = 0;
      { // set_trade_point
         int x;
         int y;
         int tp;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
            heap(x, y, 1, true);
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, landhight[x][y], true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
      }
      xt = landscape_dx / 2;
      yt = landscape_dy - 1;
      { // set_trade_point
         int x;
         int y;
         int tp;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
            heap(x, y, 1, true);
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, landhight[x][y], true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
      }
      xt = 0;
      yt = landscape_dy / 2;
      { // set_trade_point
         int x;
         int y;
         int tp = 0;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
            heap(x, y, 1, true);
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, landhight[x][y], true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
      }
      xt = landscape_dx - 1;
      yt = landscape_dy / 2;
      { // set_trade_point
         int x;
         int y;
         int tp = 0;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
            heap(x, y, 1, true);
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, landhight[x][y], true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
      }
   }
   { // rnd_wood
      int f = landscape_dx * landscape_dy;

      for (int hi = f / (9 * 6); hi > 0; hi--)
      {
         { // gen_wood
            int x;
            int y;

            { // rnd_xy
               x = i_random(1, landscape_dx - 2);
               y = i_random(1, landscape_dy - 2);
            }
            for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx - 1, x + 2); xx++)
               for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy - 1, y + 2); yy++)
                  if (landpic[xx][yy] == land_grass)
                  {
                     { // woody
                        if (xx == x && yy == y)
                        {
                           landpic[xx][yy] = land_wood;
                           landscape[xx][yy] = land_wood;
                        }
                        else
                        {
                           landpic[xx][yy] = land_wood - 1;
                           landscape[xx][yy] = land_wood - 1;
                        }
                     }
                  }
         }
      }
   }
}

void island(int i, int x, int y, int &size, int length)
{
   int cx[10000];
   int cy[10000];
   int c0;
   int c1;
   int s;

   { // init
      c1 = 0;
      s = 0;
      c0 = 0;
   }
   { // gen_root

      for (int l = 0; l < length; l++)
      {
         cx[c1] = x + l;
         cy[c1] = y;
         c1++;
         cx[c1] = x + l;
         cy[c1] = y + 1;
         c1++;
         cx[c1] = x + l;
         cy[c1] = y - 1;
         c1++;
      }
   }
   while (c1 > c0)
   {
      { // heap_a_bit
         if (0 <= cx[c0] && cx[c0] < landscape_dx && 0 <= cy[c0] && cy[c0] < landscape_dy)
         { // handle_cand
            if (landscape[cx[c0]][cy[c0]] == land_sea)
            {
               heap(cx[c0], cy[c0], 1, true);
               landscape[cx[c0]][cy[c0]] = land_grass;
            }
            if (i >= 0)
            {
               xx_il[i][s] = cx[c0];
               yy_il[i][s] = cy[c0];
               ilid[cx[c0]][cy[c0]] = i;
            }
            s++;
            if (s < size && c1 < 10000)
            { // push_new_cands
               for (int xx = cx[c0] - 1; xx <= cx[c0] + 1; xx++)
                  for (int yy = cy[c0] - 1; yy <= cy[c0] + 1; yy++)
                     if (0 <= xx && xx < landscape_dx && 0 <= yy && yy < landscape_dy
                         && landhight[xx][yy] < 1 && (i_random(0, 100) > 70 || size < 50))
                     {
                        cx[c1] = xx;
                        cy[c1] = yy;
                        c1++;
                        landhight[xx][yy] = 1;
                     }
            }
         }
         c0++;
      }
   }
   size = s;
}

void seperate_islands()
{
   for (int x = 1; x < landscape_dx - 1; x++)
      for (int y = 1; y < landscape_dy - 1; y++)
         if (ilid[x][y] != -1 && ilid[x][y] != 57)
         { // check_field
            bool border = false;

            { // check_border
               for (int xx = x - 1; xx < x + 2 && !border; xx++)
                  for (int yy = y - 1; yy < y + 2 && !border; yy++)
                     if (ilid[xx][yy] != -1 && ilid[xx][yy] != ilid[x][y]
                         && landhight[xx][yy] >= -1)
                        border = true;
            }
            if (border)
               heap(x, y, -2, false);
         }
}

void close_islands()
{
   bool any_hit = true;

   while (any_hit)
   {
      { // session
         any_hit = false;
         for (int x = 0; x < landscape_dx; x++)
            for (int y = 0; y < landscape_dy; y++)
               if (landhight[x][y] > 0)
               { // handle_field
                  int n;

                  { // count_n
                     n = 0;
                     for (int xx = x - 1; xx <= x + 1; xx++)
                        for (int yy = y - 1; yy <= y + 1; yy++)
                           if (xx == x || yy == y)
                           { // check_f
                              if (xx < 0 || xx >= landscape_dx || yy < 0 || yy >= landscape_dy
                                  || landhight[xx][yy] > 0)
                                 n++;
                           }
                  }
                  if (n < 3 && unit[x][y] == none)
                  { // remove_field
                     landhight[x][y] = 0;
                     landscape[x][y] = land_water;
                     any_hit = true;
                  }
               }
      }
   }
}

/* A random free land square on island i.  False if there is none
   (it used to try forever, hanging the start of some games). */
bool island_rnd(int i, int &x, int &y)
{
   if (ss[i] <= 0)
      return false;
   for (int tries = 0; tries < 2000; tries++)
   { // rnd
      int k = i_random(0, ss[i] - 1);

      x = xx_il[i][k];
      y = yy_il[i][k];
      if (landhight[x][y] > 0 && unit[x][y] == none)
         return true;
   }
   for (int k = 0; k < ss[i]; k++)
   { // first_free_square
      x = xx_il[i][k];
      y = yy_il[i][k];
      if (landhight[x][y] > 0 && unit[x][y] == none)
         return true;
   }
   return false;
}

void prop_il_id()
{
   for (int i = 0; i < 15; i++)
      ss[i] = 0;
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
         if (ilid[x][y] != -1)
         {
            xx_il[ilid[x][y]][ss[ilid[x][y]]] = x;
            yy_il[ilid[x][y]][ss[ilid[x][y]]] = y;
            ss[ilid[x][y]]++;
         }
}

void rnd_water_land()
{

   { // rnd_size
      landscape_dx = i_random((int)((double)min_rland_dx * 1.0), (int)((double)max_rland_dx * 1.0));
      landscape_dy = i_random((int)((double)min_rland_dy * 1.5), (int)((double)max_rland_dy * 1.5));
      if (landscape_dx > 120 || landscape_dy > 120)
         landscale = 1;
      else
         landscale = 2;
   }
   { // init_land
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            landhight[x][y] = -10;
            landscape[x][y] = land_sea;
            landpic[x][y] = land_sea;
            landoverlay[x][y][0] = none;
            landoverlay[x][y][1] = none;
            ilid[x][y] = -1;
         }
      init_land_units();
   }
   { // set_islands
      num_il = i_random(i_max(num_players, 3), 6);
      for (int i = 0; i < num_il; i++)
      { // gen_island
         int ll = i_random(1, 5);

         { // get_ix_iy
            x_il[i] = i_random(0, landscape_dx - 1);
            y_il[i] = i_random(0, landscape_dy - 1);

            while (landhight[x_il[i]][y_il[i]] > -10)
            {
               x_il[i] = i_random(0, landscape_dx - 1);
               y_il[i] = i_random(0, landscape_dy - 1);
            }
         }
         ss[i] = i_random(50, 200);
         island(i, x_il[i], y_il[i], ss[i], ll);
      }
   }
   { // rnd_hight
      for (int i = 0; i < num_il; i++)
         for (int hi = i_random(4, 10); hi > 0; hi--)
         {
            { // gen_hill
               int h = i_random(1, 3);
               int x;
               int y;
               int dxx = i_random(6 - h, 12 - h);
               int dyy = i_random(6 - h, 12 - h);

               { // rnd_mxy
                  if (!island_rnd(i, x, y))
                     continue;
               }
               for (int xx = i_max(0, x - dxx); xx < i_min(landscape_dx - 1, x + dxx); xx++)
                  for (int yy = i_max(0, y - dyy); yy < i_min(landscape_dy - 1, y + dyy); yy++)
                  {
                     heap(xx, yy, h, true);
                     landscape[xx][yy] = land_grass;
                     landpic[xx][yy] = land_grass;
                  }
            }
         }
      { // readjust_land
         for (int x = 0; x < landscape_dx; x++)
            for (int y = 0; y < landscape_dy; y++)
            {
               if (landhight[x][y] > 0
                   && (landpic[x][y] == land_sea || landpic[x][y] == land_water))
               {
                  landscape[x][y] = land_grass;
                  landpic[x][y] = land_grass;
               }
               if (landhight[x][y] <= -1)
                  landhight[x][y] = -20;
               if (landhight[x][y] <= 0 && landhight[x][y] > -3)
               {
                  landscape[x][y] = land_water;
                  landpic[x][y] = land_water;
                  landhight[x][y] = 0;
               }
               if (landhight[x][y] <= -3)
               {
                  landscape[x][y] = land_sea;
                  landpic[x][y] = land_sea;
               }
               if (landscape[x][y] == land_grass)
                  landpic[x][y] = landscape[x][y] + land_profile(x, y, 1);
            }
      }
   }
   { // expand_hight
      for (int y = 2; y < landscape_dy - 3; y++)
         for (int x = 2; x < landscape_dx - 3; x++)
            if (landhight[x][y] > 1)
            { // check_h_field
               bool any_water;

               { // check_any_water
                  any_water = false;
                  for (int xx = i_max(0, x - 3); !any_water && xx < i_min(landscape_dx, x + 4);
                       xx++)
                     for (int yy = i_max(0, y - 3); !any_water && yy < i_min(landscape_dy, y + 4);
                          yy++)
                        any_water = landhight[xx][yy] < 1;
               }
               if (any_water)
                  heap(x, y, 1, false);
            }
   }
   prop_il_id();
   seperate_islands();
   { // rnd_mines
      for (int i = 0; i < num_il; i++)
         for (int z = 0; z < 3; z++)
         {
            { // set_mine
               int x;
               int y;
               int m;

               { // rnd_mxy
                  if (!island_rnd(i, x, y))
                     continue;
               }
               for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
                  for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
                  {
                     if (landhight[x][y] < 1)
                     {
                        heap(xx, yy, 1, true);
                        if (xx != x || yy != y)
                           landscape[xx][yy] = land_grass;
                     }
                  }
               m = 8000;
               objects->create_building(x, y, object_mine, m, 11, none);
            }
         }
   }
   { // rnd_trade_points
      int num_t_wood = 2;
      int num_t_gold = 2;
      int xt;
      int yt;
      int dx;
      int dy;

      xt = landscape_dx / 2;
      yt = 0;
      dx = 0;
      dy = 1;
      { // set_trade_point
         int x;
         int y;
         int tp;
         int h;
         int s;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
         {
            heap(x, y, 1, true);
         }
         h = landhight[x][y];
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, h, true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
         s = 30;
         x_il[num_il] = x + 2 * dx;
         y_il[num_il] = y + 2 * dy;
         num_il++;
         island(ilid[x + 2 * dx][y + 2 * dy], x + 2 * dx, y + 2 * dy, s, 1);
      }
      xt = landscape_dx / 2;
      yt = landscape_dy - 1;
      dx = 0;
      dy = -1;
      { // set_trade_point
         int x;
         int y;
         int tp;
         int h;
         int s;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
         {
            heap(x, y, 1, true);
         }
         h = landhight[x][y];
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, h, true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
         s = 30;
         x_il[num_il] = x + 2 * dx;
         y_il[num_il] = y + 2 * dy;
         num_il++;
         island(ilid[x + 2 * dx][y + 2 * dy], x + 2 * dx, y + 2 * dy, s, 1);
      }
      xt = 0;
      yt = landscape_dy / 2;
      dx = 1;
      dy = 0;
      { // set_trade_point
         int x;
         int y;
         int tp = 0;
         int h;
         int s;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
         {
            heap(x, y, 1, true);
         }
         h = landhight[x][y];
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, h, true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
         s = 30;
         x_il[num_il] = x + 2 * dx;
         y_il[num_il] = y + 2 * dy;
         num_il++;
         island(ilid[x + 2 * dx][y + 2 * dy], x + 2 * dx, y + 2 * dy, s, 1);
      }
      xt = landscape_dx - 1;
      yt = landscape_dy / 2;
      dx = -1;
      dy = 0;
      { // set_trade_point
         int x;
         int y;
         int tp = 0;
         int h;
         int s;

         { // get_tp
            if (num_t_wood > 0 && num_t_gold > 0)
            {
               if (i_random(0, 100) > 50)
               {
                  tp = land_t_gold;
                  num_t_gold--;
               }
               else
               {
                  tp = land_t_wood;
                  num_t_wood--;
               }
            }
            else if (num_t_wood > 0)
            {
               tp = land_t_wood;
               num_t_wood--;
            }
            else if (num_t_gold > 0)
            {
               tp = land_t_gold;
               num_t_gold--;
            }
         }
         { // rnd_tp
            x = xt;
            y = yt;
            if (x == 0 || x == landscape_dx - 1)
               y += i_random(-15, 15);
            if (y == 0 || y == landscape_dy - 1)
               x += i_random(-15, 15);
         }
         if (landhight[x][y] < 1)
         {
            heap(x, y, 1, true);
         }
         h = landhight[x][y];
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               heap(xx, yy, h, true);
               if (xx != x || yy != y)
                  landscape[xx][yy] = land_grass;
            }
         landscape[x][y] = tp;
         landpic[x][y] = tp;
         for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx, x + 2); xx++)
            for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy, y + 2); yy++)
            {
               if (landhight[xx][yy] > 0)
                  landpic[xx][yy] = landscape[xx][yy] + land_profile(xx, yy, 1);
            }
         s = 30;
         x_il[num_il] = x + 2 * dx;
         y_il[num_il] = y + 2 * dy;
         num_il++;
         island(ilid[x + 2 * dx][y + 2 * dy], x + 2 * dx, y + 2 * dy, s, 1);
      }
   }
   { // rnd_wood
      int j;

      for (int i = 0; i < num_il - 4; i++)
      {
         j = ss[i] / 3 * 100;
         for (int hi = 0; hi < ss[i] / 6 && j-- > 0;)
         { // gen_wood
            int x;
            int y;

            { // rnd_wxy
               int k = i_random(0, ss[i] - 1);

               x = xx_il[i][k];
               y = yy_il[i][k];
            }
            for (int xx = i_max(0, x - 1); xx < i_min(landscape_dx - 1, x + 2); xx++)
               for (int yy = i_max(0, y - 1); yy < i_min(landscape_dy - 1, y + 2); yy++)
                  if (landscape[xx][yy] == land_grass)
                  {
                     { // woody
                        if (xx == x && yy == y)
                        {
                           landpic[xx][yy] = land_wood;
                           landscape[xx][yy] = land_wood;
                        }
                        else
                        {
                           landpic[xx][yy] = land_wood - 1;
                           landscape[xx][yy] = land_wood - 1;
                        }
                     }
                     { // reheap
                        for (int xa = i_max(0, xx - 3); xa < i_min(landscape_dx - 1, xx + 4); xa++)
                           for (int ya = i_max(0, yy - 3); ya < i_min(landscape_dy - 1, yy + 4);
                                ya++)
                              if (landhight[xa][ya] < 1)
                              {
                                 heap(xa, ya, 1, true);
                                 landscape[xa][ya] = land_grass;
                              }
                     }
                     hi++;
                  }
         }
      }
   }
   seperate_islands();
   { // readjust_land
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            if (landhight[x][y] > 0 && (landpic[x][y] == land_sea || landpic[x][y] == land_water))
            {
               landscape[x][y] = land_grass;
               landpic[x][y] = land_grass;
            }
            if (landhight[x][y] <= -1)
               landhight[x][y] = -20;
            if (landhight[x][y] <= 0 && landhight[x][y] > -3)
            {
               landscape[x][y] = land_water;
               landpic[x][y] = land_water;
               landhight[x][y] = 0;
            }
            if (landhight[x][y] <= -3)
            {
               landscape[x][y] = land_sea;
               landpic[x][y] = land_sea;
            }
            if (landscape[x][y] == land_grass)
               landpic[x][y] = landscape[x][y] + land_profile(x, y, 1);
         }
   }
   { // deep_rivers
      for (int x = 0; x < landscape_dx; x++)
      { // fill_up_col
         int last_y;
         int y;

         last_y = 0;
         for (y = 0; y < landscape_dy && landhight[x][y] < 1; y++)
            ;
         { // perhaps_fill_up_y
            if (y - last_y > 1 && y - last_y < 4)
            { // fill_y
               for (int yy = last_y; yy < y; yy++)
               {
                  heap(x, yy, 1, true);
               }
            }
         }
         y = landscape_dx;
         for (last_y = landscape_dy; last_y >= 0 && landhight[x][last_y] < 1; last_y--)
            ;
         { // perhaps_fill_up_y
            if (y - last_y > 1 && y - last_y < 4)
            { // fill_y
               for (int yy = last_y; yy < y; yy++)
               {
                  heap(x, yy, 1, true);
               }
            }
         }
      }
      for (int y = 0; y < landscape_dy; y++)
      { // fill_up_row
         int last_x;
         int x;

         last_x = 0;
         for (x = 0; x < landscape_dx && landhight[x][y] < 1; x++)
            ;
         { // perhaps_fill_up_x
            if (x - last_x > 1 && x - last_x < 4)
            { // fill_x
               for (int xx = last_x; xx < x; xx++)
               {
                  heap(xx, y, 1, true);
               }
            }
         }
         x = landscape_dx;
         for (last_x = landscape_dx; last_x >= 0 && landhight[last_x][y] < 1; last_x--)
            ;
         { // perhaps_fill_up_x
            if (x - last_x > 1 && x - last_x < 4)
            { // fill_x
               for (int xx = last_x; xx < x; xx++)
               {
                  heap(xx, y, 1, true);
               }
            }
         }
      }
   }
   close_islands();
   { // readjust_land
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            if (landhight[x][y] > 0 && (landpic[x][y] == land_sea || landpic[x][y] == land_water))
            {
               landscape[x][y] = land_grass;
               landpic[x][y] = land_grass;
            }
            if (landhight[x][y] <= -1)
               landhight[x][y] = -20;
            if (landhight[x][y] <= 0 && landhight[x][y] > -3)
            {
               landscape[x][y] = land_water;
               landpic[x][y] = land_water;
               landhight[x][y] = 0;
            }
            if (landhight[x][y] <= -3)
            {
               landscape[x][y] = land_sea;
               landpic[x][y] = land_sea;
            }
            if (landscape[x][y] == land_grass)
               landpic[x][y] = landscape[x][y] + land_profile(x, y, 1);
         }
   }
}

void save_land(char name[])
{
   FILE *f;
   FILE *f1;
   FILE *f2;

   { // open_f
      f_open(f, complete(name, ".land"), "w");
      f_open(f1, complete(name, ".lhight"), "w");
      f_open(f2, complete(name, ".units"), "w");
   }
   { // write_size

      fprintf(f, "%d %d %d\n", 1, landscape_dx, landscape_dy);
   }
   { // write_landscape
      for (int y = 0; y < landscape_dy; y++)
      {
         for (int x = 0; x < landscape_dx; x++)
         {
            if (landscape[x][y] == land_building)
               fprintf(f, "%d ", land_grass);
            else
               fprintf(f, "%d ", landscape[x][y]);
            fprintf(f1, "%d ", landhight[x][y]);
         }
         fprintf(f, "\n");
         fprintf(f1, "\n");
      }
   }
   { // save_units
      int num_mines = 0;
      int i;

      for (i = 0; i < max_objects; i++)
         if (!objects->is_free[i]
             && (objects->type[i] == object_mine || objects->type[i] == object_home))
            num_mines++;
      fprintf(f2, "%d\n", num_mines);
      for (i = 0; i < max_objects; i++)
         if (!objects->is_free[i]
             && (objects->type[i] == object_mine || objects->type[i] == object_home))
            fprintf(f2, "%d %d %d %d %d\n", objects->type[i], objects->x[i], objects->y[i],
                    objects->color[i], objects->money[i]);
   }
   fclose(f);
   fclose(f1);
   fclose(f2);
}

void load_land_props()
{
   { // set_defaults
      for (int i = 0; i < max_land_types; i++)
      {
         land_properties[i].overview_color = black;
         land_properties[i].walk_possible = false;
         land_properties[i].with_hl = false;
         land_properties[i].is_forest = false;
         land_properties[i].is_grass = false;
         land_properties[i].is_dig = false;
         land_properties[i].is_water = false;
         land_properties[i].can_grow = false;
      }
   }
   { // load_properties
      FILE *infos;
      int lno;

      { // open_infos
         f_open(infos, info_name, "r");
      }
      { // skip_first_line
         while (fgetc(infos) != '\n')
         {
         }
      }
      while ((fscanf(infos, "%d", &lno) != EOF))
      {
         { // load_info
            char d[128];

            read_check(fscanf(infos, "%d %d %d %d %d %d %d %d %127s",
                              &land_properties[lno].overview_color,
                              &land_properties[lno].walk_possible, &land_properties[lno].with_hl,
                              &land_properties[lno].is_forest, &land_properties[lno].is_grass,
                              &land_properties[lno].is_dig, &land_properties[lno].is_water,
                              &land_properties[lno].can_grow, d),
                       9, "land info file");
         }
      }
      fclose(infos);
   }
}

void land_push(int id, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
   { // perform_push
      int j;

      for (j = 0; j < 8; j++)
         if (upic[x][y][j] == id)
            return;
      for (j = 0; j < 8; j++)
         if (upic[x][y][j] == none)
         { // store
            upic[x][y][j] = id;
            return;
         }
   }
}

void land_pop(int id, int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
   { // perform_pop
      for (int j = 0; j < 8; j++)
         if (upic[x][y][j] == id)
         { // remove
            upic[x][y][j] = none;
         }
   }
}

void land_push(int id, int x, int y, int dx, int dy)
{
   land_push(id, x, y);
   if (dx != 0 || dy != 0)
   {
      land_push(id, x + dx, y + dy);
      land_push(id, x + dx, y);
      land_push(id, x, y + dy);
   }
}

void land_push(int id, int x, int y, int dx, int dy, int nx, int ny)
{
   for (int sx = 0; sx <= nx; sx++)
      for (int sy = 0; sy <= ny; sy++)
         land_push(id, x + sx * dx, y + sy * dy);
}

void land_pop(int id, int x, int y, int dx, int dy, int nx, int ny)
{
   for (int sx = 0; sx <= nx; sx++)
      for (int sy = 0; sy <= ny; sy++)
         land_pop(id, x + sx * dx, y + sy * dy);
}

void land_pop(int id, int x, int y, int dx, int dy)
{
   if (dx != 0 || dy != 0)
      land_pop(id, x + dx, y + dy);
   if (dx != 0)
      land_pop(id, x + dx, y);
   if (dy != 0)
      land_pop(id, x, y + dy);
}

int land_profile(int x, int y, int min_d)
{
   int h = landhight[x][y];
   int p = 0;

   if (h <= 0)
      return 0;
   else
   { // handle_up_and_down
      if (0 <= y - 1 && h - landhight[x][y - 1] >= min_d)
         p += 1;
      if (0 <= x - 1 && h - landhight[x - 1][y] >= min_d)
         p += 10;
      if (y + 1 < landscape_dy && h - landhight[x][y + 1] >= min_d)
         p += 100;
      if (x + 1 < landscape_dx && h - landhight[x + 1][y] >= min_d)
         p += 1000;

      if (p == 0)
         return 0;
      if (p == 1)
         return 1;
      if (p == 11)
         return 2;
      if (p == 10)
         return 3;
      if (p == 110)
         return 4;
      if (p == 100)
         return 5;
      if (p == 1100)
         return 6;
      if (p == 1000)
         return 7;
      if (p == 1001)
         return 8;

      if (p == 1011)
         return 9;
      if (p == 111)
         return 10;
      if (p == 1110)
         return 11;
      if (p == 1101)
         return 12;

      if (p == 1010)
         return 13;
      if (p == 101)
         return 14;

      if (p == 1111)
         return 15;

      return 0;
   }
}

bool anything_on_land(int x, int y)
{
   for (int j = 0; j < 8; j++)
      if (upic[x][y][j] != none)
         return true;
   return false;
}

bool is_water(int id, int x, int y)
{
   for (int xx = 0; xx < 3; xx++)
      for (int yy = 0; yy < 3; yy++)
         if (!land_properties[landscape[x + xx][y + yy]].is_water
             || (unit[x + xx][y + yy] != none && unit[x + xx][y + yy] != id))
            return false;
   return true;
}

bool is_water(int id, int x, int y, int gid)
{
   for (int xx = 0; xx < 3; xx++)
      for (int yy = 0; yy < 3; yy++)
         if (!land_properties[landscape[x + xx][y + yy]].is_water
             || (unit[x + xx][y + yy] != none && unit[x + xx][y + yy] != id
                 && unit[x + xx][y + yy] != gid))
            return false;
   return true;
}
