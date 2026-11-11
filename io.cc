#include "io.h"

void ack()
{
   while (getchar() != '\n')
      ;
}

void s_ack(char msg[])
{
   printf("%s ", msg);
   ack();
}

bool ok()
{
   return (getchar() != 'q');
}

double d_get(char msg[])
{
   float d = 0;

   printf("%s", msg);
   if (scanf("%f", &d) != 1)
      d = 0;
   return (double)d;
}

int i_get(char msg[])
{
   int i = 0;

   printf("%s", msg);
   if (scanf("%d", &i) != 1)
      i = 0;
   return i;
}
