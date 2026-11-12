/*
 * Zoom for large screens. Added in 2026 by dleecurt/craft; not part of
 * the CRAFT 3.5 Standard Version (see LICENSE).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include "zoom.h"
#include "settings.h"

/* The game screen at zoom 1, plus room for a title bar and a panel. */
#define game_dx 1016
#define game_dy 740
#define spare_dx 20
#define spare_dy 120

static const double steps[] = { 1.0, 1.25, 1.5, 2.0, 3.0 };
static const int num_steps = sizeof(steps) / sizeof(steps[0]);

static bool loaded = false;
static double setting = zoom_auto_setting;
static double override_z = 0;
static bool have_render = true;
static double auto_z = 0;

static bool valid_step(double z)
{
   for (int i = 0; i < num_steps; i++)
      if (fabs(steps[i] - z) < 0.001)
         return true;
   return false;
}

static void load()
{
   const char *value;

   if (loaded)
      return;
   loaded = true;
   value = settings_get("zoom");
   if (value == NULL || strcmp(value, "auto") == 0)
      setting = zoom_auto_setting;
   else if (valid_step(atof(value)))
      setting = atof(value);
}

static void save()
{
   char value[32];

   if (setting == zoom_auto_setting)
      snprintf(value, sizeof(value), "auto");
   else
      snprintf(value, sizeof(value), "%g", setting);
   settings_set("zoom", value);
}

double zoom_auto()
{
   if (auto_z == 0)
   {
      const char *name = getenv("DISPLAY");
      Display *d = XOpenDisplay(name != NULL ? name : "");

      auto_z = 1;
      if (d == NULL)
         return auto_z;
      {
         int ev;
         int err;

         have_render = XRenderQueryExtension(d, &ev, &err);
      }
      if (have_render)
      {
         int w = DisplayWidth(d, DefaultScreen(d));
         int h = DisplayHeight(d, DefaultScreen(d));

         for (int i = 0; i < num_steps; i++)
            if (game_dx * steps[i] <= w - spare_dx && game_dy * steps[i] <= h - spare_dy)
               auto_z = steps[i];
      }
      XCloseDisplay(d);
   }
   return auto_z;
}

double zoom_setting()
{
   load();
   return setting;
}

double zoom_current()
{
   double a = zoom_auto(); /* also checks for XRender */

   if (!have_render)
      return 1;
   if (override_z != 0)
      return override_z;
   load();
   return setting == zoom_auto_setting ? a : setting;
}

/* From "auto", + and - move to the step above or below the zoom auto
   picked. Going past 1x or 3x returns to "auto". */
double zoom_step(int direction)
{
   double now;
   int i;

   load();
   now = setting == zoom_auto_setting ? zoom_auto() : setting;
   i = 0;
   while (i < num_steps - 1 && steps[i] < now - 0.001)
      i++;
   if (setting == zoom_auto_setting && direction > 0 && steps[i] < now + 0.001)
      i++; /* auto already shows steps[i]; go one above it */
   else if (setting == zoom_auto_setting && direction < 0)
      i--;
   else
      i += direction;
   if (i < 0 || i >= num_steps)
      setting = zoom_auto_setting;
   else
      setting = steps[i];
   override_z = 0;
   save();
   return zoom_current();
}

const char *zoom_label()
{
   static char s[64];

   if (override_z != 0)
      snprintf(s, sizeof(s), "%gx (-scale)", override_z);
   else if (zoom_setting() == zoom_auto_setting)
      snprintf(s, sizeof(s), "auto (%gx)", zoom_current());
   else
      snprintf(s, sizeof(s), "%gx", zoom_current());
   return s;
}

bool zoom_override(double z)
{
   if (z < 1 || z > 4)
      return false;
   override_z = z;
   return true;
}
