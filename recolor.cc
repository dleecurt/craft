/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from recolor.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- recolor.cc recolor.hc
 */

#include <stdio.h>
#include "win.h"
#include "cmap.h"
#include "masks.h"
#include "player.h"
#include "craft_def.h"
#include "object_handler.h"
#include "land.h"
#include "cmap_edit.h"

int main(int num_params, char *shell_params[])
{
   if (num_params < 4)
   {
      printf("usage: craft-recolor NAME LIST COLOR\n       make player-colour versions of the "
             "pictures listed in LIST\n");
      return 1;
   }
   FILE *f;

   { // open_f
      f = fopen(shell_params[2], "r");
   }
   { // exec_recolor
      int c;

      while ((fscanf(f, "%d", &c) != EOF))
      {
         { // recolor_pic
            cmap_edit *m;

            { // open_d
               char name[128];

               sprintf(name, "%s.%d.cmap", shell_params[1], c);
               m = new cmap_edit(name);
            }
            { // perform_d
               m->recolor(atoi(shell_params[3]));
            }
            { // save_d
               char name[128];

               sprintf(name, "%s.%d.cmap", shell_params[1], c + (atoi(shell_params[3]) + 1) * 3000);
               m->save_as(name);
            }
            delete (m);
         }
      }
   }
   fclose(f);
}