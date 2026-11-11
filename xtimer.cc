/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from xtimer.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- xtimer.cc xtimer.hc
 */


#include "xtimer.h"

#include "unistd.h"
#include <errno.h>
#include <time.h>
#include "sys/time.h"

/*----------------------------------------------------------------------*/
/* CLASS xtimer (funktions)                                              */
/*----------------------------------------------------------------------*/

long x_sys_time()
{
   struct timeval tv;

   gettimeofday(&tv, NULL);
   return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void xtimer::start()
{
   stamp = x_sys_time();
   dt = 0;
}

void xtimer::stop()
{
   dt += (double)(x_sys_time() - stamp);
   stamp = x_sys_time();
}

void xtimer::cont()
{
   stamp = x_sys_time();
}

double xtimer::exec()
{
   return (double)(x_sys_time() - stamp) + dt;
}

double xtimer::read()
{
   return dt;
}

/* Sleep for dt milliseconds.
   The original converted dt with tv_usec = dt * 10, so a 50 ms wait
   became 0.5 ms. On 1990s hardware a game cycle took about 50 ms anyway,
   which hid the bug; on modern CPUs the game ran up to ~80x too fast. */
void delay(int dt)
{
   struct timespec ts;

   if (dt <= 0)
      return;
   ts.tv_sec = dt / 1000;
   ts.tv_nsec = (long)(dt % 1000) * 1000000L;
   while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
   {
   }
}
