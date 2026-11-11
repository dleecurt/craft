/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from color_trans.hc (hcc refinements
 *              expanded) to plain C++
 * Exact changes: git log -- color_trans.cc color_trans.hc
 */


#include "color_trans.h"

color_trans::color_trans()
{
   for (int i = 0; i < 256; i++)
      translate[i] = i;
}

color_trans::~color_trans() {}

void color_trans::add(int c1, int c2)
{
   translate[c1] = c2;
}

int color_trans::trans(int c)
{
   return translate[c];
}
