/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from xbm.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes
 * Exact changes: git log -- xbm.cc xbm.hc
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 111094 hua    xbm.hc     created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include "string.h"
#include "stdio.h"

#include "errorhandling.h"
#include "xbm.h"
#include "xstring.h"
#include "io.h"
#include "paramreader.h"

xbm::xbm(char name[])
{
   FILE *xbm_data;

   { // open_data
      char f_name[128];

      strcpy(f_name, name);
      f_open(xbm_data, f_name, "r");
   }
   { // get_size
      char d[128];

      read_check(fscanf(xbm_data, "%127s %127s %d", d, d, &dx), 3, "XBM bitmap");
      read_check(fscanf(xbm_data, "%127s %127s %d", d, d, &dy), 3, "XBM bitmap");
      { // skip_line
         while (fgetc(xbm_data) != '\n')
         {
         }
      }
      { // skip_line
         while (fgetc(xbm_data) != '\n')
         {
         }
      }
   }
   { // alloc_data
      data = new int[dx * dy];
   }
   { // read_pixels
      int b = 0;

      for (int y = 0; y < dy; y++)
         for (int x = 0; x < dx; x++)
         {    // read_pixel
            { // perhaps_get_b

               if (x % 8 == 0)
               { // get_new_b
                  char bb[128];

                  read_check(fscanf(xbm_data, "%127s", bb), 1, "XBM bitmap");
                  b = hextoint(substring(bb, 2));
               }
            }
            data[ind(x, y)] = b_on(b, x % 8);
         }
   }
   fclose(xbm_data);
}

xbm::xbm(int bdx, int bdy)
{
   { // store_size
      dx = bdx;
      dy = bdy;
   }
   { // alloc_data
      data = new int[dx * dy];
   }
}

xbm::~xbm()
{
   delete (data);
}

void xbm::save(char name[])
{
   FILE *f;

   { // open_f
      f_open(f, name, "w");
   }
   { // write_size
      fprintf(f, "#define x_width %d\n", dx);
      fprintf(f, "#define x_height %d\n", dy);
      fprintf(f, "static char x_bits[] = {");
   }
   { // write_data
      int c = 0;
      int b = 0;
      int x;
      int y;

      for (y = 0; y < dy; y++)
         for (x = 0; x < dx; x++)
         {
            {    // write_byte
               { // perhaps_flush
                  if (((x % 8) == 0) && !(x == 0))
                  { // flush_byte
                     if (!(x == 8 && y == 0))
                        fprintf(f, ", ");
                     if (c % 12 == 0)
                        fprintf(f, "\n");
                     c++;
                     fprintf(f, "0x%s", inttohex(b));
                     b = 0;
                  }
               }
               if (data[ind(x, y)])
                  b = b_set(b, x % 8);
            }
            { // perhaps_flush_last_byte
               if (!((x % 8) == 0) && x == dx - 1)
               { // flush_byte
                  if (!(x == 8 && y == 0))
                     fprintf(f, ", ");
                  if (c % 12 == 0)
                     fprintf(f, "\n");
                  c++;
                  fprintf(f, "0x%s", inttohex(b));
                  b = 0;
               }
            }
         }
   }
   { // write_tail
      fprintf(f, "};");
   }
   fclose(f);
}

void xbm::bit(int x, int y, int &b)
{
   b = data[ind(x, y)];
}

void xbm::set(int x, int y, int b)
{
   data[ind(x, y)] = b;
}

int xbm::ind(int x, int y)
{
   return y * dx + x;
}
