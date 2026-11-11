#include "objects.h"
#include "io.h"
#include "math.h"
#include "xmath.h"
#include "robot.h"

/*----------------------------------------------------------------------*/
/* point (functions)                                                    */
/*----------------------------------------------------------------------*/

point new_point(double x, double y, double z)
{
   point p;

   p.x = x;
   p.y = y;
   p.z = z;
   return p;
}

void print(point p)
{
   if (p == nil_point)
      printf("(NIL)");
   else
      printf("(%f %f %f)", p.x, p.y, p.z);
}

void scan(point &p)
{
   float x;
   float y;
   float z;

   scanf("%f %f %f", &x, &y, &z);
   p.x = x;
   p.y = y;
   p.z = z;
}

point operator+(point p, vector v)
{
   return new_point(p.x + v.dx, p.y + v.dy, p.z + v.dz);
}

point operator-(point p, vector v)
{
   return new_point(p.x - v.dx, p.y - v.dy, p.z - v.dz);
}

point operator+(point p, point p1)
{
   return new_point(p.x + p1.x, p.y + p1.y, p.z + p1.z);
}

point operator*(point p, point p1)
{
   return new_point(p.x * p1.x, p.y * p1.y, p.z * p1.z);
}

point operator/(point p, point p1)
{
   return new_point(p.x / p1.x, p.y / p1.y, p.z / p1.z);
}

point operator/(point p, double d)
{
   return new_point(p.x / d, p.y / d, p.z / d);
}

point operator-(point p, point p1)
{
   return new_point(p.x - p1.x, p.y - p1.y, p.z - p1.z);
}

point floor(point p)
{
   return new_point(floor(p.x), floor(p.y), floor(p.z));
}

point rotate(point pivot, point p, double ax, double ay, double az)
{
   point pp;

   pp = p - pivot;
   { // exec_rotate
      if (ax != 0)
      { // rotate_x
         double x = pp.x;
         double y = pp.y;

         pp.x = x * g_cos(ax) + y * g_sin(ax);
         pp.y = -x * g_sin(ax) + y * g_cos(ax);
      }
      if (ay != 0)
      { // rotate_y
         double x = pp.x;
         double z = pp.z;

         pp.x = x * g_cos(ay) - z * g_cos(ay);
         pp.z = x * g_sin(ay) + z * g_cos(ay);
      }
      if (az != 0)
      { // rotate_z
         double y = pp.y;
         double z = pp.z;

         pp.y = y * g_cos(az) + z * g_sin(az);
         pp.z = -y * g_sin(az) + z * g_cos(az);
      }
   }
   return pp + pivot;
}

double qdist(point p1, point p2)
{
   double dx = p1.x - p2.x;
   double dy = p1.y - p2.y;
   double dz = p1.z - p2.z;

   { // check_nil
      if (p1 == nil_point || p2 == nil_point)
         return DBL_MAX;
   }
   return (dx * dx + dy * dy + dz * dz);
}

double dist(point p1, point p2)
{
   double dx = p1.x - p2.x;
   double dy = p1.y - p2.y;
   double dz = p1.z - p2.z;

   { // check_nil
      if (p1 == nil_point || p2 == nil_point)
         return DBL_MAX;
   }
   return sqrt(dx * dx + dy * dy + dz * dz);
}

bool operator<=(point p1, point p2)
{
   return (p1.x <= p2.x || p1.y <= p2.y || p1.z <= p2.z);
}

bool operator<(point p1, point p2)
{
   return (p1.x < p2.x || p1.y < p2.y || p1.z < p2.z);
}

bool operator<<(point p1, point p2)
{
   return (p1.x < p2.x && p1.y < p2.y && p1.z < p2.z);
}

bool operator==(point p1, point p2)
{
   return (p1.x == p2.x && p1.y == p2.y && p1.z == p2.z);
}

bool operator!=(point p1, point p2)
{
   return !(p1 == p2);
}

point p_min(point p1, point p2)
{
   if (p1 == nil_point)
      return p2;
   if (p2 == nil_point)
      return p1;
   return new_point(d_min(p1.x, p2.x), d_min(p1.y, p2.y), d_min(p1.z, p2.z));
}

point p_max(point p1, point p2)
{
   return new_point(d_max(p1.x, p2.x), d_max(p1.y, p2.y), d_max(p1.z, p2.z));
}

point p_bound(point p1, point p2, point p3)
{
   return new_point(d_bound(p1.x, p2.x, p3.x), d_bound(p1.y, p2.y, p3.y),
                    d_bound(p1.z, p2.z, p3.z));
}

point rotate(point p, line axis, double angle)
{
   vector t;
   vector xx;

   xx = new_vector(axis.p0, p);
   t = (xx * g_cos(angle))
       + (new_vector(axis.p0, axis.p1)
          * ((xx * new_vector(axis.p0, axis.p1))
             / (new_vector(axis.p0, axis.p1) * new_vector(axis.p0, axis.p1)))
          * (1.0 - g_cos(angle)))
       - (perp(xx, new_vector(axis.p0, axis.p1) / length(new_vector(axis.p0, axis.p1)))
          * g_sin(angle));
   return (axis.p0 + t);
}

/*----------------------------------------------------------------------*/
/* vector (functions)                                                   */
/*----------------------------------------------------------------------*/

vector new_vector(point p1, point p2)
{
   vector v;

   v.dx = p2.x - p1.x;
   v.dy = p2.y - p1.y;
   v.dz = p2.z - p1.z;
   return v;
}

vector new_vector(double dx, double dy, double dz)
{
   vector v;

   v.dx = dx;
   v.dy = dy;
   v.dz = dz;
   return v;
}

vector new_vector(line l)
{
   vector v;

   v.dx = l.p1.x - l.p0.x;
   v.dy = l.p1.y - l.p0.y;
   v.dz = l.p1.z - l.p0.z;
   return v;
}

void print(vector v)
{
   if (v == nil_vector)
      printf("<NIL>");
   else
      printf("<%f %f %f>", v.dx, v.dy, v.dz);
}

vector operator-(vector v)
{
   return new_vector(-v.dx, -v.dy, -v.dz);
}

vector operator+(vector v1, vector v2)
{
   return new_vector(v1.dx + v2.dx, v1.dy + v2.dy, v1.dz + v2.dz);
}

vector operator-(vector v1, vector v2)
{
   return new_vector(v1.dx - v2.dx, v1.dy - v2.dy, v1.dz - v2.dz);
}

double operator*(vector v1, vector v2)
{
   return (v1.dx * v2.dx + v1.dy * v2.dy + v1.dz * v2.dz);
}

vector operator*(vector v, double s)
{
   return new_vector(v.dx * s, v.dy * s, v.dz * s);
}

vector operator/(vector v, double s)
{
   return new_vector(v.dx / s, v.dy / s, v.dz / s);
}

bool operator==(vector p1, vector p2)
{
   return (p1.dx == p2.dx && p1.dy == p2.dy && p1.dz == p2.dz);
}

bool operator!=(vector p1, vector p2)
{
   return !(p1 == p2);
}

vector cross(vector v1, vector v2)
{
   return new_vector(v1.dy * v2.dz - v1.dz * v2.dy, v1.dz * v2.dx - v1.dx * v2.dz,
                     v1.dx * v2.dy - v1.dy * v2.dx);
}

vector abs(vector p)
{
   return new_vector(d_abs(p.dx), d_abs(p.dy), d_abs(p.dz));
}

vector sign(vector p, int round_factor)
{
   return new_vector(d_sign(d_round(p.dx, round_factor)), d_sign(d_round(p.dy, round_factor)),
                     d_sign(d_round(p.dz, round_factor)));
}

vector sign(vector p)
{
   return new_vector(d_sign(p.dx), d_sign(p.dy), d_sign(p.dz));
}

double length(vector v)
{
   return sqrt(v.dx * v.dx + v.dy * v.dy + v.dz * v.dz);
}

vector set_length(vector v, double the_length)
{
   double now_length = length(v);

   if (now_length != 0)
      return v * (the_length / now_length);
   else
      return v;
}

point gen(point p0, vector v[3], vector a)
{
   return p0 + v[0] * a.dx + v[1] * a.dy + v[2] * a.dz;
}

vector perp(vector v1, vector v2)
{
   return (new_vector(v1.dy * v2.dz - v1.dz * v2.dy, v1.dz * v2.dx - v1.dx * v2.dz,
                      v1.dx * v2.dy - v1.dy * v2.dx));
}

double angle(vector v1, vector v2)
{
   double a = (v1 * v2) / (length(v1) * length(v2));

   if (a == 0)
      return 90;
   else if (a > 0.99999999)
      return 0;
   else if (a < -0.99999999)
      return 180;
   else
      return acos(a) * 180 / M_PI;
}

double preangle(vector v1, vector v2)
{
   return ((v1 * v2) / (length(v1) * length(v2)));
}

bool parallel(vector v1, vector v2)
{
   double a = angle(v1, v2);

   return (a < 2 || d_abs(180.0 - a) < 2);
}

bool parallel(vector v1, vector v2, double d)
{
   double a = angle(v1, v2);

   return (a < d || d_abs(180.0 - a) < d);
}

void get_gen(point p, cube c, vector &v)
{
   vector p0p = new_vector(c.p0, p);
   vector v1 = new_vector(c.p0, c.p1);
   vector v2 = new_vector(c.p0, c.p2);
   vector v3 = new_vector(c.p0, c.p3);
   double l1 = length(v1);
   double l2 = length(v2);
   double l3 = length(v3);

   v.dx = (p0p * v1) / (l1 * l1);
   v.dy = (p0p * v2) / (l2 * l2);
   v.dz = (p0p * v3) / (l3 * l3);
}

point gen(cube c, vector v)
{
   return (c.p0 + (new_vector(c.p0, c.p1) * v.dx) + (new_vector(c.p0, c.p2) * v.dy)
           + (new_vector(c.p0, c.p3) * v.dz));
}

double det(vector v1, vector v2)
{
   return (v1.dx * v2.dy - v1.dy * v2.dx);
}

void get_gen(point p, point p1, point p2, point p3, double &a, double &b)

{
   double d = det(new_vector(p1, p2), new_vector(p1, p3));

   b = det(new_vector(p1, p2), new_vector(p1, p)) / d;
   a = det(new_vector(p1, p), new_vector(p1, p3)) / d;
}

/*----------------------------------------------------------------------*/
/* vector (triangle)                                                    */
/*----------------------------------------------------------------------*/

triangle new_triangle(point p0, point p1, point p2)
{
   triangle t;

   t.p0 = p0;
   t.p1 = p1;
   t.p2 = p2;
   return t;
}

void hull(triangle t, point &pmin, point &pmax)
{
   pmin = p_min(t.p0, p_min(t.p1, t.p2));
   pmax = p_max(t.p0, p_max(t.p1, t.p2));
}

bool inside(triangle t, point p)
{
   return (angle(new_vector(t.p0, t.p1), new_vector(t.p0, p))
               <= angle(new_vector(t.p0, t.p1), new_vector(t.p0, t.p2))
           && angle(new_vector(t.p1, t.p0), new_vector(t.p1, p))
                  <= angle(new_vector(t.p1, t.p0), new_vector(t.p1, t.p2)));
}

/*----------------------------------------------------------------------*/
/* vector (line)                                                        */
/*----------------------------------------------------------------------*/

line new_line(point p0, point p1)
{
   line l;

   l.p0 = p0;
   l.p1 = p1;
   return l;
}

void print(line l)
{
   if (l == nil_line)
      printf("|NIL|");
   else
   {
      printf("|");
      print(l.p0);
      print(l.p1);
      printf("|");
   }
}

bool operator==(line l1, line l2)
{
   return (l1.p0 == l2.p0 && l1.p1 == l2.p1);
}

line expand(line l, double extent)
{
   line r;

   if (extent < 0)
   { // extent_towards_p0
      r = new_line(l.p0 - set_length(new_vector(l.p0, l.p1), -extent), l.p1);
   }
   else
   { // extent_towards_p1
      r = new_line(l.p0, l.p1 + set_length(new_vector(l.p0, l.p1), extent));
   }
   return r;
}

/*----------------------------------------------------------------------*/
/* rectangle (functions)                                                */
/*----------------------------------------------------------------------*/

rectangle new_rectangle(point p0, point p1, point p2, point p3)
{
   rectangle t;

   t.p0 = p0;
   t.p1 = p1;
   t.p2 = p2;
   t.p3 = p3;
   return t;
}

rectangle new_rectangle(double p[12])
{
   rectangle t;

   t.p0 = new_point(p[0], p[1], p[2]);
   t.p1 = new_point(p[3], p[4], p[5]);
   t.p2 = new_point(p[6], p[7], p[8]);
   t.p3 = new_point(p[9], p[10], p[11]);
   return t;
}

/*----------------------------------------------------------------------*/
/* cube (functions)                                                     */
/*----------------------------------------------------------------------*/

cube new_cube(point p0, point p1, point p2, point p3)
{
   cube c;

   c.p0 = p0;
   c.p1 = p1;
   c.p2 = p2;
   c.p3 = p3;
   return c;
}

cube new_cube(point p, double dx, double dy, double dz)
{
   return new_cube(p, p + new_vector(dx, 0, 0), p + new_vector(0, dy, 0), p + new_vector(0, 0, dz));
}

cube new_cube(double p[12])
{
   return normalize(new_cube(new_point(p[0], p[1], p[2]), new_point(p[3], p[4], p[5]),
                             new_point(p[6], p[7], p[8]), new_point(p[9], p[10], p[11])));
}

bool operator==(cube a, cube b)
{
   return (a.p0 == b.p0 && a.p1 == b.p1 && a.p2 == b.p2 && a.p3 == b.p3);
}

bool operator!=(cube a, cube b)
{
   return !(a.p0 == b.p0 && a.p1 == b.p1 && a.p2 == b.p2 && a.p3 == b.p3);
}

cube operator+(cube a, vector v)
{
   return new_cube(a.p0 + v, a.p1 + v, a.p2 + v, a.p3 + v);
}

void print(cube c)
{
   if (c == nil_cube)
      printf("[NIL]");
   else
   { // print_none_nil_cube
      printf("[");
      print(c.p0);
      print(c.p1);
      print(c.p2);
      print(c.p3);
      printf("]");
   }
}

point corner(cube c, int corner_no)
{
   switch (corner_no)
   {
   case 0:
      return c.p0;
   case 1:
      return c.p1;
   case 2:
      return c.p2;
   case 3:
      return c.p3;
   case 4:
      return c.p2 + new_vector(c.p0, c.p1);
   case 5:
      return c.p3 + new_vector(c.p0, c.p1);
   case 6:
      return c.p3 + new_vector(c.p0, c.p1) + new_vector(c.p0, c.p2);
   case 7:
      return c.p3 + new_vector(c.p0, c.p2);
   }
   return c.p0;
}

point center(cube c)
{
   return (c.p0 + (new_vector(c.p0, c.p1) * 0.5) + (new_vector(c.p0, c.p2) * 0.5)
           + (new_vector(c.p0, c.p3) * 0.5));
}

void edges(cube c, line edge_list[12])
{
   point cc[8];

   { // get_corners
      for (int i = 0; i < 8; i++)
         cc[i] = corner(c, i);
   }
   { // fill_edge_list
      edge_list[0] = new_line(cc[0], cc[1]);
      edge_list[1] = new_line(cc[0], cc[2]);
      edge_list[2] = new_line(cc[1], cc[4]);
      edge_list[3] = new_line(cc[2], cc[4]);

      edge_list[4] = new_line(cc[3], cc[5]);
      edge_list[5] = new_line(cc[3], cc[7]);
      edge_list[6] = new_line(cc[5], cc[6]);
      edge_list[7] = new_line(cc[7], cc[6]);

      edge_list[8] = new_line(cc[1], cc[5]);
      edge_list[9] = new_line(cc[4], cc[6]);
      edge_list[10] = new_line(cc[0], cc[3]);
      edge_list[11] = new_line(cc[2], cc[7]);
   }
}

void xplane(cube c, int corner_no, int side_no, point &p0, point &p1, point &p2, point &pl)

{
   point corner_p;

   { // get_plane
      xplane(c, side_no, p0, p1, p2, pl);
   }
   { // adjust_plane
      corner_p = corner(c, corner_no);
      for (int i = 0; i < 4; i++)
      { // check_orient
         if ((corner_p == p0))
            break;
         else
         { // rotate
            point d;
            point p3;

            p3 = p1 + new_vector(p0, p2);
            d = p0;
            p0 = p1;
            p1 = p3;
            p2 = d;
            pl = p0 + new_vector(d, pl);
         }
      }
   }
   { // perhaps_wrong_params
      if (!(corner_p == p0))
         errorstop(45, "OBJECTS", "plane () corner not on plane");
   }
}

plane xplane(cube c, int side_no)
{
   plane p;
   point pl;

   xplane(c, side_no, p.p0, p.p1, p.p2, pl);
   return p;
}

void xplane(cube c, int side_no, point &p0, point &p1, point &p2, point &pl)
{
   switch (side_no)
   {
   case 0:
      p0 = corner(c, 0);
      p1 = corner(c, 1);
      p2 = corner(c, 2);
      pl = corner(c, 3);
      break;
   case 1:
      p0 = corner(c, 3);
      p1 = corner(c, 5);
      p2 = corner(c, 0);
      pl = corner(c, 7);
      break;
   case 2:
      p0 = corner(c, 2);
      p1 = corner(c, 4);
      p2 = corner(c, 7);
      pl = corner(c, 0);
      break;
   case 3:
      p0 = corner(c, 7);
      p1 = corner(c, 6);
      p2 = corner(c, 3);
      pl = corner(c, 2);
      break;
   case 4:
      p0 = corner(c, 5);
      p1 = corner(c, 6);
      p2 = corner(c, 1);
      pl = corner(c, 3);
      break;
   case 5:
      p0 = corner(c, 3);
      p1 = corner(c, 7);
      p2 = corner(c, 0);
      pl = corner(c, 5);
      break;
   }
}

bool no_cut(cube c1, cube c2, bool is_dump)
{
   line edge_list[12];

   { // perhaps_open_dump
   }
   edges(c2, edge_list);
   for (int e = 0; e < 12; e++)
   {
      { // check_c2_edge
         point p;
         point p1;

         { // perhaps_dump
         }
         if (intersect(c1, edge_list[e], p, p1))
         {    // indicate_cut
            { // perhaps_close_dump
            }
            return false;
         }
      }
   }
   { // perhaps_close_dump
   }
   return !inside(c1, c2.p0);

   /*

.  perhaps_open_dump
     {if (is_dump)
         open_dump;
     }.

.  open_dump
     {d = new environment ("dump", 
                           janus_params->i_param ("world_dx"),
                           janus_params->i_param ("world_dy"),
                           janus_params->i_param ("world_dz"),
                           by_fix, by_fix, by_fix, by_fix,
                           true);
     }.

.  perhaps_close_dump
     {if (is_dump)
         delete (d);
     }.

.  perhaps_dump
     {if (is_dump)
         perform_dump;
     }.

.  perform_dump
     {d->create_object (c1, the_yellow);
      d->create_object (c2, the_green);
      d->create_object (edge_list [e].p0, edge_list [e].p1, the_red);
      if   (intersect (c1, edge_list [e], p, p1))
           d->ack ("cut");
      else d->ack ("no cut");
     }.

*/
}

bool hulls_disjunct(cube c1, cube c2)
{
   point min1;
   point max1;
   point min2;
   point max2;

   { // get_hulls
      hull(c1, min1, max1);
      hull(c2, min2, max2);
   }
   return (max1 < min2 || max2 < min1);
}

bool disjunct(cube c1, cube c2, bool with_trace)
{
   return (hulls_disjunct(c1, c2) || (no_cut(c1, c2, with_trace) && no_cut(c2, c1, with_trace)));
}

cube shift(cube c, vector d)
{
   return new_cube(c.p0 + d, c.p1 + d, c.p2 + d, c.p3 + d);
}

cube normalize(cube c)
{
   return c;
}

cube cover(cube c, double d)
{
   vector v01 = new_vector(c.p0, c.p1);
   vector v02 = new_vector(c.p0, c.p2);
   vector v03 = new_vector(c.p0, c.p3);
   double l01 = length(v01);
   double l02 = length(v02);
   double l03 = length(v03);
   double m1 = (d / l01);
   double m2 = (d / l02);
   double m3 = (d / l03);
   double d2 = 2 * d;
   double f1 = (l01 + d2) / l01;
   double f2 = (l02 + d2) / l02;
   double f3 = (l03 + d2) / l03;

   return shift(new_cube(c.p0, c.p0 + (v01 * f1), c.p0 + (v02 * f2), c.p0 + (v03 * f3)),
                -((v01 * m1) + (v02 * m2) + (v03 * m3)));
}

cube set_size(cube c, double sx, double sy, double sz)
{
   vector dx = new_vector(c.p0, c.p1);
   vector dy = new_vector(c.p0, c.p2);
   vector dz = new_vector(c.p0, c.p3);

   { // modify_lengths
      if (sx != 0.0)
         dx = set_length(dx, sx);
      if (sy != 0.0)
         dy = set_length(dy, sy);
      if (sz != 0.0)
         dz = set_length(dz, sz);
   }
   return new_cube(c.p0, c.p0 + dx, c.p0 + dy, c.p0 + dz);
}

void hull(cube c, point &min, point &max)
{
   { // init
      min = new_point(DBL_MAX, DBL_MAX, DBL_MAX);
      max = new_point(DBL_MIN, DBL_MIN, DBL_MIN);
   }
   for (int cn = 0; cn < 8; cn++)
   { // check_corner
      point cor = corner(c, cn);

      min.x = d_min(min.x, cor.x);
      min.y = d_min(min.y, cor.y);
      min.z = d_min(min.z, cor.z);
      max.x = d_max(max.x, cor.x);
      max.y = d_max(max.y, cor.y);
      max.z = d_max(max.z, cor.z);
   }
}

bool inside(cube c, point p)
{
   vector gen;

   get_gen(p, c, gen);
   return (0 <= gen.dx && gen.dx <= 1 && 0 <= gen.dy && gen.dy <= 1 && 0 <= gen.dz && gen.dz <= 1);
}

cube rotate(cube c, line axis, double angle)
{
   return new_cube(rotate(c.p0, axis, angle), rotate(c.p1, axis, angle), rotate(c.p2, axis, angle),
                   rotate(c.p3, axis, angle));
}

cube rotate(cube c, point p, vector axis, double angle)
{
   return new_cube(
       rotate(c.p0, new_line(p, p + axis), angle), rotate(c.p1, new_line(p, p + axis), angle),
       rotate(c.p2, new_line(p, p + axis), angle), rotate(c.p3, new_line(p, p + axis), angle));
}

cube rotate(cube c, point pivot, double ax, double ay, double az)
{
   return new_cube(rotate(pivot, c.p0, ax, ay, az), rotate(pivot, c.p1, ax, ay, az),
                   rotate(pivot, c.p2, ax, ay, az), rotate(pivot, c.p3, ax, ay, az));
}

int parside(cube c, vector v)
{
   int r;
   double par = DBL_MAX;
   double p;

   p = angle(abs(v), abs(new_vector(c.p0, c.p1)));
   if (p < par)
   {
      par = p;
      r = 1;
   };
   p = angle(abs(v), abs(new_vector(c.p0, c.p2)));
   if (p < par)
   {
      par = p;
      r = 2;
   };
   p = angle(abs(v), abs(new_vector(c.p0, c.p3)));
   if (p < par)
   {
      par = p;
      r = 3;
   };
   return r;
}

vector sidevect(cube c, int s)
{
   if (s == 1)
      return new_vector(c.p0, c.p1);
   if (s == 2)
      return new_vector(c.p0, c.p2);
   if (s == 3)
      return new_vector(c.p0, c.p3);
   return new_vector(c.p0, c.p1);
}

bool is_edge(cube c, vector v)
{
   vector vv = abs(v);

   if (length(vv) == 0)
      return false;
   return (angle(vv, abs(new_vector(c.p0, c.p1))) < 0.5
           || angle(vv, abs(new_vector(c.p0, c.p2))) < 0.5
           || angle(vv, abs(new_vector(c.p0, c.p3))) < 0.5);
}

vector shortest_side(cube c)
{
   vector v = new_vector(DBL_MAX, DBL_MAX, DBL_MAX);

   if (length(new_vector(c.p0, c.p1)) < length(v))
      v = new_vector(c.p0, c.p1);
   if (length(new_vector(c.p0, c.p2)) < length(v))
      v = new_vector(c.p0, c.p2);
   if (length(new_vector(c.p0, c.p3)) < length(v))
      v = new_vector(c.p0, c.p3);
   return v;
}

vector longest_side(cube c)
{
   vector v = new_vector(0, 0, 0);

   if (length(new_vector(c.p0, c.p1)) > length(v))
      v = new_vector(c.p0, c.p1);
   if (length(new_vector(c.p0, c.p2)) > length(v))
      v = new_vector(c.p0, c.p2);
   if (length(new_vector(c.p0, c.p3)) > length(v))
      v = new_vector(c.p0, c.p3);
   return v;
}

plane bottom(cube c)
{
   point cp = center(c);
   line l = new_line(cp, new_point(cp.x, cp.y, cp.z - 100000));

   for (int i = 0; i < 6; i++)
   { // check_plane
      plane pl = xplane(c, i);
      point p;

      if (intersect(pl, l, p))
         return pl;
   }
}

plane top(cube c)
{
   point cp = center(c);
   line l = new_line(cp, new_point(cp.x, cp.y, cp.z + 100000));

   for (int i = 0; i < 6; i++)
   { // check_plane
      plane pl = xplane(c, i);
      point p;

      if (intersect(pl, l, p))
         return pl;
   }
}

bool intersect(cube c, line l, point &ip, point &ip1)
{
   ip = nil_point;
   ip1 = nil_point;
   for (int s = 0; s < 6; s++)
   { // check_plane
      point pp;

      if (intersect(xplane(c, s), l, pp))
      { // store_pp
         if (ip == nil_point)
            ip = pp;
         else
            ip1 = pp;
      }
   }
   return (ip != nil_point);
}

bool inters(cube c, cube c1, point ip[32])
{
   line edge_list[12];

   edges(c1, edge_list);
   for (int e = 0; e < 12; e++)
   { // check_edge
      point p1;
      point p2;

      if (intersect(c, edge_list[e], p1, p2))
      { // store_pp
         if (p1 != nil_point)
         { // store_p1
            for (int i = 0; i < 8; i++)
               if (ip[i] == nil_point)
               {
                  ip[i] = p1;
                  break;
               }
         }
         if (p2 != nil_point)
         { // store_p2
            for (int i = 0; i < 8; i++)
               if (ip[i] == nil_point)
               {
                  ip[i] = p2;
                  break;
               }
         }
      }
   }
   return (ip[0] != nil_point);
}

bool intersect(cube c, cube c1, point p[32])
{
   bool a;
   bool b;

   { // init_ip
      for (int i = 0; i < 32; i++)
         p[i] = nil_point;
   }
   { // perhaps_completly_disjunct
      if (hulls_disjunct(c, c1))
         return false;
   }
   a = inters(c, c1, p);
   b = inters(c1, c, p);
   return (a | b);
}

void check_move(cube c1, cube c2, point &p1, point &p2, vector &pv1, vector &pv2, vector v[2][3],
                vector d1, vector d2, bool &any_improve)

{
   vector ppv1 = pv1 + d1;
   vector ppv2 = pv2 + d2;
   point pp1 = gen(c1.p0, v[0], ppv1);
   point pp2 = gen(c2.p0, v[1], ppv2);

   if (qdist(pp1, pp2) < qdist(p1, p2) && 0 <= ppv1.dx && ppv1.dx <= 1 && 0 <= ppv1.dy
       && ppv1.dy <= 1 && 0 <= ppv1.dz && ppv1.dz <= 1 && 0 <= ppv2.dx && ppv2.dx <= 1
       && 0 <= ppv2.dy && ppv2.dy <= 1 && 0 <= ppv2.dz && ppv2.dz <= 1)
   { // grab_points
      pv1 = ppv1;
      pv2 = ppv2;
      p1 = pp1;
      p2 = pp2;
      any_improve = true;
   }
}

void nearest_points(cube c1, cube c2, point &p1, point &p2)
{
   bool step_size_ok = true;
   vector v[2][3];
   vector last_delta[2];
   vector pv[2];
   vector delta_a;

   { // init_points
      delta_a = new_vector(0.25, 0.25, 0.25);
      last_delta[0] = new_vector(1, 1, 1);
      last_delta[1] = new_vector(1, 1, 1);
      v[0][0] = new_vector(c1.p0, c1.p1);
      v[0][1] = new_vector(c1.p0, c1.p2);
      v[0][2] = new_vector(c1.p0, c1.p3);
      v[1][0] = new_vector(c2.p0, c2.p1);
      v[1][1] = new_vector(c2.p0, c2.p2);
      v[1][2] = new_vector(c2.p0, c2.p3);
      pv[0] = new_vector(0.5, 0.5, 0.5);
      pv[1] = new_vector(0.5, 0.5, 0.5);
      p1 = gen(c1.p0, v[0], pv[0]);
      p2 = gen(c2.p0, v[1], pv[1]);
   }
   while (step_size_ok)
   {
      { // delta_climb
         bool any_improve;

         { // check_moves
            any_improve = false;
            { // check_last_step
               check_move(c1, c2, p1, p2, pv[0], pv[1], v, last_delta[0], last_delta[1],
                          any_improve);
            }
            for (int x1 = -1; x1 < 2 && !any_improve; x1++)
               for (int x2 = -1; x2 < 2 && !any_improve; x2++)
                  for (int x3 = -1; x3 < 2 && !any_improve; x3++)
                     for (int y1 = -1; y1 < 2 && !any_improve; y1++)
                        for (int y2 = -1; y2 < 2 && !any_improve; y2++)
                           for (int y3 = -1; y3 < 2 && !any_improve; y3++)
                           { // single_step
                              check_move(
                                  c1, c2, p1, p2, pv[0], pv[1], v,
                                  new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz),
                                  new_vector(y1 * delta_a.dx, y2 * delta_a.dy, y3 * delta_a.dz),
                                  any_improve);
                              { // perhaps_grab_dir
                                 if (any_improve)
                                 { // grab_dir
                                    last_delta[0] = new_vector(x1 * delta_a.dx, x2 * delta_a.dy,
                                                               x3 * delta_a.dz);
                                    last_delta[1] = new_vector(y1 * delta_a.dx, y2 * delta_a.dy,
                                                               y3 * delta_a.dz);
                                 }
                              }
                           }
         }
         while (any_improve)
         {
            { // check_moves
               any_improve = false;
               { // check_last_step
                  check_move(c1, c2, p1, p2, pv[0], pv[1], v, last_delta[0], last_delta[1],
                             any_improve);
               }
               for (int x1 = -1; x1 < 2 && !any_improve; x1++)
                  for (int x2 = -1; x2 < 2 && !any_improve; x2++)
                     for (int x3 = -1; x3 < 2 && !any_improve; x3++)
                        for (int y1 = -1; y1 < 2 && !any_improve; y1++)
                           for (int y2 = -1; y2 < 2 && !any_improve; y2++)
                              for (int y3 = -1; y3 < 2 && !any_improve; y3++)
                              { // single_step
                                 check_move(
                                     c1, c2, p1, p2, pv[0], pv[1], v,
                                     new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz),
                                     new_vector(y1 * delta_a.dx, y2 * delta_a.dy, y3 * delta_a.dz),
                                     any_improve);
                                 { // perhaps_grab_dir
                                    if (any_improve)
                                    { // grab_dir
                                       last_delta[0] = new_vector(x1 * delta_a.dx, x2 * delta_a.dy,
                                                                  x3 * delta_a.dz);
                                       last_delta[1] = new_vector(y1 * delta_a.dx, y2 * delta_a.dy,
                                                                  y3 * delta_a.dz);
                                    }
                                 }
                              }
            }
         }
         { // reduce_delta
            delta_a = delta_a * 0.1;
            step_size_ok = dist(gen(new_point(0, 0, 0), v[0], delta_a), new_point(0, 0, 0)) >= 1
                           && dist(gen(new_point(0, 0, 0), v[1], delta_a), new_point(0, 0, 0)) >= 1;
         }
      }
   }
}

/*

void check_move (cube   c,
                 point  &pc,
                 point  &p,
                 vector dc,
                 bool   &any_improve)

  {if (points_are_better)
      grab_points;

.  grab_points
     {pc          = ppc;
      any_improve = true;
     }.

.  points_are_better
     qdist (ppc, p) < qdist (pc, p) && inside (c, ppc).

.  ppc (pc + dc).

  }

point nearest_point (cube c, point p)
  {bool   any_improve;
   point  p1;
   double delta;
   vector last_step;

   init_point;
   itterate;
   return p1;

.  init_point
     {p1        = center     (c);
      delta     = length     (longest_side (c)) / 2.0;
      last_step = new_vector (0, 0, 0);
     }.

.  itterate
     {while (delta >= 1.0)
       {delta_climb;
       }
     }.

.  delta_climb
     {check_moves;
      while (any_improve)
        {check_moves;
        }
       reduce_delta;
     }.

.  reduce_delta
     {if   (delta <= 1.0)
           delta = 0.0;
      else delta = d_max (1.0, delta / 10.0);
     }.

.  check_moves
     {any_improve = false;
      try_last_step;
      for (int x1 = -1; x1 < 2 && ! any_improve; x1++)
        for (int x2 = -1; x2 < 2 && ! any_improve; x2++)
          for (int x3 = -1; x3 < 2 && ! any_improve; x3++)
            single_step;
     }.

.  single_step
     {check_move (c,p1,p,new_vector (x1*delta,x2*delta,x3*delta),any_improve);
      if (any_improve)
         last_step = new_vector (x1 * delta, x2 * delta, x3 * delta);
     }.

.  try_last_step
     {check_move (c, p1, p, last_step, any_improve);
     }.

  }

*/

void check_move(cube c, point &p, point p2, vector &pv, vector v[3], vector d, bool &any_improve)

{
   vector ppv = pv + d;
   point pp = gen(c.p0, v, ppv);

   if (qdist(pp, p2) < qdist(p, p2) && 0 <= ppv.dx && ppv.dx <= 1 && 0 <= ppv.dy && ppv.dy <= 1
       && 0 <= ppv.dz && ppv.dz <= 1)
   { // grab_points
      pv = ppv;
      p = pp;
      any_improve = true;
   }
}

point nearest_point(cube c, point p)
{
   bool step_size_ok = true;
   vector v[3];
   vector last_delta;
   vector pv;
   vector delta_a;
   point p1;

   { // init_points
      delta_a = new_vector(0.25, 0.25, 0.25);
      last_delta = new_vector(1, 1, 1);
      v[0] = new_vector(c.p0, c.p1);
      v[1] = new_vector(c.p0, c.p2);
      v[2] = new_vector(c.p0, c.p3);
      pv = new_vector(0.5, 0.5, 0.5);
      p1 = gen(c.p0, v, pv);
   }
   while (step_size_ok)
   {
      { // delta_climb
         bool any_improve;

         { // check_moves
            any_improve = false;
            { // check_last_step
               check_move(c, p1, p, pv, v, last_delta, any_improve);
            }
            for (int x1 = -1; x1 < 2 && !any_improve; x1++)
               for (int x2 = -1; x2 < 2 && !any_improve; x2++)
                  for (int x3 = -1; x3 < 2 && !any_improve; x3++)
                  { // single_step
                     check_move(c, p1, p, pv, v,
                                new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz),
                                any_improve);
                     { // perhaps_grab_dir
                        if (any_improve)
                        { // grab_dir
                           last_delta
                               = new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz);
                        }
                     }
                  }
         }
         while (any_improve)
         {
            { // check_moves
               any_improve = false;
               { // check_last_step
                  check_move(c, p1, p, pv, v, last_delta, any_improve);
               }
               for (int x1 = -1; x1 < 2 && !any_improve; x1++)
                  for (int x2 = -1; x2 < 2 && !any_improve; x2++)
                     for (int x3 = -1; x3 < 2 && !any_improve; x3++)
                     { // single_step
                        check_move(c, p1, p, pv, v,
                                   new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz),
                                   any_improve);
                        { // perhaps_grab_dir
                           if (any_improve)
                           { // grab_dir
                              last_delta
                                  = new_vector(x1 * delta_a.dx, x2 * delta_a.dy, x3 * delta_a.dz);
                           }
                        }
                     }
            }
         }
         { // reduce_delta
            delta_a = delta_a * 0.1;
            step_size_ok = dist(gen(new_point(0, 0, 0), v, delta_a), new_point(0, 0, 0)) >= 1;
         }
      }
   }
   return p1;
}

double dist(cube c, point p)
{
   return dist(p, nearest_point(c, p));
}

double dist(cube c1, cube c2)
{
   point a;
   point b;

   nearest_points(c1, c2, a, b);
   return dist(a, b);
}

/*----------------------------------------------------------------------*/
/* config (functions)                                                   */
/*----------------------------------------------------------------------*/

config new_config(double values[])
{
   config c;

   { // enter_values
      for (int i = 0; i < max_config_size; i++)
         c.v[i] = values[i];
   }
   return c;
}

config nil_config()
{
   config c;

   for (int a = 0; a < max_config_size; a++)
      c.v[a] = nil_angle;
   return c;
}

void print(config c)
{
   if (is_nil(c))
   { // print_nil_config
      printf("{NIL}");
   }
   else
   { // print_config
      printf("{");
      for (int i = 0; i < max_config_size; i++)
      {
         printf("%f", c.v[i]);
         { // perhaps_seperator
            if (i < max_config_size - 1)
               printf(", ");
         }
      }
      printf("}");
   }
}

double qdist(config c1, config c2)
{
   double d = 0;

   { // calc_dist
      for (int i = 0; i < max_config_size; i++)
      { // add_dist
         double di;

         if (c1.v[i] == nil_angle || c2.v[i] == nil_angle)
            di = 0;
         else
            di = c1.v[i] - c2.v[i];
         d += (di * di);
      }
   }
   return d;
}

double dist(config c1, config c2)
{
   return sqrt(qdist(c1, c2));
}

config operator+(config c1, config c2)
{
   config r;

   { // perform_add
      for (int i = 0; i < max_config_size; i++)
         if (c1.v[i] == nil_angle || c2.v[i] == nil_angle)
            r.v[i] = nil_angle;
         else
            r.v[i] = c1.v[i] + c2.v[i];
   }
   return r;
}

config operator-(config c1, config c2)
{
   config r;

   { // perform_sub
      for (int i = 0; i < max_config_size; i++)
         if (c1.v[i] == nil_angle || c2.v[i] == nil_angle)
            r.v[i] = nil_angle;
         else
            r.v[i] = c1.v[i] - c2.v[i];
   }
   return r;
}

config operator/(config c1, double val)
{
   config r;

   { // perform_div
      for (int i = 0; i < max_config_size; i++)
         if (c1.v[i] == nil_angle)
            r.v[i] = nil_angle;
         else
            r.v[i] = c1.v[i] / val;
   }
   return r;
}

config sign(config c)
{
   config r;

   { // perform_sign
      for (int i = 0; i < max_config_size; i++)
         r.v[i] = d_sign(c.v[i]);
   }
   return r;
}

bool is_nil(config c)
{
   for (int i = 0; i < max_config_size; i++)
      if (c.v[i] != nil_angle)
         return false;
   return true;
}

bool operator==(config c1, config c2)
{
   for (int i = 0; i < max_config_size; i++)
      if (c1.v[i] != c2.v[i])
         return false;
   return true;
}

bool operator!=(config c1, config c2)
{
   return (!(c1 == c2));
}

/*----------------------------------------------------------------------*/
/* plane (functions)                                                    */
/*----------------------------------------------------------------------*/

plane new_plane(point p0, point p1, point p2)
{
   plane p;

   p.p0 = p0;
   p.p1 = p1;
   p.p2 = p2;
   return p;
}

plane new_plane(vector v0, vector v1)
{
   point pp = new_point(0.0, 0.0, 0.0);

   return new_plane(pp, pp + v0, pp + v1);
}

plane operator-(plane a)
{
   point p = a.p0 - new_vector(a.p0, a.p1);
   ;

   return new_plane(a.p0, p, a.p2);
}

bool operator==(plane a, plane b)
{
   return (a.p0 == b.p0 && a.p1 == b.p1 && a.p2 == b.p2);
}

bool operator!=(plane a, plane b)
{
   return !(a.p0 == b.p0 && a.p1 == b.p1 && a.p2 == b.p2);
}

bool on(plane a, point p)
{
   return (a.p0 == p || a.p1 == p || a.p2 == p || (a.p1 + new_vector(a.p0, a.p2)) == p);
}

void print(plane p)
{
   if (p == nil_plane)
      printf("/NIL/");
   else
   { // print_none_nil_plane
      printf("/ ");
      print(p.p0);
      print(p.p1);
      print(p.p2);
      printf(" /");
   }
}

point center(plane p)
{
   return (p.p0 + (new_vector(p.p0, p.p1) / 2.0) + (new_vector(p.p0, p.p2) / 2.0));
}

double det(vector v1, vector v2, vector v3)
{
   return (v1.dx * (v2.dy * v3.dz - v3.dy * v2.dz) - v1.dy * (v2.dx * v3.dz - v3.dx * v2.dz)
           + v1.dz * (v2.dx * v3.dy - v3.dx * v2.dy));
}

bool intersect(plane p, line l, point &ip)
{
   double v1v2v3;
   double v1p1v3;
   double p1v2v3;
   double v1v2p1;
   double a;
   double b;
   double c;

   ip = nil_point;
   v1v2v3 = det(new_vector(p.p0, p.p1), new_vector(p.p0, p.p2), new_vector(l.p0, l.p1));
   if (v1v2v3 == 0)
      return false;
   v1p1v3
       = det(new_vector(p.p0, p.p1), new_vector((l.p0 - p.p0).x, (l.p0 - p.p0).y, (l.p0 - p.p0).z),
             new_vector(l.p0, l.p1));
   p1v2v3 = det(new_vector((l.p0 - p.p0).x, (l.p0 - p.p0).y, (l.p0 - p.p0).z),
                new_vector(p.p0, p.p2), new_vector(l.p0, l.p1));
   v1v2p1 = det(new_vector(p.p0, p.p1), new_vector(p.p0, p.p2),
                new_vector((l.p0 - p.p0).x, (l.p0 - p.p0).y, (l.p0 - p.p0).z));
   b = v1p1v3 / v1v2v3;
   a = p1v2v3 / v1v2v3;
   c = v1v2p1 / v1v2v3;
   if (0.0 <= a && a <= 1.0 && 0.0 <= b && b <= 1.0 && c < 0 && d_abs(c) <= 1)
   {
      ip = p.p0 + (new_vector(p.p0, p.p2) * b) + (new_vector(p.p0, p.p1) * a);
      return true;
   }
   else
      return false;
}

point socket(plane p, point op)
{
   point pp;
   vector u;

   u = perp(new_vector(p.p0, p.p1), new_vector(p.p0, p.p2));
   pp = op - (u * (new_vector(p.p0, op) * u) / (u * u));
   return pp;
}

double angle(plane p1, plane p2)
{
   return (angle(perp(new_vector(p1.p0, p1.p1), new_vector(p1.p0, p1.p2)),
                 perp(new_vector(p2.p0, p2.p1), new_vector(p2.p0, p2.p2))));
}

double angle(plane p, vector v)
{
   point intersection = socket(p, p.p0 + v);

   if (p.p0 == intersection)
   {
      if (perp(new_vector(p.p0, p.p1), new_vector(p.p0, p.p2)) * v > 0)
         return 90;
      else
         return -90;
   }

   double ang = angle(new_vector(p.p0, intersection), v);

   if (perp(new_vector(p.p0, p.p1), new_vector(p.p0, p.p2)) * v > 0)
      return ang;
   else
      return -ang;
}

/*----------------------------------------------------------------------*/
/* scene (functions)                                                    */
/*----------------------------------------------------------------------*/

scene new_scene(char s_name[])
{
   scene s;

   s.num_elements = 0;
   strcpy(s.name, s_name);
   return s;
}

void add(scene &s, cube c, char c_name[])
{
   { // perhaps_scene_overflow
      if (s.num_elements == max_scene_elements)
         errorstop(46, "OBJECTS", "scene overflow");
   }
   { // add_data
      s.cubes[s.num_elements] = c;
      strcpy(s.names[s.num_elements], c_name);
      s.num_elements++;
   }
}

void print(scene s)
{
   { // print_name
      printf("=%s===================\n", s.name);
   }
   { // print_elements
      for (int i = 0; i < s.num_elements; i++)
      {
         printf("   ");
         print(s.cubes[i]);
         printf("-%s-\n", s.names[i]);
      }
   }
   { // print_close
      printf("=================================\n");
   }
}

cube xcube(scene s, char c_name[])
{
   for (int i = 0; i < s.num_elements; i++)
   { // check_element
      if (strcmp(s.names[i], c_name) == 0)
         return s.cubes[i];
   }
   return nil_cube;
}
