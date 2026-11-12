/*
 * Zoom for large screens. Added in 2026 by dleecurt/craft; not part of
 * the CRAFT 3.5 Standard Version (see LICENSE).
 */
#ifndef zoom_h
#define zoom_h

#include "bool.h"

/*----------------------------------------------------------------------*/
/* Craft draws everything at its original size (the game screen is      */
/* 1016x740). With a zoom above 1, every window is made that much       */
/* larger and the X server enlarges the picture (XRender): sharp pixels */
/* for whole-number steps, smooth scaling for the others.               */
/*                                                                      */
/* The setting is "auto" (chosen from the screen size) or one of the    */
/* steps below. It is kept in $XDG_CONFIG_HOME/craft/settings (or       */
/* ~/.config/craft/settings). The -scale option overrides it for one    */
/* run without changing it.                                             */
/*----------------------------------------------------------------------*/

#define zoom_auto_setting 0.0

/* The zoom in effect: the -scale value, else the saved setting, else the
   automatic choice. Always 1 if the X server lacks XRender. */
double zoom_current();

/* The setting as stored: zoom_auto_setting or a step. */
double zoom_setting();

/* The zoom "auto" picks for this screen. */
double zoom_auto();

/* Move the setting one step up (+1) or down (-1) through 1, 1.25, 1.5, 2
   and 3, starting from the zoom shown; going past either end returns to
   "auto". Saves the setting and returns the new zoom in effect. */
double zoom_step(int direction);

/* Describe the setting for menus, e.g. "auto (1.25x)" or "1.5x". */
const char *zoom_label();

/* Use this zoom for this run only (the -scale option). Returns false if
   the value is not between 1 and 4. */
bool zoom_override(double z);

#endif
