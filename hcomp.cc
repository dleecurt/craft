#include "compress.h"

main(int num_params, char *shell_params[])
{
   if ((num_params < 3))
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
