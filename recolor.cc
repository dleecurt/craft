#include "win.h"
#include "cmap.h"
#include "masks.h"
#include "player.h"
#include "craft_def.h"
#include "object_handler.h"
#include "land.h"
#include "cmap_edit.h"

main(int num_params, char *shell_params[])
{
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