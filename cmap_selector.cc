#include "cmap_selector.h"
#include "errorhandling.h"
#include "bool.h"
#include "xstring.h"
#include "masks.h"
#include "pattern_match.h"

#define cmap_selector_line_color gray70
#define cmap_selector_color_1 gray50
#define cmap_selector_color_2 gray90
#define cmap_selector_background_color gray
#define cmap_selector_marked_case_color yellow
#define cmap_selector_unmarked_case_color black

cmap_selector::cmap_selector(const char v_name[], win *v_w, int v_num_cases,
                             char v_case_string[max_cmap_selector_cases][128], int v_x, int v_y,
                             int v_dx, int v_dy, bool v_multiple_select)

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
   }
   { // load_colors
      c_border_light = win_default_c("selector_border_light");
      c_border_dark = win_default_c("selector_border_dark");
      c_background = win_default_c("selector_background");
      c_foreground = win_default_c("selector_foreground");
      c_line = win_default_c("selector_line");
      c_mark = win_default_c("selector_mark");
   }
   { // load_maps
      case_dy = 0;
      case_dx = 0;
      for (int i = 0; i < v_num_cases; i++)
      {
         case_map[i] = new cmap(w, v_case_string[i]);
         case_dy = i_max(case_dy, case_map[i]->dy);
         case_dx = i_max(case_dx, case_map[i]->dx);
      }
      case_dy += 2;
   }
   set_cases(v_num_cases, v_case_string);
}

cmap_selector::~cmap_selector()
{
   delete (sc);
}

void cmap_selector::set_cases(int v_num_cases, char v_case_string[max_cmap_selector_cases][128])

{
   { // set_num_cases
      num_cases = v_num_cases;
   }
   { // show_cmap_selector
      w->set_color(c_background);
      w->fill(x, y, dx, dy);
   }
   {    // set_scroller
      { // perhaps_delete_scroller
         if (is_scroller)
            delete (sc);
      }
      is_scroller = true;
      sc_pos = 0;
      sc = new scroller(name, w, x + dx + 1, y, 1, dy, 0, num_cases - 1, dy / (case_dy + 2),
                        sc_pos);
   }
   { // set_case_strings
      for (int i = 0; i < num_cases; i++)
      {
         mark[i] = false;
         set_string(i, v_case_string[i]);
      }
   }
   { // draw_frame
      frame(w, x, y, x + dx, y + dy, c_border_dark, c_border_light);
   }
   w->tick();
}

void cmap_selector::set_mark(int case_no, bool mode)
{
   mark[case_no] = mode;
   refresh(case_no);
}

void cmap_selector::set_string(int case_no, char string[])
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

void cmap_selector::refresh(int case_no)
{
   if ((y <= (case_no)*case_dy + y - sc_pos * case_dy + 2
        && (case_no)*case_dy + y - sc_pos * case_dy + 2 + case_dy <= y + dy))
   {    // draw_string
      { // set_colors
         w->set_background(c_background);
         if (is_mark(case_no))
            w->set_color(c_mark);
         else
            w->set_color(c_foreground);
      }

      w->set_clip(x + 2, y + 2, dx - 4, dy - 4);
      case_map[case_no]->show(x + 2, (case_no)*case_dy + y - sc_pos * case_dy + 2);
      w->write(x + case_dx + 8, (case_no)*case_dy + y - sc_pos * case_dy + 2 + case_dy / 2,
               substring(case_strings[case_no], 0, draw_length[case_no]));
      w->tick();
      w->set_clip(0, 0, w->dx(), w->dy());
   }
}

bool cmap_selector::is_mark(int case_no)
{
   return mark[case_no];
}

bool cmap_selector::on()
{
   int xm;
   int ym;
   int button;

   return (w->is_mouse(xm, ym, button) && x <= xm && xm < x + dx && y <= ym && ym <= y + dy);
}

bool cmap_selector::eval(int &case_no)
{
   bool d;

   return eval(case_no, d);
}

bool cmap_selector::eval(int &case_no, bool &is_quit)
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

         case_no = cmap_selector_no_case;
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

bool cmap_sel(char name[], const char pattern[])
{
   int num_cases;
   char cases[max_cmap_selector_cases][128];
   bool quit;

   { // get_files
      get_file_list(pattern, num_cases, cases);
   }
   { // perform_sel
      win *w;
      cmap_selector *sel;
      int case_no;
      button *cancel;

      { // open_sel
         quit = false;
         w = new win("cmap_sel", "", by_fix, by_fix, 400, 700);
         frame(w);
         cancel = new button(w, "cancel", 180, 640);
         sel = new cmap_selector("cmap_sel", w, num_cases, cases, 10, 10, 340, 600);
      }
      w->mark_mouse();
      while (!sel->eval(case_no) && !quit)
      {
         w->scratch_mouse();
         w->mark_mouse();
         quit = cancel->eval();
      }
      if (!quit)
         strcpy(name, cases[case_no]);
      delete (cancel);
      delete (sel);
      delete (w);
   }
   return !quit;
}

void get_file_list(const char pattern[], int &num_files,
                   char file_list[max_cmap_selector_cases][128])

{
   char file_name[256];
   char file_pattern[256];

   { // set_name_and_pattern
      strcpy(file_name, f_path(pattern));
      strcpy(file_pattern, f_name(pattern));
   }
   { // get_file_list
      char cmd[512];

      if (strlen(file_name) == 0)
         sprintf(cmd, "ls -F > filesel.temp");
      else
         sprintf(cmd, "ls -F %s > filesel.temp", file_name);
      run_command(cmd);
   }
   { // read_file_list
      FILE *l_file;
      char f_line[1024];

      l_file = fopen("filesel.temp", "r");
      num_files = 0;
      while ((fscanf(l_file, "%1023s", f_line) != EOF && num_files < max_cmap_selector_cases))
         if (p_match(f_line, file_pattern))
         { // store_name
            sprintf(file_list[num_files], "%s/%s", file_name, f_tail(f_line));
            num_files = i_min(max_cmap_selector_cases - 1, num_files + 1);
         }
      { // perhaps_skip_empty_msg
         if (num_files == 4 && strcmp(file_list[3], "found") == 0
             && strcmp(file_list[2], "not") == 0)
            num_files = 0;
         if (num_files == 4 && strcmp(file_list[3], "match.") == 0
             && strcmp(file_list[2], "No") == 0)
            num_files = 0;
      }
      fclose(l_file);
   }
}
