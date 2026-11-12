#ifndef screens_h
#define screens_h

/*----------------------------------------------------------------------*/
/* Opening splash screen and closing credits screen.                    */
/*                                                                      */
/* Both close on a mouse click or key press, or by themselves after a   */
/* few seconds. The splash shows hcraft/splash.cmap when it exists and  */
/* falls back to a text-only title otherwise.                           */
/*----------------------------------------------------------------------*/

void show_splash ();
void show_credits ();

#endif
