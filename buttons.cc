/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 070793 hua    buttons.hc created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include "buttons.h"
#include "datadir.h"
#include "win.h"

button::button(win *b_w, const char b_label[], int b_x, int b_y, bool b_with_repeat, int b_dx,
               int b_dy, int b_dx_border, int b_dy_border, int b_label_color, char b_label_font[])

{
   { // init_status
      strcpy(label, b_label);
      w = b_w;
      x = b_x;
      y = b_y;
      dx = b_dx;
      dy = b_dy;
      dx_border = win_default_i(b_dx_border, "button_border_dx");
      dy_border = win_default_i(b_dy_border, "button_border_dy");
      border_color_light = win_default_c("button_border_light");
      border_color_dark = win_default_c("button_border_dark");
      button_color = win_default_c("button_background");
      label_color = win_default_c(b_label_color, "button_foreground");
      with_repeat = b_with_repeat;
      is_pressed = false;
      snprintf(button_dir, sizeof(button_dir), "%s", craft_data(win_default_s("button_dir")));
   }
   { // calc_font
      if (b_label_font == NULL)
         strcpy(label_font, default_button_font);
      else
         strcpy(label_font, b_label_font);
      /*
      w->set_font (label_font);
*/
   }
   if (dx == by_default || dy == by_default)
   {
      if (label[0] != '$' && label[0] != '/')
      { // calc_text_dim
         int label_dx;
         int label_dy;

         is_icon = false;
         w->text_size(label, label_dx, label_dy);
         dx = label_dx + 2 * dx_border + 10;
         dy = label_dy + 2 * dy_border + 10;
         x_label = x + dx_border + (((dx - 2 * dx_border) - label_dx) / 2);
         y_label = y + dy_border + label_dy + (((dy - 2 * dy_border) - label_dy) / 2);
      }
      else
      { // calc_icon_dim
         char button_name[2048];

         { // calc_button_name
            if (label[0] == '/')
               snprintf(button_name, sizeof(button_name), "%s/%s", button_dir, &label[1]);
            else
               strcpy(button_name, &label[1]);
         }
         bitmap_size(button_name, dx, dy);
         dx += 2 * dx_border;
         dy += 2 * dy_border;
         x_label = x + dx_border;
         y_label = y + dy_border;
         is_icon = true;
      }
   }
   { // get_background

      /* 
      w->store_map (background, x, y, dx, dy);
*/
   }
   write(label);
}

button::~button()
{
   /*
   w->show_map   (x, y, background, dx, dy);
   w->delete_map (background); 
*/
   w->tick();
}

void button::press()
{
   press(!is_pressed);
}
void button::press(bool mode)
{
   is_pressed = mode;
   write(label, is_pressed);
}

bool button::eval()
{
   int d;

   return eval(d);
}

bool button::eval(int &button)
{
   int d;
   int xe;
   int ye;
   int but;
   bool is_mouse;

   w->tick();
   { // skip_release_events
      while (w->is_mouse(xe, ye, but) && (x < xe && xe < x + dx && y < ye && ye < y + dy)
             && (but == button1release || but == button2release || but == button3release))
         w->mouse(xe, ye, but);
   }
   is_mouse = w->is_mouse(xe, ye, button);
   { // perhaps_grab_event
      if (is_mouse && (x < xe && xe < x + dx && y < ye && ye < y + dy))
         w->mouse(xe, ye, button);
   }
   if (is_mouse && (x < xe && xe < x + dx && y < ye && ye < y + dy) && !with_repeat)
   { // skip_input
      w->tick();
      while (w->mouse(d, d, but))
      {
         w->tick();
      }
   }
   return (is_mouse && (x < xe && xe < x + dx && y < ye && ye < y + dy))
          || (with_repeat && w->mouse_is_pressed(0)
              && (x < xe && xe < x + dx && y < ye && ye < y + dy));
}

void button::write(const char label_string[])
{
   if (label_string != label)
      strcpy(label, label_string);
   write(label, is_pressed);
}

void button::write(char label[], bool is_pressed)
{
   { // draw_border
      w->function(GXcopy);
      if (is_pressed)
      {    // pressed_border
         { // clear
            w->set_color(button_color);
            w->fill(x, y, dx, dy);
         }
         for (int i = 0; i < dx_border; i++)
         { // draw_pressed_line
            if (i <= dx_border / 2)
               w->set_color(black);
            else
               w->set_color(border_color_light);
            w->line(x + i, y + i, x + dx - i, y + i);
            w->line(x + i, y + i, x + i, y + dy - i);
            if (i <= dx_border / 2)
               w->set_color(black);
            else
               w->set_color(border_color_dark);
            w->line(x + i, y + dy - i, x + dx - i, y + dy - i);
            w->line(x + dx - i, y + i, x + dx - i, y + dy - i);
         }
      }
      else
      {    // none_pressed_border
         { // clear
            w->set_color(button_color);
            w->fill(x, y, dx, dy);
         }
         for (int i = 0; i < dx_border; i++)
         { // draw_released_line
            w->set_color(border_color_light);
            w->line(x + i, y + i, x + dx - i, y + i);
            w->line(x + i, y + i, x + i, y + dy - i);
            w->set_color(border_color_dark);
            w->line(x + i, y + dy - i, x + dx - i, y + dy - i);
            w->line(x + dx - i, y + i, x + dx - i, y + dy - i);
         }
      }
   }
   if (is_icon)
   { // write_icon
      char button_name[2048];

      { // calc_button_name
         if (label[0] == '/')
            snprintf(button_name, sizeof(button_name), "%s/%s", button_dir, &label[1]);
         else
            strcpy(button_name, &label[1]);
      }
      w->set_background(button_color);
      w->set_color(label_color);
      w->show_map(x_label, y_label, button_name);
   }
   else
   { // write_text
      w->function(GXcopy);
      w->set_background(button_color);
      w->set_color(label_color);
      w->write(x_label, y_label, label);
      w->tick();
   }
   w->tick();
}
