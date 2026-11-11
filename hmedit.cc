/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from hmedit.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- hmedit.cc hmedit.hc
 */

#include "morph_edit.h"

int main(int num_params, char *shell_params[])
{
   if (num_params < 4)
   { // usage
      printf("morph  <regions> <orig> <dest>\n");
      exit(0);
   }
   else
   { // perform_morph
      morph_edit *me;
      bool is_quit = false;

      { // open_edit
         me = new morph_edit(shell_params[1], shell_params[2], shell_params[3]);
      }
      { // perform_edit
         while (!is_quit)
         {
            me->eval(is_quit);
         }
      }
      delete (me);
   }
}
