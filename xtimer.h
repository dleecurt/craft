/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: compiler, warning and bug fixes
 * Exact changes: git log -- xtimer.h
 */

#include "stdio.h"
#include "sys/types.h"
#include <sys/time.h>
#include "sys/time.h"

/*----------------------------------------------------------------------*/
/* CLASS xtimer (deklarations)                                          */
/*----------------------------------------------------------------------*/

#ifndef xtimer_h
#define xtimer_h

class xtimer
  {private :

     long   stamp;
     double dt;

   public :

     void   start ();
     void   cont  ();
     void   stop  ();
     double read  ();
     double exec  ();

  };

long x_sys_time ();
void delay (int dt);

#endif
