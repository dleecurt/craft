#include "morph.h"
#include "errorhandling.h"
#include "win.h"
#include "xstring.h"

#define max_regions 5000

bool inside_2d(triangle t, point p)
{
   double av01v02 = angle(new_vector(t.p0, t.p1), new_vector(t.p0, t.p2));
   double av10v12 = angle(new_vector(t.p1, t.p0), new_vector(t.p1, t.p2));
   double s = 0.5;

   return p == t.p0 || p == t.p1 || p == t.p2
          || (angle(new_vector(t.p0, p), new_vector(t.p0, t.p1)) <= av01v02 + s
              && angle(new_vector(t.p0, p), new_vector(t.p0, t.p2)) <= av01v02 + s
              && angle(new_vector(t.p1, p), new_vector(t.p1, t.p0)) <= av10v12 + s
              && angle(new_vector(t.p1, p), new_vector(t.p1, t.p2)) <= av10v12 + s);
}

void morph(ppm *pa, ppm *pb, ppm *pc, triangle ta, triangle tb, double s)

{
   triangle mreg;

   { // calc_mreg
      mreg = new_triangle(
          ta.p0 + set_length(new_vector(ta.p0, tb.p0), length(new_vector(ta.p0, tb.p0)) * s),
          ta.p1 + set_length(new_vector(ta.p1, tb.p1), length(new_vector(ta.p1, tb.p1)) * s),
          ta.p2 + set_length(new_vector(ta.p2, tb.p2), length(new_vector(ta.p2, tb.p2)) * s));
   }
   { // fill_mreg
      point pmin;
      point pmax;

      hull(mreg, pmin, pmax);
      for (int x = (int)pmin.x; x <= (int)pmax.x; x++)
         for (int y = (int)pmin.y; y <= (int)pmax.y; y++)
            if (inside_2d(mreg, new_point(x, y, 0)))
            { // morph_pixel
               double a;
               double b;
               int pixel_r[2];
               int pixel_g[2];
               int pixel_b[2];

               { // get_morph_gen
                  get_gen(new_point(x, y, 0), mreg.p0, mreg.p1, mreg.p2, a, b);
               }
               { // get_orig_rgb
                  point p;
                  point pmin;
                  point pmax;

                  hull(ta, pmin, pmax);
                  p = ta.p0 + (new_vector(ta.p0, ta.p1) * a) + (new_vector(ta.p0, ta.p2) * b);
                  p = p_min(pmax, p_max(p, pmin));
                  pa->rgb((int)p.x, (int)p.y, pixel_r[0], pixel_g[0], pixel_b[0]);
               }
               { // get_dest_rgb
                  point p;
                  point pmin;
                  point pmax;

                  hull(tb, pmin, pmax);
                  p = tb.p0 + (new_vector(tb.p0, tb.p1) * a) + (new_vector(tb.p0, tb.p2) * b);
                  p = p_min(pmax, p_max(p, pmin));
                  pb->rgb((int)p.x, (int)p.y, pixel_r[1], pixel_g[1], pixel_b[1]);
               }
               { // gen_morph_pixel
                  pc->set(x, y, (int)(((1.0 - s) * (double)pixel_r[0] + s * (double)pixel_r[1])),
                          (int)(((1.0 - s) * (double)pixel_g[0] + s * (double)pixel_g[1])),
                          (int)(((1.0 - s) * (double)pixel_b[0] + s * (double)pixel_b[1])));
               }
            }
   }
}

void morph(char orig[], char dest[], char background[], char regions[], int number_of_steps)

{
   ppm *pa;
   ppm *pb;
   ppm *pc;
   ppm *back;
   triangle region[max_regions][2];
   int num_regions;

   { // start_message
      printf("morphing (%s, %s) -> %s\n", orig, background, dest);
   }
   { // open_ppms
      pa = new ppm(orig);
      pb = new ppm(dest);
      back = new ppm(background);
      pc = new ppm(background);
   }
   { // load_regions
      FILE *t_data;
      bool another_region;

      { // open_t_data
         f_open(t_data, regions, "r");
      }
      num_regions = 0;
      { // scan_region
         float p[12];

         another_region
             = (fscanf(t_data, "%f %f %f %f %f %f %f %f %f %f %f %f", &p[0], &p[1], &p[2], &p[3],
                       &p[4], &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11])
                != EOF);
         if (another_region)
         { // store_region
            region[num_regions][0] = new_triangle(
                new_point(p[0], p[1], 0), new_point(p[2], p[3], 0), new_point(p[4], p[5], 0));
            region[num_regions][1] = new_triangle(
                new_point(p[6], p[7], 0), new_point(p[8], p[9], 0), new_point(p[10], p[11], 0));
            num_regions++;
         }
      }
      while (another_region)
      {
         { // scan_region
            float p[12];

            another_region
                = (fscanf(t_data, "%f %f %f %f %f %f %f %f %f %f %f %f", &p[0], &p[1], &p[2], &p[3],
                          &p[4], &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11])
                   != EOF);
            if (another_region)
            { // store_region
               region[num_regions][0] = new_triangle(
                   new_point(p[0], p[1], 0), new_point(p[2], p[3], 0), new_point(p[4], p[5], 0));
               region[num_regions][1] = new_triangle(
                   new_point(p[6], p[7], 0), new_point(p[8], p[9], 0), new_point(p[10], p[11], 0));
               num_regions++;
            }
         }
      }
      fclose(t_data);
   }
   { // perform_morph
      for (int s = 0; s < number_of_steps; s++)
      {    // perform_morph_step
         { // load_background
            copy(pc, back);
         }
         {    // morph_all_regions
            { // show_state
               printf("\n  %5.2f ", (double)(s + 1) / (double)number_of_steps * 100.0);
               fflush(stdout);
            }
            for (int r = 0; r < num_regions; r++)
            { // morph_region
               printf(".");
               fflush(stdout);
               morph(pa, pb, pc, region[r][0], region[r][1],
                     (double)s / (double)(number_of_steps - 1));
            }
         }
         { // save_result
            char r_name[128];

            sprintf(r_name, "%s_%04d", regions, s);
            changeall(r_name, 128, ".morph", "");
            strcat(r_name, ".ppm");
            pc->save(r_name);
            { // convert_to_pcx
               char cmd[512];
               char r_name[128];

               sprintf(r_name, "%s_%04d", regions, s);
               changeall(r_name, 128, ".morph", "");

               snprintf(cmd, sizeof(cmd), "ppmquant 256 %s.ppm > %s.tmp", r_name, r_name);
               run_command(cmd);
               snprintf(cmd, sizeof(cmd), "ppmtopcx %s.tmp > %s.pcx", r_name, r_name);
               run_command(cmd);
               sprintf(cmd, "%s.ppm", r_name);
               remove_file(cmd);
               sprintf(cmd, "%s.tmp", r_name);
               remove_file(cmd);
            }
         }
      }
   }
   { // delete_ppms
      delete (pa);
      delete (pb);
      delete (back);
      delete (pc);
   }
   { // finish_message
      printf("finsihed\n");
   }
}
