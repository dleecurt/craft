/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 180193 hua    menu.hc    only operate if button1press              =*/
/*=                                                                    =*/
/*= 050293 hua    menu.hc    added enable                              =*/
/*=                                                                    =*/
/*======================================================================*/

#include "menu.h"
#include "datadir.h"

/*----------------------------------------------------------------------*/
/* CLASS menu (static initializations)                                  */
/*----------------------------------------------------------------------*/

bool menu::manager_init = false;
bool menu::group_enabled[max_menu_groups];

/*----------------------------------------------------------------------*/
/* CLASS menu (funktions)                                               */
/*----------------------------------------------------------------------*/

menu::menu() {}

menu::menu(const char w_name[], int w_x, int w_y, const char w_cmds[], int w_mode, int menu_group)

{
   if (!manager_init)
   { // init_group_manager
      manager_init = true;
      for (int i = 0; i < max_menu_groups; i++)
         group_enabled[i] = false;
      group_enabled[std_menu_group] = true;
   }
   { // init_status
      group = menu_group;
      x = w_x;
      y = w_y;
      is_open = false;
      mode = w_mode;
      is_press = false;
      last_mouse_button = nobutton;
      pressed_button = no_menu_request;
      strcpy(name, w_name);
   }
   { // load_colors
      c_border_light = win_default_c("menu_border_light");
      c_background = win_default_c("menu_background");
      c_border_dark = win_default_c("menu_border_dark");
      c_foreground = win_default_c("menu_foreground");
   }
   if (w_cmds[0] != '$' && w_cmds[0] != '/')
   {    // calc_text_dim
      { // scan_buttons
         int p = 0;
         char cmd[128];
         XFontStruct *font_info;
         Display *display;

         { // open_font_info
            char name[128];

            strcpy(name, getenv("DISPLAY"));
            display = XOpenDisplay(name);
            font_info = XLoadQueryFont(display, men_font);
         }
         button_dx = 0;
         num_buttons = 0;
         while (get_cmd(w_cmds, p, cmd))
         {    // handle_button
            { // calc_tdx_tdy
               tdx[num_buttons] = XTextWidth(font_info, cmd, strlen(cmd));
               tdy[num_buttons] = font_info->max_bounds.width;
            }

            strcpy(cmds[num_buttons], cmd);
            is_pressed[num_buttons] = false;
            button_dy = tdy[num_buttons] + 2 * border_dy * 2;
            button_dx = i_max(button_dx, tdx[num_buttons] + 2 * border_dx + 10);
            num_buttons++;
         }
         { // close_font_info
            XFreeFont(display, font_info);
            XCloseDisplay(display);
         }
      }
      dy = button_dy * num_buttons;
      dx = button_dx;
      icon_mode = false;
   }
   else
   { // calc_icon_dim
      int icon_dx;
      int icon_dy;

      { // calc_button_dir
         if (w_cmds[0] == '$')
            strcpy(button_dir, "");
         else
            snprintf(button_dir, sizeof(button_dir), "%s/",
                     craft_data(win_default_s("button_dir")));
      }
      { // scan_buttons
         int p = 0;
         char cmd[128];
         XFontStruct *font_info;
         Display *display;

         { // open_font_info
            char name[128];

            strcpy(name, getenv("DISPLAY"));
            display = XOpenDisplay(name);
            font_info = XLoadQueryFont(display, men_font);
         }
         button_dx = 0;
         num_buttons = 0;
         while (get_cmd(w_cmds, p, cmd))
         {    // handle_button
            { // calc_tdx_tdy
               tdx[num_buttons] = XTextWidth(font_info, cmd, strlen(cmd));
               tdy[num_buttons] = font_info->max_bounds.width;
            }

            strcpy(cmds[num_buttons], cmd);
            is_pressed[num_buttons] = false;
            button_dy = tdy[num_buttons] + 2 * border_dy * 2;
            button_dx = i_max(button_dx, tdx[num_buttons] + 2 * border_dx + 10);
            num_buttons++;
         }
         { // close_font_info
            XFreeFont(display, font_info);
            XCloseDisplay(display);
         }
      }
      { // get_icon_size
         char name[2048];

         snprintf(name, sizeof(name), "%s%s", button_dir, cmds[0]);
         bitmap_size(name, icon_dx, icon_dy);
      }
      icon_mode = true;
      button_dx = icon_dx + 2 * border_dx;
      button_dy = icon_dy + 2 * border_dy;
      dy = button_dy * num_buttons;
      dx = button_dx;
   }
   tick();
}

menu::~menu()
{
   if (is_open)
      delete (w);
}

void menu::open()
{
   { // open_window
      w = new win(name, "", x, y, dx, dy);
      w->set_font(men_font);
   }
   { // write_cmds
      for (int i = 0; i < num_buttons; i++)
         write(i, cmds[i], is_pressed[i]);
   }
   is_open = true;
   w->tick();
}

void menu::close()
{
   delete (w);
   is_open = false;
}

int menu::mouse_button()
{
   return last_mouse_button;
}

void menu::enable(int group, bool mode)
{
   group_enabled[group] = mode;
}

void menu::tick()
{
   if (group_enabled[group] && !is_open)
      open();
   else if (!group_enabled[group] && is_open)
      close();
}

void menu::write(int no, char string[], bool pressed)
{
   { // draw_border
      if (pressed)
      {    // pressed_border
         { // clear
            w->set_color(c_background);
            w->fill(0, no * button_dy, button_dx, button_dy);
         }
         for (int i = 0; i < border_dx; i++)
         { // draw_pressed_line
            w->set_color(c_border_dark);
            if (i < border_dx - 1)
               w->set_color(black);
            w->line(0 + i, no * button_dy + i, 0 + button_dx - i, no * button_dy + i);
            w->line(0 + i, no * button_dy + i, 0 + i, no * button_dy + button_dy - i);
            w->set_color(c_border_light);
            if (i < border_dx - 1)
               w->set_color(black);
            w->line(0 + i, no * button_dy + button_dy - i, 0 + button_dx - i,
                    no * button_dy + button_dy - i);
            w->line(0 + button_dx - i, no * button_dy + i, 0 + button_dx - i,
                    no * button_dy + button_dy - i);
         }
      }
      else
      {    // none_pressed_border
         { // clear
            w->set_color(c_background);
            w->fill(0, no * button_dy, button_dx, button_dy);
         }
         for (int i = 0; i < border_dx; i++)
         { // draw_released_line
            w->set_color(c_border_light);
            w->line(0 + i, no * button_dy + i, 0 + button_dx - i, no * button_dy + i);
            w->line(0 + i, no * button_dy + i, 0 + i, no * button_dy + button_dy - i);
            w->set_color(c_border_dark);
            w->line(0 + i, no * button_dy + button_dy - i, 0 + button_dx - i,
                    no * button_dy + button_dy - i);
            w->line(0 + button_dx - i, no * button_dy + i, 0 + button_dx - i,
                    no * button_dy + button_dy - i);
         }
      }
   }
   if (icon_mode)
   { // write_icon
      char m_name[2048];

      snprintf(m_name, sizeof(m_name), "%s%s", button_dir, string);

      w->set_background(c_background);
      w->set_color(c_foreground);
      w->show_map(border_dx, no * button_dy + border_dy, m_name);
      w->tick();
   }
   else
   { // write_text
      w->function(GXcopy);
      w->set_background(c_background);
      w->set_color(c_foreground);
      w->write(border_dx + (((button_dx - 2 * border_dx) - tdx[no]) / 2),
               no * button_dy + border_dy + tdy[no] + 2
                   + (((button_dy - 2 * border_dy) - tdy[no]) / 2),
               string);
      w->tick();
   }
}

int menu::eval(bool is_repeat)
{
   tick();
   if (is_open)
   { // exec_eval
      int x;
      int y;
      int button;

      { // get_button
         int d;

         w->tick();
         w->mouse(d, d, x, y, button);
         if (button == button1press || button == button3press)
         { // handle_new_press
            last_mouse_button = button;
            if (pressed_button != (y / button_dy))
               release();
            press((y / button_dy));
            is_press = true;
            pressed_button = (y / button_dy);
         }
         else if (button == button1release || button == button3release)
         { // handle_release
            release();
            is_press = false;
            pressed_button = no_menu_request;
         }
         else if (!is_repeat && button != button1press && button != button3press)
         { // handle_release
            release();
            is_press = false;
            pressed_button = no_menu_request;
         }
      }
      { // return_result
         return pressed_button;
      }
   }
   else
      return no_menu_request;
}

void menu::press(int no)
{
   write(no, cmds[no], true);
   is_pressed[no] = true;
}

void menu::release()
{
   if (pressed_button != no_menu_request)
      release(pressed_button);
   pressed_button = no_menu_request;
}

void menu::release(int no)
{
   write(no, cmds[no], false);
   is_pressed[no] = false;
}

bool menu::get_cmd(const char cmds[], int &p, char cmd[])
{
   if ((p >= (int)strlen(cmds)))
      return false;
   else
   { // read_cmd
      int c = 0;
      bool skip_mode = false;

      if (cmds[p] == '$' || cmds[p] == '&')
         p++;
      while (cmds[p] != ':' && !(p >= (int)strlen(cmds)))
      {
         { // handle_skip_mode
            if (cmds[p] == ';')
            { // switch_skip_mode
               skip_mode = !skip_mode;
               p++;
            }
         }
         { // store_char
            if (!skip_mode)
               cmd[c++] = cmds[p];
         }
         p++;
      }
      cmd[c] = 0;
      p++;
   }
   return true;
}
