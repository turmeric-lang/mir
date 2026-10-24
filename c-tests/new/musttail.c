/* __attribute__ ((musttail)) return f (...): c2mir makes the call a MIR tail
   call (MIR_TCALL).  The generator turns the ones whose args and results are
   all in registers into jumps, the interpreter runs a callee it interprets in
   the caller's activation, and both make the rest a call and a return.  This
   checks the results of both kinds at a depth any stack takes --
   musttail-deep.c checks the constant stack. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __MIRC_MUSTTAIL__
#define MT __attribute__ ((musttail))
#else
#define MT
#endif

static long odd (long n, long acc);
static long even (long n, long acc) {
  if (n == 0) return acc;
  MT return odd (n - 1, acc + 3);
}
static long odd (long n, long acc) {
  if (n == 0) return acc;
  MT return even (n - 1, acc - 1);
}

static double fodd (long n, double x, double y);
static double feven (long n, double x, double y) {
  if (n == 0) return x + y;
  MT return fodd (n - 1, x + 0.5, y);
}
static double fodd (long n, double x, double y) {
  if (n == 0) return x - y;
  MT return feven (n - 1, x, y + 0.25);
}

typedef int (*step_t) (int n, int acc);
static step_t steps[2];
static int ping (int n, int acc) {
  if (n <= 0) return acc;
  MT return steps[1](n - 1, acc + 1);
}
static int pong (int n, int acc) {
  if (n <= 0) return acc;
  MT return steps[0](n - 1, acc ^ n);
}

static unsigned char uc (unsigned char x, int n);
static unsigned char uc2 (unsigned char x, int n) {
  if (n == 0) return x;
  MT return uc ((unsigned char) (x + 7), n - 1);
}
static unsigned char uc (unsigned char x, int n) { MT return uc2 (x, n); }

/* Values in callee-saved regs across a call, then a tail call: the epilogue
   before the jump restores them. */
static long sink (long a) { return a; }
static long press_tail (long n, long a);
static long press (long n, long a) {
  long v0 = a + 1, v1 = a * 3, v2 = a ^ 5, v3 = a - 7, v4 = a + 11, v5 = a * 13, v6 = a + 17;
  long s = sink (n), t = v0 + v1 + v2 + v3 + v4 + v5 + v6 + s;

  if (n == 0) return t;
  MT return press_tail (n - 1, t & 0xffff);
}
static long press_tail (long n, long a) { MT return press (n, a + 1); }

static long use_buf (long n, long acc);
static long with_alloca (long n, long acc) {
  char *p = __builtin_alloca (64);

  memset (p, (int) (n & 0x7f), 64);
  acc += p[63];
  if (n == 0) return acc;
  MT return use_buf (n - 1, acc);
}
static long use_buf (long n, long acc) { MT return with_alloca (n, acc + 1); }

/* Calls the generator can not make jumps -- args on the stack, a vararg
   callee -- and calls c2mir keeps ordinary -- a converted result, a struct
   result: */
static long g7 (long a, long b, long c, long d, long e, long f, long g) {
  return a + b + c + d + e + f + g;
}
static long only_g7 (long a) { MT return g7 (a, 1, 2, 3, 4, 5, 6); }
static long two (long a, long b) { return a * b; }
static long mixed (long k, long x) {
  if (k == 0) return x;
  if (k == 1) {
    MT return g7 (x, x, x, x, x, x, x);
  }
  MT return two (x, k);
}
static long vsum (long n, ...) {
  va_list ap;
  long s = 0;

  va_start (ap, n);
  for (long i = 0; i < n; i++) s += va_arg (ap, long);
  va_end (ap);
  return s;
}
static long to_vsum (long a) { MT return vsum (3, a, a, a); }
static long vcaller (long n, ...) {
  va_list ap;
  long s = 0;

  va_start (ap, n);
  for (long i = 0; i < n; i++) s += va_arg (ap, long);
  va_end (ap);
  MT return two (s, 2);
}
static long lg (long x) { return x * 3; }
static int conv (long x) { MT return (int) lg (x); }
struct S {
  long a, b, c;
};
static struct S mk (long x) {
  struct S s = {x, x + 1, x + 2};
  return s;
}
static struct S smk (long x) { MT return mk (x); }

static int fall (int x) {
  switch (x) {
  case 0: x += 10; __attribute__ ((fallthrough));
  case 1: x += 100; break;
  default: x = -1;
  }
  return x;
}

int main (void) {
  struct S s = smk (40);

  steps[0] = ping;
  steps[1] = pong;
  printf ("%ld\n", even (1001, 0));
  printf ("%.2f\n", feven (1001, 0.1, 0.2));
  printf ("%d\n", ping (5001, 7));
  printf ("%d\n", uc (3, 301));
  printf ("%ld\n", press (300, 1));
  printf ("%ld\n", with_alloca (300, 0));
  printf ("%ld %ld %ld %ld %ld\n", only_g7 (10), mixed (0, 5), mixed (1, 5), mixed (3, 5),
          to_vsum (4));
  printf ("%ld %d %ld %ld %ld\n", vcaller (4, 1L, 2L, 3L, 4L), conv (1L << 33 | 5), s.a, s.b, s.c);
  printf ("%d %d %d\n", fall (0), fall (1), fall (2));
  return 0;
}
