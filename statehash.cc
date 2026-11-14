/*
 * Whole-game state hash.  See statehash.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "statehash.h"
#include "craft.h"
#include "object_handler.h"
#include "player.h"
#include "ship.h"

namespace
{

class fnv
{
 public:
   uint64_t h = 1469598103934665603ull;

   void bytes(const void *p, size_t n)
   {
      const unsigned char *c = (const unsigned char *)p;

      for (size_t i = 0; i < n; i++)
      {
         h ^= c[i];
         h *= 1099511628211ull;
      }
   }

   void i(int v)
   {
      int32_t w = v;
      bytes(&w, sizeof(w));
   }

   void d(double v)
   {
      /* Compare bit patterns: lockstep needs bit-identical results. */
      bytes(&v, sizeof(v));
   }

   /* Only the cells that are part of the map, row by row. */
   void land(int a[max_land_dx][max_land_dy])
   {
      for (int x = 0; x < landscape_dx; x++)
         bytes(a[x], sizeof(int) * landscape_dy);
   }
};

void hash_object(fnv &f, object_handler *o, int id, FILE *out = NULL)
{
   f.i(id);
   f.i(o->type[id]);
   if (out)
      fprintf(out, " type=%d", (int)o->type[id]);
   f.i(o->color[id]);
   if (out)
      fprintf(out, " color=%d", (int)o->color[id]);
   f.i(o->oid[id]);
   if (out)
      fprintf(out, " oid=%d", (int)o->oid[id]);
   f.i(o->on_ship[id]);
   if (out)
      fprintf(out, " on_ship=%d", (int)o->on_ship[id]);
   f.i(o->moving_id[id]);
   if (out)
      fprintf(out, " moving_id=%d", (int)o->moving_id[id]);
   f.i(o->trace[id]);
   if (out)
      fprintf(out, " trace=%d", (int)o->trace[id]);
   f.i(o->trapped[id]);
   if (out)
      fprintf(out, " trapped=%d", (int)o->trapped[id]);
   f.i(o->pic[id]);
   if (out)
      fprintf(out, " pic=%d", (int)o->pic[id]);
   f.i(o->x[id]);
   if (out)
      fprintf(out, " x=%d", (int)o->x[id]);
   f.i(o->y[id]);
   if (out)
      fprintf(out, " y=%d", (int)o->y[id]);
   f.i(o->wx[id]);
   if (out)
      fprintf(out, " wx=%d", (int)o->wx[id]);
   f.i(o->wy[id]);
   if (out)
      fprintf(out, " wy=%d", (int)o->wy[id]);
   f.i(o->home_id[id]);
   if (out)
      fprintf(out, " home_id=%d", (int)o->home_id[id]);
   f.i(o->moving_goal[id]);
   if (out)
      fprintf(out, " moving_goal=%d", (int)o->moving_goal[id]);
   f.i(o->dir[id]);
   if (out)
      fprintf(out, " dir=%d", (int)o->dir[id]);
   f.i(o->t[id]);
   if (out)
      fprintf(out, " t=%d", (int)o->t[id]);
   f.i(o->age[id]);
   if (out)
      fprintf(out, " age=%d", (int)o->age[id]);
   f.i(o->power[id]);
   if (out)
      fprintf(out, " power=%d", (int)o->power[id]);
   f.i(o->wood[id]);
   if (out)
      fprintf(out, " wood=%d", (int)o->wood[id]);
   f.i(o->money[id]);
   if (out)
      fprintf(out, " money=%d", (int)o->money[id]);
   f.i(o->health[id]);
   if (out)
      fprintf(out, " health=%d", (int)o->health[id]);
   f.i(o->speed[id]);
   if (out)
      fprintf(out, " speed=%d", (int)o->speed[id]);
   f.i(o->harvest_type[id]);
   if (out)
      fprintf(out, " harvest_type=%d", (int)o->harvest_type[id]);
   f.i(o->interrupted[id]);
   if (out)
      fprintf(out, " interrupted=%d", (int)o->interrupted[id]);
   f.i(o->support[id]);
   if (out)
      fprintf(out, " support=%d", (int)o->support[id]);
   f.i(o->atta_x[id]);
   if (out)
      fprintf(out, " atta_x=%d", (int)o->atta_x[id]);
   f.i(o->atta_y[id]);
   if (out)
      fprintf(out, " atta_y=%d", (int)o->atta_y[id]);
   f.i(o->ex[id]);
   if (out)
      fprintf(out, " ex=%d", (int)o->ex[id]);
   f.i(o->ey[id]);
   if (out)
      fprintf(out, " ey=%d", (int)o->ey[id]);
   f.i(o->delay[id]);
   if (out)
      fprintf(out, " delay=%d", (int)o->delay[id]);
   f.i(o->in_formation[id]);
   if (out)
      fprintf(out, " in_formation=%d", (int)o->in_formation[id]);
   f.i(o->attack_x_min[id]);
   if (out)
      fprintf(out, " attack_x_min=%d", (int)o->attack_x_min[id]);
   f.i(o->attack_y_min[id]);
   if (out)
      fprintf(out, " attack_y_min=%d", (int)o->attack_y_min[id]);
   f.i(o->attack_x_max[id]);
   if (out)
      fprintf(out, " attack_x_max=%d", (int)o->attack_x_max[id]);
   f.i(o->attack_y_max[id]);
   if (out)
      fprintf(out, " attack_y_max=%d", (int)o->attack_y_max[id]);
   f.i(o->master[id]);
   if (out)
      fprintf(out, " master=%d", (int)o->master[id]);
   f.i(o->idle_attack_r[id]);
   if (out)
      fprintf(out, " idle_attack_r=%d", (int)o->idle_attack_r[id]);
   f.i(o->money_limit[id]);
   if (out)
      fprintf(out, " money_limit=%d", (int)o->money_limit[id]);
   f.i(o->wood_limit[id]);
   if (out)
      fprintf(out, " wood_limit=%d", (int)o->wood_limit[id]);
   f.i(o->with_rob[id]);
   if (out)
      fprintf(out, " with_rob=%d", (int)o->with_rob[id]);
   f.bytes(o->cmd[id], sizeof(o->cmd[id]));
   if (out)
   {
      fnv q;
      q.bytes(o->cmd[id], sizeof(o->cmd[id]));
      fprintf(out, " cmd#%08x", (unsigned)q.h);
   }
   f.bytes(o->gx[id], sizeof(o->gx[id]));
   if (out)
   {
      fnv q;
      q.bytes(o->gx[id], sizeof(o->gx[id]));
      fprintf(out, " gx#%08x", (unsigned)q.h);
   }
   f.bytes(o->gy[id], sizeof(o->gy[id]));
   if (out)
   {
      fnv q;
      q.bytes(o->gy[id], sizeof(o->gy[id]));
      fprintf(out, " gy#%08x", (unsigned)q.h);
   }
   f.bytes(o->step[id], sizeof(o->step[id]));
   if (out)
   {
      fnv q;
      q.bytes(o->step[id], sizeof(o->step[id]));
      fprintf(out, " step#%08x", (unsigned)q.h);
   }
   f.bytes(o->subalterns[id], sizeof(o->subalterns[id]));
   if (out)
   {
      fnv q;
      q.bytes(o->subalterns[id], sizeof(o->subalterns[id]));
      fprintf(out, " subalterns#%08x", (unsigned)q.h);
   }

   ship *s = o->s[id];
   if (s != NULL)
   {
      f.i(s->capa);
      f.i(s->num_man);
      f.bytes(s->unit, sizeof(s->unit));
   }
}

void hash_player(fnv &f, player *p)
{
   f.i(p->active);
   f.i(p->master_dead);
   f.i(p->num_man);
   f.i(p->money);
   f.i(p->wood);
   f.d(p->food);
   f.d(p->bonus);
   f.d(p->grow_speed);
   f.i(p->num_town_halls);
   f.i(p->num_farms);
   f.i(p->num_markets);
   f.i(p->num_tents);
   f.i(p->num_camps);
   f.i(p->num_mills);
   f.i(p->num_smiths);
   f.i(p->num_docks);
   f.i(p->num_unis);
   f.i(p->num_mans);
   f.i(p->num_building_sites);
   f.i(p->town_hall_in_progress);
   f.i(p->market_in_progress);
}

} // namespace

uint64_t game_state_hash()
{
   return game_state_hash_dump(NULL);
}

uint64_t game_state_hash_dump(FILE *out)
{
   fnv f;

   f.i(ticker);
   f.i(objects->ticker);
   f.i(objects->qticker);
   f.i(landscape_dx);
   f.i(landscape_dy);

   /* Random number generator: peek by swapping a state in and back. */
   {
      unsigned short tmp[3] = { 0, 0, 0 };
      unsigned short rng[3];

      memcpy(rng, seed48(tmp), sizeof(rng));
      seed48(rng);
      f.bytes(rng, sizeof(rng));
   }
   if (out != NULL)
      fprintf(out, "head %016llx\n", (unsigned long long)f.h);

   f.land(landscape);
   f.land(landhight);
   f.land(landpic);
   f.land(unit);
   f.land(ilid);
   f.land(objects->is_a_trap);
   f.bytes(objects->talk_text, sizeof(objects->talk_text));
   for (int x = 0; x < landscape_dx; x++)
   {
      f.bytes(landoverlay[x], sizeof(landoverlay[x][0]) * landscape_dy);
      f.bytes(upic[x], sizeof(upic[x][0]) * landscape_dy);
   }

   if (out != NULL)
      fprintf(out, "land %016llx\n", (unsigned long long)f.h);
   for (int id = 0; id < max_objects; id++)
      if (!objects->is_free[id])
      {
         hash_object(f, objects, id);
         if (out != NULL)
         {
            fnv one;

            fprintf(out, "obj %3d:", id);
            hash_object(one, objects, id, out);
            fprintf(out, "\n");
            fprintf(out, "obj %3d type %3d color %d at %3d,%3d cmd %3d t %5d health %4d  %016llx\n",
                    id, objects->type[id], objects->color[id], objects->x[id], objects->y[id],
                    objects->cmd[id][0], objects->t[id], objects->health[id],
                    (unsigned long long)one.h);
         }
      }

   for (int i = 0; i < max_players; i++)
   {
      f.i(active[i]);
      if (active[i] && players[i] != NULL)
      {
         hash_player(f, players[i]);
         if (out != NULL)
         {
            /* num_mans is kept as a running count; show the real one too */
            int real = 0, swim = 0, ghost = 0;

            for (int id = 0; id < max_objects; id++)
               if (!objects->is_free[id] && objects->color[id] == player_color[i])
                  switch (objects->type[id])
                  {
                  case object_worker:
                  case object_knight:
                  case object_archer:
                  case object_pawn:
                  case object_cata:
                  case object_doktor:
                  case object_trader:
                  case object_scout:
                  case object_ship1:
                     real++;
                     { /* a man that is neither on the map nor on a live ship */
                        int sh = objects->on_ship[id];
                        bool on_map = unit[objects->x[id]][objects->y[id]] == id;

                        if (objects->type[id] == object_ship1)
                           on_map = true; /* ships cover 4x4; skip */
                        if (sh == -1 ? !on_map
                                     : (sh < 0 || sh >= max_objects || objects->is_free[sh]
                                        || objects->type[sh] != object_ship1))
                        {
                           ghost++;
                           fprintf(out, "ghost %d type %d at %d,%d on_ship %d\n", id,
                                   objects->type[id], objects->x[id], objects->y[id], sh);
                        }
                     }
                     break;
                  case object_swim:
                     swim++;
                     break;
                  }
            fprintf(
                out,
                "player %d money %d wood %d food %g mans %d (counted %d, swimming %d, ghosts %d)\n",
                i, players[i]->money, players[i]->wood, players[i]->food, players[i]->num_mans,
                real, swim, ghost);
         }
      }
   }
   if (out != NULL)
      fprintf(out, "total %016llx\n", (unsigned long long)f.h);
   return f.h;
}
