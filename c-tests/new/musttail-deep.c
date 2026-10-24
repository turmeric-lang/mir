/* __attribute__ ((musttail)) return f (...) in constant stack: cycles deeper
   than an 8 MiB stack holds as calls, in the generator (a jump) and in the
   interpreter (the callee replaces the caller's activation), directly and
   through a function pointer. */
#include <stdio.h>

#ifdef __MIRC_MUSTTAIL__
#define MT __attribute__ ((musttail))
#else
#define MT
#endif

#define DEPTH 2000000

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

static long sink (long a) { return a; }
static long press_tail (long n, long a);
static long press (long n, long a) {
  long v0 = a + 1, v1 = a * 3, v2 = a ^ 5, v3 = a - 7, v4 = a + 11, v5 = a * 13, v6 = a + 17;
  long s = sink (n), t = v0 + v1 + v2 + v3 + v4 + v5 + v6 + s;

  if (n == 0) return t;
  MT return press_tail (n - 1, t & 0xffff);
}
static long press_tail (long n, long a) { MT return press (n, a + 1); }

int main (void) {
  steps[0] = ping;
  steps[1] = pong;
  printf ("%ld\n", even (DEPTH, 0));
  printf ("%.2f\n", feven (DEPTH + 1, 0.1, 0.2));
  printf ("%d\n", ping (DEPTH + 1, 7));
  printf ("%ld\n", press (DEPTH, 1));
  return 0;
}
