/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 * Exact changes: git log -- compress.h
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 111094 hua    compress.h created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include "win.h"
#include "ppm.h"

#ifndef compress_h
#define compress_h

class compress
  {public:

     ppm  *act;
     int  snr;
     char name [128];

     FILE *f;

     compress  ();
     ~compress ();

     char *frame_name (const char postfix []);
     void encode      (char name    []);
     void decode      (char name    []);
 
   };


#endif

