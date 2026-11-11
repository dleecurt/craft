#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datadir.h"

#ifndef CRAFT_DATADIR
#define CRAFT_DATADIR "."
#endif

#define max_path 1024
#define num_buffers 8

static bool has_data(const char dir[])
{
   char probe[max_path];

   snprintf(probe, sizeof(probe), "%s/.windefaults.params", dir);
   if (access(probe, R_OK) != 0)
      return false;
   snprintf(probe, sizeof(probe), "%s/hcraft/pic_cata_new", dir);
   return access(probe, R_OK) == 0;
}

const char *craft_datadir()
{
   static char base[max_path] = "";

   if (base[0] == 0)
   {
      const char *env = getenv("CRAFT_DATADIR");

      if (env != NULL && env[0] != 0)
         snprintf(base, sizeof(base), "%s", env);
      else if (has_data("."))
         strcpy(base, ".");
      else
         snprintf(base, sizeof(base), "%s", CRAFT_DATADIR);

      size_t l = strlen(base);
      while (l > 1 && base[l - 1] == '/')
         base[--l] = 0;
   }
   return base;
}

const char *craft_data(const char rel_name[])
{
   static char buffers[num_buffers][max_path];
   static int next = 0;
   char *result = buffers[next];

   next = (next + 1) % num_buffers;
   int len;

   if (rel_name[0] == '/')
      len = snprintf(result, max_path, "%s", rel_name);
   else
      len = snprintf(result, max_path, "%s/%s", craft_datadir(), rel_name);
   if (len >= max_path)
      fprintf(stderr, "craft: data path too long: %s/%s\n", craft_datadir(), rel_name);
   return result;
}
