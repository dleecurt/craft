/*
 * CRAFT 3.5 - modified version, see LICENSE.
 *   2026       dleecurt/craft: converted from sound.hc (hcc refinements
 *              expanded) to plain C++; the unused /dev/audio stub replaced
 *              with sound effects and music using miniaudio
 * Exact changes: git log -- sound.cc sound.hc
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 300195 hua    sound.hc   created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "third_party/miniaudio/miniaudio.h"
#pragma GCC diagnostic pop

#include "sound.h"
#include "datadir.h"
#include "settings.h"
#include "player.h"
#include "craft.h"

/*--- state -------------------------------------------------------------*/

static bool is_open = false;
static ma_engine engine;
static ma_sound_group music_group;
static ma_sound_group effects_group;

static const char *const effect_names[num_sound_effects]
    = { "chop",     "mine", "dig",  "build", "sword", "bow",   "catapult", "explosion", "death",
        "collapse", "heal", "trap", "built", "ready", "alert", "ack",      "click" };

/* Shortest gap between two of the same effect (ms), and how many may
   sound at once, so a big battle stays a battle and not a roar. */
static const int effect_gap[num_sound_effects]
    = { 90, 110, 120, 90, 70, 60, 150, 150, 120, 300, 200, 100, 400, 300, 12000, 80, 0 };
static const int effect_max[num_sound_effects]
    = { 3, 3, 3, 3, 4, 4, 3, 3, 3, 2, 2, 2, 1, 2, 1, 1, 2 };

static ma_sound effect_proto[num_sound_effects];
static bool effect_loaded[num_sound_effects];
static double effect_last[num_sound_effects];
static double alert_last[max_players];

#define max_voices 32

struct voice
{
   ma_sound s;
   bool used;
   int effect;
};

static voice voices[max_voices];

/* music */
static std::string menu_track;
static std::string win_track;
static std::string lose_track;
static std::vector<std::string> playlist;
static size_t playlist_next = 0;
static ma_sound *music_now = NULL;
static ma_sound *music_fading = NULL;

enum music_mode
{
   mode_none,
   mode_menu,
   mode_game,
   mode_over
};

static music_mode mode = mode_none;
static bool menu_after_cue = false; /* menu music once the win/lose cue ends */
static bool game_running = false;

/* volumes */
static int volume[2] = { 70, 80 };
static bool muted = false;

/* recording instead of playing (CRAFT_SOUND_RECORD=file.wav, for tests) */
static bool recording = false;
static ma_encoder recorder;
static double record_last = 0;

/* CRAFT_SOUND_DEBUG: log every effect asked for, and what became of it */
static int debug = -1;

static void debug_log(int effect, const char what[], double vol)
{
   if (debug < 0)
      debug = getenv("CRAFT_SOUND_DEBUG") != NULL;
   if (debug && effect >= 0 && effect < num_sound_effects)
      fprintf(stderr, "craft: sound %-9s %-10s %.2f\n", effect_names[effect], what, vol);
}

static double now_ms()
{
   struct timespec ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/*--- volumes -------------------------------------------------------------*/

static const char *const volume_keys[2] = { "music_volume", "effects_volume" };

static void apply_volumes()
{
   if (!is_open)
      return;
   /* square law: the steps sound even to the ear */
   ma_sound_group_set_volume(&music_group, (float)pow(volume[sound_music] / 100.0, 2));
   ma_sound_group_set_volume(&effects_group, (float)pow(volume[sound_effects] / 100.0, 2));
   ma_engine_set_volume(&engine, muted ? 0.0f : 1.0f);
}

static void load_volumes()
{
   for (int c = 0; c < 2; c++)
   {
      const char *v = settings_get(volume_keys[c]);

      if (v != NULL)
         volume[c] = std::max(0, std::min(100, atoi(v)));
   }
}

void sound_step_volume(int channel, int direction)
{
   char v[16];

   volume[channel] = std::max(0, std::min(100, volume[channel] + 10 * direction));
   snprintf(v, sizeof(v), "%d", volume[channel]);
   settings_set(volume_keys[channel], v);
   apply_volumes();
   if (channel == sound_effects)
      sfx_ui(sfx_click);
}

const char *sound_label(int channel)
{
   static char s[2][32];
   const char *what = channel == sound_music ? "music" : "effects";

   if (!is_open)
      snprintf(s[channel], sizeof(s[channel]), "%s -", what);
   else if (volume[channel] == 0)
      snprintf(s[channel], sizeof(s[channel]), "%s off", what);
   else
      snprintf(s[channel], sizeof(s[channel]), "%s %d%%", what, volume[channel]);
   return s[channel];
}

bool sound_toggle_mute()
{
   muted = !muted;
   apply_volumes();
   return muted;
}

/*--- finding files -------------------------------------------------------*/

static bool is_music_file(const char name[])
{
   const char *dot = strrchr(name, '.');
   char ext[8];
   int i;

   if (dot == NULL || strlen(dot) > 5)
      return false;
   for (i = 0; dot[i + 1] != 0; i++)
      ext[i] = (char)tolower((unsigned char)dot[i + 1]);
   ext[i] = 0;
   return strcmp(ext, "ogg") == 0 || strcmp(ext, "mp3") == 0 || strcmp(ext, "flac") == 0
          || strcmp(ext, "wav") == 0;
}

/* "menu.ogg" -> "menu" */
static std::string stem(const char name[])
{
   std::string s(name);
   size_t dot = s.rfind('.');

   s = s.substr(0, dot);
   for (char &c : s)
      c = (char)tolower((unsigned char)c);
   return s;
}

static void scan_music_dir(const std::string &dir)
{
   DIR *d = opendir(dir.c_str());
   struct dirent *e;
   std::vector<std::string> found;

   if (d == NULL)
      return;
   while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.' && is_music_file(e->d_name))
         found.push_back(e->d_name);
   closedir(d);
   std::sort(found.begin(), found.end());
   for (const std::string &f : found)
   {
      std::string path = dir + "/" + f;
      std::string s = stem(f.c_str());

      if (s == "menu")
      {
         if (menu_track.empty())
            menu_track = path;
      }
      else if (s == "win")
      {
         if (win_track.empty())
            win_track = path;
      }
      else if (s == "lose")
      {
         if (lose_track.empty())
            lose_track = path;
      }
      else
         playlist.push_back(path);
   }
}

static void find_music()
{
   const char *xdg = getenv("XDG_DATA_HOME");
   const char *home = getenv("HOME");

   /* the player's own folder first, so its menu.* etc. win */
   if (xdg != NULL && xdg[0] == '/')
      scan_music_dir(std::string(xdg) + "/craft/music");
   else if (home != NULL)
      scan_music_dir(std::string(home) + "/.local/share/craft/music");
   scan_music_dir(craft_data("music"));
}

/*--- open and close ------------------------------------------------------*/

void sound_open()
{
   const char *env = getenv("CRAFT_SOUND");
   const char *record = getenv("CRAFT_SOUND_RECORD");
   ma_engine_config cfg;

   if (is_open || (env != NULL && strcmp(env, "off") == 0))
      return;
   load_volumes();
   cfg = ma_engine_config_init();
   if (record != NULL && record[0] != 0)
   {
      cfg.noDevice = MA_TRUE;
      cfg.channels = 2;
      cfg.sampleRate = 44100;
   }
   if (ma_engine_init(&cfg, &engine) != MA_SUCCESS)
   {
      fprintf(stderr, "craft: no sound device found; playing without sound\n");
      return;
   }
   if (ma_sound_group_init(&engine, 0, NULL, &music_group) != MA_SUCCESS
       || ma_sound_group_init(&engine, 0, NULL, &effects_group) != MA_SUCCESS)
   {
      ma_engine_uninit(&engine);
      fprintf(stderr, "craft: cannot set up sound; playing without sound\n");
      return;
   }
   if (record != NULL && record[0] != 0)
   {
      ma_encoder_config ec
          = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);

      if (ma_encoder_init_file(record, &ec, &recorder) == MA_SUCCESS)
      {
         recording = true;
         record_last = now_ms();
      }
      else
         fprintf(stderr, "craft: cannot record sound to %s\n", record);
   }
   is_open = true;
   for (int i = 0; i < num_sound_effects; i++)
   {
      char rel[64];

      snprintf(rel, sizeof(rel), "sounds/%s.wav", effect_names[i]);
      effect_loaded[i]
          = ma_sound_init_from_file(&engine, craft_data(rel),
                                    MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                    &effects_group, NULL, &effect_proto[i])
            == MA_SUCCESS;
      effect_last[i] = -1e9;
   }
   for (int p = 0; p < max_players; p++)
      alert_last[p] = -1e9;
   find_music();
   apply_volumes();
   atexit(sound_close); /* however the game ends */
}

static void free_track(ma_sound *&s)
{
   if (s != NULL)
   {
      ma_sound_uninit(s);
      delete s;
      s = NULL;
   }
}

void sound_close()
{
   if (!is_open)
      return;
   for (int i = 0; i < max_voices; i++)
      if (voices[i].used)
      {
         ma_sound_uninit(&voices[i].s);
         voices[i].used = false;
      }
   for (int i = 0; i < num_sound_effects; i++)
      if (effect_loaded[i])
         ma_sound_uninit(&effect_proto[i]);
   free_track(music_now);
   free_track(music_fading);
   ma_sound_group_uninit(&music_group);
   ma_sound_group_uninit(&effects_group);
   if (recording)
   {
      sound_tick(); /* the last few milliseconds */
      ma_encoder_uninit(&recorder);
      recording = false;
   }
   is_open = false; /* before uninit: sound_tick must not run on it */
   ma_engine_uninit(&engine);
}

/*--- music ---------------------------------------------------------------*/

/* Fade out what is playing and start path (empty: just fade out). */
static void play_track(const std::string &path, bool loop)
{
   if (music_now != NULL)
   {
      free_track(music_fading);
      ma_sound_stop_with_fade_in_milliseconds(music_now, 1200);
      music_fading = music_now;
      music_now = NULL;
   }
   if (path.empty())
      return;
   music_now = new ma_sound;
   if (ma_sound_init_from_file(&engine, path.c_str(),
                               MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, &music_group,
                               NULL, music_now)
       != MA_SUCCESS)
   {
      fprintf(stderr, "craft: cannot play %s\n", path.c_str());
      delete music_now;
      music_now = NULL;
      return;
   }
   ma_sound_set_looping(music_now, loop ? MA_TRUE : MA_FALSE);
   ma_sound_set_fade_in_milliseconds(music_now, 0.0f, 1.0f, 1500);
   ma_sound_start(music_now);
}

static void next_in_playlist()
{
   if (playlist.empty())
   {
      play_track("", false);
      return;
   }
   if (playlist_next == 0)
   { // shuffle, but never repeat the last track first
      std::string last = playlist.back();

      for (size_t i = playlist.size() - 1; i > 0; i--)
         std::swap(playlist[i], playlist[(size_t)rand() % (i + 1)]);
      if (playlist.size() > 1 && playlist[0] == last)
         std::swap(playlist[0], playlist[1]);
   }
   play_track(playlist[playlist_next], playlist.size() == 1);
   playlist_next = (playlist_next + 1) % playlist.size();
}

void music_menu()
{
   game_running = false;
   if (!is_open || mode == mode_menu)
      return;
   if (mode == mode_over && music_now != NULL && !ma_sound_at_end(music_now))
   { // let the win/lose cue finish first
      menu_after_cue = true;
      return;
   }
   menu_after_cue = false;
   mode = mode_menu;
   play_track(menu_track, true);
}

void music_game()
{
   game_running = true;
   if (!is_open)
      return;
   mode = mode_game;
   playlist_next = 0;
   next_in_playlist();
}

void music_game_over(bool won)
{
   game_running = false;
   if (!is_open)
      return;
   mode = mode_over;
   play_track(won ? win_track : lose_track, false);
}

/*--- tick ----------------------------------------------------------------*/

void sound_tick()
{
   if (!is_open)
      return;
   if (music_fading != NULL && !ma_sound_is_playing(music_fading))
      free_track(music_fading);
   if (mode == mode_game && music_now != NULL && ma_sound_at_end(music_now))
      next_in_playlist();
   if (menu_after_cue && (music_now == NULL || ma_sound_at_end(music_now)))
   {
      menu_after_cue = false;
      mode = mode_none;
      music_menu();
   }
   for (int i = 0; i < max_voices; i++)
      if (voices[i].used && !ma_sound_is_playing(&voices[i].s))
      {
         ma_sound_uninit(&voices[i].s);
         voices[i].used = false;
      }
   if (recording)
   { // record what would have been played since the last tick
      double t = now_ms();
      ma_uint64 frames = (ma_uint64)((t - record_last) * 44.1);
      float buf[2 * 1024];

      record_last += frames / 44.1;
      while (frames > 0)
      {
         ma_uint64 n = std::min<ma_uint64>(frames, 1024);
         ma_uint64 got = 0;

         ma_engine_read_pcm_frames(&engine, buf, n, &got);
         if (got == 0)
            break;
         ma_encoder_write_pcm_frames(&recorder, buf, got, NULL);
         frames -= got;
      }
   }
}

/*--- effects -------------------------------------------------------------*/

static void play_effect(int effect, double vol, double pan)
{
   double t = now_ms();
   int same = 0;
   int free_v = -1;

   if (!is_open || effect < 0 || effect >= num_sound_effects || !effect_loaded[effect])
      return;
   if (t - effect_last[effect] < effect_gap[effect])
   {
      debug_log(effect, "too soon", vol);
      return;
   }
   for (int i = 0; i < max_voices; i++)
   {
      if (voices[i].used && !ma_sound_is_playing(&voices[i].s))
      {
         ma_sound_uninit(&voices[i].s);
         voices[i].used = false;
      }
      if (voices[i].used && voices[i].effect == effect)
         same++;
      if (!voices[i].used && free_v < 0)
         free_v = i;
   }
   if (same >= effect_max[effect] || free_v < 0)
   {
      debug_log(effect, "too many", vol);
      return;
   }
   if (ma_sound_init_copy(&engine, &effect_proto[effect], MA_SOUND_FLAG_NO_SPATIALIZATION,
                          &effects_group, &voices[free_v].s)
       != MA_SUCCESS)
      return;
   voices[free_v].used = true;
   voices[free_v].effect = effect;
   effect_last[effect] = t;
   ma_sound_set_volume(&voices[free_v].s, (float)vol);
   ma_sound_set_pan(&voices[free_v].s, (float)pan);
   /* a little pitch variety, so repeats don't sound mechanical */
   if (effect != sfx_click && effect != sfx_ack)
      ma_sound_set_pitch(&voices[free_v].s, (float)(0.94 + 0.12 * (rand() / (double)RAND_MAX)));
   ma_sound_start(&voices[free_v].s);
   debug_log(effect, "played", vol);
}

static bool is_listener(int p)
{
   return active[p] && players[p] != NULL && !players[p]->is_robot;
}

void sfx_at(int effect, int x, int y)
{
   double best = 0;
   double pan = 0;

   if (!is_open || !game_running || x < 0 || y < 0 || x >= landscape_dx || y >= landscape_dy)
      return;
   for (int p = 0; p < max_players; p++)
      if (is_listener(p) && (!is_suny || players[p]->sun_cnt[x][y] > 0))
      {
         player *pl = players[p];
         double half_w = pl->wdx / pic_dx / 2.0;
         double half_h = pl->wdy / pic_dy / 2.0;
         double dx = x + 0.5 - (pl->x0 + half_w);
         double dy = y + 0.5 - (pl->y0 + half_h);
         /* squares outside the view */
         double out = std::max(0.0, std::max(fabs(dx) - half_w, fabs(dy) - half_h));
         double vol = 1.0 / (1.0 + out / 5.0);

         if (vol > best)
         {
            best = vol;
            pan = std::max(-1.0, std::min(1.0, dx / (half_w + out))) * 0.7;
         }
      }
   if (best >= 0.15)
      play_effect(effect, best, pan);
   else
      debug_log(effect, best > 0 ? "too far" : "not seen", best);
}

void sfx_for(int effect, int color)
{
   int p;

   if (!is_open || !game_running || color < 0 || color >= max_cols)
      return;
   p = color_player[color];
   if (p < 0 || p >= max_players || !is_listener(p))
      return;
   if (effect == sfx_alert)
   { // at most one alert per player every few seconds
      double t = now_ms();

      if (t - alert_last[p] < effect_gap[sfx_alert])
         return;
      alert_last[p] = t;
      effect_last[sfx_alert] = -1e9; /* the per-player gap above decides */
   }
   play_effect(effect, effect == sfx_alert ? 0.8 : 0.7, 0);
}

void sfx_ui(int effect)
{
   play_effect(effect, 0.8, 0);
}
