#include <stdio.h>
#include <string.h>

#include "screens.h"
#include "sound.h"
#include "win.h"
#include "cmap.h"
#include "masks.h"
#include "xfile.h"
#include "xtimer.h"
#include "datadir.h"
#include "paramreader.h"

#ifndef VERSION
#define VERSION "3.5.1"
#endif

#define splash_name "hcraft/splash"
#define splash_seconds 5
#define credits_seconds 10
#define caption_dy 75
#define poll_ms 50

/* Wait until the window gets a full mouse click (press and release) or a
   key press, or until max_seconds have passed. Waiting for the release
   matters: closing on the press would let the release fall through to
   the window underneath, where it blocks that window's buttons. */
static void wait_for_dismiss(win *w, int max_seconds)
{
   long start = x_sys_time();
   bool pressed = false;

   w->tick();
   while (x_sys_time() - start < (long)max_seconds * 1000)
   {
      int x;
      int y;
      int button;
      int key;

      w->tick();
      while (w->mouse(x, y, button))
      {
         if (button == button1press || button == button2press || button == button3press)
            pressed = true;
         else if (pressed
                  && (button == button1release || button == button2release
                      || button == button3release))
            return;
      }
      if (w->inchar(key) != 0 || key != 0)
         return;
      delay(poll_ms);
   }
}

static void write_centered(win *w, int w_dx, int y, const char text[])
{
   int dx;
   int dy;

   w->text_size(text, dx, dy);
   w->write((w_dx - dx) / 2, y, text);
}

/* Read a picture's size from its .cmap header ("version dx dy ..."). */
static bool picture_size(const char name[], int &dx, int &dy)
{
   FILE *f = fopen(name, "r");
   int version;
   bool ok;

   if (f == NULL)
      return false;
   ok = fscanf(f, "%d %d %d", &version, &dx, &dy) == 3 && dx > 0 && dy > 0;
   fclose(f);
   return ok;
}

void show_splash()
{
   int pic_dx = 0;
   int pic_dy = 0;
   bool with_picture = picture_size(craft_data(splash_name ".cmap"), pic_dx, pic_dy);
   int w_dx = with_picture ? (pic_dx > 300 ? pic_dx + 20 : 320) : 400;
   int w_dy = with_picture ? pic_dy + 20 + caption_dy : 200;
   cmap *picture = NULL;
   win *w;

   w = new win("CRAFT", "", 100, 100, w_dx, w_dy, true, false);
   w->set_background(win_default_c("tell_background"));
   w->set_color(win_default_c("tell_background"));
   w->fill(0, 0, w_dx, w_dy);
   frame(w);
   if (with_picture)
   {
      picture = new cmap(w, craft_data(splash_name));
      picture->show((w_dx - pic_dx) / 2, 10);
   }
   w->set_color(win_default_c("tell_foreground"));
   {
      int text_y = with_picture ? pic_dy + 35 : w_dy / 2 - 20;

      write_centered(w, w_dx, text_y, "CRAFT: The Vicious Vikings");
      write_centered(w, w_dx, text_y + 20, "Version " VERSION);
      write_centered(w, w_dx, text_y + 40, "click or press a key");
   }
   w->tick();
   wait_for_dismiss(w, splash_seconds);
   delete picture;
   delete w;
}

void show_credits()
{
   music_menu();
   static const char *lines[] = { "CRAFT: The Vicious Vikings",
                                  "",
                                  "Original game (version 3.5, 1997)",
                                  "by Uwe Beyer and team, GMD",
                                  "",
                                  "Fixes for modern compilers",
                                  "Quipyowert2 (2018-2019)",
                                  "",
                                  "This version " VERSION,
                                  "dleecurt/craft (2026)",
                                  "",
                                  "@art",
                                  "Released under the Artistic License",
                                  "",
                                  "Thanks for playing!",
                                  NULL };
   int w_dx = 420;
   int line_dy = 18;
   int num_lines = 0;
   int w_dy;
   win *w;

   bool with_art = f_exists(craft_data(splash_name ".cmap"));
   const char *shown[32];
   int num_shown = 0;

   while (lines[num_lines] != NULL)
   {
      if (strcmp(lines[num_lines], "@art") != 0)
         shown[num_shown++] = lines[num_lines];
      else if (with_art)
      { // credit the splash picture only when it is installed
         shown[num_shown++] = "Splash artwork: \"Viking warrior\",";
         shown[num_shown++] = "OpenClipart via freesvg.org (CC0)";
         shown[num_shown++] = "";
      }
      num_lines++;
   }
   w_dy = num_shown * line_dy + 60;
   w = new win("CRAFT credits", "", 100, 100, w_dx, w_dy, true, false);
   w->set_background(win_default_c("tell_background"));
   w->set_color(win_default_c("tell_background"));
   w->fill(0, 0, w_dx, w_dy);
   frame(w);
   w->set_color(win_default_c("tell_foreground"));
   for (int i = 0; i < num_shown; i++)
      write_centered(w, w_dx, 30 + i * line_dy, shown[i]);
   w->tick();
   wait_for_dismiss(w, credits_seconds);
   delete w;
}
