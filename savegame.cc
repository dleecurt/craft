#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "savegame.h"
#include "craft.h"
#include "craft_def.h"
#include "object_handler.h"
#include "player.h"
#include "robot.h"
#include "ship.h"
#include "field.h"
#include "ilfield.h"
#include "masks.h"
#include "xfile.h"

/* Game state defined in craft.cc and land.cc that craft.h doesn't declare */
extern int num_humans;
extern double p_speed[max_players];
extern int overall_money;
extern int num_il;
extern int x_il[15];
extern int y_il[15];
extern int xx_il[15][2000];
extern int yy_il[15][2000];
extern int ss[15];

bool restoring = false;
char restore_name[128] = "";

#define save_magic "CRAFTSAV"
#define save_version 1
#define save_suffix ".craftsave"
#define num_sizes 12

struct save_header
{
   char magic[8];
   int32_t version;
   int32_t sizes[num_sizes];
   int64_t saved_at;
   char label[128];
};

static gzFile restore_file = NULL;

/*----------------------------------------------------------------------*/
/* Files and names                                                      */
/*----------------------------------------------------------------------*/

static void make_dirs(const char path[])
{
   char p[1024];

   snprintf(p, sizeof(p), "%s", path);
   for (char *s = p + 1; *s != 0; s++)
      if (*s == '/')
      {
         *s = 0;
         mkdir(p, 0755);
         *s = '/';
      }
   mkdir(p, 0755);
}

const char *saves_dir()
{
   static char dir[1024] = "";

   if (dir[0] == 0)
   {
      const char *xdg = getenv("XDG_DATA_HOME");
      const char *home = getenv("HOME");

      if (xdg != NULL && xdg[0] == '/')
         snprintf(dir, sizeof(dir), "%s/craft/saves", xdg);
      else
         snprintf(dir, sizeof(dir), "%s/.local/share/craft/saves", home != NULL ? home : ".");
      make_dirs(dir);
   }
   return dir;
}

/* Keep save names to characters that are safe in file names and in the
   game's menus (which use ':' and ';' as separators). */
static void clean_name(const char in[], char out[128])
{
   int n = 0;

   for (int i = 0; in[i] != 0 && n < 60; i++)
   {
      char c = in[i];

      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' '
          || c == '-' || c == '_' || c == '.')
         out[n++] = c;
      else
         out[n++] = '_';
   }
   while (n > 0 && out[n - 1] == ' ')
      n--;
   out[n] = 0;
   if (out[0] == '.')
      out[0] = '_';
}

static void save_path(const char save_name[], char path[1100])
{
   if (snprintf(path, 1100, "%s/%s%s", saves_dir(), save_name, save_suffix) >= 1100)
      path[0] = 0; /* too long: fails to open instead of using a cut-off name */
}

static void fill_sizes(int32_t s[num_sizes])
{
   s[0] = sizeof(object_handler);
   s[1] = sizeof(player);
   s[2] = sizeof(robot);
   s[3] = sizeof(ship);
   s[4] = sizeof(field);
   s[5] = sizeof(ilfield);
   s[6] = max_objects;
   s[7] = max_land_dx;
   s[8] = max_land_dy;
   s[9] = max_players;
   s[10] = max_pics;
   s[11] = sizeof(void *);
}

/*----------------------------------------------------------------------*/
/* Tagged blocks: 4-byte tag, 8-byte size, data                        */
/*----------------------------------------------------------------------*/

static bool put(gzFile f, const char tag[5], const void *p, size_t n)
{
   uint64_t n64 = n;

   return gzwrite(f, tag, 4) == 4 && gzwrite(f, &n64, 8) == 8
          && (n == 0 || gzwrite(f, p, (unsigned)n) == (int)n);
}

static bool get(gzFile f, const char tag[5], void *p, size_t n)
{
   char t[4];
   uint64_t n64;

   if (gzread(f, t, 4) != 4 || memcmp(t, tag, 4) != 0)
   {
      fprintf(stderr, "craft: save file: expected block '%s'\n", tag);
      return false;
   }
   if (gzread(f, &n64, 8) != 8 || n64 != n)
   {
      fprintf(stderr, "craft: save file: block '%s' has the wrong size\n", tag);
      return false;
   }
   if (n > 0 && gzread(f, p, (unsigned)n) != (int)n)
   {
      fprintf(stderr, "craft: save file: block '%s' is truncated\n", tag);
      return false;
   }
   return true;
}

static bool io(gzFile f, bool writing, const char tag[5], void *p, size_t n)
{
   return writing ? put(f, tag, p, n) : get(f, tag, p, n);
}

#define IO(tag, var)                                                                               \
   if (!io(f, writing, tag, (void *)&(var), sizeof(var)))                                          \
   return false

/* Options and landscape: everything init_craft() needs before it
   creates the players. */
static bool globals_io(gzFile f, bool writing)
{
   IO("actv", active);
   IO("host", host);
   IO("name", name);
   IO("self", is_self);
   IO("suny", is_suny);
   IO("watr", is_water_world);
   IO("sped", speed);
   IO("pspd", p_speed);
   IO("s_st", speed_stag);
   IO("s_wo", speed_worker);
   IO("s_tr", speed_trader);
   IO("s_kn", speed_knight);
   IO("s_pa", speed_pawn);
   IO("s_sc", speed_scout);
   IO("s_ar", speed_archer);
   IO("s_ho", speed_home);
   IO("s_fa", speed_farm);
   IO("s_fe", speed_farm_exec);
   IO("s_zo", speed_zombi);
   IO("s_sh", speed_schrott);
   IO("s_sw", speed_swim);
   IO("s_wa", speed_water);
   IO("s_hi", speed_hit);
   IO("s_bz", speed_building_zombi);
   IO("s_ah", speed_archer_hit);
   IO("s_ca", speed_cata);
   IO("s_cl", speed_cata_load);
   IO("s_ex", speed_explosion);
   IO("s_do", speed_doktor);
   IO("s_dh", speed_doktor_hit);
   IO("s_ge", speed_general);
   IO("s_tp", speed_trap);
   IO("s_aw", speed_arrow);
   IO("s_so", speed_stone);
   IO("s_s1", speed_ship1);
   IO("s_s2", speed_ship2);
   IO("s_sz", speed_ship_zombi);
   IO("rpow", robo_power);
   IO("ragr", robo_agress);
   IO("qstp", q_step);
   IO("omon", overall_money);
   IO("rlx0", min_rland_dx);
   IO("rlx1", max_rland_dx);
   IO("rly0", min_rland_dy);
   IO("rly1", max_rland_dy);
   IO("lscl", landscale);
   IO("ldx ", landscape_dx);
   IO("ldy ", landscape_dy);
   IO("lnam", land_name);
   IO("land", landscape);
   IO("lhgt", landhight);
   IO("lpic", landpic);
   IO("lovl", landoverlay);
   IO("unit", unit);
   IO("upic", upic);
   IO("ilid", ilid);
   IO("lprp", land_properties);
   IO("nil ", num_il);
   IO("xil ", x_il);
   IO("yil ", y_il);
   IO("xxil", xx_il);
   IO("yyil", yy_il);
   IO("ilsz", ss);
   return true;
}

/*----------------------------------------------------------------------*/
/* Objects with pointers: save the raw object, and on restore keep the  */
/* live pointers (windows, buttons, pictures, sub-objects).             */
/*----------------------------------------------------------------------*/

#define KEEP(live, old, member)                                                                    \
   memcpy((void *)&(live)->member, (void *)&(old)->member, sizeof((live)->member))

static bool read_into(gzFile f, const char tag[5], void *obj, size_t n)
{
   return get(f, tag, obj, n);
}

static bool restore_field(gzFile f, const char tag[5], field *live)
{
   field *old = (field *)malloc(sizeof(field));
   bool ok;

   memcpy((void *)old, (void *)live, sizeof(field));
   ok = read_into(f, tag, live, sizeof(field));
   KEEP(live, old, w);
   KEEP(live, old, w_is_open);
   free(old);
   return ok;
}

static bool restore_robot(gzFile f, robot *live)
{
   robot *old = (robot *)malloc(sizeof(robot));
   int32_t has_ilf = 0;
   bool ok;

   memcpy((void *)old, (void *)live, sizeof(robot));
   ok = read_into(f, "robt", live, sizeof(robot));
   KEEP(live, old, ilf);
   KEEP(live, old, f);
   KEEP(live, old, pf);
   KEEP(live, old, ppf);
   free(old);
   ok = ok && restore_field(f, "rf  ", live->f) && restore_field(f, "rpf ", live->pf)
        && restore_field(f, "rppf", live->ppf) && get(f, "hilf", &has_ilf, sizeof(has_ilf));
   if (ok && has_ilf)
   {
      if (live->ilf == NULL)
         live->ilf = new ilfield();
      ok = get(f, "ilf ", live->ilf, sizeof(ilfield));
   }
   return ok;
}

static bool restore_player(gzFile f, player *live)
{
   player *old = (player *)malloc(sizeof(player));
   int32_t has_robot = 0;
   bool ok;

   memcpy((void *)old, (void *)live, sizeof(player));
   ok = read_into(f, "plyr", live, sizeof(player));
   /* windows, buttons, dials, pictures and the command panel */
   KEEP(live, old, w_craft);
   KEEP(live, old, w_land);
   KEEP(live, old, w_overview);
   KEEP(live, old, w_status);
   KEEP(live, old, w_admin);
   KEEP(live, old, w_inform);
   KEEP(live, old, w_pause);
   KEEP(live, old, w_unit);
   KEEP(live, old, w_msg);
   KEEP(live, old, ct);
   KEEP(live, old, g_msg);
   KEEP(live, old, pic_used);
   KEEP(live, old, pics);
   KEEP(live, old, ship_pics);
   KEEP(live, old, debug_on);
   KEEP(live, old, is_debug);
   KEEP(live, old, b_debug);
   KEEP(live, old, b_quit);
   KEEP(live, old, b_pause);
   KEEP(live, old, b_pause_down);
   KEEP(live, old, b_save);
   KEEP(live, old, b_unit_s);
   KEEP(live, old, b_unit_l);
   KEEP(live, old, cmds);
   KEEP(live, old, cmd_code);
   KEEP(live, old, cmd_active);
   KEEP(live, old, cmd_char);
   KEEP(live, old, num_cmds);
   KEEP(live, old, bname);
   KEEP(live, old, running_cmd);
   KEEP(live, old, w_limit);
   KEEP(live, old, m_limit);
   KEEP(live, old, limit_zero);
   KEEP(live, old, is_dial);
   KEEP(live, old, rob);
   KEEP(live, old, p_pressed);
   KEEP(live, old, p_button);
   KEEP(live, old, host);
   KEEP(live, old, name);
   KEEP(live, old, wdx);
   KEEP(live, old, wdy);
   free(old);
   { // start_with_no_selection_and_a_fresh_status_panel
      for (int k = 0; k < 5; k++)
      {
         live->num_marked[k] = 0;
         live->num_marked_ships[k] = 0;
      }
      live->last_num_mans = -1;
      live->last_money = -1;
      live->last_wood = -1;
      live->last_food = -1000;
   }
   ok = ok && get(f, "hrob", &has_robot, sizeof(has_robot));
   if (ok && has_robot)
   {
      if (live->rob == NULL)
      {
         fprintf(stderr, "craft: save file: a computer player became human\n");
         return false;
      }
      ok = restore_robot(f, live->rob);
   }
   return ok;
}

/*----------------------------------------------------------------------*/
/* Random generator (drand48)                                           */
/*----------------------------------------------------------------------*/

static void get_rng(unsigned short state[3])
{
   unsigned short tmp[3] = { 0, 0, 0 };
   unsigned short *old = seed48(tmp);

   memcpy(state, old, sizeof(unsigned short) * 3);
   seed48(state);
}

/*----------------------------------------------------------------------*/
/* Save                                                                 */
/*----------------------------------------------------------------------*/

static bool write_all(gzFile f, const char label[])
{
   save_header h;
   bool writing = true;

   memset(&h, 0, sizeof(h));
   memcpy(h.magic, save_magic, 8);
   h.version = save_version;
   fill_sizes(h.sizes);
   h.saved_at = (int64_t)time(NULL);
   snprintf(h.label, sizeof(h.label), "%s", label);
   if (gzwrite(f, &h, sizeof(h)) != (int)sizeof(h))
      return false;
   if (!globals_io(f, writing))
      return false;
   IO("tick", ticker);
   IO("olsc", olandscale);
   if (!put(f, "objs", objects, sizeof(object_handler)))
      return false;
   for (int i = 0; i < max_objects; i++)
      if (objects->s[i] != NULL && !objects->is_free[i])
      {
         int32_t idx = i;

         if (!put(f, "shpi", &idx, sizeof(idx)) || !put(f, "ship", objects->s[i], sizeof(ship)))
            return false;
      }
   {
      int32_t end = -1;

      if (!put(f, "shpi", &end, sizeof(end)))
         return false;
   }
   for (int i = 0; i < max_players; i++)
      if (active[i])
      {
         player *p = players[i];
         int32_t has_robot = (p->rob != NULL);

         if (!put(f, "plyr", p, sizeof(player)) || !put(f, "hrob", &has_robot, sizeof(has_robot)))
            return false;
         if (has_robot)
         {
            robot *r = p->rob;
            int32_t has_ilf = (r->ilf != NULL);

            if (!put(f, "robt", r, sizeof(robot)) || !put(f, "rf  ", r->f, sizeof(field))
                || !put(f, "rpf ", r->pf, sizeof(field)) || !put(f, "rppf", r->ppf, sizeof(field))
                || !put(f, "hilf", &has_ilf, sizeof(has_ilf)))
               return false;
            if (has_ilf && !put(f, "ilf ", r->ilf, sizeof(ilfield)))
               return false;
         }
      }
   {
      unsigned short rng[3];

      get_rng(rng);
      if (!put(f, "rng ", rng, sizeof(rng)) || !put(f, "end ", NULL, 0))
         return false;
   }
   return true;
}

bool save_game(const char save_name[], char error[256])
{
   char clean[128];
   char path[1100];
   char tmp_path[1200];
   gzFile f;
   bool ok;

   clean_name(save_name, clean);
   if (clean[0] == 0)
   {
      snprintf(error, 256, "Please give the save a name.");
      return false;
   }
   save_path(clean, path);
   snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
   f = gzopen(tmp_path, "wb6");
   if (f == NULL)
   {
      snprintf(error, 256, "Cannot write to the saves folder (%s)", strerror(errno));
      return false;
   }
   ok = write_all(f, clean);
   if (gzclose(f) != Z_OK)
      ok = false;
   if (!ok || rename(tmp_path, path) != 0)
   {
      unlink(tmp_path);
      snprintf(error, 256, "Saving failed (disk full?)");
      return false;
   }
   return true;
}

/*----------------------------------------------------------------------*/
/* Restore                                                              */
/*----------------------------------------------------------------------*/

static bool read_header(gzFile f, save_header &h)
{
   int32_t sizes[num_sizes];

   if (gzread(f, &h, sizeof(h)) != (int)sizeof(h) || memcmp(h.magic, save_magic, 8) != 0)
      return false;
   fill_sizes(sizes);
   return h.version == save_version && memcmp(h.sizes, sizes, sizeof(sizes)) == 0;
}

bool restore_game_globals(const char save_name[])
{
   char path[1100];
   save_header h;
   bool writing = false;

   save_path(save_name, path);
   restore_file = gzopen(path, "rb");
   if (restore_file == NULL)
   {
      fprintf(stderr, "craft: cannot open saved game %s\n", path);
      return false;
   }
   if (!read_header(restore_file, h))
   {
      fprintf(stderr, "craft: %s was saved by a different version of Craft\n", path);
      gzclose(restore_file);
      restore_file = NULL;
      return false;
   }
   {
      gzFile f = restore_file;

      if (!globals_io(f, writing))
      {
         gzclose(restore_file);
         restore_file = NULL;
         return false;
      }
   }
   return true;
}

bool restore_game_objects()
{
   gzFile f = restore_file;
   bool writing = false;
   bool ok = true;

   if (f == NULL)
      return false;
   IO("tick", ticker);
   IO("olsc", olandscale);
   { // objects: keep the live ship pointers out of the raw copy
      if (!get(f, "objs", objects, sizeof(object_handler)))
         return false;
      for (int i = 0; i < max_objects; i++)
      {
         objects->s[i] = NULL;
         objects->rob[i] = NULL;
         objects->is_marked[i] = false;
      }
   }
   for (;;)
   { // ships
      int32_t idx;

      if (!get(f, "shpi", &idx, sizeof(idx)))
         return false;
      if (idx < 0)
         break;
      if (idx >= max_objects)
         return false;
      objects->s[idx] = new ship(idx);
      if (!get(f, "ship", objects->s[idx], sizeof(ship)))
         return false;
   }
   for (int i = 0; i < max_players && ok; i++)
      if (active[i])
         ok = restore_player(f, players[i]);
   if (ok)
   { // random generator, so the game continues exactly as it would have
      unsigned short rng[3];

      ok = get(f, "rng ", rng, sizeof(rng)) && get(f, "end ", NULL, 0);
      if (ok)
         seed48(rng);
   }
   gzclose(restore_file);
   restore_file = NULL;
   if (ok)
      for (int i = 0; i < max_players; i++)
         if (active[i])
            players[i]->initial_display();
   return ok;
}

/*----------------------------------------------------------------------*/
/* Listing saves                                                        */
/*----------------------------------------------------------------------*/

int list_saves(char names[][128], char labels[][160], int max)
{
   DIR *d = opendir(saves_dir());
   struct dirent *e;
   int n = 0;
   int64_t when[max_saves * 4];
   int cap = max < max_saves * 4 ? max : max_saves * 4;

   if (d == NULL)
      return 0;
   while ((e = readdir(d)) != NULL && n < cap)
   {
      size_t l = strlen(e->d_name);
      size_t sl = strlen(save_suffix);
      char path[1100];
      gzFile f;
      save_header h;

      if (l <= sl || strcmp(e->d_name + l - sl, save_suffix) != 0 || l - sl >= 128)
         continue;
      snprintf(path, sizeof(path), "%s/%s", saves_dir(), e->d_name);
      f = gzopen(path, "rb");
      if (f == NULL)
         continue;
      if (read_header(f, h))
      {
         char date[64];
         time_t t = (time_t)h.saved_at;

         memcpy(names[n], e->d_name, l - sl);
         names[n][l - sl] = 0;
         /* no ':' or ';' here: the game's menus use them as separators */
         strftime(date, sizeof(date), "%Y-%m-%d %H.%M", localtime(&t));
         snprintf(labels[n], 160, "%s   (%s)", names[n], date);
         when[n] = h.saved_at;
         n++;
      }
      gzclose(f);
   }
   closedir(d);
   for (int i = 1; i < n; i++) // newest first
      for (int j = i; j > 0 && when[j] > when[j - 1]; j--)
      {
         char tn[128];
         char tl[160];
         int64_t tw = when[j];

         memcpy(tn, names[j], 128);
         memcpy(names[j], names[j - 1], 128);
         memcpy(names[j - 1], tn, 128);
         memcpy(tl, labels[j], 160);
         memcpy(labels[j], labels[j - 1], 160);
         memcpy(labels[j - 1], tl, 160);
         when[j] = when[j - 1];
         when[j - 1] = tw;
      }
   return n;
}

/*----------------------------------------------------------------------*/
/* Dialogs                                                              */
/*----------------------------------------------------------------------*/

void save_game_dialog()
{
   char title[] = "craft:save_game"; /* no spaces: window names are stored in .winfix */
   char s[256];
   char clean[128];
   char path[1100];
   char error[256];
   char msg[300];
   time_t now = time(NULL);

   strftime(s, sizeof(s), "%Y-%m-%d %H.%M", localtime(&now));
   if (!get_line(s, title, 300, false))
      return;
   clean_name(s, clean);
   if (clean[0] == 0)
      return;
   save_path(clean, path);
   if (f_exists(path) && !yes("A save with this name exists. Replace it?"))
      return;
   if (save_game(clean, error))
      snprintf(msg, sizeof(msg), "Game saved as \"%s\"", clean);
   else
      snprintf(msg, sizeof(msg), "%s", error);
   ack(msg);
}

bool choose_saved_game(char save_name[128])
{
   static char names[max_saves][128];
   static char labels[max_saves][160];
   char menu_string[max_saves * 170 + 16];
   char title[] = "craft:restore_game";
   int n = list_saves(names, labels, max_saves);
   int choice;

   if (n == 0)
   {
      char msg[] = "There are no saved games yet.";

      ack(msg);
      return false;
   }
   menu_string[0] = 0;
   for (int i = 0; i < n; i++)
   {
      strcat(menu_string, labels[i]);
      strcat(menu_string, ":");
   }
   choice = select(menu_string, title);
   if (choice < 0 || choice >= n)
      return false;
   snprintf(save_name, 128, "%s", names[choice]);
   return true;
}
