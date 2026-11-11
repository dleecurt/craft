/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2026       dleecurt/craft: converted from id_handler.hc (hcc refinements
 *              expanded) to plain C++
 * Exact changes: git log -- id_handler.cc id_handler.hc
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file           subject                               =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 021192 hua    id_handler.hc  created                               =*/
/*=                                                                    =*/
/*======================================================================*/

#include "id_handler.h"

/*======================================================================*/
/*                                                                      */
/* ID HANDLER                                                           */
/*                                                                      */
/* generation of janus wide unique ids for div. purposes                */
/*                                                                      */
/*======================================================================*/

int id_counter = 0;

int new_id()
{
   return ++id_counter;
}
