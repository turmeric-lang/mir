/* A conversion to _Bool is 0 when the value compares equal to zero and 1
   otherwise (C11 6.3.1.2).  c2mir converted by casting to _Bool's MIR type,
   an unsigned byte, so it kept the low 8 bits -- (_Bool) 256 was 0 -- and
   truncated a floating value to an integer first -- (_Bool) 0.5 was 0.  The
   constant folder did the same through mir_bool, a uint8_t.  Every
   conversion context and source type, at run time and folded.  */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

struct flags {
  _Bool b;
  _Bool bit : 1;
  int pad;
};

static _Bool ret_long (long x) { return x; }
static _Bool ret_double (double x) { return x; }
static _Bool ret_ptr (void *p) { return p; }
static int take (_Bool b) { return b; }

static _Bool sb = 256; /* static initializers: folded */
static _Bool sd = 0.5;
static _Bool sl = 1LL << 40;
static struct flags sf = {512, 1024, 0};
static _Bool sarr[] = {0, 256, -256, 0x10000};

int main (void) {
  volatile long l = 256, z = 0;
  volatile unsigned long long big = 1ULL << 40;
  volatile short s = 0x100;
  volatile signed char sc = -128;
  volatile double d = 0.5, nz = -0.0, nan_v = NAN;
  volatile float f = 0.25f;
  volatile long double ld = 0.125L;
  int x = 0;
  char *p = (char *) &x, *np = 0;
  _Bool a = l, b = big, c = s, e = sc, g = d, h = nz, i = nan_v, j = f, k = ld, m = p, n = np,
        o = z;
  struct flags fl = {l, l, 0};
  _Bool arr[3] = {l, d, z};
  _Bool t;

  printf ("init %d %d %d %d %d %d %d %d %d %d %d %d\n", a, b, c, e, g, h, i, j, k, m, n, o);
  printf ("cast %d %d %d %d %d\n", (_Bool) l, (_Bool) big, (_Bool) d, (_Bool) p, (_Bool) z);
  t = l;
  printf ("assign %d", t);
  t = d;
  printf (" %d", t);
  t = z;
  printf (" %d\n", t);
  t = 0;
  t += 256;
  printf ("compound %d", t);
  t = 1;
  t *= 512;
  printf (" %d", t);
  t = 1;
  t -= 2;
  printf (" %d\n", t);
  t = 0;
  t++;
  printf ("incdec %d", t);
  t++;
  printf (" %d", t);
  t--;
  printf (" %d", t);
  t--;
  printf (" %d\n", t);
  printf ("call %d %d %d %d %d %d\n", ret_long (l), ret_long (z), ret_double (d), ret_ptr (p),
          take (l), take (d));
  printf ("member %d %d %d\n", fl.b, fl.bit, (fl.bit = l, fl.bit));
  printf ("array %d %d %d\n", arr[0], arr[1], arr[2]);
  printf ("static %d %d %d %d %d %d %d %d %d\n", sb, sd, sl, sf.b, sf.bit, sarr[0], sarr[1],
          sarr[2], sarr[3]);
  printf ("folded %d %d %d %d\n", (_Bool) 256, (_Bool) 0.5, (_Bool) (1LL << 33), (_Bool) 0);
  return 0;
}
