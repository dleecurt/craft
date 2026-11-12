/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: converted from win.hc (hcc refinements
 *              expanded) to plain C++; compiler, warning and bug fixes;
 *              new features (see README), including zoom
 * Exact changes: git log -- win.cc win.hc
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 050293 hua    win.hc     created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include "stdlib.h"
#include "datadir.h"

#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h> /* before win.h, whose colour macros clash */
#include "win.h"
#include "zoom.h"
#include "xfile.h"
#include "xstring.h"
#include "errorhandling.h"

/*----------------------------------------------------------------------*/
/* ROUTINES bitmap size information (deklarations)                      */
/*----------------------------------------------------------------------*/

bool bitmap_size(char name[], int &dx, int &dy)
{
   if (f_exists(name))
   { // get_size
      FILE *f;
      char d[128];

      f = fopen(name, "r");
      read_check(fscanf(f, "%127s %127s %d", d, d, &dx), 3, "XBM bitmap");
      read_check(fscanf(f, "%127s %127s %d", d, d, &dy), 3, "XBM bitmap");
      fclose(f);
      return true;
   }
   else
      return false;
}

/*----------------------------------------------------------------------*/
/* ROUTINES text size information (deklarations)                        */
/*----------------------------------------------------------------------*/

void text_size(char s[], const char font[], int &dx, int &dy, int &num_lines)
{
   XFontStruct *font_info;
   Display *display;

   { // open_font_info
      char name[128];

      strcpy(name, getenv("DISPLAY"));
      display = XOpenDisplay(name);
      font_info = XLoadQueryFont(display, font);
   }
   { // init
      num_lines = 0;
      dx = 0;
      dy = 0;
   }
   { // pharse_string
      char record[1024];
      int rp;

      rp = 0;
      for (int i = 0; i < (int)strlen(s); i++)
      { // handle_char
         if (s[i] == '\n')
         { // handle_new_record
            int tdx;
            int tdy;

            record[rp] = 0;
            num_lines++;
            { // calc_tdx_tdy
               tdx = XTextWidth(font_info, record, strlen(record));
               tdy = font_info->max_bounds.descent + font_info->max_bounds.ascent;
            }
            dx = i_max(dx, tdx);
            dy += tdy + 4;
            rp = 0;
         }
         else
            record[rp++] = s[i];
      }
      { // handle_new_record
         int tdx;
         int tdy;

         record[rp] = 0;
         num_lines++;
         { // calc_tdx_tdy
            tdx = XTextWidth(font_info, record, strlen(record));
            tdy = font_info->max_bounds.descent + font_info->max_bounds.ascent;
         }
         dx = i_max(dx, tdx);
         dy += tdy + 4;
         rp = 0;
      }
   }
   { // close_font_info
      XFreeFont(display, font_info);
      XCloseDisplay(display);
   }
}

/*----------------------------------------------------------------------*/
/* Default Handler (funktions)                                          */
/*----------------------------------------------------------------------*/

bool win_handler_init = false;
paramreader *win_defaults;

void init_default_handler()
{
   if (!win_handler_init)
   { // perform_init
      win_handler_init = true;
      if (!f_exists(craft_data(".windefaults.params")))
      {
         fprintf(stderr,
                 "craft: cannot find data files in '%s'\n"
                 "       (set CRAFT_DATADIR to the directory holding hcraft/ and buttons/)\n",
                 craft_datadir());
         exit(1);
      }
      win_defaults = new paramreader(craft_data(".windefaults"));
   }
}

int win_default_i(int param, const char default_name[])
{
   init_default_handler();
   if (param != by_default)
      return param;
   else
      return win_default_i(default_name);
}

int win_default_i(const char default_name[])
{
   init_default_handler();
   return win_defaults->i_param(default_name);
}

char *win_default_s(const char default_name[])
{
   init_default_handler();
   return win_defaults->s_param(default_name);
}

int win_default_c(int param, const char default_name[])
{
   init_default_handler();
   if (param != by_default)
      return param;
   else
      return win_default_c(default_name);
}

int win_default_c(const char default_name[])
{
   char color_name[128];

   init_default_handler();
   strcpy(color_name, win_defaults->s_param(default_name));
   if (isdigit(color_name[0]))
      return atoi(color_name);
   else
      return win_color(color_name);
}

int win_color(char name[])
{
   if (strcmp(name, "white") == 0)
      return 0;
   if (strcmp(name, "red") == 0)
      return 1;
   if (strcmp(name, "red1") == 0)
      return 2;
   if (strcmp(name, "red2") == 0)
      return 3;
   if (strcmp(name, "red3") == 0)
      return 4;
   if (strcmp(name, "red4") == 0)
      return 5;
   if (strcmp(name, "sienna") == 0)
      return 6;
   if (strcmp(name, "sienna1") == 0)
      return 7;
   if (strcmp(name, "sienna2") == 0)
      return 8;
   if (strcmp(name, "sienna3") == 0)
      return 9;
   if (strcmp(name, "sienna4") == 0)
      return 10;
   if (strcmp(name, "yellow") == 0)
      return 11;
   if (strcmp(name, "yellow1") == 0)
      return 12;
   if (strcmp(name, "yellow2") == 0)
      return 13;
   if (strcmp(name, "yellow3") == 0)
      return 14;
   if (strcmp(name, "yellow4") == 0)
      return 15;
   if (strcmp(name, "green") == 0)
      return 16;
   if (strcmp(name, "green1") == 0)
      return 17;
   if (strcmp(name, "green2") == 0)
      return 18;
   if (strcmp(name, "green3") == 0)
      return 19;
   if (strcmp(name, "green4") == 0)
      return 20;
   if (strcmp(name, "blue") == 0)
      return 21;
   if (strcmp(name, "blue1") == 0)
      return 22;
   if (strcmp(name, "blue2") == 0)
      return 23;
   if (strcmp(name, "blue3") == 0)
      return 24;
   if (strcmp(name, "blue4") == 0)
      return 25;
   if (strcmp(name, "slategray") == 0)
      return 26;
   if (strcmp(name, "slategray1") == 0)
      return 27;
   if (strcmp(name, "slategray2") == 0)
      return 28;
   if (strcmp(name, "slategray3") == 0)
      return 29;
   if (strcmp(name, "slategray4") == 0)
      return 30;
   if (strcmp(name, "darkgreen") == 0)
      return 31;
   if (strcmp(name, "seagreen") == 0)
      return 32;
   if (strcmp(name, "seagreen1") == 0)
      return 33;
   if (strcmp(name, "seagreen2") == 0)
      return 34;
   if (strcmp(name, "seagreen3") == 0)
      return 35;
   if (strcmp(name, "cyan") == 0)
      return 36;
   if (strcmp(name, "cyan1") == 0)
      return 37;
   if (strcmp(name, "cyan2") == 0)
      return 38;
   if (strcmp(name, "cyan3") == 0)
      return 39;
   if (strcmp(name, "cyan4") == 0)
      return 40;
   if (strcmp(name, "purple") == 0)
      return 41;
   if (strcmp(name, "purple1") == 0)
      return 42;
   if (strcmp(name, "purple2") == 0)
      return 43;
   if (strcmp(name, "purple3") == 0)
      return 44;
   if (strcmp(name, "purple4") == 0)
      return 45;
   if (strcmp(name, "gold") == 0)
      return 46;
   if (strcmp(name, "gold1") == 0)
      return 47;
   if (strcmp(name, "gold2") == 0)
      return 48;
   if (strcmp(name, "gold3") == 0)
      return 49;
   if (strcmp(name, "gold4") == 0)
      return 50;
   if (strcmp(name, "gray") == 0)
      return 51;
   if (strcmp(name, "gray10") == 0)
      return 52;
   if (strcmp(name, "gray20") == 0)
      return 53;
   if (strcmp(name, "gray30") == 0)
      return 54;
   if (strcmp(name, "gray40") == 0)
      return 55;
   if (strcmp(name, "gray50") == 0)
      return 56;
   if (strcmp(name, "gray60") == 0)
      return 57;
   if (strcmp(name, "gray70") == 0)
      return 58;
   if (strcmp(name, "gray80") == 0)
      return 59;
   if (strcmp(name, "gray90") == 0)
      return 60;
   if (strcmp(name, "gray100") == 0)
      return 61;
   if (strcmp(name, "navyblue") == 0)
      return 62;
   if (strcmp(name, "black") == 0)
      return 63;
   errorstop(1, "WIN", "unknown color :", name);
   return -1;
}

/*----------------------------------------------------------------------*/
/* CLASS ploylin (funktions)                                            */
/*----------------------------------------------------------------------*/

polyline::polyline()
{
   n = 0;
}

polyline::~polyline() {}

void polyline::add(int x, int y)
{
   p[n].x = x;
   p[n].y = y;
   n++;
}

void polyline::close()
{
   add(p[0].x, p[0].y);
}

/*----------------------------------------------------------------------*/
/* CLASS win (statics)                                                  */
/*----------------------------------------------------------------------*/

char win::fix_name[128];
char win::fix_dir[128];
bool win::fix_initialized = false;

/*----------------------------------------------------------------------*/
/* Off-screen buffer                                                    */
/*                                                                      */
/* Craft used to ask the X server to keep window contents (backing      */
/* store) and ignored Expose events. Most X servers today don't keep    */
/* them, so covered or minimised windows came back white. Every window  */
/* now also draws into an off-screen pixmap, and exposed parts are      */
/* copied back from it.                                                 */
/*----------------------------------------------------------------------*/

win *win::all_wins[256];
int win::num_wins = 0;

void win::init_buffer(int buffer_dx, int buffer_dy)
{
   mybuffer = XCreatePixmap(mydisplay, mywindow, i_max(1, buffer_dx), i_max(1, buffer_dy),
                            DefaultDepth(mydisplay, myscreen));
   mycopy_gc = XCreateGC(mydisplay, mywindow, 0, 0);
   XSetGraphicsExposures(mydisplay, mycopy_gc, False);
   XSetForeground(mydisplay, mycopy_gc, mybackground);
   XFillRectangle(mydisplay, mybuffer, mycopy_gc, 0, 0, i_max(1, buffer_dx), i_max(1, buffer_dy));
   if (num_wins < 256)
      all_wins[num_wins++] = this;
}

void win::free_buffer()
{
   for (int i = 0; i < num_wins; i++)
      if (all_wins[i] == this)
      {
         all_wins[i] = all_wins[--num_wins];
         break;
      }
   free_zoom();
   if (mybuffer != None)
   {
      XFreePixmap(mydisplay, mybuffer);
      XFreeGC(mydisplay, mycopy_gc);
   }
   mybuffer = None;
}

/* x, y, dx, dy are window (zoomed) coordinates, as in an Expose event. */
void win::repaint(int x, int y, int dx, int dy)
{
   if (mybuffer == None)
      return;
   if (to_window())
      XCopyArea(mydisplay, mybuffer, mywindow, mycopy_gc, x, y, dx, dy, x, y);
   else
      present_phys(x, y, dx, dy);
}

/* Repaint the exposed parts of every window, including windows the game
   is not polling at the moment, and show what has been drawn into the
   buffers of zoomed windows since the last call. */
void win::repaint_exposed()
{
   XEvent ev;

   for (int i = 0; i < num_wins; i++)
   {
      win *w = all_wins[i];

      while (XCheckTypedWindowEvent(w->mydisplay, w->mywindow, Expose, &ev))
         w->repaint(ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
   }
   present_all();
}

/*----------------------------------------------------------------------*/
/* Zoom                                                                 */
/*                                                                      */
/* A zoomed window (myscale > 1) is drawn only into mybuffer, at the    */
/* original size. Each drawing call records the area it changed         */
/* (touch), and present_all, called whenever the game polls for events, */
/* has the X server enlarge those areas into the windows with XRender.  */
/* Mouse positions are divided back down, so the game never sees the    */
/* zoom.                                                                */
/*----------------------------------------------------------------------*/

bool win::to_window()
{
   return mybuffer == None || src_pic == 0;
}

int win::zoom_in(int v)
{
   return (int)floor(v * myscale + 0.5);
}

int win::zoom_out(int v)
{
   return (int)floor(v / myscale);
}

void win::touch(int x, int y, int dx, int dy)
{
   if (to_window() || dx <= 0 || dy <= 0)
      return;
   /* one pixel more on each side: smooth scaling reads neighbours */
   x -= 1;
   y -= 1;
   dx += 2;
   dy += 2;
   if (!is_dirty)
   {
      dirty_x0 = x;
      dirty_y0 = y;
      dirty_x1 = x + dx;
      dirty_y1 = y + dy;
      is_dirty = true;
   }
   else
   {
      dirty_x0 = i_min(dirty_x0, x);
      dirty_y0 = i_min(dirty_y0, y);
      dirty_x1 = i_max(dirty_x1, x + dx);
      dirty_y1 = i_max(dirty_y1, y + dy);
   }
}

void win::touch_all()
{
   touch(0, 0, w_dx, w_dy);
}

void win::touch_poly(polyline *p)
{
   if (p->n <= 0)
      return;
   {
      int x0 = p->p[0].x;
      int y0 = p->p[0].y;
      int x1 = x0;
      int y1 = y0;

      for (int i = 1; i < p->n; i++)
      {
         x0 = i_min(x0, p->p[i].x);
         y0 = i_min(y0, p->p[i].y);
         x1 = i_max(x1, p->p[i].x);
         y1 = i_max(y1, p->p[i].y);
      }
      touch(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
   }
}

void win::present_phys(int x, int y, int dx, int dy)
{
   if (to_window() || dx <= 0 || dy <= 0)
      return;
   XRenderComposite(mydisplay, PictOpSrc, (Picture)src_pic, None, (Picture)dst_pic, x, y, 0, 0, x,
                    y, dx, dy);
}

void win::present()
{
   if (!is_dirty)
      return;
   is_dirty = false;
   {
      int x0 = i_max(0, (int)floor(dirty_x0 * myscale));
      int y0 = i_max(0, (int)floor(dirty_y0 * myscale));
      int x1 = (int)ceil(dirty_x1 * myscale);
      int y1 = (int)ceil(dirty_y1 * myscale);

      present_phys(x0, y0, x1 - x0, y1 - y0);
   }
   XFlush(mydisplay);
}

void win::present_all()
{
   for (int i = 0; i < num_wins; i++)
      all_wins[i]->present();
}

/* Set up XRender pictures for a window that has a buffer and a zoom
   other than 1. Falls back to no zoom if anything is missing. */
void win::init_zoom()
{
   XRenderPictFormat *fmt;
   XRenderPictureAttributes pa;
   XTransform t;
   int ev;
   int err;

   src_pic = 0;
   dst_pic = 0;
   is_dirty = false;
   if (mybuffer == None || myscale == 1.0)
      return;
   fmt = XRenderFindVisualFormat(mydisplay, DefaultVisual(mydisplay, myscreen));
   if (!XRenderQueryExtension(mydisplay, &ev, &err) || fmt == NULL)
   {
      myscale = 1.0;
      return;
   }
   pa.repeat = RepeatPad; /* edges blend with themselves, not black */
   src_pic = XRenderCreatePicture(mydisplay, mybuffer, fmt, CPRepeat, &pa);
   dst_pic = XRenderCreatePicture(mydisplay, mywindow, fmt, 0, NULL);
   memset(&t, 0, sizeof(t));
   t.matrix[0][0] = XDoubleToFixed(1.0 / myscale);
   t.matrix[1][1] = XDoubleToFixed(1.0 / myscale);
   t.matrix[2][2] = XDoubleToFixed(1.0);
   XRenderSetPictureTransform(mydisplay, (Picture)src_pic, &t);
   /* whole-number zoom keeps pixels sharp; other steps are smoothed */
   if (myscale == floor(myscale))
      XRenderSetPictureFilter(mydisplay, (Picture)src_pic, FilterNearest, NULL, 0);
   else
      XRenderSetPictureFilter(mydisplay, (Picture)src_pic, FilterBilinear, NULL, 0);
}

void win::free_zoom()
{
   if (src_pic != 0)
      XRenderFreePicture(mydisplay, (Picture)src_pic);
   if (dst_pic != 0)
      XRenderFreePicture(mydisplay, (Picture)dst_pic);
   src_pic = 0;
   dst_pic = 0;
   is_dirty = false;
}

/* Turn the unzoomed size hints (and, for child windows, position) into
   window coordinates. */
void win::scale_hints()
{
   if (myscale == 1.0)
      return;
   myhint->width = zoom_in(myhint->width);
   myhint->height = zoom_in(myhint->height);
   myhint->min_width = zoom_in(myhint->min_width);
   myhint->min_height = zoom_in(myhint->min_height);
   myhint->max_width = zoom_in(myhint->max_width);
   myhint->max_height = zoom_in(myhint->max_height);
   if (is_child)
   {
      myhint->x = zoom_in(myhint->x);
      myhint->y = zoom_in(myhint->y);
   }
}

/* Change the zoom of an open window: resize it (and move it, if it is a
   child) and show its whole buffer again. */
void win::set_scale(double s)
{
   if (!is_enable || mybuffer == None || s == myscale)
      return;
   free_zoom();
   myscale = s;
   init_zoom();
   myhint->width = zoom_in(w_dx);
   myhint->height = zoom_in(w_dy);
   myhint->min_width = myhint->max_width = myhint->width;
   myhint->min_height = myhint->max_height = myhint->height;
   if (is_child)
      XMoveResizeWindow(mydisplay, mywindow, zoom_in(log_x), zoom_in(log_y), myhint->width,
                        myhint->height);
   else
   {
      XSetWMNormalHints(mydisplay, mywindow, myhint);
      XResizeWindow(mydisplay, mywindow, myhint->width, myhint->height);
   }
   if (to_window())
      XCopyArea(mydisplay, mybuffer, mywindow, mycopy_gc, 0, 0, w_dx, w_dy, 0, 0);
   else
      touch_all();
}

/* Change the zoom of every open window (the start menu's zoom buttons). */
void win::rezoom_all(double s)
{
   for (int i = 0; i < num_wins; i++)
      all_wins[i]->set_scale(s);
   present_all();
}

/*----------------------------------------------------------------------*/
/* CLASS win (funktions)                                                */
/*----------------------------------------------------------------------*/

win::win(const char title[], const char host[], int x, int y, int dx, int dy, bool enable,
         bool resize_enable)

{
   if (!fix_initialized)
   { // init_fix
      fix_initialized = true;
      strcpy(fix_name, ".winfix");
      strcpy(fix_dir, "");
   }
   mybuffer = None;
   myscale = 1.0;
   src_pic = 0;
   dst_pic = 0;
   is_dirty = false;
   is_child = false;
   log_x = 0;
   log_y = 0;
   is_enable = enable;
   if (is_enable)
   { // perform_open
      char full_title[128] = "win:";
      char full_host[128];
      int argc = 0;
      char **argv = NULL;
      XSetWindowAttributes new_attr;
      int log_w;
      int log_h;

      { // init_mouse
         num_mouse_events = 0;
         event_id = 0;
         mouse_x = 0;
         mouse_y = 0;
         mouse_inside = false;
         mouse_press[0] = false;
         mouse_press[1] = false;
         mouse_press[2] = false;
      }
      { // set_parameters
         open_x = -100;
         open_y = -100;
         open_dx = -100;
         open_dy = -100;
         with_fix = false;
         strcpy(name, title);
         strcat(full_title, title);
         { // set_host
            if (strcmp(host, "") == 0)
            {
               const char *env_display = getenv("DISPLAY");

               snprintf(full_host, sizeof(full_host), "%s", env_display != NULL ? env_display : "");
            }
            else
               sprintf(full_host, "%s:0.0", host);
         }
         argc = 0;

         event_mask = ButtonReleaseMask | PropertyChangeMask | ButtonPressMask | KeyPressMask
                      | KeyReleaseMask | ExposureMask | ButtonMotionMask | EnterWindowMask
                      | LeaveWindowMask;

         mydisplay = XOpenDisplay(full_host);
         if (mydisplay == NULL)
         {
            fprintf(stderr, "craft: cannot open X display '%s'%s\n", XDisplayName(full_host),
                    full_host[0] == 0 ? " (is DISPLAY set?)" : "");
            exit(1);
         }
         myscreen = DefaultScreen(mydisplay);
         mybackground = WhitePixel(mydisplay, myscreen);
         myforeground = BlackPixel(mydisplay, myscreen);
         cmap = DefaultColormap(mydisplay, myscreen);
         press_cnt = 0;
         keycount = 0;
         w_dx = dx;
         w_dy = dy;
         myhint = XAllocSizeHints();
         myhint->min_width = 10;
         myhint->min_height = 10;
         myhint->max_width = 1141;
         myhint->max_height = 864;
         myhint->flags = PMinSize | PMaxSize;
         is_alien = false;
         is_child = false;
         myscale = resize_enable ? 1.0 : zoom_current();
      }
      { // perhaps_user_position
         if (x == by_fix || y == by_fix)
         {
            fix_pos(x, y);
            with_fix = true;
         }
         { // perhaps_use_user_pos
            if (x != by_user && y != by_user)
            { // set_to_pos
               myhint->flags |= PPosition;
               myhint->x = x;
               myhint->y = y;
               open_x = x;
               open_y = y;
            }
            else
            { // set_to_external_position
               /* myhint->flags |= USPosition; */
               myhint->x = 400;
               myhint->y = 400;
            }
         }
      }
      { // perhaps_user_size
         if (dx == by_fix || dy == by_fix)
         {
            fix_size(w_dx, w_dy);
            with_fix = true;
            myhint->flags |= PSize;
         }
         { // perhaps_use_user_size
            if (w_dx != by_user && w_dy != by_user)
            { // set_to_size
               myhint->flags |= PSize;
               myhint->width = w_dx;
               myhint->height = w_dy;
               open_dx = w_dx;
               open_dy = w_dy;
            }
            else
            { // set_external
               myhint->flags |= USSize;
               myhint->width = 10;
               myhint->height = 10;
            }
         }
      }
      if (!resize_enable)
      {
         myhint->min_width = myhint->width;
         myhint->max_width = myhint->width;
         myhint->min_height = myhint->height;
         myhint->max_height = myhint->height;
      }
      log_x = myhint->x;
      log_y = myhint->y;
      log_w = myhint->width;
      log_h = myhint->height;
      scale_hints();
      { // create_window
         mywindow
             = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay), myhint->x, myhint->y,
                                   myhint->width, myhint->height, 5, myforeground, mybackground);
      }
      create_color_map();
      { // set_window_attributes
         new_attr.backing_store = Always;
         XChangeWindowAttributes(mydisplay, mywindow, CWBackingStore, &new_attr);
         XSetStandardProperties(mydisplay, mywindow, full_title, title, None, argv, argc, myhint);
         mygc = XCreateGC(mydisplay, mywindow, 0, 0);
         XSetBackground(mydisplay, mygc, mybackground);
         XSetForeground(mydisplay, mygc, myforeground);
         if (resize_enable)
            init_buffer(i_max(log_w, DisplayWidth(mydisplay, myscreen)),
                        i_max(log_h, DisplayHeight(mydisplay, myscreen)));
         else
            init_buffer(log_w, log_h);
         init_zoom();
         XSelectInput(mydisplay, mywindow,
                      PropertyChangeMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                          | KeyPressMask | KeyReleaseMask | ExposureMask | EnterWindowMask
                          | LeaveWindowMask | PointerMotionMask);
         XMapRaised(mydisplay, mywindow);
         XDefineCursor(mydisplay, mywindow, XCreateFontCursor(mydisplay, XC_arrow));
         function(GXcopy);
         set_font(win_default_font);
      }
      { // raise_window
         tick(false);
      }
      { // set_font_info
         XGCValues gcinfo;

         XGetGCValues(mydisplay, mygc, GCFont, &gcinfo);
         font_info = XQueryFont(mydisplay, gcinfo.font);
      }
   }
}

win::win(win *parent, const char title[], char host[], int x, int y, int dx, int dy, bool enable,
         bool resize_enable)

{
   if (!fix_initialized)
   { // init_fix
      fix_initialized = true;
      strcpy(fix_name, ".winfix");
      strcpy(fix_dir, "");
   }
   mybuffer = None;
   myscale = 1.0;
   src_pic = 0;
   dst_pic = 0;
   is_dirty = false;
   is_child = false;
   log_x = 0;
   log_y = 0;
   is_enable = enable;
   if (is_enable)
   { // perform_open
      char full_title[128] = "win:";
      char full_host[128];
      int argc = 0;
      char **argv = NULL;
      XSetWindowAttributes new_attr;
      int log_w;
      int log_h;

      { // init_mouse
         num_mouse_events = 0;
         event_id = 0;
         mouse_x = 0;
         mouse_y = 0;
         mouse_inside = false;
         mouse_press[0] = false;
         mouse_press[1] = false;
         mouse_press[2] = false;
      }
      { // set_parameters
         open_x = -100;
         open_y = -100;
         open_dx = -100;
         open_dy = -100;
         with_fix = false;
         strcpy(name, title);
         strcat(full_title, title);
         { // set_host
            if (strcmp(host, "") == 0)
            {
               const char *env_display = getenv("DISPLAY");

               snprintf(full_host, sizeof(full_host), "%s", env_display != NULL ? env_display : "");
            }
            else
               sprintf(full_host, "%s:0.0", host);
         }
         argc = 0;
         event_mask = ButtonReleaseMask | PropertyChangeMask | ButtonPressMask | KeyPressMask
                      | KeyReleaseMask | ExposureMask | ButtonMotionMask | EnterWindowMask
                      | LeaveWindowMask;

         mydisplay = parent->mydisplay;
         myscreen = parent->myscreen;
         mybackground = parent->mybackground;
         myforeground = parent->myforeground;
         cmap = parent->cmap;
         press_cnt = 0;
         keycount = 0;
         w_dx = dx;
         w_dy = dy;
         myhint = XAllocSizeHints();
         myhint->min_width = 10;
         myhint->min_height = 10;
         myhint->max_width = 1141;
         myhint->max_height = 864;
         myhint->flags = PMinSize | PMaxSize;
         is_alien = true;
         is_child = true;
         myscale = parent->myscale;
      }
      { // perhaps_user_position
         if (x == by_fix || y == by_fix)
         {
            fix_pos(x, y);
            with_fix = true;
         }
         { // perhaps_use_user_pos
            if (x != by_user && y != by_user)
            { // set_to_pos
               myhint->flags |= PPosition;
               myhint->x = x;
               myhint->y = y;
               open_x = x;
               open_y = y;
            }
            else
            { // set_to_external_position
               /* myhint->flags |= USPosition; */
               myhint->x = 400;
               myhint->y = 400;
            }
         }
      }
      { // perhaps_user_size
         if (dx == by_fix || dy == by_fix)
         {
            fix_size(w_dx, w_dy);
            with_fix = true;
            myhint->flags |= PSize;
         }
         { // perhaps_use_user_size
            if (w_dx != by_user && w_dy != by_user)
            { // set_to_size
               myhint->flags |= PSize;
               myhint->width = w_dx;
               myhint->height = w_dy;
               open_dx = w_dx;
               open_dy = w_dy;
            }
            else
            { // set_external
               myhint->flags |= USSize;
               myhint->width = 10;
               myhint->height = 10;
            }
         }
      }
      if (!resize_enable)
      {
         myhint->min_width = myhint->width;
         myhint->max_width = myhint->width;
         myhint->min_height = myhint->height;
         myhint->max_height = myhint->height;
      }
      create_color_map();
      log_x = myhint->x;
      log_y = myhint->y;
      log_w = myhint->width;
      log_h = myhint->height;
      scale_hints();
      { // create_window
         mywindow
             = XCreateSimpleWindow(mydisplay, parent->mywindow, myhint->x, myhint->y, myhint->width,
                                   myhint->height, 0, myforeground, mybackground);
      }
      { // set_window_attributes
         new_attr.backing_store = Always;
         XChangeWindowAttributes(mydisplay, mywindow, CWBackingStore, &new_attr);
         XSetStandardProperties(mydisplay, mywindow, full_title, title, None, argv, argc, myhint);
         mygc = XCreateGC(mydisplay, mywindow, 0, 0);
         XSetBackground(mydisplay, mygc, mybackground);
         XSetForeground(mydisplay, mygc, myforeground);
         if (resize_enable)
            init_buffer(i_max(log_w, DisplayWidth(mydisplay, myscreen)),
                        i_max(log_h, DisplayHeight(mydisplay, myscreen)));
         else
            init_buffer(log_w, log_h);
         init_zoom();
         XSelectInput(mydisplay, mywindow,
                      PropertyChangeMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                          | KeyPressMask | KeyReleaseMask | ExposureMask | EnterWindowMask
                          | LeaveWindowMask | PointerMotionMask);
         XMapRaised(mydisplay, mywindow);
         XDefineCursor(mydisplay, mywindow, XCreateFontCursor(mydisplay, XC_arrow));
         function(GXcopy);
         set_font(win_default_font);
      }
      { // raise_window
         tick(false);
      }
      { // set_font_info
         XGCValues gcinfo;

         XGetGCValues(mydisplay, mygc, GCFont, &gcinfo);
         font_info = XQueryFont(mydisplay, gcinfo.font);
      }
   }
}

win::win(char title[])
{
   mydisplay = XOpenDisplay("");
   myscreen = DefaultScreen(mydisplay);
   cmap = DefaultColormap(mydisplay, myscreen);
   mybuffer = None;
   myscale = 1.0;
   src_pic = 0;
   dst_pic = 0;
   is_dirty = false;
   is_child = false;
   mywindow = grab(mydisplay, RootWindow(mydisplay, myscreen), title);
   is_enable = (mywindow != 0);
   if (is_enable)
      mygc = XCreateGC(mydisplay, mywindow, 0, 0);
   is_alien = true;
}

Window win::grab(Display *dsp, Window wnd, char name[])
{
   Window *children, dummy;
   unsigned int nchildren;
   int i;
   Window w = 0;
   char *window_name;

   if (XFetchName(dsp, wnd, &window_name) && !strcmp(window_name, name))
      return (wnd);

   if (!XQueryTree(dsp, wnd, &dummy, &dummy, &children, &nchildren))
      return (0);

   for (i = 0; i < (int)nchildren; i++)
   {
      w = grab(dsp, children[i], name);
      if (w)
         break;
   }
   if (children)
      XFree((char *)children);
   return (w);
}

win::~win()
{
   free_buffer();
   if (is_alien)
   { // close_alien
      if (is_enable)
         XDestroyWindow(mydisplay, mywindow);
   }
   else if (is_enable)
   {    // close_own
      { // perhaps_fix
         if (x() != open_x || y() != open_y || dx() != open_dx || dy() != open_dy)
            fix();
      }
      tick(false);
      XFreeGC(mydisplay, mygc);
      XFree(myhint);
      XDestroyWindow(mydisplay, mywindow);
      { // forget_windows_on_this_display
         /* Child windows that were never deleted share this connection;
            closing it frees their pixmaps on the server side. */
         for (int i = num_wins - 1; i >= 0; i--)
            if (all_wins[i]->mydisplay == mydisplay)
            {
               all_wins[i]->mybuffer = None;
               all_wins[i]->src_pic = 0;
               all_wins[i]->dst_pic = 0;
               all_wins[i] = all_wins[--num_wins];
            }
      }
      XCloseDisplay(mydisplay);
   }
}

#undef red
#undef green
#undef blue

void win::alloc_color(const char name[], int no)
{
   XColor c;
   XColor d;

   XAllocNamedColor(mydisplay, cmap, name, &c, &d);
   colors[no] = c.pixel;
   color_r[no] = c.red / 256;
   color_g[no] = c.green / 256;
   color_b[no] = c.blue / 256;
}

void win::create_color_map()
{
   alloc_color("red", 1);
   alloc_color("red1", 2);
   alloc_color("red2", 3);
   alloc_color("red3", 4);
   alloc_color("red4", 5);
   alloc_color("sienna", 6);
   alloc_color("sienna1", 7);
   alloc_color("sienna2", 8);
   alloc_color("sienna3", 9);
   alloc_color("sienna4", 10);
   alloc_color("yellow", 11);
   alloc_color("yellow1", 12);
   alloc_color("yellow2", 13);
   alloc_color("yellow3", 14);
   alloc_color("yellow4", 15);
   alloc_color("green", 16);
   alloc_color("green1", 17);
   alloc_color("green2", 18);
   alloc_color("green3", 19);
   alloc_color("green4", 20);
   alloc_color("blue", 21);
   alloc_color("blue1", 22);
   alloc_color("blue2", 23);
   alloc_color("blue3", 24);
   alloc_color("blue4", 25);
   alloc_color("SlateGray", 26);
   alloc_color("SlateGray1", 27);
   alloc_color("SlateGray2", 28);
   alloc_color("SlateGray3", 29);
   alloc_color("SlateGray4", 30);
   alloc_color("DarkGreen", 31);
   alloc_color("SeaGreen", 32);
   alloc_color("SeaGreen1", 33);
   alloc_color("SeaGreen2", 34);
   alloc_color("SeaGreen3", 35);
   alloc_color("cyan", 36);
   alloc_color("cyan1", 37);
   alloc_color("cyan2", 38);
   alloc_color("cyan3", 39);
   alloc_color("cyan4", 40);
   alloc_color("purple", 41);
   alloc_color("purple1", 42);
   alloc_color("purple2", 43);
   alloc_color("purple3", 44);
   alloc_color("purple4", 45);
   alloc_color("gold", 46);
   alloc_color("gold1", 47);
   alloc_color("gold2", 48);
   alloc_color("gold3", 49);
   alloc_color("gold4", 50);
   alloc_color("gray", 51);
   alloc_color("gray10", 52);
   alloc_color("gray20", 53);
   alloc_color("gray30", 54);
   alloc_color("gray40", 55);
   alloc_color("gray50", 56);
   alloc_color("gray60", 57);
   alloc_color("gray70", 58);
   alloc_color("gray80", 59);
   alloc_color("gray90", 60);
   alloc_color("gray100", 61);
   alloc_color("NavyBlue", 62);
   alloc_color("SaddleBrown", 63);
   last_r = -1;
}

void win::tick()
{
   tick(false);
}

void win::tick(bool pause)
{
   bool first = true;
   XEvent myevent;

   repaint_exposed();
   if (is_enable)
   { // handle_tick
      bool any_event;

      { // get_event
         any_event = true;
         if (!(pause && first))
            any_event = XCheckWindowEvent(mydisplay, mywindow, event_mask, &myevent);
         else
            XWindowEvent(mydisplay, mywindow, event_mask, &myevent);
      }
      while (any_event)
      { // handle_event
         first = false;
         switch (myevent.type)
         {
         case Expose:
            repaint(myevent.xexpose.x, myevent.xexpose.y, myevent.xexpose.width,
                    myevent.xexpose.height);
            break;
         case PropertyNotify:
            break;
         case ResizeRequest:
            break;
         case ButtonRelease:
         { // handle_mouse_button_release
            switch (((XButtonReleasedEvent *)&myevent)->button)
            {
            case Button1:
               mouse_event[num_mouse_events] = button1release;
               mouse_press[0] = false;
               break;
            case Button2:
               mouse_event[num_mouse_events] = button2release;
               mouse_press[1] = false;
               break;
            case Button3:
               mouse_event[num_mouse_events] = button3release;
               mouse_press[2] = false;
               break;
            }
            mouse_inside = true;
            mouse_x = zoom_out(myevent.xbutton.x);
            mouse_y = zoom_out(myevent.xbutton.y);
            mouse_ex[num_mouse_events] = mouse_x;
            mouse_ey[num_mouse_events] = mouse_y;
            num_mouse_events = i_min(max_mouse_events - 1, num_mouse_events + 1);
         };
         break;
         case ButtonPress:
         { // handle_mouse_button_press
            switch (((XButtonPressedEvent *)&myevent)->button)
            {
            case Button1:
               mouse_event[num_mouse_events] = button1press;
               mouse_press[0] = true;
               break;
            case Button2:
               mouse_event[num_mouse_events] = button2press;
               mouse_press[1] = true;
               break;
            case Button3:
               mouse_event[num_mouse_events] = button3press;
               mouse_press[2] = true;
               break;
            }
            mouse_inside = true;
            mouse_x = zoom_out(myevent.xbutton.x);
            mouse_y = zoom_out(myevent.xbutton.y);
            mouse_ex[num_mouse_events] = mouse_x;
            mouse_ey[num_mouse_events] = mouse_y;
            mouse_eid[num_mouse_events] = event_id++;
            num_mouse_events = i_min(max_mouse_events - 1, num_mouse_events + 1);
         };
         break;
         case GraphicsExpose: /*jupp di dei*/
            break;
         case MappingNotify:
            XRefreshKeyboardMapping((XMappingEvent *)&myevent);
            break;
         case KeyPress:
         { // handle_press_key
            char instring[10];

            press_cnt++;
            { // decode_input
               KeySym mykey;

               XLookupString((XKeyEvent *)&myevent, instring, 10, &mykey, 0);
               act_key = ((XButtonPressedEvent *)&myevent)->button;
               if (XKeysymToString(mykey))
                  strcpy(cntlbuffer[keycount], XKeysymToString(mykey));
            }
            { // push_to_queue
               inbuffer[keycount] = instring[0];
               keybuffer[keycount] = act_key;
               keycount = i_min(keybuf_size, keycount + 1);
            }
            { // perhaps_skip_shift
               if (act_key == 106)
                  keycount--;
            }
         };
         break;
         case KeyRelease:
         { // handle_release_key
            press_cnt--;
         };
         break;
         case EnterNotify:
         { // handle_enter
            mouse_inside = true;
         };
         break;
         case LeaveNotify:
         { // handle_leave
            mouse_inside = false;
         };
         break;
         case MotionNotify:
         { // send_motion_coords
            mouse_x = zoom_out(((XMotionEvent *)&myevent)->x);
            mouse_y = zoom_out(((XMotionEvent *)&myevent)->y);
            mouse_inside = true;
         };
         break;
         default:
         { // send_coords

            /*
     mouse_x      = myevent.xbutton.x;
     mouse_y      = myevent.xbutton.y;

     mouse_inside = true;
*/
         };
         break;
         }
         { // get_event
            any_event = true;
            if (!(pause && first))
               any_event = XCheckWindowEvent(mydisplay, mywindow, event_mask, &myevent);
            else
               XWindowEvent(mydisplay, mywindow, event_mask, &myevent);
         }
      }
   }
}

void win::iconify()
{
   XIconifyWindow(mydisplay, mywindow, myscreen);
}

void win::xsync()
{
   present_all();
   XSync(mydisplay, true);
}

int win::getkey()
{
   if (press_cnt <= 0)
      return 0;
   else
      return act_key;
}

char win::inchar()
{
   int d;

   return inchar(d);
}

char win::inchar(int &key)
{
   char cntl[32];

   return inchar(key, cntl);
}

char win::inchar(int &key, char *cntl)
{
   if (keycount <= 0)
   { // handle_no_char
      key = 0;
      cntl[0] = 0;
      return 0;
   }
   else
   { // get_top_of_queue
      char c;

      c = inbuffer[0];
      key = keybuffer[0];
      strcpy(cntl, cntlbuffer[0]);
      { // shift_rest
         delchar(inbuffer, 0);
         for (int i = 0; i < keybuf_size - 1; i++)
         {
            keybuffer[i] = keybuffer[i + 1];
            strcpy(cntlbuffer[i], cntlbuffer[i + 1]);
         }
         keycount--;
      }
      return c;
   }
}

bool win::on()
{
   return mouse_inside;
}

void win::mark_mouse()
{
   event_mark = event_id;
}

void win::scratch_mouse()
{
   while (num_mouse_events > 0 && mouse_eid[num_mouse_events - 1] < event_mark)
      num_mouse_events--;
}

bool win::mouse_is_pressed(int button)
{
   return mouse_press[button];
}

bool win::mouse(int &x, int &y, int &button)
{
   int ex;
   int ey;

   return (mouse(x, y, ex, ey, button));
}

bool win::mouse(int &x, int &y, int &xe, int &ye, int &button)
{

   if (is_enable)
   { // handle_mouse
      if (num_mouse_events > 0)
      { // grab_event
         num_mouse_events--;
         x = mouse_x;
         y = mouse_y;
         xe = mouse_ex[0];
         ye = mouse_ey[0];
         button = mouse_event[0];
         { // shift_buffer
            for (int i = 0; i < max_mouse_events - 1; i++)
            {
               mouse_ex[i] = mouse_ex[i + 1];
               mouse_ey[i] = mouse_ey[i + 1];
               mouse_event[i] = mouse_event[i + 1];
            }
         }
         return true;
      }
      else
      { // grab_pos
         x = mouse_x;
         y = mouse_y;
         button = nobutton;
         return false;
      }
      return false;
   }
   else
      return false;
}

bool win::is_mouse(int &xe, int &ye, int &button)
{
   if (is_enable && num_mouse_events > 0)
   { // handle_mouse
      xe = mouse_ex[0];
      ye = mouse_ey[0];
      button = mouse_event[0];
      return true;
   }
   else
   { // handle_no_mouse
      xe = mouse_x;
      ye = mouse_y;
      return false;
   }
}

void win::set_cursor(char name[])
{
   int xh;
   int yh;
   Pixmap cur;
   Pixmap mask;

   { // load_pixmaps
      unsigned int dx;
      unsigned int dy;
      char cur_name[128];
      char mask_name[128];

      sprintf(cur_name, "%s.cursor", name);
      sprintf(mask_name, "%s.mask", name);
      XReadBitmapFile(mydisplay, mywindow, cur_name, &dx, &dy, &cur, &xh, &yh);
      XReadBitmapFile(mydisplay, mywindow, mask_name, &dx, &dy, &mask, &xh, &yh);
   }
   /*  XDefineCursor (mydisplay, mywindow, new_cursor);
*/

   /*
     XCreatePixmapCursor (mydisplay,
                          cur, mask,
                          colors [0], colors [63],
                          xhot, yhot).
*/
}

void win::set_cursor(int cursor)
{
   if (is_enable)
   { // perform_set_cursor
      Cursor c;

      c = XCreateFontCursor(mydisplay, cursor);
      XDefineCursor(mydisplay, mywindow, c);
   }
}

void win::set_background(int color)
{
   int c = colors[color];

   if (color == 0)
      c = mybackground;
   if (color == 63)
      c = myforeground;
   XSetBackground(mydisplay, mygc, c);
}

void win::win_rgb(int color, int &r, int &g, int &b)
{
   if (color == white)
   {
      r = 1;
      g = 1;
      b = 1;
   }
   else if (color == black)
   {
      r = 0;
      g = 0;
      b = 0;
   }
   else
   {
      r = color_r[color];
      g = color_g[color];
      b = color_b[color];
   }
}

int win::win_color(int r, int g, int b)
{
   if (r == last_r && g == last_g && b == last_b)
      return last_best_color;
   else
   { // search_best_color
      double best_dist;
      int best_color = 0;

      { // init
         best_dist = DBL_MAX;
      }
      { // check_colors
         for (int i = 1; i < max_colors; i++)
         { // check_color
            int d;

            { // calc_d
               int dr = color_r[i] - r;
               int dg = color_g[i] - g;
               int db = color_b[i] - b;

               d = dr * dr + dg * dg + db * db;
            }
            if (d < best_dist)
            { // grab_color
               best_dist = d;
               best_color = i;
            }
         }
         { // check_black
            int d = r * r + g * g + b * b;

            if (d < best_dist)
            {
               best_dist = d;
               best_color = 63;
            }
         }
         { // check_white
            int d = (255 - r) * (255 - r) + (255 - g) * (255 - g) + (255 - b) * (255 - b);

            if (d < best_dist)
            {
               best_dist = d;
               best_color = 0;
            }
         }
      }
      { // store_result
         last_r = r;
         last_g = g;
         last_b = b;
         last_best_color = best_color;
      }
      return best_color;
   }
}

void win::set_color(int r, int g, int b)
{
   set_color(win_color(r, g, b));
}

void win::set_color(int color)
{
   if (is_enable)
   { // perform_set_color
      unsigned long c = colors[color];
      XGCValues gcval;

      if (color == 0)
         c = mybackground;
      if (color == 63)
         c = myforeground;
      gcval.foreground = c;
      XChangeGC(mydisplay, mygc, GCForeground, &gcval);
   }
}

void win::function(int func)
{
   XGCValues gcval;

   if (is_enable)
   { // perform_function
      gcval.function = func;
      XChangeGC(mydisplay, mygc, GCFunction, &gcval);
   }
}

void win::pixel(int x, int y)
{
   if (is_enable)
   { // perform_draw
      if (to_window())
         XDrawPoint(mydisplay, mywindow, mygc, x, y);
      if (mybuffer != None)
         XDrawPoint(mydisplay, mybuffer, mygc, x, y);
      touch(x, y, 1, 1);
   }
}

void win::pixel(Pixmap p, int x, int y)
{
   if (is_enable)
   { // perform_draw
      XDrawPoint(mydisplay, p, mygc, x, y);
   }
}

void win::draw(polyline *p)
{
   if (is_enable)
   { // perform_draw
      if (to_window())
         XDrawLines(mydisplay, mywindow, mygc, p->p, p->n, CoordModeOrigin);
      if (mybuffer != None)
         XDrawLines(mydisplay, mybuffer, mygc, p->p, p->n, CoordModeOrigin);
      touch_poly(p);
   }
}

void win::line(int x1, int y1, int x2, int y2)
{
   if (is_enable)
   { // perform_line
      if (to_window())
         XDrawLine(mydisplay, mywindow, mygc, x1, y1, x2, y2);
      if (mybuffer != None)
         XDrawLine(mydisplay, mybuffer, mygc, x1, y1, x2, y2);
      touch(i_min(x1, x2), i_min(y1, y2), i_abs(x2 - x1) + 1, i_abs(y2 - y1) + 1);
   }
}

void win::box(int x1, int y1, int x2, int y2)
{
   line(x1, y1, x2, y1);
   line(x2, y1, x2, y2);
   line(x2, y2, x1, y2);
   line(x1, y2, x1, y1);
}

void win::move(int x, int y, int x1, int y1, int dx, int dy)
{
   if (mybuffer != None)
   {
      XCopyArea(mydisplay, mybuffer, mybuffer, mygc, x, y, dx, dy, x1, y1);
      if (to_window())
         XCopyArea(mydisplay, mybuffer, mywindow, mygc, x1, y1, dx, dy, x1, y1);
      touch(x1, y1, dx, dy);
   }
   else
      XCopyArea(mydisplay, mywindow, mywindow, mygc, x, y, dx, dy, x1, y1);
}

void win::load_map(char name[], Pixmap &m, int &dx, int &dy)
{
   { // check_file
      if (!f_exists(name))
         errorstop(2, "WIN", "load_map with unknown map", name);
   }
   if (is_enable)
   { // perform_load
      unsigned int dxx;
      unsigned int dyy;
      int xh;
      int yh;

      XReadBitmapFile(mydisplay, mywindow, name, &dxx, &dyy, &m, &xh, &yh);
      dx = dxx;
      dy = dyy;
   }
}

void win::store_map(char name[], int x, int y, int dx, int dy)
{
   Pixmap p;

   p = XCreatePixmap(mydisplay, mywindow, dx, dy, 8);
   function(GXcopy);
   XCopyArea(mydisplay, mybuffer != None ? (Drawable)mybuffer : (Drawable)mywindow, p, mygc, x, y,
             dx, dy, 0, 0);
   XWriteBitmapFile(mydisplay, name, p, dx, dy, 0, 0);
}

void win::store_map(Pixmap &p, int x, int y, int dx, int dy)
{
   p = XCreatePixmap(mydisplay, mywindow, dx, dy, DefaultDepth(mydisplay, myscreen));
   function(GXcopy);
   XCopyArea(mydisplay, mybuffer != None ? (Drawable)mybuffer : (Drawable)mywindow, p, mygc, x, y,
             dx, dy, 0, 0);
}

void win::store_map(Pixmap &p, int x, int y, int dx, int dy, Pixmap mask)
{
   p = XCreatePixmap(mydisplay, mywindow, dx, dy, DefaultDepth(mydisplay, myscreen));
   function(GXcopy);
   XSetClipMask(mydisplay, mygc, mask);
   XSetClipOrigin(mydisplay, mygc, 0, 0);
   XCopyArea(mydisplay, mybuffer != None ? (Drawable)mybuffer : (Drawable)mywindow, p, mygc, x, y,
             dx, dy, 0, 0);
   XSetClipMask(mydisplay, mygc, None);
}

void win::set_clip(int x, int y, int dx, int dy)
{
   XRectangle clips[1];

   clips[0].x = x;
   clips[0].y = y;
   clips[0].width = (short int)dx;
   clips[0].height = (short int)dy;
   XSetClipRectangles(mydisplay, mygc, 0, 0, clips, 1, 0);
}

void win::show_map(int x, int y, char name[])
{
   { // check_file
      if (!f_exists(name))
         errorstop(2, "WIN", "load_map with unknown map", name);
   }
   if (is_enable)
   { // perform_show
      Pixmap p;
      unsigned int dx;
      unsigned int dy;
      int xh;
      int yh;

      { // read_pixmap
         XReadBitmapFile(mydisplay, mywindow, name, &dx, &dy, &p, &xh, &yh);
      }
      { // show_pixmap
         if (to_window())
            XCopyPlane(mydisplay, p, mywindow, mygc, 0, 0, dx, dy, x, y, 1L);
         if (mybuffer != None)
            XCopyPlane(mydisplay, p, mybuffer, mygc, 0, 0, dx, dy, x, y, 1L);
         touch(x, y, dx, dy);
      }
   }
}

void win::show_map(int x, int y, Pixmap m, int dx, int dy)
{
   show_map(x, y, 0, 0, m, dx, dy);
}

void win::show_map(int x, int y, int src_x, int src_y, Pixmap m, int dx, int dy)
{
   if (is_enable)
   {    // perform_show
      { // show_pixmap
         if (to_window())
            XCopyArea(mydisplay, m, mywindow, mygc, src_x, src_y, dx, dy, x, y);
         if (mybuffer != None)
            XCopyArea(mydisplay, m, mybuffer, mygc, src_x, src_y, dx, dy, x, y);
         touch(x, y, dx, dy);
      }
   }
}

void win::show_map(int x, int y, Pixmap m, int dx, int dy, Pixmap mask)
{
   show_map(x, y, m, dx, dy, mask, x, y);
}

void win::show_map(int x, int y, Pixmap m, int dx, int dy, Pixmap mask, int clip_x, int clip_y)

{
   if (is_enable)
   { // perform_show
      int src_x;
      int src_y;
      int dest_x;
      int dest_y;
      int dest_dx;
      int dest_dy;

      { // calc_params
         if (clip_x <= x)
         {
            src_x = 0;
            dest_x = x;
            dest_dx = dx - (x - clip_x);
         }
         else
         {
            src_x = clip_x - x;
            dest_x = clip_x;
            dest_dx = dx - (clip_x - x);
         }

         if (clip_y <= y)
         {
            src_y = 0;
            dest_y = y;
            dest_dy = dy - (y - clip_y);
         }
         else
         {
            src_y = clip_y - y;
            dest_y = clip_y;
            dest_dy = dy - (clip_y - y);
         }
      }
      XSetClipMask(mydisplay, mygc, mask);
      XSetClipOrigin(mydisplay, mygc, x, y);
      if (to_window())
         XCopyArea(mydisplay, m, mywindow, mygc, src_x, src_y, dest_dx, dest_dy, dest_x, dest_y);
      if (mybuffer != None)
         XCopyArea(mydisplay, m, mybuffer, mygc, src_x, src_y, dest_dx, dest_dy, dest_x, dest_y);
      touch(dest_x, dest_y, dest_dx, dest_dy);
      XSetClipMask(mydisplay, mygc, None);
   }
}

void win::show_map(Pixmap m, Pixmap mask, int x_source, int y_source, int x_screen, int y_screen,
                   int dx, int dy, int x_mask, int y_mask)

{
   if (is_enable)
   { // perform_show
      XSetClipMask(mydisplay, mygc, mask);
      XSetClipOrigin(mydisplay, mygc, x_mask, y_mask);
      if (to_window())
         XCopyArea(mydisplay, m, mywindow, mygc, x_source, y_source, dx, dy, x_screen, y_screen);
      if (mybuffer != None)
         XCopyArea(mydisplay, m, mybuffer, mygc, x_source, y_source, dx, dy, x_screen, y_screen);
      touch(x_screen, y_screen, dx, dy);
      XSetClipMask(mydisplay, mygc, None);
   }
}

void win::create_map(Pixmap &p, int dx, int dy)
{
   p = XCreatePixmap(mydisplay, mywindow, dx, dy, DefaultDepth(mydisplay, myscreen));
}

void win::delete_map(Pixmap &m)
{
   XFreePixmap(mydisplay, m);
}

void win::fill(polyline *p)
{
   if (is_enable)
   { // perform_fill
      if (to_window())
         XFillPolygon(mydisplay, mywindow, mygc, p->p, p->n, Convex, CoordModeOrigin);
      if (mybuffer != None)
         XFillPolygon(mydisplay, mybuffer, mygc, p->p, p->n, Convex, CoordModeOrigin);
      touch_poly(p);
   }
}

void win::fill(int x1, int y1, int dx, int dy)
{
   if (is_enable)
   { // perform_fill
      if (to_window())
         XFillRectangle(mydisplay, mywindow, mygc, x1, y1, dx, dy);
      if (mybuffer != None)
         XFillRectangle(mydisplay, mybuffer, mygc, x1, y1, dx, dy);
      touch(x1, y1, dx, dy);
   }
}

void win::clear()
{
   if (is_enable)
   { // perform_clear
      fill(0, 0, w_dx, w_dy);
   }
}

void win::shift(int x1, int y1, int x2, int y2, int dx, int dy)
{
   if (is_enable)
   { // perform_shift
      if (mybuffer != None)
      {
         XCopyArea(mydisplay, mybuffer, mybuffer, mygc, x1, y1, dx, dy, x2, y2);
         if (to_window())
            XCopyArea(mydisplay, mybuffer, mywindow, mygc, x2, y2, dx, dy, x2, y2);
         touch(x2, y2, dx, dy);
      }
      else
         XCopyArea(mydisplay, mywindow, mywindow, mygc, x1, y1, dx, dy, x2, y2);
   }
}

void win::text_size(const char string[], int &dx, int &dy)
{
   if (is_enable)
   { // perform_get_size
      XGCValues gcinfo;

      XGetGCValues(mydisplay, mygc, GCFont, &gcinfo);
      dx = XTextWidth(font_info, string, strlen(string));
      dy = font_info->max_bounds.width;
   }
}

void win::set_font(const char name[])
{
   if (is_enable)
   { // perform_load
      Font f;

      f = XLoadFont(mydisplay, name);
      XSetFont(mydisplay, mygc, f);
      font_info = XLoadQueryFont(mydisplay, name);
   }
}

void win::write(int x, int y, double d)
{
   char s[128];

   sprintf(s, "%f   ", d);
   write(x, y, s);
}

void win::write(int x, int y, int d)
{
   char s[128];

   sprintf(s, "%d   ", d);
   write(x, y, s);
}

void win::write(int x, int y, const char string[])
{
   if (is_enable)
   { // perform_write
      if (to_window())
         XDrawImageString(mydisplay, mywindow, mygc, x, y, string, strlen(string));
      if (mybuffer != None)
         XDrawImageString(mydisplay, mybuffer, mygc, x, y, string, strlen(string));
      if (font_info != NULL)
         touch(x + font_info->min_bounds.lbearing, y - font_info->max_bounds.ascent,
               XTextWidth(font_info, string, strlen(string)) + font_info->max_bounds.width,
               font_info->max_bounds.ascent + font_info->max_bounds.descent);
      else
         touch_all();
   }
}

/* tvtwm

int win::x ()
  {if   (is_enable)
        calc_x
   else return 0;

.  calc_x
     {XWindowAttributes attr;
      Window            parent;
      Window            *dd [100];
      Window            d;
      unsigned int      di;

      XQueryTree           (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      while (parent == DefaultRootWindow (mydisplay))
        XQueryTree (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      XGetWindowAttributes (mydisplay, parent, &attr);
      return attr.x;
     }.

  }

int win::y ()
  {if   (is_enable)
        calc_y
   else return 0;

.  calc_y
     {XWindowAttributes attr;
      Window            parent;
      Window            *dd [100];
      Window            d;
      unsigned int      di;

      XQueryTree           (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      while (parent == DefaultRootWindow (mydisplay))
        XQueryTree (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      XGetWindowAttributes (mydisplay, parent, &attr);
      return attr.y;
     }.

  }

int win::dx ()
  {if   (is_enable)
        calc_x
   else return 0;

.  calc_x
     {XWindowAttributes attr;
      Window            parent;
      Window            *dd [100];
      Window            d;
      unsigned int      di;

      XQueryTree           (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      while (parent == DefaultRootWindow (mydisplay))
        XQueryTree (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      XGetWindowAttributes (mydisplay, parent, &attr);
      return attr.width;
     }.

  }

int win::dy ()
  {if   (is_enable)
        calc_x
   else return 0;

.  calc_x
     {XWindowAttributes attr;
      Window            parent;
      Window            *dd [100];
      Window            d;
      unsigned int      di;

      XQueryTree (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      while (parent == DefaultRootWindow (mydisplay))
        XQueryTree (mydisplay, mywindow, &d, &parent, &dd [0], &di);
      XGetWindowAttributes (mydisplay, parent, &attr);
      return attr.height;
     }.

  }

tvtwm */

int win::x()
{
   if (is_enable)
   { // calc_x
      XWindowAttributes attr;
      Window parent;
      Window *dd[100];
      Window d;
      unsigned int di;
      Window par = mywindow;

      XQueryTree(mydisplay, mywindow, &d, &parent, &dd[0], &di);
      while (parent)
      {
         par = parent;
         XQueryTree(mydisplay, par, &d, &parent, &dd[0], &di);
      }
      XGetWindowAttributes(mydisplay, par, &attr);
      return attr.x;
   }
   else
      return 0;
}

int win::y()
{
   if (is_enable)
   { // calc_y
      XWindowAttributes attr;
      Window parent;
      Window *dd[100];
      Window d;
      unsigned int di;
      Window par = mywindow;

      XQueryTree(mydisplay, mywindow, &d, &parent, &dd[0], &di);
      while (parent)
      {
         par = parent;
         XQueryTree(mydisplay, par, &d, &parent, &dd[0], &di);
      }
      XGetWindowAttributes(mydisplay, par, &attr);
      return attr.y;
   }
   else
      return 0;
}

int win::dx()
{
   if (is_enable)
   { // calc_dx
      XWindowAttributes attr;

      XGetWindowAttributes(mydisplay, mywindow, &attr);
      return (int)floor(attr.width / myscale + 0.5);
   }
   else
      return 0;
}

int win::dy()
{
   if (is_enable)
   { // calc_dy
      XWindowAttributes attr;

      XGetWindowAttributes(mydisplay, mywindow, &attr);
      return (int)floor(attr.height / myscale + 0.5);
   }
   else
      return 0;
}

void win::fix()
{
   table *fixes;
   int entry;

   if (with_fix)
   {    // perform_fix
      { // open_fixes
         bool is_new;

         fixes = new table(fix_dir, fix_name, is_new);
         if (is_new)
         { // handle_dd
            fixes->add_column("name", col_type_text);
            fixes->add_column("x", col_type_int);
            fixes->add_column("y", col_type_int);
            fixes->add_column("dx", col_type_int);
            fixes->add_column("dy", col_type_int);
         }
      }
      { // get_entry
         for (entry = 0; entry < fixes->number_of_rows; entry++)
            if (strcmp(fixes->read_text(entry, 0), name) == 0)
               break;
         if (entry == fixes->number_of_rows)
            entry = fixes->add_row();
      }
      { // modify_entry
         fixes->write(entry, 0, name);
         fixes->write(entry, 1, x());
         fixes->write(entry, 2, y());
         fixes->write(entry, 3, dx());
         fixes->write(entry, 4, dy());
      }
      delete (fixes);
   }
}

void win::fix_pos(int &x, int &y)
{
   table *fixes;
   int e;

   { // open_fixes
      bool d;

      fixes = new table(fix_dir, fix_name, d);
   }
   { // get_entry
      for (e = 0; e < fixes->number_of_rows; e++)
         if (strcmp(fixes->read_text(e, 0), name) == 0)
            break;
   }
   { // get_pos
      if (e != fixes->number_of_rows)
      { // read_data
         x = fixes->read_int(e, 1);
         y = fixes->read_int(e, 2);
         open_x = x;
         open_y = y;
      }
      else
      { // no_data
         x = by_user;
         y = by_user;
      }
   }
   delete (fixes);
}

void win::fix_size(int &dx, int &dy)
{
   table *fixes;
   int e;

   { // open_fixes
      bool d;

      fixes = new table(fix_dir, fix_name, d);
   }
   { // get_entry
      for (e = 0; e < fixes->number_of_rows; e++)
         if (strcmp(fixes->read_text(e, 0), name) == 0)
            break;
   }
   { // get_pos
      if (e != fixes->number_of_rows)
      { // read_data
         w_dx = fixes->read_int(e, 3);
         w_dy = fixes->read_int(e, 4);
         open_dx = w_dx;
         open_dy = w_dy;
      }
      else
      { // no_data
         dx = by_user;
         dy = by_user;
      }
   }
   delete (fixes);
}

void win::get_image(XImage *&i, int x, int y, int dx, int dy)
{
   if (!is_alien)
   { // do_some_sync
      tick();
      xsync();
   }
   i = XGetImage(mydisplay, mybuffer != None ? (Drawable)mybuffer : (Drawable)mywindow, x, y, dx,
                 dy, AllPlanes, XYPixmap);
   is_last_pixel = false;
}

void win::put_image(XImage *i, int x, int y, int dx, int dy)
{
   if (to_window())
      XPutImage(mydisplay, mywindow, mygc, i, 0, 0, x, y, dx, dy);
   if (mybuffer != None)
      XPutImage(mydisplay, mybuffer, mygc, i, 0, 0, x, y, dx, dy);
   touch(x, y, dx, dy);
}

#undef red
#undef blue
#undef green

void win::get_color(XImage *i, int x, int y, int &r, int &g, int &b)
{
   XColor cc;

   cc.pixel = XGetPixel(i, x, y);
   if (is_last_pixel && cc.pixel == last_pixel)
   { // grab_last_rgb
      r = last_r;
      g = last_g;
      b = last_b;
   }
   else
   { // grab_new_rgb
      XQueryColor(mydisplay, cmap, &cc);
      r = (int)cc.red >> 8;
      g = (int)cc.green >> 8;
      b = (int)cc.blue >> 8;
      is_last_pixel = true;
      last_pixel = cc.pixel;
      last_r = r;
      last_g = g;
      last_b = b;
   }
}

void win::set_pixel(XImage *i, int x, int y, int color)
{
   unsigned long c = colors[color];

   XPutPixel(i, x, y, c);
}

void win::ppm(char file_name[], int x, int y, int dx, int dy)
{
   FILE *ppm_file;
   XImage *image;

   { // open_file
      char name[128];

      sprintf(name, "%s.ppm", file_name);
      ppm_file = fopen(name, "w");
   }
   { // grab_image
      get_image(image, x, y, dx, dy);
   }
   {    // write_ppm
      { // write_magic_number
         fprintf(ppm_file, "P6\n");
      }
      { // write_name
         fprintf(ppm_file, "#%s\n", file_name);
      }
      { // write_size
         fprintf(ppm_file, "%d %d\n", dx, dy);
      }
      { // write_max_intensity
         fprintf(ppm_file, "255\n");
      }
      { // write_picture
         set_color(white);
         function(GXxor);
         for (int yy = y; yy < y + dy; yy++)
         {
            for (int xx = x; xx < x + dx; xx++)
            { // write_pixel
               int r;
               int g;
               int b;

               get_color(image, xx, yy, r, g, b);
               fprintf(ppm_file, "%c%c%c", r, g, b);
            }
         }
      }
   }
   fclose(ppm_file);
   XDestroyImage(image);
}
