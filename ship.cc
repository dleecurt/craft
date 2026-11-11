/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from ship.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- ship.cc ship.hc
 */

#include "stdio.h"

#include "ship.h"
#include "player.h"
#include "object_handler.h"

ship::ship(int id)
{
   myid = id;
   num_man = 0;
   capa = 12;
   should_refresh = false;
   is_shown = false;
   with_master = true;
   is_idle = true;
}

ship::~ship() {}

void ship::show(int wx, int wy)
{
   int pno = color_player[objects->color[myid]];

   px = wx;
   py = wy;
   is_shown = true;
   for (int i = 0; i < capa; i++)
      refresh(i);
   x_cap = px;
   y_cap = py + 5 * (pic_dy + 8);
   x_crew = px + pic_dx + 2;
   y_crew = py + 5 * (pic_dy + 8);
   players[pno]->ship_pics[pic_captain_on]->show(x_cap, y_cap);
   players[pno]->ship_pics[pic_crew_on]->show(x_crew, y_crew);
   players[pno]->w_status->tick();
}

void ship::unshow(int color)
{
   if (objects->color[myid] == color)
      is_shown = false;
}

void ship::refresh(int i)
{
   int pno = color_player[objects->color[myid]];

   if (is_shown)
   {    // perform_show
      { // template
         if (i == 0 && with_master)
            players[pno]->w_status->set_color(gray80);
         else
            players[pno]->w_status->set_color(gray60);
         players[pno]->w_status->function(GXcopy);
         players[pno]->w_status->fill(((i % 3) * (pic_dx + 2) + px),
                                      ((i / 3) * (pic_dy + 8) + py) - 6, pic_dx, pic_dy + 6);
      }
      if (i < num_man && unit[i] != none)
      { // show_unit
         players[pno]->ship_pics[u_pic[i]]->show(((i % 3) * (pic_dx + 2) + px),
                                                 ((i / 3) * (pic_dy + 8) + py));
         if (objects->is_marked[unit[i]])
         {
            players[pno]->w_status->set_color(red);
            players[pno]->w_status->box(((i % 3) * (pic_dx + 2) + px),
                                        ((i / 3) * (pic_dy + 8) + py),
                                        ((i % 3) * (pic_dx + 2) + px) + pic_dx - 1,
                                        ((i / 3) * (pic_dy + 8) + py) + pic_dy - 1);
         }
         { // show_state
            players[pno]->w_status->set_color(black);
            players[pno]->w_status->fill(((i % 3) * (pic_dx + 2) + px),
                                         ((i / 3) * (pic_dy + 8) + py) - 6, pic_dx - 1, 6);
            if (objects->health[unit[i]] < 25)
               players[pno]->w_status->set_color(red);
            else if (objects->health[unit[i]] < 50)
               players[pno]->w_status->set_color(yellow);
            else
               players[pno]->w_status->set_color(green);
            players[pno]->w_status->fill(
                ((i % 3) * (pic_dx + 2) + px) + 2, ((i / 3) * (pic_dy + 8) + py) - 5,
                (int)((double)(pic_dx - 4) / 100.0 * (double)(objects->health[unit[i]])), 4);
         }
      }
      players[pno]->w_status->tick();
   }
}

bool ship::enter(int id)
{
   if (num_man < capa)
   {
      { // perform_enter
         int pno = color_player[objects->color[myid]];

         unit[num_man] = id;
         u_pic[num_man] = objects->pic[id];
         players[pno]->mark(id, false);
         num_man++;
         should_refresh = true;
      }
      return true;
   }
   return false;
}

bool ship::leave(int id)
{
   int i;

   { // get_i
      for (i = 0; i < num_man; i++)
         if (unit[i] == id)
            break;
   }
   if (i < num_man)
   {    // perform_remove
      { // remove_from_field
         unit[i] = none;
      }
      { // remove_from_mans
         for (int j = i; j < num_man - 1; j++)
         {
            u_pic[j] = u_pic[j + 1];
            unit[j] = unit[j + 1];
         }
         num_man--;
      }
      should_refresh = true;
      return true;
   }
   else
      return false;
}

void ship::eval()
{
   int xe;
   int ye;
   int b;
   int pno = color_player[objects->color[myid]];

   { // perhaps_reshow
      for (int i = 0; i < num_man; i++)
         if (objects->pic[unit[i]] != u_pic[i])
         {
            u_pic[i] = objects->pic[unit[i]];
            refresh(i);
         }
   }
   { // perhaps_refresh
      if (should_refresh)
         show(px, py);
      should_refresh = false;
   }
   if (players[pno]->w_status->is_mouse(xe, ye, b)
       && (px <= xe && xe <= px + 3 * (pic_dx + 2) && py <= ye && ye <= py + 4 * (pic_dy + 8)))
   { // handle_eval
      int d;
      int ind;

      players[pno]->w_status->mouse(d, d, xe, ye, b);
      { // get_ind
         ind = capa + 3;
         for (int i = 0; i < num_man; i++)
            if (xe >= ((i % 3) * (pic_dx + 2) + px) && xe <= ((i % 3) * (pic_dx + 2) + px) + pic_dx
                && ye >= ((i / 3) * (pic_dy + 8) + py)
                && ye <= ((i / 3) * (pic_dy + 8) + py) + pic_dy)
            {
               ind = i;
               break;
            }
      }
      if (ind < num_man)
      { // handle_event
         int pno = color_player[objects->color[myid]];

         if (b == button1press)
         {    // handle_mark_first
            { // handle_unmark
               for (int i = 0; i < num_man; i++)
               {
                  players[pno]->mark(unit[ind], false);
                  refresh(i);
               }
            }
            players[pno]->mark(unit[ind], true);
            refresh(ind);
         }
         else if (b == button2press)
         { // handle_mark_toggle
            players[pno]->mark(unit[ind], !objects->is_marked[unit[ind]]);
            refresh(ind);
         }
      }
   }
   else if (players[pno]->w_status->is_mouse(xe, ye, b)
            && (x_cap <= xe && xe <= x_cap + pic_dx && y_cap <= ye && ye <= y_cap + pic_dy))
   { // handle_cap
      int d;

      players[pno]->w_status->mouse(d, d, xe, ye, b);
      if (b == button1press && num_man > 0)
      {
         players[pno]->mark(unit[0], !objects->is_marked[unit[0]]);
         refresh(0);
      }
   }
   else if (players[pno]->w_status->is_mouse(xe, ye, b)
            && (x_crew <= xe && xe <= x_crew + pic_dx && y_crew <= ye && ye <= y_crew + pic_dy))
   { // handle_crew
      int d;

      players[pno]->w_status->mouse(d, d, xe, ye, b);
      if (b == button1press && num_man > 0)
      { // handle_crew_marking
         for (int i = 1; i < num_man; i++)
         {
            players[pno]->mark(unit[i], !objects->is_marked[unit[i]]);
            refresh(i);
         }
      }
   }
   else
      return;
}

void ship::move(int dx, int dy)
{
   for (int i = 0; i < num_man; i++)
   { // move_man
      int id = unit[i];

      objects->x[id] += dx;
      objects->y[id] += dy;
      objects->wx[id] = objects->x_center(objects->x[id]);
      objects->wy[id] = objects->y_center(objects->y[id]);
   }
}

void ship::hit(int idh, int power)
{
   if (num_man == 0 || i_random(0, 100) > 90)
   { // perform_ship_hit

      objects->health[myid] -= i_max(1, power - amour_ship1);
      objects->version[myid]++;
      if (objects->health[myid] <= 0)
      { // sink_ship
         objects->new_order(myid, cmd_die, 0, 0);
      }
   }
   else
   { // perform_man_hit
      int i;
      int id;

      {    // get_i
         { // choose_any
            i = i_random(0, num_man - 1);
            /*
      i = i_random (0, i_min (3, num_man-1));
*/
         }
      }
      id = unit[i];
      if (0 <= id && id < max_objects
          && (!objects->is_free[id] && objects->type[id] != object_zombi
              && objects->type[id] != object_ship_zombi && objects->type[id] != object_schrott))
      { // handle_obj
         objects->hit(idh, id, power);
         refresh(i);
         if (objects->health[id] <= 0)
         {
            leave(id);
            if (i == 0)
               with_master = false;
         }
      }
   }

   /*
     {bool any_man = false;

      check_healer;
      if (! any_man) check_cata;
      if (! any_man) choose_any;
     }.
*/
}

bool ship::empty()
{
   for (int i = 0; i < num_man; i++)
   { // check_u
      int u = objects->type[unit[i]];

      if (u != object_schrott && u != object_zombi)
         return false;
   }
   return true;
}

bool ship::no_fighter()
{
   if (empty())
      return true;
   else
   { // check_mans
      for (int i = 0; i < num_man; i++)
      { // check_man
         int t = objects->type[unit[i]];

         if (t == object_cata || t == object_archer || t == object_pawn || t == object_knight)
            return false;
      }
      return true;
   }
}

void ship::get_crew(int &n_worker, int &n_fighter, int &n_healer)
{
   n_worker = 0;
   n_fighter = 0;
   n_healer = 0;
   for (int i = 1; i < num_man; i++)
      switch (objects->type[unit[i]])
      {
      case object_worker:
         n_worker++;
         break;
      case object_pawn:
      case object_knight:
      case object_archer:
      case object_cata:
         n_fighter++;
         break;
      case object_doktor:
         n_healer++;
         break;
      }
}

int ship::num_cata()
{
   int n_cata = 0;

   for (int i = 1; i < num_man; i++)
      if (objects->type[unit[i]] == object_cata)
         n_cata++;
   return n_cata;
}

bool ship_on_side(int xs, int ys, int ide, int xe, int ye)
{
   if (ide == none)
      return false;
   if (objects->type[ide] == object_ship1)
   { // handle_ship_to_ship
      if (i_abs(xe - xs) > 3)
         return false;
      if (i_abs(ye - ys) > 3)
         return false;
      return true;
   }
   else
   { // handle_ship_to_man
      if (xe < xs && i_abs(xe - xs) > 1)
         return false;
      if (ye < ys && i_abs(ye - ys) > 1)
         return false;
      if (i_abs(xe - xs) > 3)
         return false;
      if (i_abs(ye - ys) > 3)
         return false;
      return true;
   }
}
