#ifndef datadir_h
#define datadir_h

/*----------------------------------------------------------------------*/
/* Locating Craft's data files (hcraft/, buttons/, .windefaults.params) */
/*                                                                      */
/* The data directory is chosen once, in this order:                    */
/*   1. $CRAFT_DATADIR, if set                                          */
/*   2. the current directory, if it holds the data (a source tree)     */
/*   3. CRAFT_DATADIR as set at configure time ($(pkgdatadir))          */
/*----------------------------------------------------------------------*/

/* Base directory, without trailing slash; "." for the current directory. */
const char *craft_datadir ();

/* Full path of a data file, given relative to the data directory.
   Absolute paths are returned unchanged. The result points to one of
   several static buffers, so a few results may be used at once. */
const char *craft_data (const char rel_name[]);

#endif
