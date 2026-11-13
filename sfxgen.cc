/*
 * sfxgen: synthesises Craft's sound effects into the WAV files in sounds/.
 * Added in 2026 by dleecurt/craft; not part of the CRAFT 3.5 Standard
 * Version (see LICENSE). The sounds it makes are original to this
 * project and are covered by the same licence as the rest of the game.
 *
 * Run "make sounds" to rebuild them. Each effect is a short recipe below:
 * oscillators, filtered noise, a plucked string and envelopes, mixed into
 * 16-bit mono WAV at 22050 Hz. The random numbers are seeded per effect,
 * so the output is the same every time.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static const int rate = 22050;
static const double two_pi = 6.283185307179586;

typedef std::vector<double> buffer;

/*--- building blocks ---------------------------------------------------*/

static uint32_t seed = 1;

static double noise()
{
   seed = seed * 1664525u + 1013904223u;
   return (double)(seed >> 8) / 8388608.0 - 1.0;
}

static int samples(double seconds)
{
   return (int)(seconds * rate);
}

/* Exponential decay: 1 at t=0, 1/e^(t/tau). */
static double decay(double t, double tau)
{
   return exp(-t / tau);
}

/* Linear attack then exponential decay. */
static double ad(double t, double attack, double tau)
{
   return t < attack ? t / attack : decay(t - attack, tau);
}

/* One-pole low-pass, cutoff in Hz. */
struct lowpass
{
   double y = 0;
   double a;
   lowpass(double cutoff)
   {
      a = 1 - exp(-two_pi * cutoff / rate);
   }
   void set(double cutoff)
   {
      a = 1 - exp(-two_pi * cutoff / rate);
   }
   double operator()(double x)
   {
      return y += a * (x - y);
   }
};

/* Band-pass biquad (constant peak gain), centre f, quality q. */
struct bandpass
{
   double b0, b2, a1, a2;
   double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
   bandpass(double f, double q)
   {
      set(f, q);
   }
   void set(double f, double q)
   {
      double w = two_pi * f / rate;
      double alpha = sin(w) / (2 * q);
      double a0 = 1 + alpha;

      b0 = alpha / a0;
      b2 = -alpha / a0;
      a1 = -2 * cos(w) / a0;
      a2 = (1 - alpha) / a0;
   }
   double operator()(double x)
   {
      double y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2;

      x2 = x1;
      x1 = x;
      y2 = y1;
      y1 = y;
      return y;
   }
};

/* A band-limited-enough sawtooth from a few harmonics. */
static double saw(double phase, int harmonics)
{
   double v = 0;

   for (int k = 1; k <= harmonics; k++)
      v += sin(phase * k) / k;
   return v * 0.6;
}

/* Add src into dst starting at time t (seconds), times gain. */
static void mix(buffer &dst, const buffer &src, double t, double gain)
{
   size_t start = (size_t)samples(t);

   if (dst.size() < start + src.size())
      dst.resize(start + src.size(), 0);
   for (size_t i = 0; i < src.size(); i++)
      dst[start + i] += src[i] * gain;
}

/* Plucked string (Karplus-Strong). */
static buffer pluck(double freq, double seconds, double damping)
{
   buffer out(samples(seconds));
   std::vector<double> line((size_t)(rate / freq));

   for (double &v : line)
      v = noise();
   for (size_t i = 0, p = 0; i < out.size(); i++)
   {
      size_t q = (p + 1) % line.size();
      double v = line[p];

      line[p] = damping * 0.5 * (line[p] + line[q]);
      out[i] = v;
      p = q;
   }
   return out;
}

/* Bell or metal ring: inharmonic partials with their own decay. */
static buffer ring(const double *freqs, const double *gains, int n, double seconds, double tau)
{
   buffer out(samples(seconds));

   for (size_t i = 0; i < out.size(); i++)
   {
      double t = (double)i / rate;
      double v = 0;

      for (int k = 0; k < n; k++)
         v += gains[k] * sin(two_pi * freqs[k] * t) * decay(t, tau / (1 + k * 0.4));
      out[i] = v;
   }
   return out;
}

/* Filtered noise burst through a band-pass that can glide. */
static buffer noise_burst(double seconds, double f0, double f1, double q, double attack, double tau)
{
   buffer out(samples(seconds));
   bandpass bp(f0, q);

   for (size_t i = 0; i < out.size(); i++)
   {
      double t = (double)i / rate;

      bp.set(f0 + (f1 - f0) * t / seconds, q);
      out[i] = bp(noise()) * ad(t, attack, tau);
   }
   return out;
}

/* Low thump: a sine that drops in pitch. */
static buffer thump(double f0, double f1, double seconds, double tau)
{
   buffer out(samples(fmax(seconds, 7 * tau))); /* run until it has died away */
   double phase = 0;

   for (size_t i = 0; i < out.size(); i++)
   {
      double t = (double)i / rate;
      double f = f1 + (f0 - f1) * decay(t, seconds / 4);

      phase += two_pi * f / rate;
      out[i] = sin(phase) * ad(t, 0.002, tau);
   }
   return out;
}

/* Horn: a sawtooth with slight vibrato through a soft low-pass. */
static buffer horn(double freq, double seconds, double attack, double release, double brightness)
{
   buffer out(samples(seconds));
   lowpass lp(brightness);
   double phase = 0;

   for (size_t i = 0; i < out.size(); i++)
   {
      double t = (double)i / rate;
      double env = t < attack ? t / attack : 1.0;
      double f = freq * (1 + 0.006 * sin(two_pi * 5.5 * t) * (t > attack ? 1 : t / attack));

      if (t > seconds - release)
         env *= (seconds - t) / release;
      phase += two_pi * f / rate;
      lp.set(brightness * (0.5 + 0.5 * env));
      out[i] = lp(saw(phase, 12)) * env;
   }
   return out;
}

/* Soft sine chime. */
static buffer chime(double freq, double seconds, double tau)
{
   buffer out(samples(fmax(seconds, 7 * tau)));

   for (size_t i = 0; i < out.size(); i++)
   {
      double t = (double)i / rate;

      out[i] = (sin(two_pi * freq * t) + 0.3 * sin(two_pi * freq * 2.01 * t)) * ad(t, 0.004, tau);
   }
   return out;
}

/*--- output ------------------------------------------------------------*/

static void put16(FILE *f, int v)
{
   fputc(v & 0xff, f);
   fputc((v >> 8) & 0xff, f);
}

static void put32(FILE *f, uint32_t v)
{
   put16(f, (int)(v & 0xffff));
   put16(f, (int)(v >> 16));
}

/* Normalise to peak level (0..1), fade the last 10 ms, write a WAV. */
static void write_wav(const char dir[], const char name[], buffer b, double peak)
{
   char path[1024];
   double m = 0;
   size_t fade = (size_t)samples(0.01);
   FILE *f;

   for (double v : b)
      m = fmax(m, fabs(v));
   for (size_t i = 0; i < b.size(); i++)
   {
      b[i] = m > 0 ? b[i] / m * peak : 0;
      if (i + fade > b.size())
         b[i] *= (double)(b.size() - i) / fade;
   }
   snprintf(path, sizeof(path), "%s/%s.wav", dir, name);
   f = fopen(path, "wb");
   if (f == NULL)
   {
      perror(path);
      exit(1);
   }
   fputs("RIFF", f);
   put32(f, (uint32_t)(36 + b.size() * 2));
   fputs("WAVEfmt ", f);
   put32(f, 16);
   put16(f, 1); /* PCM */
   put16(f, 1); /* mono */
   put32(f, rate);
   put32(f, rate * 2);
   put16(f, 2);
   put16(f, 16);
   fputs("data", f);
   put32(f, (uint32_t)(b.size() * 2));
   for (double v : b)
      put16(f, (int)lrint(fmax(-1.0, fmin(1.0, v)) * 32767));
   fclose(f);
   printf("  %-10s %.2f s\n", name, (double)b.size() / rate);
}

/*--- the effects -------------------------------------------------------*/

static buffer fx_chop() /* axe into wood */
{
   buffer b;

   seed = 11;
   mix(b, noise_burst(0.05, 2500, 1200, 1.2, 0.001, 0.012), 0, 0.8);
   mix(b, thump(260, 150, 0.16, 0.035), 0, 1.0);
   mix(b, noise_burst(0.16, 700, 500, 3, 0.002, 0.04), 0.004, 0.5);
   return b;
}

static buffer fx_mine() /* pick on rock */
{
   static const double f[] = { 1870, 2710, 3930, 5200 };
   static const double g[] = { 1, 0.7, 0.5, 0.3 };
   buffer b;

   seed = 12;
   mix(b, noise_burst(0.03, 4000, 3000, 1, 0.0005, 0.006), 0, 1.0);
   mix(b, ring(f, g, 4, 0.3, 0.05), 0.002, 0.6);
   mix(b, noise_burst(0.12, 900, 600, 2, 0.002, 0.03), 0.004, 0.4);
   return b;
}

static buffer fx_dig() /* spade into earth */
{
   buffer b;

   seed = 13;
   mix(b, noise_burst(0.26, 500, 1100, 1.5, 0.05, 0.07), 0, 1.0);
   mix(b, thump(120, 80, 0.12, 0.04), 0.05, 0.4);
   return b;
}

static buffer fx_build() /* hammer on timber */
{
   buffer b;

   seed = 14;
   mix(b, thump(620, 480, 0.12, 0.02), 0, 1.0);
   mix(b, thump(1250, 1100, 0.06, 0.01), 0, 0.4);
   mix(b, noise_burst(0.03, 3000, 2500, 1.5, 0.0005, 0.005), 0, 0.5);
   return b;
}

static buffer fx_sword() /* blades clash */
{
   static const double f[] = { 2380, 3470, 4950, 6120 };
   static const double g[] = { 1, 0.8, 0.6, 0.35 };
   buffer b;

   seed = 15;
   mix(b, noise_burst(0.05, 5000, 3500, 0.8, 0.0005, 0.01), 0, 1.0);
   mix(b, ring(f, g, 4, 0.4, 0.09), 0.001, 0.55);
   return b;
}

static buffer fx_bow() /* string twang and arrow whoosh */
{
   buffer b;

   seed = 16;
   mix(b, pluck(196, 0.18, 0.985), 0, 0.9);
   mix(b, noise_burst(0.28, 1200, 3200, 2.5, 0.06, 0.08), 0.02, 0.45);
   return b;
}

static buffer fx_catapult() /* creak, arm thump, stone away */
{
   buffer b;

   seed = 17;
   for (int i = 0; i < 6; i++)
      mix(b, noise_burst(0.025, 900 + i * 60, 700, 6, 0.002, 0.008), i * 0.03, 0.35);
   mix(b, thump(140, 55, 0.35, 0.09), 0.18, 1.0);
   mix(b, noise_burst(0.3, 500, 1800, 2, 0.08, 0.1), 0.2, 0.3);
   return b;
}

static buffer fx_explosion() /* stone or fire bursts */
{
   buffer b(samples(1.1));
   lowpass lp(3000);

   seed = 18;
   for (size_t i = 0; i < b.size(); i++)
   {
      double t = (double)i / rate;

      lp.set(150 + 2800 * decay(t, 0.12));
      b[i] = lp(noise()) * ad(t, 0.004, 0.28);
   }
   mix(b, thump(90, 35, 0.8, 0.25), 0, 0.9);
   return b;
}

static buffer fx_death() /* a short groan */
{
   buffer b(samples(0.5));
   bandpass f1(650, 4);
   bandpass f2(1100, 5);
   double phase = 0;

   seed = 19;
   for (size_t i = 0; i < b.size(); i++)
   {
      double t = (double)i / rate;
      double f = 165 - 70 * t / 0.5 + 3 * sin(two_pi * 7 * t);
      double env = ad(t, 0.03, 0.18);
      double src;

      phase += two_pi * f / rate;
      src = saw(phase, 20) + 0.15 * noise();
      f1.set(650 - 200 * t, 4);
      b[i] = (f1(src) + 0.6 * f2(src)) * env;
   }
   return b;
}

static buffer fx_collapse() /* a building falls */
{
   buffer b(samples(1.4));
   lowpass lp(600);

   seed = 20;
   for (size_t i = 0; i < b.size(); i++)
   {
      double t = (double)i / rate;

      b[i] = lp(noise()) * ad(t, 0.05, 0.45);
   }
   for (int i = 0; i < 9; i++)
      mix(b, fx_chop(), 0.04 + i * 0.11 + 0.03 * noise(), 0.25 * decay(i, 4));
   mix(b, thump(80, 40, 1.0, 0.3), 0.02, 0.8);
   return b;
}

static buffer fx_heal() /* rising soft chimes */
{
   static const double notes[] = { 523.25, 659.25, 783.99, 1046.5 };
   buffer b;

   for (int i = 0; i < 4; i++)
      mix(b, chime(notes[i], 0.5, 0.18), i * 0.07, 0.6);
   return b;
}

static buffer fx_trap() /* snap and spring */
{
   buffer b(samples(0.4));

   seed = 21;
   for (size_t i = 0; i < b.size(); i++)
   {
      double t = (double)i / rate;
      double f = 310 * (1 + 0.25 * sin(two_pi * 18 * t) * decay(t, 0.15));

      b[i] = 0.5 * sin(two_pi * f * t) * decay(t, 0.12);
   }
   mix(b, noise_burst(0.03, 3500, 2500, 1, 0.0003, 0.004), 0, 1.4);
   mix(b, thump(500, 300, 0.05, 0.01), 0, 0.8);
   return b;
}

static buffer fx_built() /* horn call: a building is finished */
{
   buffer b;

   mix(b, horn(196.0, 0.28, 0.03, 0.06, 1800), 0, 0.8);
   mix(b, horn(293.66, 0.5, 0.03, 0.18, 2200), 0.3, 1.0);
   return b;
}

static buffer fx_ready() /* short horn: a unit is ready */
{
   buffer b;

   seed = 22;
   mix(b, horn(261.63, 0.32, 0.025, 0.12, 2000), 0, 1.0);
   mix(b, thump(110, 70, 0.15, 0.05), 0, 0.5);
   return b;
}

static buffer fx_alert() /* low war horn: you are under attack */
{
   buffer b;

   mix(b, horn(110.0, 0.45, 0.08, 0.1, 900), 0, 1.0);
   mix(b, horn(110.0, 0.7, 0.08, 0.3, 1000), 0.55, 1.0);
   mix(b, horn(220.0, 0.7, 0.08, 0.3, 1400), 0.55, 0.25);
   return b;
}

static buffer fx_ack() /* order given: a wooden knock */
{
   buffer b;

   seed = 23;
   mix(b, thump(880, 700, 0.07, 0.015), 0, 1.0);
   mix(b, thump(440, 380, 0.07, 0.02), 0.035, 0.6);
   return b;
}

static buffer fx_click() /* button click */
{
   buffer b;

   seed = 24;
   mix(b, noise_burst(0.02, 3000, 2500, 1.5, 0.0003, 0.003), 0, 0.7);
   mix(b, thump(1500, 1200, 0.03, 0.005), 0, 1.0);
   return b;
}

int main(int argc, char *argv[])
{
   const char *dir = argc > 1 ? argv[1] : "sounds";

   printf("sfxgen: writing %s/\n", dir);
   write_wav(dir, "chop", fx_chop(), 0.7);
   write_wav(dir, "mine", fx_mine(), 0.6);
   write_wav(dir, "dig", fx_dig(), 0.6);
   write_wav(dir, "build", fx_build(), 0.6);
   write_wav(dir, "sword", fx_sword(), 0.7);
   write_wav(dir, "bow", fx_bow(), 0.6);
   write_wav(dir, "catapult", fx_catapult(), 0.8);
   write_wav(dir, "explosion", fx_explosion(), 0.9);
   write_wav(dir, "death", fx_death(), 0.6);
   write_wav(dir, "collapse", fx_collapse(), 0.85);
   write_wav(dir, "heal", fx_heal(), 0.5);
   write_wav(dir, "trap", fx_trap(), 0.7);
   write_wav(dir, "built", fx_built(), 0.6);
   write_wav(dir, "ready", fx_ready(), 0.55);
   write_wav(dir, "alert", fx_alert(), 0.75);
   write_wav(dir, "ack", fx_ack(), 0.45);
   write_wav(dir, "click", fx_click(), 0.4);
   return 0;
}
