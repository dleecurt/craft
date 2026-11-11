/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from shade.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- shade.cc shade.hc
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
   if (num_params < 5)
   {
      printf(
          "usage: craft-shade NAME N AMOUNT DD\n       make 15 edge-shaded variants of NAME.N\n");
      return 1;
   }
   cmap_edit *m;
   int h = 1;

   {    // x1
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x1
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x2
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x2
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x3
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x3
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x4
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x4
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x5
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x5
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x6
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x6
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x7
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x7
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x8
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x8
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x9
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x9
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x10
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x10
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x11
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x11
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x12
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x12
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x13
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x13
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x14
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x14
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
   {    // x15
      { // open_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]));
         m = new cmap_edit(name);
      }
      { // perform_x15
         m->colorize(atoi(shell_params[4]), 0, 20, atoi(shell_params[3]));
         m->colorize(-atoi(shell_params[4]), 0, -20, atoi(shell_params[3]));
         m->colorize(0, -atoi(shell_params[4]), -20, atoi(shell_params[3]));
         m->colorize(0, atoi(shell_params[4]), 20, atoi(shell_params[3]));
      };
      { // save_d
         char name[128];

         sprintf(name, "%s.%d.cmap", shell_params[1], atoi(shell_params[2]) + h);
         m->save_as(name);
      }
      delete (m);
   };
   h++;
}