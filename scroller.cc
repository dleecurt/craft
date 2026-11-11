/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from scroller.hc (hcc refinements
 *              expanded) to plain C++
 * Exact changes: git log -- scroller.cc scroller.hc
 */

#include "scroller.h"
#include "masks.h"

#define bar_button_size 14
#define bar_size 16

scroller::scroller(const char v_name[], win *v_w, int v_x, int v_y, int v_dx, int v_dy, int v_min,
                   int v_max, int v_size, int v_pos, int v_step_small, int v_step_large)

{
   { // store_params
      strcpy(name, v_name);
      w = v_w;
      x = v_x;
      y = v_y;
      dx = v_dx;
      dy = v_dy;
      min = v_min;
      max = v_max;
      size = v_size;
      pos = v_pos;
      step_small = v_step_small;
      step_large = v_step_large;
   }
   { // get_colors
      c_border_light = win_default_c("scroller_border_light");
      c_border_dark = win_default_c("scroller_border_dark");
      c_background = win_default_c("scroller_background");
      c_foreground = win_default_c("scroller_foreground");
   }
   { // calc_display_params
      if (dx > dy)
         dy = bar_size;
      else
         dx = bar_size;
   }
   { // init_status
      was_event = false;
      if (step_small == 0)
         step_small = i_max(1, (max - min) / 20);
      if (step_large == 0)
         step_large = i_max(1, (max - min) / 5);
   }
   display();
}

scroller::~scroller()
{
   delete (button_incr);
   delete (button_decr);
}

void scroller::display()
{
   {    // display_bar
      { // draw_background
         w->set_color(c_background);
         w->fill(x, y, dx, dy);
         w->set_color(black);
         frame(w, x, y, x + dx, y + dy, c_border_dark, c_border_light);
         w->tick();
      }
      { // draw_buttons
         if (dx > dy)
         { // draw_horizontal
            button_decr = new button(w, "/scroll_left", x + 1, y + 1, true);
            button_incr = new button(w, "/scroll_right", x + dx - bar_button_size, y + 1, true);
         }
         else
         { // draw_vertical
            button_decr = new button(w, "/scroll_up", x + 1, y + 1, true);
            button_incr = new button(w, "/scroll_down", x + 1, y + dy - bar_button_size, true);
         }
      }
   }
   calc_ruler_params();
   display_ruler();
}

void scroller::calc_ruler_params()
{
   if (dx > dy)
   { // calc_horizontal_params
      ruler_x = x + (bar_button_size + 2)
                + (int)(((double)(dx - 4 - 2 * bar_button_size) / (double)(max - min))
                        * (double)(pos - min));
      ruler_y = y + 2;
      ruler_dy = i_max(4, bar_button_size - 1);
      ruler_dx = i_min((dx - 4 - 2 * bar_button_size),
                       i_max(4, (int)(((double)(dx - 4 - 2 * bar_button_size) / (double)(max - min))
                                      * (double)size)));
   }
   else
   { // calc_vertical_params
      ruler_y = y + (bar_button_size + 2)
                + (int)(((double)(dy - 4 - 2 * bar_button_size) / (double)(max - min))
                        * (double)(pos - min));
      ruler_x = x + 2;
      ruler_dx = i_max(4, bar_button_size - 1);
      ruler_dy = i_min((dy - 4 - 2 * bar_button_size),
                       i_max(4, (int)(((double)(dy - 4 - 2 * bar_button_size) / (double)(max - min))
                                      * (double)size)));
   }
   { // calc_centers
      ruler_x_center = ruler_x + ruler_dx / 2;
      ruler_y_center = ruler_y + ruler_dy / 2;
   }
}

void scroller::display_ruler()
{
   w->set_color(c_foreground);
   w->fill(ruler_x, ruler_y, ruler_dx, ruler_dy);
   w->tick();
}

void scroller::clear_ruler()
{
   w->set_color(c_background);
   w->fill(ruler_x, ruler_y, ruler_dx, ruler_dy);
}

bool scroller::on()
{
   int xm;
   int ym;
   int button;

   w->tick();
   return w->is_mouse(xm, ym, button) && (x <= xm && xm <= x + dx && y <= ym && ym <= y + dy);
}

bool scroller::eval(int &v_pos)
{
   { // handle_mouse_events
      if (on())
      { // handle_event
         if ((button_incr->eval()))
         { // handle_small_incr_event
            set(pos + step_small);
            was_event = true;
         }
         else if ((button_decr->eval()))
         { // handle_small_decr_event
            set(pos - step_small);
            was_event = true;
         }
         else
         { // handle_none_button_event
            int d;
            int xm;
            int ym;
            int button;

            w->tick();
            w->mouse(d, d, xm, ym, button);
            if (button == button1press)
            { // handle_move_event
               if ((ruler_x <= xm && xm <= ruler_x + ruler_dx && ruler_y <= ym
                    && ym <= ruler_y + ruler_dy))
               { // handle_ruler_event
                  while (((w->mouse(xm, ym, button) || true) && button != button1release))
                  {
                     { // shift_ruler
                        if (dx > dy)
                        { // horizontal_shift
                           if (xm < ruler_x_center)
                              set(pos - 1);
                           else if (xm > ruler_x_center)
                              set(pos + 1);
                        }
                        else
                        { // vertical_shift
                           if (ym < ruler_y_center)
                              set(pos - 1);
                           else if (ym > ruler_y_center)
                              set(pos + 1);
                        }
                     }
                     w->tick();
                  }
                  was_event = true;
               }
               else
               { // handle_bar_event
                  if (xm < ruler_x || ym < ruler_y)
                  { // handle_large_decr_event
                     set(pos - step_large);
                     was_event = true;
                  }
                  else
                  { // handle_large_incr_event
                     set(pos + step_large);
                     was_event = true;
                  }
               }
            }
         }
      }
   }
   { // store_results
      bool there_was_an_event = was_event;

      v_pos = pos;
      was_event = false;
      return there_was_an_event;
   }
}

void scroller::set(int v_pos)
{
   clear_ruler();
   pos = i_bound(min, v_pos, i_max(min, max - size));
   calc_ruler_params();
   display_ruler();
}

void scroller::resize(int v_size)
{
   clear_ruler();
   size = i_min(v_size, max - min);
   pos = i_bound(min, pos, max - size);
   calc_ruler_params();
   display_ruler();
}
