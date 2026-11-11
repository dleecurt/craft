/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from shade1.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- shade1.cc shade1.hc
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
      printf("usage: craft-shade1 NAME N DD\n       make 15 edge-shaded variants of NAME.N (older "
             "variant)\n");
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
      {    // perform_x1
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
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
      {    // perform_x2
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
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
      {    // perform_x3
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
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
      {    // perform_x4
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x5
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x6
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x7
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
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
      {    // perform_x8
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
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
      {    // perform_x9
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
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
      {    // perform_x10
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x11
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
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
      {    // perform_x12
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x13
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
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
      {    // perform_x14
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
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
      {    // perform_x15
         { // l
            m->colorize(atoi(shell_params[3]), 0);
            m->colorize(atoi(shell_params[3]), 0);
         };
         { // r
            m->colorize(-atoi(shell_params[3]), 0);
            m->colorize(-atoi(shell_params[3]), 0);
         };
         { // b
            m->colorize(0, -atoi(shell_params[3]));
            m->colorize(0, -atoi(shell_params[3]));
         };
         { // t
            m->colorize(0, atoi(shell_params[3]));
            m->colorize(0, atoi(shell_params[3]));
         };
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