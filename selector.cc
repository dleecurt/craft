#include "selector.h"
#include "bool.h"
#include "xstring.h"
#include "masks.h"

#define selector_line_color gray70
#define selector_color_1 gray50
#define selector_color_2 gray90
#define selector_background_color gray
#define selector_marked_case_color yellow
#define selector_unmarked_case_color black

selector::selector(const char v_name[], win *v_w, int v_num_cases,
                   char v_case_string[max_selector_cases][128], int v_x, int v_y, int v_dx,
                   int v_dy, bool v_multiple_select, bool v_with_lines)

{
   { // store_params
      strcpy(name, v_name);
      w = v_w;
      x = v_x;
      y = v_y;
      dx = v_dx;
      dy = v_dy;
      is_multiple = v_multiple_select;
      is_scroller = false;
      with_lines = v_with_lines;
   }
   { // load_colors
      c_border_light = win_default_c("selector_border_light");
      c_border_dark = win_default_c("selector_border_dark");
      c_background = win_default_c("selector_background");
      c_foreground = win_default_c("selector_foreground");
      c_line = win_default_c("selector_line");
      c_mark = win_default_c("selector_mark");
   }
   { // calc_params
      int d;

      w->text_size("lala", d, case_dy);
      case_dy += 8;
   }
   set_cases(v_num_cases, v_case_string);
}

selector::~selector()
{
   delete (sc);
}

void selector::set_cases(int v_num_cases, char v_case_string[max_selector_cases][128])

{
   { // set_num_cases
      num_cases = v_num_cases;
   }
   { // show_selector
      w->set_color(c_background);
      w->fill(x, y, dx, dy);
      frame(w, x, y, x + dx, y + dy, c_border_dark, c_border_light);
   }
   {    // set_scroller
      { // perhaps_delete_scroller
         if (is_scroller)
            delete (sc);
      }
      is_scroller = true;
      sc_pos = 0;
      sc = new scroller(name, w, x + dx + 1, y, 1, dy, 0, num_cases - 1, dy / (case_dy + 1),
                        sc_pos);
   }
   { // set_case_strings
      for (int i = 0; i < num_cases; i++)
      {
         mark[i] = false;
         set_string(i, v_case_string[i]);
      }
   }
   w->tick();
}

void selector::set_mark(int case_no, bool mode)
{
   mark[case_no] = mode;
   refresh(case_no);
}

void selector::set_string(int case_no, char string[])
{
   strcpy(case_strings[case_no], string);
   { // get_draw_length
      int s_dx;

      draw_length[case_no] = strlen(case_strings[case_no]);
      { // get_s_dx
         int d;

         w->text_size(substring(case_strings[case_no], 0, draw_length[case_no]), s_dx, d);
      }
      for (draw_length[case_no] = strlen(case_strings[case_no]);
           draw_length[case_no] > 0 && (s_dx > dx - 8); draw_length[case_no]--)
      { // get_s_dx
         int d;

         w->text_size(substring(case_strings[case_no], 0, draw_length[case_no]), s_dx, d);
      }
   }
   refresh(case_no);
}

void selector::refresh(int case_no)
{
   if ((y + case_dy <= (case_no + 1) * case_dy + y - sc_pos * case_dy
        && (case_no + 1) * case_dy + y - sc_pos * case_dy <= y + dy))
   {    // draw_string
      { // set_colors
         w->set_background(c_background);
         if (is_mark(case_no))
            w->set_color(c_mark);
         else
            w->set_color(c_foreground);
      }
      w->write(x + 2, (case_no + 1) * case_dy + y - sc_pos * case_dy - 2,
               substring(case_strings[case_no], 0, draw_length[case_no]));
      { // perhaps_line
         if (with_lines)
         { // draw_line
            w->set_color(c_line);
            w->line(x + 2, (case_no + 1) * case_dy + y - sc_pos * case_dy, x + dx - 2,
                    (case_no + 1) * case_dy + y - sc_pos * case_dy);
         }
      }
   }
}

bool selector::is_mark(int case_no)
{
   return mark[case_no];
}

bool selector::on()
{
   int xm;
   int ym;
   int button;

   return (w->is_mouse(xm, ym, button) && x <= xm && xm < x + dx && y <= ym && ym <= y + dy);
}

bool selector::eval(int &case_no)
{
   bool d;

   return eval(case_no, d);
}

bool selector::eval(int &case_no, bool &is_quit)
{
   bool any_select = false;

   { // handle_scroller
      if (sc->eval(sc_pos))
      {    // scroll_to_new_pos
         { // clear_fields
            w->set_color(c_background);
            w->fill(x + 1, y + 1, dx - 1, dy - 1);
         }
         for (int i = 0; i < num_cases; i++)
            refresh(i);
         w->tick();
      }
   }
   { // handle_list_event
      if (on())
      { // perform_list_event
         int xm;
         int ym;
         int button;

         case_no = selector_no_case;
         is_quit = false;
         { // get_event
            int d;

            w->mouse(d, d, xm, ym, button);
         }
         if ((button == button1press || button == button2press || button == button3press)
             && (ym - y) / case_dy + sc_pos < num_cases)
         { // handle_mark_event
            switch (button)
            {
            case button1press:
            {    // handle_mark
               { // perhaps_clear_all_marks
                  if (!is_multiple)
                  { // clear_all_marks
                     for (int i = 0; i < num_cases; i++)
                        set_mark(i, false);
                  }
               }
               set_mark((ym - y) / case_dy + sc_pos, true);
               w->tick();
               any_select = true;
               case_no = (ym - y) / case_dy + sc_pos;
            };
            break;
            case button2press:
            { // handle_unmark
               set_mark((ym - y) / case_dy + sc_pos, false);
               w->tick();
               any_select = true;
            };
            break;
            case button3press:
            { // handle_quit
               any_select = true;
               is_quit = true;
            };
            break;
            }
         }
      }
   }
   return any_select;
}
