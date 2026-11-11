/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from multi_dial.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- multi_dial.cc multi_dial.hc
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file          subject                                =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 060794 hua    multi_dial.hc created                                =*/
/*=                                                                    =*/
/*======================================================================*/

#include "multi_dial.h"
#include "errorhandling.h"
#include "masks.h"

#define font "-misc-*-*-*-*-*-*-*-*-*-*-*-*-*"

#define line_dy 20

multi_dial::multi_dial(char name[], char buttons[], bool used_history)

{
   { // init
      is_history = used_history;
      strcpy(title, name);
   }
   { // pharse_params
      int p = 0;
      XFontStruct *font_info;
      Display *display;

      num_buttons = 0;
      max_b_title_dx = 0;
      { // open_font_info
         char name[128];

         strcpy(name, getenv("DISPLAY"));
         display = XOpenDisplay(name);
         font_info = XLoadQueryFont(display, font);
      }
      while (get_cmd(buttons, p, b_title[num_buttons]))
      {
         { // grab_button
            max_b_title_dx = i_max(max_b_title_dx, XTextWidth(font_info, b_title[num_buttons],
                                                              strlen(b_title[num_buttons])));
            num_buttons++;
         }
      }
      { // close_font_info
         XFreeFont(display, font_info);
         XCloseDisplay(display);
      }
   }
   { // open_w
      w = new win(title, "", by_fix, by_fix, max_b_title_dx + 44, num_buttons * line_dy + 24);
      background(w);
   }
   { // init_values
      for (int i = 0; i < num_buttons; i++)
         values[i] = true;
   }
   { // perhaps_load
      if (is_history)
         load(name);
   }
   { // open_dials
      for (int i = 0; i < num_buttons; i++)
         d_buttons[i]
             = new dial(w, b_title[i], max_b_title_dx + 12, 12, i * line_dy + 14, values[i]);
   }
}

multi_dial::~multi_dial()
{
   if (is_history)
      save(title);
   for (int i = 0; i < num_buttons; i++)
      delete (d_buttons[i]);
   delete (w);
}

void multi_dial::save(char name[])
{
   FILE *f;

   { // open_f
      f = fopen(complete(name, ".mulopt"), "w");
   }
   { // write_values
      for (int i = 0; i < num_buttons; i++)
         fprintf(f, "%d\n", values[i]);
   }
   fclose(f);
}

void multi_dial::load(char name[])
{
   FILE *f;

   if ((f = fopen(complete(name, ".mulopt"), "r")) != NULL)
   {    // perform_load
      { // read_values
         for (int i = 0; i < num_buttons; i++)
            if (fscanf(f, "%d", &values[i]) != 1)
               break;
      }
      fclose(f);
   }
}

bool multi_dial::eval()
{
   bool anything = false;

   w->mark_mouse();
   for (int i = 0; i < num_buttons; i++)
      anything |= d_buttons[i]->eval(values[i]);
   w->scratch_mouse();
   return anything;
}

bool multi_dial::pressed(int bno)
{
   return values[bno];
}

bool multi_dial::press(int bno, bool mode)
{
   values[bno] = mode;
   eval();
   return true;
}

bool multi_dial::get_cmd(char cmds[], int &p, char cmd[])
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
