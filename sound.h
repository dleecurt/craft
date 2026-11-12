/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 300195 hua    sound.h    created                                   =*/
/*=                                                                    =*/
/*======================================================================*/
/*
 * 2026 dleecurt/craft: replaced the unused /dev/audio stub with sound
 * effects and music (miniaudio). Exact changes: git log -- sound.h
 */

#ifndef sound_h
#define sound_h

#include "bool.h"

/*----------------------------------------------------------------------*/
/* Sound effects and music.                                             */
/*                                                                      */
/* Effects are WAV files in the data directory's sounds/ folder, one    */
/* per effect below (made by sfxgen; replace any of them freely). Music */
/* is any .ogg, .mp3, .flac or .wav file in music/ in the data          */
/* directory or in $XDG_DATA_HOME/craft/music: menu.* plays on the      */
/* start menu, win.* and lose.* at the end of a game, and every other   */
/* file is shuffled into the playlist during a game.                    */
/*                                                                      */
/* Without a sound device the game simply runs silent.                  */
/*----------------------------------------------------------------------*/

enum sound_effect
{
   sfx_chop,      /* felling a tree */
   sfx_mine,      /* digging gold */
   sfx_dig,       /* digging or heaping earth */
   sfx_build,     /* hammering on a building site */
   sfx_sword,     /* hand-to-hand fighting */
   sfx_bow,       /* an arrow is shot */
   sfx_catapult,  /* a stone is thrown */
   sfx_explosion, /* a stone lands */
   sfx_death,     /* a man dies */
   sfx_collapse,  /* a building or ship is destroyed */
   sfx_heal,      /* a doctor heals */
   sfx_trap,      /* a trap springs */
   sfx_built,     /* one of your buildings is finished */
   sfx_ready,     /* one of your men is trained */
   sfx_alert,     /* your men are under attack */
   sfx_ack,       /* you gave an order */
   sfx_click,     /* a menu button */
   num_sound_effects
};

void sound_open  ();
void sound_close ();

/* Keep music playing and tidy finished sounds; called often. */
void sound_tick ();

/* An effect at a map square. Each human player hears it if they can see
   that square: full volume on their screen, fainter further away, and
   panned left or right. */
void sfx_at (int effect, int x, int y);

/* An effect for the human player of this colour only (training done,
   building finished, under attack). */
void sfx_for (int effect, int color);

/* An effect for every human player, not tied to the map. */
void sfx_ui (int effect);

/* Music. */
void music_menu ();          /* start menu and credits */
void music_game ();          /* a game starts: playlist; map effects on */
void music_game_over (bool won);

/* Volumes, 0 to 100, kept in the settings file. */
enum sound_channel
{
   sound_music,
   sound_effects
};

void        sound_step_volume (int channel, int direction);
const char *sound_label       (int channel); /* e.g. "music 70%" */
bool        sound_toggle_mute ();            /* returns true if now muted */

#endif
