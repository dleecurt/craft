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
