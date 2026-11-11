/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from field.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- field.cc field.hc
 */

#include "field.h"
#include "masks.h"
#include "win.h"
#include "building.h"
#include "player.h"
#include "dir.h"

#define blocked 1000000

/*
 #define f_trace
*/

field::field()
{
   w_is_open = false;
}

field::~field() {}

void field::init_building(int color)
{
   init_cand();
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
      { // handle_field
         int ll = landscape[x][y];

         { // set_f
            if (unit[x][y] != none)
            { // handle_unit
               int u = unit[x][y];

               if (objects->color[u] == color
                   && (objects->type[u] == object_worker || is_building(objects->type[u])))
               { // handle_o
                  int p = 0;

                  /*
      get_n;
*/
                  switch (objects->type[u])
                  {
                  case object_home:
                     p = 100;
                     break;
                  case object_building_site:
                     p = 100;
                     break;
                  case object_camp:
                     p = 100;
                     break;
                  case object_farm:
                     p = 100;
                     break;
                  case object_mill:
                     p = 100;
                     break;
                  case object_smith:
                     p = 105;
                     break;
                  case object_uni:
                     p = 100;
                     break;
                  case object_worker:
                     p = 95;
                     break;
                  }

                  f[x][y] = p;
                  push_neighbors(x, y);
               }
               else
                  f[x][y] = 0;
            }
            else
            { // handle_free_f
               if (ll == land_water)
                  f[x][y] = blocked + 1;
               else if (land_properties[ll].walk_possible)
                  f[x][y] = 0;
               else
                  f[x][y] = blocked;
            }
         }
         { // set_water
            is_water[x][y] = (ll == land_water);
         }
      }
}

void field::set_zero()
{
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
      {
         { // set_f
            f[x][y] = 0;
         }
         { // set_water
            is_water[x][y] = (landscape[x][y] == land_water);
         }
      }
}

void field::incr(int x, int y, int p)
{
   int xmin = i_max(0, x - p);
   int xmax = i_min(landscape_dx - 1, x + p);
   int ymin = i_max(0, y - p);
   int ymax = i_min(landscape_dy - 1, y + p);

   for (int xx = xmin; xx < xmax; xx++)
      for (int yy = ymin; yy < ymax; yy++)
      {
         f[xx][yy] += i_max(0, p - (int)dist(x, y, xx, yy));
      }
}

void field::decr()
{
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
      {
         f[x][y] = i_max(0, f[x][y] - 1);
         if (f[x][y] != 0)
            players[0]->show_int(x, y, f[x][y]);
      }
}

void field::add_forti(int x, int y, int p, int dx, int dy, int p0)
{
   if (dx == 0)
   { // handle_dy
      for (int i = 0; i < p; i++)
         for (int xs = x - i; xs <= x + i; xs++)
            if (0 <= xs && xs < landscape_dx && 0 <= y + i * dy && y + i * dy < landscape_dy)
            {
               f[xs][y + i * dy] = i_max(f[xs][y + i * dy], p0 + (p - i));
#ifdef f_trace
               players[0]->show_int(xs, y + i * dy, f[xs][y + i * dy]);
#endif
            }
   }
   else
   { // handle_dx
      for (int i = 0; i < p; i++)
         for (int ys = y - i; ys <= y + i; ys++)
            if (0 <= x + i * dx && x + i * dx < landscape_dx && 0 <= ys && ys < landscape_dy)
            {
               f[x + i * dx][ys] = i_max(f[x + i * dx][ys], p0 + (p - i));
#ifdef f_trace
               players[0]->show_int(x + i * dx, ys, f[x + i * dx][ys]);
#endif
            }
   }
}

void field::add_stayaway(int x, int y, int p, int dx, int dy)
{
   if (dx == 0)
   { // handle_dy
      for (int i = 0; i < p; i++)
         for (int xs = x - i; xs <= x + i; xs++)
            if (0 <= xs && xs < landscape_dx && 0 <= y + i * dy && y + i * dy < landscape_dy
                && f[xs][y + i * dy] <= 0)
            {
               f[xs][y + i * dy] = i_min(f[xs][y + i * dy], -(p - i));
#ifdef f_trace
               players[0]->show_int(xs, y + i * dy, f[xs][y + i * dy]);
#endif
            }
   }
   else
   { // handle_dx
      for (int i = 0; i < p; i++)
         for (int ys = y - i; ys <= y + i; ys++)
            if (0 <= x + i * dx && x + i * dx < landscape_dx && 0 <= ys && ys < landscape_dy
                && f[x + i * dx][ys] <= 0)
            {
               f[x + i * dx][ys] = i_min(f[x + i * dx][ys], -(p - i));
#ifdef f_trace
               players[0]->show_int(x + i * dx, ys, f[x + i * dx][ys]);
#endif
            }
   }
}

void field::sub_forti(int x, int y, int p, int dx, int dy)
{
   if (dx == 0)
   { // handle_dy
      for (int i = 0; i < p; i++)
         for (int xs = x - i; xs <= x + i; xs++)
            if (0 <= xs && xs < landscape_dx && 0 <= y + i * dy && y + i * dy < landscape_dy
                && f[xs][y + i * dy] <= p - i)
            {
               f[xs][y + i * dy] = 0;
#ifdef f_trace
               players[0]->show_int(xs, y + i * dy, f[xs][y + i * dy]);
#endif
            }
   }
   else
   { // handle_dx
      for (int i = 0; i < p; i++)
         for (int ys = y - i; ys <= y + i; ys++)
            if (0 <= x + i * dx && x + i * dx < landscape_dx && 0 <= ys && ys < landscape_dy
                && f[x + i * dx][ys] <= p - i)
            {
               f[x + i * dx][ys] = 0;
#ifdef f_trace
               players[0]->show_int(x + i * dx, ys, f[x + i * dx][ys]);
#endif
            }
   }
}

void field::init_man(int color)
{
   { // init
      init_cand();
   }
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
      {
         { // set_f
            if (unit[x][y] != none)
            { // handle_unit
               int u = unit[x][y];

               if (objects->color[u] == color && objects->type[u] != object_worker
                   && !is_building(objects->type[u]))
               { // handle_o
                  int n;

                  { // get_n
                     int xmin = i_max(0, x - 1);
                     int xmax = i_min(landscape_dx - 1, x + 1);
                     int ymin = i_max(0, y - 1);
                     int ymax = i_min(landscape_dy - 1, y + 1);

                     n = 0;
                     for (int x1 = xmin; x1 < xmax; x1++)
                        for (int y1 = ymin; y1 < ymax; y1++)
                        {
                           int u1 = unit[x1][y1];

                           if (u1 != none && u != u1 && objects->color[u1] == color
                               && !is_building(objects->type[u1]))
                              n++;
                        }
                  }
                  f[x][y] = 100 + n;
                  push_neighbors(x, y);
               }
               else
                  f[x][y] = 0;
            }
            else
            { // handle_free_f
               if (land_properties[landscape[x][y]].walk_possible)
                  f[x][y] = 0;
               else
                  f[x][y] = blocked;
            }
         }
         { // set_water
            is_water[x][y] = (landscape[x][y] == land_water);
         }
      }
}

bool field::expand(int /* max_time */, int color)
{
   bool any_change = false;

   { // init
      max_p = 0;
   }
   while (another_cand())
   {
      { // handle_cand
         int x = cand_x[first_cand];
         int y = cand_y[first_cand];

         { // set_pot
            if (f[x][y] < blocked)
            { // handle_f
               int p = -INT_MAX;
               int d;

               if (is_water[x][y])
                  d = 3;
               else
                  d = 1;
               { // get_p
                  int xmin = i_max(0, x - 1);
                  int xmax = i_min(landscape_dx - 1, x + 1);
                  int ymin = i_max(0, y - 1);
                  int ymax = i_min(landscape_dy - 1, y + 1);

                  for (int x1 = xmin; x1 <= xmax; x1++)
                     for (int y1 = ymin; y1 <= ymax; y1++)
                        if (f[x1][y1] < blocked)
                           p = i_max(p, f[x1][y1]);
               }
               if ((p - d) > f[x][y])
               {
                  f[x][y] = p - d;
                  any_change = true;
               }
            }
            else if (is_water[x][y])
            { // handle_w
               int p = -INT_MAX;

               { // get_p
                  int xmin = i_max(0, x - 1);
                  int xmax = i_min(landscape_dx - 1, x + 1);
                  int ymin = i_max(0, y - 1);
                  int ymax = i_min(landscape_dy - 1, y + 1);

                  for (int x1 = xmin; x1 <= xmax; x1++)
                     for (int y1 = ymin; y1 <= ymax; y1++)
                        if (f[x1][y1] < blocked)
                           p = i_max(p, f[x1][y1]);
               }
               f[x][y] = p - 3;
               any_change = true;
            }
            if (f[x][y] < blocked)
               max_p = i_max(max_p, f[x][y]);
         }
         pop_cand();
         if (any_change)
            push_neighbors(x, y);
      }
   }
   { // get_b_max
      int b_pot[2];
      int a_pot[2];

      { // init_pots
         b_pot[0] = -1;
         b_pot[1] = -1;
         a_pot[0] = INT_MAX;
         a_pot[1] = INT_MAX;
         b_x[0] = landscape_dx / 2;
         b_y[0] = landscape_dy / 2;
         b_x[1] = landscape_dx / 2;
         b_y[1] = landscape_dy / 2;
         a_x[0] = landscape_dx / 2;
         a_y[0] = landscape_dy / 2;
         a_x[1] = landscape_dx / 2;
         a_y[1] = landscape_dy / 2;
      }
      for (int x = 0; x < landscape_dx; x++)
         for (int y = 0; y < landscape_dy; y++)
         {
            { // check_b
               int u = unit[x][y];
               int ff = f[x][y];

               if (u != none && (ff > b_pot[0] || ff > b_pot[1]) && objects->color[u] != color
                   && objects->type[u] != object_mine && is_building(objects->type[u]))
               { // grab_b
                  if (ff > b_pot[0] && i_abs(x - b_x[0]) > 2 && i_abs(y - b_y[0]) > 2)
                  { // grab_b0
                     b_pot[1] = b_pot[0];
                     b_x[1] = b_x[0];
                     b_y[1] = b_y[0];
                     b_pot[0] = ff;
                     b_x[0] = x;
                     b_y[0] = y;
                  }
                  else if (i_abs(x - b_x[0]) > 2 && i_abs(y - b_y[0] > 2))
                  { // grab_b1
                     b_pot[1] = ff;
                     b_x[1] = x;
                     b_y[1] = y;
                  }
               }
            }
            { // check_a
               int u = unit[x][y];
               int ff = f[x][y];

               if (u != none && (ff < a_pot[0] || ff < a_pot[1]) && objects->color[u] == color
                   && objects->type[u] != object_zombi && objects->type[u] != object_schrott
                   && objects->type[u] != object_arrow && objects->type[u] != object_stone
                   && objects->type[u] != object_mine)
               { // grab_a
                  if (ff < a_pot[0] && i_abs(x - b_x[0]) > 2 && i_abs(y - b_y[0]) > 2)
                  { // grab_a0
                     a_pot[1] = a_pot[0];
                     a_x[1] = a_x[0];
                     a_y[1] = a_y[0];
                     a_pot[0] = ff;
                     a_x[0] = x;
                     a_y[0] = y;
                  }
                  else if (i_abs(x - b_x[0]) > 2 && i_abs(y - b_y[0] > 2))
                  { // grab_a1
                     a_pot[1] = ff;
                     a_x[1] = x;
                     a_y[1] = y;
                  }
               }
            }
         }
   }
   return any_change;
}

void field::dump(int x, int y)
{
   { // open_w
      if (!w_is_open)
         w = new win("dd", "", by_fix, by_fix, landscape_dx * 2, landscape_dy * 2);
      w_is_open = true;
   }
   {    // perform_dump
      { // show_f
         int min;
         int max;

         { // get_min_max
            min = INT_MAX;
            max = -INT_MAX;
            for (int x = 0; x < landscape_dx; x++)
               for (int y = 0; y < landscape_dy; y++)
                  if (f[x][y] < blocked)
                  {
                     min = i_min(min, f[x][y]);
                     max = i_max(max, f[x][y]);
                  }
         }
         for (int x = 0; x < landscape_dx; x++)
            for (int y = 0; y < landscape_dy; y++)
            { // dump_f
               int xx = x * 2;
               int yy = y * 2;

               if (f[x][y] == blocked)
                  w->set_color(black);
               else
                  w->set_color((int)(62.0 * (double)(f[x][y] - min) / (double)(max - min)));

               w->pixel(xx, yy);
               w->pixel(xx + 1, yy);
               w->pixel(xx, yy + 1);
               w->pixel(xx + 1, yy + 1);
            }
      }
      { // show_p
         w->set_color(black);
         w->line(0, y * 2, landscape_dx * 2, y * 2);
         w->line(x * 2, 0, x * 2, landscape_dy * 2);
      }
   }
}

void field::nbest(int xmin, int ymin, int xmax, int ymax, int p0, int &pbest, int &xx, int &yy)

{
   int x1;
   int y1;

   y1 = ymin;
   for (x1 = xmin; x1 <= xmax; x1++)
   { // look_up
      if (i_abs(p0 - f[x1][y1]) < pbest && unit[x1][y1] == none && !any_water(x1, y1))
      {
         xx = x1;
         yy = y1;
         pbest = i_abs(p0 - f[x1][y1]);
      }
   }
   if (pbest == 0)
      return;
   x1 = xmin;
   for (y1 = ymin; y1 <= ymax; y1++)
   { // look_up
      if (i_abs(p0 - f[x1][y1]) < pbest && unit[x1][y1] == none && !any_water(x1, y1))
      {
         xx = x1;
         yy = y1;
         pbest = i_abs(p0 - f[x1][y1]);
      }
   }
}

void field::nbest(int &x, int &y)
{
   int p0 = f[x][y];
   int pbest = INT_MAX;
   int xx;
   int yy;

   for (int j = 0; j < 10; j++)
   { // check_new
      nbest(i_max(0, x - j), i_max(0, y - j), i_min(landscape_dx - 1, x + j), i_max(0, y - j), p0,
            pbest, xx, yy);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_max(0, x - j), i_max(0, y - j), i_max(0, x - j), i_min(landscape_dy - 1, y + j), p0,
            pbest, xx, yy);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_max(0, x - j), i_min(landscape_dy - 1, y + j), i_min(landscape_dx - 1, x + j),
            i_min(landscape_dy - 1, y + j), p0, pbest, xx, yy);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_min(landscape_dx - 1, x + j), i_max(0, y - j), i_min(landscape_dx - 1, x + j),
            i_min(landscape_dy - 1, y + j), p0, pbest, xx, yy);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
   }
   if (pbest != INT_MAX)
   {
      x = xx;
      y = yy;
   }
}

void field::nbest(int xmin, int ymin, int xmax, int ymax, int p0, int &pbest, int &xx, int &yy,
                  field *pf)

{
   int x1;
   int y1;

   y1 = ymin;
   for (x1 = xmin; x1 <= xmax; x1++)
   { // look_up
      if (i_abs(p0 - f[x1][y1]) < pbest && unit[x1][y1] == none && pf->f[x1][y1] == 0
          && !any_water(x1, y1))
      {
         xx = x1;
         yy = y1;
         pbest = i_abs(p0 - f[x1][y1]);
      }
   }
   if (pbest == 0)
      return;
   x1 = xmin;
   for (y1 = ymin; y1 <= ymax; y1++)
   { // look_up
      if (i_abs(p0 - f[x1][y1]) < pbest && unit[x1][y1] == none && pf->f[x1][y1] == 0
          && !any_water(x1, y1))
      {
         xx = x1;
         yy = y1;
         pbest = i_abs(p0 - f[x1][y1]);
      }
   }
}

void field::nbest(int &x, int &y, field *pf)
{
   int p0 = f[x][y];
   int pbest = INT_MAX;
   int xx;
   int yy;

   for (int j = 0; j < 10; j++)
   { // check_new
      nbest(i_max(0, x - j), i_max(0, y - j), i_min(landscape_dx - 1, x + j), i_max(0, y - j), p0,
            pbest, xx, yy, pf);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_max(0, x - j), i_max(0, y - j), i_max(0, x - j), i_min(landscape_dy - 1, y + j), p0,
            pbest, xx, yy, pf);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_max(0, x - j), i_min(landscape_dy - 1, y + j), i_min(landscape_dx - 1, x + j),
            i_min(landscape_dy - 1, y + j), p0, pbest, xx, yy, pf);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
      nbest(i_min(landscape_dx - 1, x + j), i_max(0, y - j), i_min(landscape_dx - 1, x + j),
            i_min(landscape_dy - 1, y + j), p0, pbest, xx, yy, pf);
      { // check_over
         if (pbest == 0)
         {
            x = xx;
            y = yy;
            return;
         }
      }
   }
   if (pbest != INT_MAX)
   {
      x = xx;
      y = yy;
   }
}

void field::climb(int &x, int &y, int p)
{
   int p0 = f[x][y];
   bool any_progress = true;

   while (f[x][y] < p0 + p && any_progress)
   {
      { // step
         int pp = f[x][y];
         int xmin = i_max(0, x - 1);
         int xmax = i_min(landscape_dx - 1, x + 1);
         int ymin = i_max(0, y - 1);
         int ymax = i_min(landscape_dy - 1, y + 1);

         any_progress = false;
         if (f[xmin][y] > pp && f[xmin][y] < blocked)
         {
            pp = f[xmin][y];
            x = xmin;
            any_progress = true;
         }
         else if (f[xmax][y] > pp && f[xmax][y] < blocked)
         {
            pp = f[xmax][y];
            x = xmax;
            any_progress = true;
         }
         else if (f[x][ymin] > pp && f[x][ymin] < blocked)
         {
            pp = f[x][ymin];
            y = ymin;
            any_progress = true;
         }
         else if (f[x][ymax] > pp && f[x][ymax] < blocked)
         {
            pp = f[x][ymax];
            y = ymax;
            any_progress = true;
         }
         else if (f[xmin][ymin] > pp && f[xmin][ymin] < blocked)
         {
            pp = f[xmin][ymin];
            x = xmin;
            y = ymin;
            any_progress = true;
         }
         else if (f[xmax][ymax] > pp && f[xmax][ymax] < blocked)
         {
            pp = f[xmax][ymax];
            x = xmax;
            y = ymax;
            any_progress = true;
         }
         else if (f[xmax][ymin] > pp && f[xmax][ymin] < blocked)
         {
            pp = f[xmax][ymin];
            y = ymin;
            x = xmax;
            any_progress = true;
         }
         else if (f[xmin][ymax] > pp && f[xmin][ymax] < blocked)
         {
            pp = f[xmin][ymax];
            y = ymax;
            x = xmin;
            any_progress = true;
         }
      }
   }
   if (any_water(x, y))
      nbest(x, y);
}

void field::down(int &x, int &y)
{
   bool any_progress = true;

   while (i_abs(f[x][y]) > 0 && any_progress)
   {
      { // step
         int pp = f[x][y];

         any_progress = false;
         for (int s = 1; s < 8 && !any_progress; s++)
            for (int d = 0; d < 8 && !any_progress; d++)
            { // check_step
               int dx;
               int dy;
               int xx;
               int yy;

               dir_dx_dy(d, dx, dy);
               xx = x + dx * s;
               yy = y + dy * s;
               if (0 <= xx && xx < landscape_dx && 0 <= yy && yy < landscape_dy)
               { // check_f
                  if (i_abs(f[xx][yy]) < i_abs(pp)
                      && land_properties[landscape[xx][yy]].walk_possible && unit[xx][yy] == none)
                  { // do_it
                     x = xx;
                     y = yy;
                     /*
players [0]->show_int (x, y, f [x][y]);
*/
                     any_progress = true;
                  }
               }
            }
      }
   }
}

void field::init_cand()
{
   first_cand = 0;
   last_cand = 0;
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
         in_cand[x][y] = false;
}

bool field::another_cand()
{
   return first_cand != last_cand;
}

void field::pop_cand()
{
   in_cand[cand_x[first_cand]][cand_y[first_cand]] = false;
   first_cand++;
   if (first_cand == 10000)
      first_cand = 0;
}

void field::push_cand(int x, int y)
{
   if (0 <= x && x < landscape_dx && 0 <= y && y < landscape_dy)
   { // perform_push
      cand_x[last_cand] = x;
      cand_y[last_cand] = y;
      in_cand[x][y] = true;
      last_cand++;
      if (last_cand == 10000)
         last_cand = 0;
      if (last_cand == first_cand)
         errorstop(1, "FIELD", "prop overflow");
   }
}

void field::push_neighbors(int x, int y)
{
   int xmin = i_max(0, x - 1);
   int xmax = i_min(landscape_dx - 1, x + 1);
   int ymin = i_max(0, y - 1);
   int ymax = i_min(landscape_dy - 1, y + 1);

   for (int x1 = xmin; x1 <= xmax; x1++)
      for (int y1 = ymin; y1 <= ymax; y1++)
         if (!in_cand[x1][y1]
             && (f[x1][y1] < f[x][y] || ((f[x1][y1] >= blocked) && is_water[x1][y1])))
            push_cand(x1, y1);
}

void add(field *a, field *b, field *c)
{
   for (int x = 0; x < landscape_dx; x++)
      for (int y = 0; y < landscape_dy; y++)
         c->f[x][y] = i_min(blocked, a->f[x][y] + b->f[x][y]);
}

bool field::any_water(int x, int y)
{
   int xmin = i_max(0, x - 3);
   int xmax = i_min(landscape_dx - 1, x + 3);
   int ymin = i_max(0, y - 3);
   int ymax = i_min(landscape_dy - 1, y + 3);

   for (int xx = xmin; xx <= xmax; xx++)
      for (int yy = ymin; yy <= ymax; yy++)
         if (is_water[xx][yy])
            return true;
   return false;
}