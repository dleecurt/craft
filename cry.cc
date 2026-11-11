/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from cry.hc (hcc refinements
 *              expanded) to plain C++
 * Exact changes: git log -- cry.cc cry.hc
 */

#include "stdlib.h"
#include "cry.h"

void cry(char *func_name)
{
   printf("\n Error : calling dummy virtual function (%s)\n", func_name);
   exit(1);
}
