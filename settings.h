/*
 * Player settings file. Added in 2026 by dleecurt/craft; not part of the
 * CRAFT 3.5 Standard Version (see LICENSE).
 */
#ifndef settings_h
#define settings_h

/*----------------------------------------------------------------------*/
/* A few "key value" lines kept in $XDG_CONFIG_HOME/craft/settings (or  */
/* ~/.config/craft/settings): zoom, music and effects volume.           */
/*----------------------------------------------------------------------*/

/* The value stored for key, or NULL if there is none. */
const char *settings_get(const char key[]);

/* Store a value and write the file at once. */
void settings_set(const char key[], const char value[]);

#endif
