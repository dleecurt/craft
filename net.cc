/*
 * Network play: lockstep over a byte stream.  See net.h.
 *
 * Messages: u32 length (of what follows), u8 type, then the fields, all
 * little-endian.  A text field is a u16 length and the bytes.
 */

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "net.h"
#include "craft.h"
#include "object_handler.h"
#include "player.h"
#include "savegame.h"
#include "statehash.h"

int net_role = net_off;
int net_delay = 4;
int net_seat = 0;

#define net_protocol 1

enum
{
   msg_hello = 1, /* guest: protocol, build, code, name */
   msg_start,     /* host: seat, host seat, delay, first step, pace, state */
   msg_orders,    /* guest: step, n, orders */
   msg_step,      /* host: step, count, (seat, n, orders)... */
   msg_check,     /* guest: step, hash */
   msg_bye,       /* either: reason */
   msg_pause,     /* host: on */
   msg_pace,      /* host: pace */
   msg_lobby,     /* host: seat lines, options summary */
   msg_delay,     /* host: the input delay from now on */
   msg_note       /* host: a note for the screen ("Waiting for Lee 12s") */
};

#define ring_size 128 /* steps kept; far more than the delay */

/*----------------------------------------------------------------------*/
/* Buffers and connections                                              */
/*----------------------------------------------------------------------*/

struct buf
{
   unsigned char *d = NULL;
   size_t len = 0;
   size_t cap = 0;

   void add(const void *p, size_t n)
   {
      if (len + n > cap)
      {
         cap = (len + n) * 2 + 256;
         d = (unsigned char *)realloc(d, cap);
         if (d == NULL)
         {
            fprintf(stderr, "craft: out of memory\n");
            exit(1);
         }
      }
      memcpy(d + len, p, n);
      len += n;
   }
   void drop(size_t n)
   {
      memmove(d, d + n, len - n);
      len -= n;
   }
};

/* An outgoing message under construction. */
struct msg
{
   buf b;

   msg(int type)
   {
      uint32_t zero = 0;
      unsigned char t = (unsigned char)type;

      b.add(&zero, 4);
      b.add(&t, 1);
   }
   ~msg()
   {
      free(b.d);
   }
   void u8(int v)
   {
      unsigned char c = (unsigned char)v;
      b.add(&c, 1);
   }
   void u32(uint32_t v)
   {
      unsigned char c[4] = { (unsigned char)v, (unsigned char)(v >> 8), (unsigned char)(v >> 16),
                             (unsigned char)(v >> 24) };
      b.add(c, 4);
   }
   void i32(int v)
   {
      u32((uint32_t)v);
   }
   void u64(uint64_t v)
   {
      u32((uint32_t)v);
      u32((uint32_t)(v >> 32));
   }
   void str(const char s[])
   {
      size_t n = strlen(s);

      if (n > 1000)
         n = 1000;
      u8((int)(n & 0xff));
      u8((int)(n >> 8));
      b.add(s, n);
   }
   void bytes(const void *p, size_t n)
   {
      u32((uint32_t)n);
      b.add(p, n);
   }
   void orders(const net_orders &o)
   {
      u32((uint32_t)o.n);
      for (int i = 0; i < o.n; i++)
      {
         i32(o.id[i]);
         i32(o.cmd[i]);
         i32(o.x[i]);
         i32(o.y[i]);
      }
   }
   void finish()
   {
      uint32_t n = (uint32_t)(b.len - 4);

      b.d[0] = (unsigned char)n;
      b.d[1] = (unsigned char)(n >> 8);
      b.d[2] = (unsigned char)(n >> 16);
      b.d[3] = (unsigned char)(n >> 24);
   }
};

/* Reading the fields of a received message. */
struct reader
{
   const unsigned char *p;
   size_t left;
   bool ok = true;

   reader(const unsigned char *d, size_t n) : p(d), left(n) {}
   bool need(size_t n)
   {
      if (!ok || left < n)
         ok = false;
      return ok;
   }
   int u8()
   {
      if (!need(1))
         return 0;
      left--;
      return *p++;
   }
   uint32_t u32()
   {
      if (!need(4))
         return 0;
      uint32_t v
          = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
      p += 4;
      left -= 4;
      return v;
   }
   int i32()
   {
      return (int)u32();
   }
   uint64_t u64()
   {
      uint64_t lo = u32();
      return lo | (uint64_t)u32() << 32;
   }
   void str(char out[], size_t size)
   {
      size_t n = (size_t)u8();

      n |= (size_t)u8() << 8;
      out[0] = 0;
      if (!need(n))
         return;
      size_t k = n < size - 1 ? n : size - 1;
      memcpy(out, p, k);
      out[k] = 0;
      for (size_t i = 0; i < k; i++)
         if ((unsigned char)out[i] < 32)
            out[i] = '?';
      p += n;
      left -= n;
   }
   bool orders(net_orders &o)
   {
      uint32_t n = u32();

      if (n > max_orders)
         ok = false;
      if (!ok)
         return false;
      o.n = (int)n;
      for (int i = 0; i < o.n; i++)
      {
         o.id[i] = i32();
         o.cmd[i] = i32();
         o.x[i] = i32();
         o.y[i] = i32();
      }
      return ok;
   }
};

struct check
{
   int step;
   uint64_t hash;
};

struct conn
{
   int fd = -1;
   int seat = -1;
   bool hello = false; /* introduced, waiting for the game to start */
   char name[128] = "";
   buf in;
   buf out;
   check pending[16]; /* guest's hashes the host could not check yet */
   int num_pending = 0;
};

static double now_ms()
{
   struct timespec ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void send_msg(conn &c, msg &m)
{
   if (c.fd < 0)
      return;
   m.finish();
   c.out.add(m.b.d, m.b.len);
}

static void close_conn(conn &c)
{
   if (c.fd >= 0)
      close(c.fd);
   c.fd = -1;
   c.in.len = 0;
   c.out.len = 0;
}

/* Write what we can; false if the connection broke. */
static bool flush_conn(conn &c)
{
   while (c.fd >= 0 && c.out.len > 0)
   {
      ssize_t n = send(c.fd, c.out.d, c.out.len, MSG_NOSIGNAL);

      if (n > 0)
         c.out.drop((size_t)n);
      else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
         return true;
      else
         return false;
   }
   return true;
}

/* Read what is there; false if the other side closed or broke. */
static bool fill_conn(conn &c)
{
   unsigned char tmp[16384];

   for (;;)
   {
      ssize_t n = recv(c.fd, tmp, sizeof(tmp), 0);

      if (n > 0)
         c.in.add(tmp, (size_t)n);
      else if (n == 0)
         return false;
      else if (errno == EAGAIN || errno == EWOULDBLOCK)
         return true;
      else if (errno != EINTR)
         return false;
   }
}

/* Write and read what we can.  Read even when writing fails: the other
   side may have said goodbye (and why) before closing. */
static bool pump(conn &c)
{
   bool wrote = flush_conn(c);
   bool read = c.fd >= 0 && fill_conn(c);

   return wrote && read;
}

/* The next whole message in c.in, if any: type and fields. */
static bool next_msg(conn &c, int &type, reader &r, size_t &used)
{
   if (c.in.len < 5)
      return false;
   uint32_t n = (uint32_t)c.in.d[0] | (uint32_t)c.in.d[1] << 8 | (uint32_t)c.in.d[2] << 16
                | (uint32_t)c.in.d[3] << 24;
   if (n < 1 || n > 64 * 1024 * 1024)
   { /* nonsense: treat as a broken connection */
      type = -1;
      used = c.in.len;
      return true;
   }
   if (c.in.len < 4 + (size_t)n)
      return false;
   type = c.in.d[4];
   r = reader(c.in.d + 5, n - 1);
   used = 4 + n;
   return true;
}

static void set_nonblocking(int fd)
{
   int one = 1;

   fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

/* Same build: the same code compiled the same way computes the same
   game.  (The save format's sizes are part of it.) */
static const char *build_id()
{
   static char id[256] = "";

   if (id[0] == 0)
      snprintf(id, sizeof(id), "craft %s / %s / objects %zu / player %zu", VERSION,
#ifdef __VERSION__
               __VERSION__,
#else
               "?",
#endif
               sizeof(object_handler), sizeof(player));
   return id;
}

/*----------------------------------------------------------------------*/
/* Step buffers                                                         */
/*----------------------------------------------------------------------*/

struct step_slot
{
   int step = -1;
   bool ready = false; /* all orders known (host: sent) */
   bool have[max_players];
   net_orders o[max_players];
};

static step_slot ring[ring_size];

static step_slot &slot(int step)
{
   step_slot &s = ring[step % ring_size];

   if (s.step != step)
   {
      s.step = step;
      s.ready = false;
      for (int p = 0; p < max_players; p++)
      {
         s.have[p] = false;
         s.o[p].n = 0;
      }
   }
   return s;
}

/*----------------------------------------------------------------------*/
/* State                                                                */
/*----------------------------------------------------------------------*/

static int listen_fd = -1;
static conn guests[max_players]; /* host: by seat */
static conn to_host;             /* guest */
static bool seat_left[max_players];
static int next_send = 0; /* host: first step not yet sent */
static int host_seat = 0;
static bool host_gone = false;
static char status_text[256] = "";
static char message_text[256] = "";
static bool message_new = false;
static double waiting_since = 0;
static int last_sub[max_players]; /* host: last step each seat sent orders for */
static int first_play_step = 0;
static int window_steps = 0; /* host: steps run, and how many we waited for a guest */
static int window_waits = 0;
static double last_note_sent = 0;
static char guest_note[256] = ""; /* guest: the host's latest note, and when */
static double guest_note_at = 0;
static bool dropped_out_of_sync = false;

#define grace_ms 15000 /* a guest whose orders stop this long is dropped */
static bool host_paused = false;

static check host_checks[64]; /* host's own hashes, by step / every */

static void say(const char text[])
{
   snprintf(message_text, sizeof(message_text), "%s", text);
   message_new = true;
}

const char *net_message()
{
   if (!message_new)
      return NULL;
   message_new = false;
   return message_text;
}

const char *net_status()
{
   return status_text[0] != 0 ? status_text : NULL;
}

bool net_host_gone()
{
   return host_gone;
}

bool net_seat_left(int p)
{
   return 0 <= p && p < max_players && seat_left[p];
}

/*----------------------------------------------------------------------*/
/* Host                                                                 */
/*----------------------------------------------------------------------*/

static char join_code[32] = "";

static void make_join_code()
{
   static const char *words[] = { "FJORD", "RUNE", "AXE",   "SAGA",  "LONGSHIP", "TROLL",
                                  "HELM",  "MEAD", "SKALD", "THING", "WOLF",     "RAVEN" };
   unsigned char r[4] = { 0, 0, 0, 0 };
   FILE *f = fopen("/dev/urandom", "rb");

   if (f != NULL)
   {
      if (fread(r, 1, sizeof(r), f) != sizeof(r))
         r[0] = r[1] = r[2] = r[3] = 0;
      fclose(f);
   }
   if (getenv("CRAFT_JOIN_CODE") != NULL) /* fixed code, for tests */
      snprintf(join_code, sizeof(join_code), "%s", getenv("CRAFT_JOIN_CODE"));
   else
      snprintf(join_code, sizeof(join_code), "%s-%04u",
               words[r[0] % (sizeof(words) / sizeof(words[0]))],
               (unsigned)(r[1] | r[2] << 8) % 10000);
}

static void reject(conn &c, const char why[])
{
   msg m(msg_bye);

   m.str(why);
   send_msg(c, m);
   flush_conn(c);
   close_conn(c);
}

/*---- the lobby: guests who have introduced themselves, in the order
       they came, until the host starts the game ----------------------*/

static conn waiting[8];
static int num_waiting = 0; /* introduced guests come first in waiting[] */

bool net_host_listen(int port)
{
   struct sockaddr_in a;
   int one = 1;

   listen_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (listen_fd < 0)
   {
      perror("craft: socket");
      return false;
   }
   setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
   memset(&a, 0, sizeof(a));
   a.sin_family = AF_INET;
   a.sin_port = htons((unsigned short)port);
   a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* only this computer (and SSH) */
   if (bind(listen_fd, (struct sockaddr *)&a, sizeof(a)) != 0 || listen(listen_fd, 8) != 0)
   {
      fprintf(stderr, "craft: cannot listen on port %d (%s)\n", port, strerror(errno));
      close(listen_fd);
      listen_fd = -1;
      return false;
   }
   set_nonblocking(listen_fd);
   make_join_code();
   num_waiting = 0;
   for (int i = 0; i < 8; i++)
      waiting[i] = conn();
   for (int p = 0; p < max_players; p++)
      seat_left[p] = false;
   return true;
}

const char *net_join_code()
{
   return join_code;
}

/* Drop waiting[i], keeping the introduced ones first and in order. */
static void lobby_remove(int i)
{
   bool was_hello = waiting[i].hello;

   close_conn(waiting[i]);
   free(waiting[i].in.d);
   free(waiting[i].out.d);
   for (int k = i; k < 7; k++)
      waiting[k] = waiting[k + 1];
   waiting[7] = conn();
   if (was_hello)
      num_waiting--;
}

int net_lobby_poll(int max_guests, char names[][128])
{
   { // accept_new
      int fd = accept(listen_fd, NULL, NULL);

      if (fd >= 0)
      {
         int i = 0;

         while (i < 8 && waiting[i].fd >= 0)
            i++;
         if (i == 8)
            close(fd);
         else
         {
            set_nonblocking(fd);
            waiting[i] = conn();
            waiting[i].fd = fd;
         }
      }
   }
   for (int i = 0; i < 8; i++)
   {
      conn &c = waiting[i];
      int type;
      reader r(NULL, 0);
      size_t used;

      if (c.fd < 0)
         continue;
      if (!pump(c))
      {
         lobby_remove(i--);
         continue;
      }
      while (c.fd >= 0 && next_msg(c, type, r, used))
      {
         if (type == msg_hello && !c.hello)
         {
            char build[256];
            char code[64];
            uint32_t proto = r.u32();

            r.str(build, sizeof(build));
            r.str(code, sizeof(code));
            r.str(c.name, sizeof(c.name));
            c.in.drop(used);
            if (!r.ok || proto != net_protocol || strcmp(build, build_id()) != 0)
               reject(c, "Different version of Craft: both computers need the same build");
            else if (strcasecmp(code, join_code) != 0) /* typed in either case */
               reject(c, "Wrong join code");
            else if (c.name[0] == 0)
               reject(c, "No player name");
            else if (num_waiting >= max_guests)
               reject(c, "The game is full");
            else
            { /* move it up to the end of the introduced ones */
               conn tmp = c;

               c.hello = true;
               tmp.hello = true;
               for (int k = i; k > num_waiting; k--)
                  waiting[k] = waiting[k - 1];
               waiting[num_waiting++] = tmp;
               break; /* waiting[i] is another connection now */
            }
         }
         else if (type == msg_bye && c.hello)
         {
            c.in.drop(used);
            lobby_remove(i);
            i = -1; /* the list moved: start over */
            break;
         }
         else
         {
            c.in.drop(used);
            reject(c, "Unexpected message");
         }
      }
      if (i >= 0 && c.fd < 0 && !c.hello)
         lobby_remove(i--);
   }
   for (int i = 0; i < num_waiting; i++)
      snprintf(names[i], 128, "%s", waiting[i].name);
   return num_waiting;
}

void net_lobby_send(const char lines[][128], int num_lines, const char summary[])
{
   for (int i = 0; i < num_waiting; i++)
   {
      msg m(msg_lobby);

      m.u32((uint32_t)num_lines);
      for (int k = 0; k < num_lines; k++)
         m.str(lines[k]);
      m.str(summary);
      send_msg(waiting[i], m);
      flush_conn(waiting[i]);
   }
}

void net_lobby_close(const char why[])
{
   while (num_waiting > 0 || waiting[0].fd >= 0)
   {
      if (waiting[0].fd >= 0 && why != NULL)
         reject(waiting[0], why);
      lobby_remove(0);
   }
   if (listen_fd >= 0)
      close(listen_fd);
   listen_fd = -1;
}

void net_lobby_seat(const int seat_of[], int num)
{
   net_role = net_host;
   for (int k = 0; k < num && k < num_waiting; k++)
   {
      int seat = seat_of[k];

      guests[seat] = waiting[k];
      guests[seat].seat = seat;
      waiting[k] = conn(); /* moved */
   }
   for (int k = num; k < 8; k++)
      if (waiting[k].fd >= 0)
         reject(waiting[k], "The game has started without you");
   num_waiting = 0;
}

static bool snapshot(buf &out)
{
   char path[] = "/tmp/craft-netXXXXXX";
   int fd = mkstemp(path);
   gzFile f;
   bool ok;

   if (fd < 0)
      return false;
   unlink(path);
   f = gzdopen(dup(fd), "wb1");
   ok = f != NULL && save_game_into(f);
   if (f != NULL)
      ok = gzclose(f) == Z_OK && ok;
   if (ok)
   {
      unsigned char tmp[65536];
      ssize_t n;

      lseek(fd, 0, SEEK_SET);
      while ((n = read(fd, tmp, sizeof(tmp))) > 0)
         out.add(tmp, (size_t)n);
   }
   close(fd);
   return ok;
}

static void mark_initial_steps(int first_step)
{
   for (int p = 0; p < max_players; p++)
      last_sub[p] = first_step + net_delay - 1;
   first_play_step = first_step;
   window_steps = window_waits = 0;
   for (int s = first_step; s < first_step + net_delay; s++)
   {
      step_slot &sl = slot(s);

      sl.ready = true;
      for (int p = 0; p < max_players; p++)
         sl.have[p] = true;
   }
}

bool net_host_start(int first_step)
{
   buf state;

   if (!snapshot(state))
   {
      fprintf(stderr, "craft: could not package the game for the guests\n");
      return false;
   }
   for (int p = 0; p < max_players; p++)
      if (guests[p].fd >= 0)
      {
         msg m(msg_start);

         m.u32((uint32_t)p);
         m.u32((uint32_t)net_seat);
         m.u32((uint32_t)net_delay);
         m.u32((uint32_t)first_step);
         m.u32((uint32_t)game_pace);
         m.bytes(state.d, state.len);
         send_msg(guests[p], m);
      }
   free(state.d);
   host_seat = net_seat;
   mark_initial_steps(first_step);
   next_send = first_step + net_delay;
   /* Send the state out before play starts; it is the big one. */
   for (bool busy = true; busy;)
   {
      busy = false;
      for (int p = 0; p < max_players; p++)
         if (guests[p].fd >= 0 && guests[p].out.len > 0)
         {
            if (!flush_conn(guests[p]))
               close_conn(guests[p]);
            else
               busy = guests[p].out.len > 0 || busy;
         }
      if (busy)
         usleep(2000);
   }
   return true;
}

static void broadcast(msg &m)
{
   m.finish();
   for (int p = 0; p < max_players; p++)
      if (guests[p].fd >= 0)
         guests[p].out.add(m.b.d, m.b.len);
}

void net_send_pause(bool on)
{
   if (net_role != net_host)
      return;
   msg m(msg_pause);

   m.u8(on ? 1 : 0);
   broadcast(m);
   net_poll(0);
}

void net_send_pace(int pace)
{
   if (net_role != net_host)
      return;
   msg m(msg_pace);

   m.u32((uint32_t)pace);
   broadcast(m);
}

/* A guest is gone: its side gives up in the first step not yet sent. */
static void guest_left(int seat, const char why[])
{
   char text[256];

   close_conn(guests[seat]);
   if (seat_left[seat])
      return;
   seat_left[seat] = true;
   {
      step_slot &s = slot(next_send);
      net_orders &o = s.o[seat];

      o.n = 0;
      o.cmd[0] = cmd_give_up;
      o.id[0] = none;
      o.x[0] = 0;
      o.y[0] = 0;
      o.n = 1;
      s.have[seat] = true;
   }
   snprintf(text, sizeof(text), "%s left: %s", name[seat], why);
   printf("craft: %s\n", text);
   fflush(stdout);
   snprintf(text, sizeof(text), "%.40s\nleft: %.40s", name[seat], why);
   say(text);
}

static void host_check(conn &g, int step, uint64_t hash)
{
   check &h = host_checks[(step / net_check_every) % 64];

   if (h.step == step)
   {
      if (h.hash != hash)
      {
         char why[128];

         snprintf(why, sizeof(why), "Dropped: out of sync at step %d", step);
         {
            msg m(msg_bye);

            m.str(why);
            send_msg(g, m);
            flush_conn(g);
         }
         guest_left(g.seat, "out of sync");
      }
   }
   else if (g.num_pending < 16)
      g.pending[g.num_pending++] = { step, hash };
}

/* A seat's orders for a step.  The delay can grow during the game, so a
   seat may skip steps: those have no orders from it. */
static void got_orders(int seat, int step, const net_orders &o)
{
   if (step < next_send || step >= next_send + ring_size / 2)
      return;
   for (int k = i_max(last_sub[seat] + 1, next_send); k < step; k++)
   {
      step_slot &g = slot(k);

      if (!g.have[seat])
      {
         g.o[seat].n = 0;
         g.have[seat] = true;
      }
   }
   {
      step_slot &t = slot(step);

      t.o[seat] = o;
      t.have[seat] = true;
   }
   if (step > last_sub[seat])
      last_sub[seat] = step;
}

/* Send every step whose orders are complete, in order. */
static void host_send_ready()
{
   for (;;)
   {
      step_slot &s = slot(next_send);
      bool all = s.have[host_seat] || seat_left[host_seat];

      for (int p = 0; p < max_players && all; p++)
         if (guests[p].fd >= 0 && !s.have[p])
            all = false;
      if (!all)
         return;
      {
         msg m(msg_step);
         int count = 0;

         for (int p = 0; p < max_players; p++)
            if (s.have[p] && s.o[p].n > 0)
               count++;
         m.u32((uint32_t)next_send);
         m.u32((uint32_t)count);
         for (int p = 0; p < max_players; p++)
            if (s.have[p] && s.o[p].n > 0)
            {
               m.u32((uint32_t)p);
               m.orders(s.o[p]);
            }
         broadcast(m);
      }
      s.ready = true;
      next_send++;
   }
}

static void host_handle(conn &g)
{
   int type;
   reader r(NULL, 0);
   size_t used;

   while (g.fd >= 0 && next_msg(g, type, r, used))
   {
      switch (type)
      {
      case msg_orders:
      {
         int step = r.i32();
         net_orders o;

         if (r.orders(o))
            got_orders(g.seat, step, o);
         break;
      }
      case msg_check:
      {
         int step = r.i32();
         uint64_t hash = r.u64();

         if (r.ok)
         {
            g.in.drop(used);
            host_check(g, step, hash);
            continue;
         }
         break;
      }
      case msg_bye:
      {
         char why[256];

         r.str(why, sizeof(why));
         g.in.drop(used);
         guest_left(g.seat, why[0] ? why : "quit");
         return;
      }
      default:
         g.in.drop(used);
         guest_left(g.seat, "sent something unexpected");
         return;
      }
      g.in.drop(used);
   }
}

/*----------------------------------------------------------------------*/
/* Guest                                                                */
/*----------------------------------------------------------------------*/

static bool connect_to(const char addr[], int port, int &fd)
{
   char host[256];
   const char *port_s = NULL;
   char port_text[16];
   struct addrinfo hints;
   struct addrinfo *res = NULL;

   snprintf(host, sizeof(host), "%s", addr);
   {
      char *colon = strrchr(host, ':');

      if (colon != NULL && strchr(host, ':') == colon)
      {
         *colon = 0;
         port_s = colon + 1;
      }
   }
   if (port_s == NULL)
   {
      snprintf(port_text, sizeof(port_text), "%d", port);
      port_s = port_text;
   }
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   if (getaddrinfo(host, port_s, &hints, &res) != 0 || res == NULL)
   {
      snprintf(status_text, sizeof(status_text), "Cannot find the computer \"%.100s\"", host);
      return false;
   }
   fd = -1;
   for (struct addrinfo *a = res; a != NULL && fd < 0; a = a->ai_next)
   {
      fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
      if (fd >= 0 && connect(fd, a->ai_addr, a->ai_addrlen) != 0)
      {
         close(fd);
         fd = -1;
      }
   }
   freeaddrinfo(res);
   if (fd < 0)
   {
      snprintf(status_text, sizeof(status_text), "No Craft game is being hosted at %.100s:%s", host,
               port_s);
      return false;
   }
   set_nonblocking(fd);
   return true;
}

/* "user@host": reach the host's game through SSH, which forwards our
   stdin/stdout to the game's port on the host (ssh -W).  SSH asks for a
   key passphrase on the terminal if it needs one. */
static pid_t ssh_pid = -1;

static bool ssh_to(const char addr[], int port, int &fd)
{
   int sv[2];
   char target[64];
   const char *ssh = getenv("CRAFT_SSH") != NULL ? getenv("CRAFT_SSH") : "ssh";

   if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0)
   {
      snprintf(status_text, sizeof(status_text), "Cannot start ssh (%s)", strerror(errno));
      return false;
   }
   snprintf(target, sizeof(target), "localhost:%d", port);
   fflush(stdout);
   ssh_pid = fork();
   if (ssh_pid < 0)
   {
      close(sv[0]);
      close(sv[1]);
      snprintf(status_text, sizeof(status_text), "Cannot start ssh (%s)", strerror(errno));
      return false;
   }
   if (ssh_pid == 0)
   { /* the child: ssh with the socket as its stdin and stdout */
      dup2(sv[1], 0);
      dup2(sv[1], 1);
      close(sv[0]);
      close(sv[1]);
      /* keepalives: if the host's computer vanishes, ssh gives up within
         about 15 seconds and the game says the host has left */
      execlp(ssh, ssh, "-T", "-o", "ExitOnForwardFailure=yes", "-o", "ConnectTimeout=15", "-o",
             "ServerAliveInterval=5", "-o", "ServerAliveCountMax=3", "-W", target, addr,
             (char *)NULL);
      fprintf(stderr, "craft: cannot run %s: %s\n", ssh, strerror(errno));
      _exit(127);
   }
   close(sv[1]);
   fd = sv[0];
   fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
   return true;
}

static void stop_ssh()
{
   if (ssh_pid > 0)
   {
      int st;

      kill(ssh_pid, SIGTERM);
      waitpid(ssh_pid, &st, 0);
   }
   ssh_pid = -1;
}

static char lobby_lines[8][128];
static int num_lobby_lines = 0;
static char lobby_summary[256] = "";

bool net_guest_connect(const char addr[], int port, const char code[], const char my_name[])
{
   int fd;
   bool ok;

   host_gone = false;
   status_text[0] = 0;
   num_lobby_lines = 0;
   lobby_summary[0] = 0;
   ok = strchr(addr, '@') != NULL ? ssh_to(addr, port, fd) : connect_to(addr, port, fd);
   if (!ok)
      return false;
   to_host = conn();
   to_host.fd = fd;
   {
      msg m(msg_hello);

      m.u32(net_protocol);
      m.str(build_id());
      m.str(code);
      m.str(my_name);
      send_msg(to_host, m);
   }
   return true;
}

int net_guest_lobby_poll(int ms, gzFile &state, char lines[][128], int &num_lines,
                         char summary[256], char why[256])
{
   struct pollfd pf = { to_host.fd, (short)(POLLIN | (to_host.out.len ? POLLOUT : 0)), 0 };
   int type;
   reader r(NULL, 0);
   size_t used;
   int result = net_lobby_waiting;

   state = NULL;
   why[0] = 0;
   if (to_host.fd < 0)
   {
      snprintf(why, 256, "%s", status_text[0] ? status_text : "Not connected");
      return net_lobby_ended;
   }
   poll(&pf, 1, ms);
   if (!pump(to_host))
   {
      /* whatever came before it closed may say why */
      if (next_msg(to_host, type, r, used) && type == msg_bye)
         r.str(why, 256);
      else if (ssh_pid > 0)
         snprintf(why, 256, "The connection closed (see the terminal for what ssh said)");
      else
         snprintf(why, 256, "The host closed the connection");
      close_conn(to_host);
      stop_ssh();
      return net_lobby_ended;
   }
   while (next_msg(to_host, type, r, used))
   {
      if (type == msg_bye)
      {
         r.str(why, 256);
         to_host.in.drop(used);
         close_conn(to_host);
         stop_ssh();
         return net_lobby_ended;
      }
      if (type == msg_lobby)
      {
         int n = (int)r.u32();

         num_lobby_lines = 0;
         for (int k = 0; k < n && r.ok; k++)
         {
            char tmp[128];

            r.str(tmp, sizeof(tmp));
            if (num_lobby_lines < 8)
               snprintf(lobby_lines[num_lobby_lines++], 128, "%s", tmp);
         }
         r.str(lobby_summary, sizeof(lobby_summary));
         to_host.in.drop(used);
         result = net_lobby_changed;
         continue;
      }
      if (type != msg_start)
      {
         to_host.in.drop(used);
         continue;
      }
      { // start
         int first_step;
         uint32_t len;
         char path[] = "/tmp/craft-netXXXXXX";
         int tfd;

         net_seat = (int)r.u32();
         host_seat = (int)r.u32();
         net_delay = (int)r.u32();
         first_step = (int)r.u32();
         game_pace = (int)r.u32();
         len = r.u32();
         if (!r.ok || !r.need(len) || net_seat >= max_players || net_delay < 1
             || net_delay > ring_size / 4)
         {
            snprintf(why, 256, "The host's game arrived damaged");
            close_conn(to_host);
            stop_ssh();
            return net_lobby_ended;
         }
         tfd = mkstemp(path);
         if (tfd < 0 || write(tfd, r.p, len) != (ssize_t)len)
         {
            snprintf(why, 256, "Cannot store the host's game (%s)", strerror(errno));
            if (tfd >= 0)
               close(tfd);
            close_conn(to_host);
            stop_ssh();
            return net_lobby_ended;
         }
         unlink(path);
         to_host.in.drop(used);
         lseek(tfd, 0, SEEK_SET);
         state = gzdopen(tfd, "rb");
         net_role = net_guest;
         for (int p = 0; p < max_players; p++)
            seat_left[p] = false;
         mark_initial_steps(first_step);
         return net_lobby_started;
      }
   }
   for (int k = 0; k < num_lobby_lines; k++)
      snprintf(lines[k], 128, "%s", lobby_lines[k]);
   num_lines = num_lobby_lines;
   snprintf(summary, 256, "%s", lobby_summary);
   return result;
}

void net_guest_cancel()
{
   if (to_host.fd >= 0)
   {
      msg m(msg_bye);

      m.str("left the lobby");
      send_msg(to_host, m);
      flush_conn(to_host);
      close_conn(to_host);
   }
   stop_ssh();
}

gzFile net_join(const char addr[], int port, const char code[], const char my_name[])
{
   gzFile state;
   char lines[8][128];
   int n;
   char summary[256];
   char why[256];

   if (!net_guest_connect(addr, port, code, my_name))
   {
      fprintf(stderr, "craft: %s\n", status_text);
      return NULL;
   }
   printf("connected; waiting for the host to start the game\n");
   fflush(stdout);
   for (;;)
      switch (net_guest_lobby_poll(500, state, lines, n, summary, why))
      {
      case net_lobby_started:
         return state;
      case net_lobby_ended:
         fprintf(stderr, "craft: %s\n", why);
         return NULL;
      default:
         break;
      }
}

static void guest_handle()
{
   int type;
   reader r(NULL, 0);
   size_t used;

   while (to_host.fd >= 0 && next_msg(to_host, type, r, used))
   {
      switch (type)
      {
      case msg_step:
      {
         int step = r.i32();
         int count = (int)r.u32();
         step_slot &s = slot(step);

         for (int i = 0; i < count && r.ok; i++)
         {
            int p = (int)r.u32();
            net_orders o;

            if (p < 0 || p >= max_players || !r.orders(o))
               r.ok = false;
            else
               s.o[p] = o;
         }
         if (!r.ok)
         {
            to_host.in.drop(used);
            host_gone = true;
            snprintf(status_text, sizeof(status_text), "The host sent damaged data: game over");
            close_conn(to_host);
            return;
         }
         s.ready = true;
         break;
      }
      case msg_pause:
      {
         char text[256];

         host_paused = r.u8() != 0;
         snprintf(text, sizeof(text), host_paused ? "Paused by\n%.40s" : "%.40s\nresumed",
                  name[host_seat]);
         say(text);
         break;
      }
      case msg_delay:
      {
         int d = (int)r.u32();

         if (r.ok && d >= 1 && d <= 12)
            net_delay = d;
         break;
      }
      case msg_note:
         r.str(guest_note, sizeof(guest_note));
         guest_note_at = now_ms();
         break;
      case msg_pace:
      {
         char text[64];

         game_pace = i_bound(1, (int)r.u32(), 1000);
         snprintf(text, sizeof(text), "Pace set to\n%d/s", game_pace);
         say(text);
         break;
      }
      case msg_bye:
      {
         char why[256];

         r.str(why, sizeof(why));
         to_host.in.drop(used);
         host_gone = true;
         dropped_out_of_sync = strncmp(why, "Dropped: out of sync", 20) == 0;
         snprintf(status_text, sizeof(status_text), "%s",
                  why[0] ? why : "The host has left the game");
         printf("craft: %s\n", status_text);
         close_conn(to_host);
         return;
      }
      default:
         break;
      }
      to_host.in.drop(used);
   }
}

/*----------------------------------------------------------------------*/
/* Both                                                                 */
/*----------------------------------------------------------------------*/

void net_poll(int ms)
{
   struct pollfd pf[max_players + 1];
   int n = 0;

   if (net_role == net_off)
      return;
   if (net_role == net_guest && to_host.fd >= 0)
   {
      pf[n].fd = to_host.fd;
      pf[n].events = (short)(POLLIN | (to_host.out.len ? POLLOUT : 0));
      n++;
   }
   if (net_role == net_host)
      for (int p = 0; p < max_players; p++)
         if (guests[p].fd >= 0)
         {
            pf[n].fd = guests[p].fd;
            pf[n].events = (short)(POLLIN | (guests[p].out.len ? POLLOUT : 0));
            n++;
         }
   if (n > 0 || ms > 0)
      poll(pf, (nfds_t)n, ms);
   if (net_role == net_guest && to_host.fd >= 0)
   {
      if (!pump(to_host))
      {
         guest_handle(); /* whatever arrived before it closed */
         if (!host_gone)
         {
            host_gone = true;
            snprintf(status_text, sizeof(status_text), "The host has left the game");
            printf("craft: %s\n", status_text);
         }
         close_conn(to_host);
      }
      else
         guest_handle();
   }
   if (net_role == net_host && listen_fd >= 0)
   { // turn_away_latecomers
      int fd = accept(listen_fd, NULL, NULL);

      if (fd >= 0)
      {
         conn late;

         late.fd = fd;
         set_nonblocking(fd);
         reject(late, "the game has already started");
         free(late.in.d);
         free(late.out.d);
      }
   }
   if (net_role == net_host)
   {
      for (int p = 0; p < max_players; p++)
         if (guests[p].fd >= 0)
         {
            if (!pump(guests[p]))
            {
               host_handle(guests[p]);
               if (guests[p].fd >= 0)
                  guest_left(p, "lost connection");
            }
            else
               host_handle(guests[p]);
         }
      host_send_ready();
      for (int p = 0; p < max_players; p++)
         if (guests[p].fd >= 0 && !flush_conn(guests[p]))
            guest_left(p, "lost connection");
   }
}

void net_submit(int step, const net_orders &mine)
{
   if (net_role == net_host)
   {
      got_orders(net_seat, step, mine);
      host_send_ready();
   }
   else if (net_role == net_guest)
   {
      msg m(msg_orders);

      m.i32(step);
      m.orders(mine);
      send_msg(to_host, m);
   }
   net_poll(0);
}

bool net_take_step(int step, net_orders all[max_players])
{
   step_slot &s = ring[step % ring_size];

   net_poll(0);
   if (s.step != step || !s.ready)
   {
      double now = now_ms();

      if (waiting_since == 0)
         waiting_since = now;
      else if (now - waiting_since > 1000)
      {
         char who[200] = "";
         int left = (int)((grace_ms - (now - waiting_since)) / 1000);

         if (net_role == net_guest)
         {
            if (host_paused)
               snprintf(status_text, sizeof(status_text), "Paused by\n%.40s", name[host_seat]);
            else if (now - guest_note_at < 2500)
               snprintf(status_text, sizeof(status_text), "%s", guest_note);
            else
               snprintf(status_text, sizeof(status_text), "Waiting for\n%.40s %ds", name[host_seat],
                        (int)((now - waiting_since) / 1000));
            return false;
         }
         { // host: whose orders are missing
            step_slot &w = slot(step);

            for (int p = 0; p < max_players; p++)
               if (guests[p].fd >= 0 && !w.have[p])
                  snprintf(who + strlen(who), sizeof(who) - strlen(who), "%s%.40s",
                           who[0] ? ", " : "", name[p]);
         }
         snprintf(status_text, sizeof(status_text), "Waiting for\n%.200s %ds", who, i_max(0, left));
         if (now - last_note_sent > 900)
         { /* so the guests see who everyone waits for */
            msg m(msg_note);

            m.str(status_text);
            broadcast(m);
            last_note_sent = now;
         }
         if (now - waiting_since >= grace_ms)
         { // drop_whoever_stalls
            step_slot &w = slot(step);

            for (int p = 0; p < max_players; p++)
               if (guests[p].fd >= 0 && !w.have[p])
               {
                  msg m(msg_bye);

                  m.str("Dropped: no answer for 15 seconds");
                  send_msg(guests[p], m);
                  flush_conn(guests[p]);
                  guest_left(p, "stopped answering");
               }
            host_send_ready();
         }
      }
      return false;
   }
   if (net_role == net_host)
   { // grow_the_delay_if_guests_keep_us_waiting
      /* Orders run net_delay steps after they are given, so that they
         reach every computer in time.  If the host often has to wait for
         a guest's orders, the network is slower than that: give orders
         one step more (up to 12).  Not in the first few seconds, while
         the guests are still setting up. */
      if (step > first_play_step + 200)
      {
         window_steps++;
         window_waits += waiting_since != 0;
         if (window_steps >= 200)
         {
            if (window_waits * 20 > window_steps && net_delay < 12)
            {
               msg m(msg_delay);

               net_delay++;
               m.u32((uint32_t)net_delay);
               broadcast(m);
               printf("craft: the network is slow; orders now take %d steps\n", net_delay);
               fflush(stdout);
            }
            window_steps = window_waits = 0;
         }
      }
   }
   if (waiting_since != 0 && !host_gone)
   {
      if (status_text[0] != 0 && !host_paused)
         say("\n"); /* clear the waiting note */
      status_text[0] = 0;
   }
   waiting_since = 0;
   for (int p = 0; p < max_players; p++)
      all[p] = s.o[p];
   return true;
}

bool net_dropped_out_of_sync()
{
   return dropped_out_of_sync;
}

void net_after_step(int step)
{
   if (net_role == net_off || step % net_check_every != 0)
      return;
   if (net_role == net_guest)
   {
      msg m(msg_check);

      { /* CRAFT_TEST_DESYNC=STEP: spoil our own state there (tests only) */
         const char *e = getenv("CRAFT_TEST_DESYNC");

         if (e != NULL && atoi(e) == step)
            players[net_seat]->money += 1;
      }
      m.i32(step);
      m.u64(game_state_hash());
      send_msg(to_host, m);
   }
   else
   {
      check &h = host_checks[(step / net_check_every) % 64];

      h.step = step;
      h.hash = game_state_hash();
      for (int p = 0; p < max_players; p++)
      {
         conn &g = guests[p];

         for (int i = 0; i < g.num_pending && g.fd >= 0; i++)
            if (g.pending[i].step == step)
            {
               check c = g.pending[i];

               g.pending[i] = g.pending[--g.num_pending];
               host_check(g, c.step, c.hash);
               i--;
            }
      }
   }
}

void net_leave(const char reason[])
{
   if (net_role == net_guest && to_host.fd >= 0)
   {
      msg m(msg_bye);

      m.str(reason);
      send_msg(to_host, m);
      for (int i = 0; i < 50 && to_host.out.len > 0 && flush_conn(to_host); i++)
         usleep(10000);
      close_conn(to_host);
   }
   stop_ssh();
   if (net_role == net_host)
   {
      msg m(msg_bye);

      m.str(strcmp(reason, "quit") == 0 ? "The host has quit the game" : "The game is over");
      broadcast(m);
      for (int i = 0; i < 50; i++)
      {
         bool busy = false;

         for (int p = 0; p < max_players; p++)
            if (guests[p].fd >= 0 && guests[p].out.len > 0 && flush_conn(guests[p]))
               busy = busy || guests[p].out.len > 0;
         if (!busy)
            break;
         usleep(10000);
      }
      for (int p = 0; p < max_players; p++)
         close_conn(guests[p]);
      if (listen_fd >= 0)
         close(listen_fd);
      listen_fd = -1;
   }
   net_role = net_off;
}
