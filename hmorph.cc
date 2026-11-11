/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from hmorph.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- hmorph.cc hmorph.hc
 */

#include "morph.h"

int main(int num_params, char *shell_params[])
{
   if (num_params < 6)
   { // usage
      printf("morph <orig> <dest> <background><regions><num_steps>\n");
      exit(0);
   }
   else
   { // perform_morph
      morph(shell_params[1], shell_params[2], shell_params[3], shell_params[4],
            atoi(shell_params[5]));
   }
}
