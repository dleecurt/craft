/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from hcomp.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- hcomp.cc hcomp.hc
 */

#include "compress.h"

int main(int num_params, char *shell_params[])
{
   if (num_params < 3)
   { // usage
      printf("compress -c-x <file_name>\n");
      exit(0);
   }
   else
   { // perform_compress
      compress *c;

      c = new compress();
      if (strcmp(shell_params[1], "-c") == 0)
         c->encode(shell_params[2]);
      else if (strcmp(shell_params[1], "-x") == 0)
         c->decode(shell_params[2]);
      else
      { // usage
         printf("compress -c-x <file_name>\n");
         exit(0);
      }
      delete (c);
   }
}
