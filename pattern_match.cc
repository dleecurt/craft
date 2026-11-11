#include "string.h"

#include "pattern_match.h"
#include "xstring.h"

bool p_match(char string[], char pattern[])
{
   char correct_tail[128];

   strcpy(correct_tail, substring(pattern, 1));
   return (strcmp((substring(string, strlen(string) - strlen(correct_tail))), correct_tail) == 0);
}
