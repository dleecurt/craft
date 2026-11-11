/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from pattern_match.hc (hcc refinements
 *              expanded) to plain C++
 * Exact changes: git log -- pattern_match.cc pattern_match.hc
 */

#include "string.h"

#include "pattern_match.h"
#include "xstring.h"

bool p_match(char string[], char pattern[])
{
   char correct_tail[128];

   strcpy(correct_tail, substring(pattern, 1));
   return (strcmp((substring(string, strlen(string) - strlen(correct_tail))), correct_tail) == 0);
}
