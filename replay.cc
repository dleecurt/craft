/*
 * Replays: recording and playing back games.  See replay.h.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "replay.h"
#include "craft_def.h"
#include "savegame.h"
#include "statehash.h"

bool replay_playing = false;

static const char replay_magic[8] = { 'C', 'R', 'A', 'F', 'T', 'R', 'P', 'L' };
static const int32_t replay_version = 2; /* 2: give up is an order */

/*----------------------------------------------------------------------*/
/* Low-level records                                                    */
/*----------------------------------------------------------------------*/

static bool w32(gzFile f, int32_t v)
{
   return gzwrite(f, &v, sizeof(v)) == (int)sizeof(v);
}

static bool r32(gzFile f, int32_t &v)
{
   return gzread(f, &v, sizeof(v)) == (int)sizeof(v);
}

static bool w64(gzFile f, uint64_t v)
{
   return gzwrite(f, &v, sizeof(v)) == (int)sizeof(v);
}

static bool r64(gzFile f, uint64_t &v)
{
   return gzread(f, &v, sizeof(v)) == (int)sizeof(v);
}

static bool wtag(gzFile f, const char tag[5])
{
   return gzwrite(f, tag, 4) == 4;
}

/*----------------------------------------------------------------------*/
/* Recording                                                            */
/*----------------------------------------------------------------------*/

static gzFile rec = NULL;
static bool rec_failed = false;

static void rec_fail()
{
   if (!rec_failed)
      fprintf(stderr, "craft: writing the replay failed; this game will not be recorded\n");
   rec_failed = true;
}

const char *replay_default_path()
{
   static char path[1100] = "";

   if (path[0] == 0)
   {
      const char *xdg = getenv("XDG_DATA_HOME");
      const char *home = getenv("HOME");
      char dir[1024];

      if (xdg != NULL && xdg[0] == '/')
         snprintf(dir, sizeof(dir), "%s/craft/replays", xdg);
      else
         snprintf(dir, sizeof(dir), "%s/.local/share/craft/replays", home != NULL ? home : ".");
      make_dirs(dir);
      snprintf(path, sizeof(path), "%s/last.craftreplay", dir);
   }
   return path;
}

bool replay_record_start(const char path[])
{
   replay_record_close();
   rec_failed = false;
   /* Level 1: orders are small, and the snapshot is written once. */
   rec = gzopen(path, "wb1");
   if (rec == NULL)
   {
      fprintf(stderr, "craft: cannot record the replay to %s (%s)\n", path, strerror(errno));
      return false;
   }
   if (gzwrite(rec, replay_magic, 8) != 8 || !w32(rec, replay_version) || !save_game_into(rec))
   {
      rec_fail();
      gzclose(rec);
      rec = NULL;
      return false;
   }
   atexit(replay_record_close);
   return true;
}

void replay_record_orders(int step, int player, int n, const int id[], const int cmd[],
                          const int x[], const int y[])
{
   bool ok;

   if (rec == NULL || rec_failed || n <= 0)
      return;
   ok = wtag(rec, "ordr") && w32(rec, step) && w32(rec, player) && w32(rec, n);
   for (int i = 0; ok && i < n; i++)
      ok = w32(rec, id[i]) && w32(rec, cmd[i]) && w32(rec, x[i]) && w32(rec, y[i]);
   if (!ok)
      rec_fail();
}

void replay_record_close()
{
   extern int ticker;

   if (rec == NULL)
      return;
   if (!rec_failed && !(wtag(rec, "end ") && w32(rec, ticker) && w64(rec, game_state_hash())))
      rec_fail();
   gzclose(rec);
   rec = NULL;
}

/*----------------------------------------------------------------------*/
/* Playback                                                             */
/*----------------------------------------------------------------------*/

static gzFile play = NULL;

/* One record of lookahead. */
static struct
{
   bool valid;
   bool eof;
   char tag[5];
   int32_t step;
   int32_t player;
   int32_t n;
   int32_t id[max_orders], cmd[max_orders], x[max_orders], y[max_orders];
   uint64_t hash;
} next;

static int mismatches = 0;

/* CRAFT_REPLAY_DUMP=DIR: write the state breakdown at every hash point,
   as DIR/rec-STEP.txt when recording and DIR/play-STEP.txt when playing
   back, so the two can be compared with diff. */
static uint64_t hash_now(const char mode[], int step)
{
   const char *dir = getenv("CRAFT_REPLAY_DUMP");
   char path[1100];
   FILE *out;
   uint64_t h;

   if (dir == NULL)
      return game_state_hash();
   snprintf(path, sizeof(path), "%s/%s-%06d.txt", dir, mode, step);
   out = fopen(path, "w");
   h = game_state_hash_dump(out);
   if (out != NULL)
      fclose(out);
   return h;
}
static int checked = 0;

static void read_next()
{
   bool ok;

   next.valid = false;
   memset(next.tag, 0, sizeof(next.tag));
   if (gzread(play, next.tag, 4) != 4)
   {
      next.eof = true;
      return;
   }
   ok = r32(play, next.step);
   if (strcmp(next.tag, "ordr") == 0)
   {
      ok = ok && r32(play, next.player) && r32(play, next.n) && next.n > 0 && next.n <= max_orders;
      for (int i = 0; ok && i < next.n; i++)
         ok = r32(play, next.id[i]) && r32(play, next.cmd[i]) && r32(play, next.x[i])
              && r32(play, next.y[i]);
   }
   else if (strcmp(next.tag, "hash") == 0 || strcmp(next.tag, "end ") == 0)
      ok = ok && r64(play, next.hash);
   else
      ok = false;
   if (!ok)
   {
      fprintf(stderr, "replay: the file is cut short or damaged after step %d\n", next.step);
      next.eof = true;
      return;
   }
   next.valid = true;
}

static void finish(int step, bool have_end)
{
   uint64_t h = game_state_hash();

   if (have_end && h != next.hash)
   {
      mismatches++;
      printf("replay: final state differs at step %d\n", step);
   }
   printf("replay: %s after %d steps, %d checks, final hash %016llx\n",
          mismatches == 0 ? "matched" : "DIVERGED", step, checked + (have_end ? 1 : 0),
          (unsigned long long)h);
   fflush(stdout);
   exit(mismatches == 0 ? 0 : 3);
}

bool replay_open(const char path[])
{
   char magic[8];
   int32_t version;

   play = gzopen(path, "rb");
   if (play == NULL)
   {
      fprintf(stderr, "craft: cannot open replay %s (%s)\n", path, strerror(errno));
      return false;
   }
   if (gzread(play, magic, 8) != 8 || memcmp(magic, replay_magic, 8) != 0)
   {
      fprintf(stderr, "craft: %s is not a Craft replay\n", path);
      return false;
   }
   if (!r32(play, version) || version != replay_version)
   {
      fprintf(stderr, "craft: %s was recorded by a different version of Craft\n", path);
      return false;
   }
   replay_playing = true;
   return restore_game_globals_from(play);
}

bool replay_open_objects()
{
   if (!restore_game_objects_from(play))
      return false;
   next.eof = false;
   read_next();
   return true;
}

void replay_playback_end(int step)
{
   if (play != NULL)
      finish(step, next.valid && strcmp(next.tag, "end ") == 0 && next.step == step);
}

void replay_take(int step, int player, int &num, int cmd[], int id[], int x[], int y[])
{
   num = 0;
   if (play == NULL)
      return;
   for (;;)
   {
      if (next.eof)
         finish(step, false);
      if (!next.valid)
         return;
      if (strcmp(next.tag, "end ") == 0 && next.step <= step)
         finish(step, true);
      if (next.step != step || next.player != player)
         return;
      if (strcmp(next.tag, "ordr") == 0)
      {
         for (int i = 0; i < next.n && num < max_orders; i++, num++)
         {
            id[num] = next.id[i];
            cmd[num] = next.cmd[i];
            x[num] = next.x[i];
            y[num] = next.y[i];
         }
      }
      else
         return;
      read_next();
   }
}

void replay_after_step(int step)
{
   if (rec != NULL && !rec_failed && step % replay_hash_every == 0)
   {
      if (!(wtag(rec, "hash") && w32(rec, step) && w64(rec, hash_now("rec", step))))
         rec_fail();
   }
   if (play != NULL && next.valid && strcmp(next.tag, "hash") == 0 && next.step == step)
   {
      uint64_t h = hash_now("play", step);

      checked++;
      if (h != next.hash)
      {
         mismatches++;
         if (mismatches <= 5)
            printf("replay: state differs at step %d (%016llx, recorded %016llx)\n", step,
                   (unsigned long long)h, (unsigned long long)next.hash);
      }
      else if (getenv("CRAFT_REPLAY_VERBOSE") != NULL)
         printf("replay: step %d ok\n", step);
      read_next();
   }
}
