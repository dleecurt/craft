/*
 * Player settings file. Added in 2026 by dleecurt/craft; not part of the
 * CRAFT 3.5 Standard Version (see LICENSE).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "settings.h"

#define max_settings 32
#define max_key 32
#define max_value 128

static char keys[max_settings][max_key];
static char values[max_settings][max_value];
static int num_settings = 0;
static int loaded = 0;

static const char *settings_file(int create_dir)
{
   static char path[1100];
   char dir[1024];
   const char *xdg = getenv("XDG_CONFIG_HOME");
   const char *home = getenv("HOME");

   if (xdg != NULL && xdg[0] == '/')
      snprintf(dir, sizeof(dir), "%s/craft", xdg);
   else
      snprintf(dir, sizeof(dir), "%s/.config/craft", home != NULL ? home : ".");
   if (create_dir)
   {
      for (char *s = dir + 1; *s != 0; s++)
         if (*s == '/')
         {
            *s = 0;
            mkdir(dir, 0755);
            *s = '/';
         }
      mkdir(dir, 0755);
   }
   snprintf(path, sizeof(path), "%s/settings", dir);
   return path;
}

static int find(const char key[])
{
   for (int i = 0; i < num_settings; i++)
      if (strcmp(keys[i], key) == 0)
         return i;
   return -1;
}

static void load()
{
   FILE *f;
   char line[256];

   if (loaded)
      return;
   loaded = 1;
   f = fopen(settings_file(0), "r");
   if (f == NULL)
      return;
   while (fgets(line, sizeof(line), f) != NULL && num_settings < max_settings)
   {
      char key[max_key];
      char value[max_value];

      if (line[0] == '#')
         continue;
      if (sscanf(line, "%31s %127s", key, value) == 2 && find(key) < 0)
      {
         snprintf(keys[num_settings], max_key, "%s", key);
         snprintf(values[num_settings], max_value, "%s", value);
         num_settings++;
      }
   }
   fclose(f);
}

static void save()
{
   FILE *f = fopen(settings_file(1), "w");

   if (f == NULL)
   {
      fprintf(stderr, "craft: cannot save settings to %s\n", settings_file(0));
      return;
   }
   fprintf(f, "# Craft settings, written by the game\n");
   for (int i = 0; i < num_settings; i++)
      fprintf(f, "%s %s\n", keys[i], values[i]);
   fclose(f);
}

const char *settings_get(const char key[])
{
   int i;

   load();
   i = find(key);
   return i < 0 ? NULL : values[i];
}

void settings_set(const char key[], const char value[])
{
   int i;

   load();
   i = find(key);
   if (i < 0)
   {
      if (num_settings >= max_settings)
         return;
      i = num_settings++;
      snprintf(keys[i], max_key, "%s", key);
   }
   snprintf(values[i], max_value, "%s", value);
   save();
}
