/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from ppm_handler.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- ppm_handler.cc ppm_handler.hc
 */

/*======================================================================*/
/*                                                                      */
/* Routines for ppm file access                                         */
/*                                                                      */
/*======================================================================*/

#include "string.h"
#include "errorhandling.h"
#include "ppm_handler.h"
#include "xfile.h"

bool ppm_size(char name[], int &dx, int &dy, int &color)
{
   if (f_exists(name))
   { // get_size
      FILE *f;
      char type[128];

      f = fopen(name, "r");
      read_check(fscanf(f, "%127s %d %d %d", type, &dx, &dy, &color), 4, "PPM image");
      fclose(f);
      return (strcmp(type, "P6") == 0);
   }
   else
      return false;
}
