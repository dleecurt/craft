/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from xtimer.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- xtimer.cc xtimer.hc
 */


#include "xtimer.h"

#include "unistd.h"
#include "sys/time.h"
#include "signal.h"

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

void sig(int /* signo */)
{
   signal(SIGALRM, sig);
}

void delay(int dt)
{
   itimerval tick;

   tick.it_value.tv_sec = dt / 10000;
   tick.it_interval.tv_sec = 0;
   tick.it_value.tv_usec = dt * 10;
   tick.it_interval.tv_usec = 600000;

   signal(SIGALRM, sig);
   if (setitimer(ITIMER_REAL, &tick, 0) == 0)
   {
      pause();
   }
}
