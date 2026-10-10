/* __int128 everywhere besides the plain operators (int128-ops.c has those):
   constant folding into static data, conversions, calls passing and returning
   it among other arguments, struct members and their layout, compound
   assignment with operands of other types, ++/--, conditions, _Generic,
   statement expressions.  c2mir lowers __int128 to pairs of 64-bit values, and
   each of these is a separate path through that lowering.  The expected
   output is gcc's.  */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

typedef unsigned __int128 u128;
typedef __int128 i128;
typedef __uint128_t U;
typedef __int128_t I;

static void p (const char *tag, u128 v) {
  printf ("%s %016llx%016llx\n", tag, (unsigned long long) (v >> 64), (unsigned long long) v);
}

/* constant folding */
static const u128 K1 = ((u128) 0x0123456789abcdefULL << 64) | 0xfedcba9876543210ULL;
static const u128 K2 = (u128) 1 << 127;
static const i128 K3 = -(i128) 12345678901234567LL * 1000000007;
static const u128 K4 = ~(u128) 0 / 10;
static const i128 K5 = (i128) -7 / 2;
static const i128 K6 = (i128) -7 % 2;
static const u128 K7 = ((u128) 1 << 100) >> 37;
static const i128 K8 = ((i128) -1 << 100) >> 99;
static const u128 K9 = (u128) -1;
static const u128 K10 = (u128) 3.5e30;
static const i128 K11 = (i128) -1.25e25;
static const double D1 = (double) ((u128) 1 << 100);
static const float F1 = (float) (~(u128) 0 >> 1);
static const double D2 = (double) (i128) - ((i128) 1 << 90);
static const int C1 = ((u128) 1 << 64) > 5;
static const int C2 = (i128) -1 < (i128) 0;
static const int C3 = (u128) -1 < (u128) 0;
static const int C4 = !((u128) 1 << 64);
static const int C5 = ((u128) 1 << 64) && 1;
static const unsigned char C6 = (unsigned char) ((u128) 0x1ff << 64 | 0x1ff);
static const _Bool C7 = (_Bool) ((u128) 1 << 64);
static const long long C8 = (long long) (((i128) 1 << 70) + 5);
static const u128 K12 = 18446744073709551615ULL * (u128) 18446744073709551615ULL;
static const i128 K13 = ((i128) 0x7fffffffffffffffLL << 64) + 0x7fffffffffffffffLL;
static u128 arr[] = {((u128) 0x0123456789abcdefULL << 64) | 0xfedcba9876543210ULL,
                     (u128) 1 << 127,
                     (u128) (-(i128) 12345678901234567LL * 1000000007),
                     ~(u128) 0 / 10,
                     5,
                     (u128) -5,
                     (u128) 1.5};
enum { E1 = (i128) 5 << 2, E2 = (int) ((u128) 1 << 64 | 3) };
static int sz[(int) ((u128) 1 << 3)];

struct S {
  char c;
  u128 v;
  short s;
  i128 w;
};
static struct S gs = {1, (u128) 7 << 70, 2, -9};

static u128 fadd (u128 a, u128 b) { return a + b; }
static i128 fmix (int a, i128 b, double c, u128 d, char e, i128 f, long g, u128 h, int i, i128 j) {
  return a + b + (i128) c + (i128) d + e + f + g + (i128) h + i + j;
}
static u128 fstruct (struct S s) { return s.v ^ (u128) s.w ^ s.c ^ s.s; }
static struct S fretstruct (u128 v) {
  struct S s = {3, v, 4, (i128) v * -2};
  return s;
}
static u128 fptr (u128 *pv, int n) {
  u128 sum = 0;
  for (int i = 0; i < n; i++) sum += pv[i] * (i + 1);
  return sum;
}
static int fnarrow (int x) { return x * 2; }
static double fdbl (double x) { return x / 2; }
static u128 (*fp) (u128, u128) = fadd;
static u128 g_u = 42;
static i128 g_i;

static u128 fact (int n) { return n <= 1 ? 1 : n * fact (n - 1); }

static void to_dec (u128 v, char *buf) {
  char tmp[64];
  int n = 0;
  do {
    tmp[n++] = (char) ('0' + (int) (v % 10));
    v /= 10;
  } while (v);
  for (int i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
  buf[n] = 0;
}

int main (void) {
  char buf[64];
  p ("K1", K1);
  p ("K2", K2);
  p ("K3", (u128) K3);
  p ("K4", K4);
  p ("K5", (u128) K5);
  p ("K6", (u128) K6);
  p ("K7", K7);
  p ("K8", (u128) K8);
  p ("K9", K9);
  p ("K10", K10);
  p ("K11", (u128) K11);
  p ("K12", K12);
  p ("K13", (u128) K13);
  printf ("D1 %.17g F1 %.9g D2 %.17g\n", D1, F1, D2);
  printf ("C %d %d %d %d %d %d %d %lld\n", C1, C2, C3, C4, C5, C6, C7, C8);
  for (int i = 0; i < 7; i++) {
    sprintf (buf, "arr%d", i);
    p (buf, arr[i]);
  }
  printf ("E %d %d sz %d\n", E1, E2, (int) (sizeof (sz) / sizeof (sz[0])));
  printf ("sizeof %d %d %d %d align %d %d\n", (int) sizeof (u128), (int) sizeof (I),
          (int) sizeof (struct S), (int) sizeof (u128[3]), (int) _Alignof (u128),
          (int) _Alignof (struct S));
  printf ("offs %d %d %d\n", (int) offsetof (struct S, v), (int) offsetof (struct S, s),
          (int) offsetof (struct S, w));
  p ("gs.v", gs.v);
  p ("gs.w", (u128) gs.w);
  p ("fadd", fadd (K1, K2));
  p ("fp", fp (K1, 5));
  p ("fmix", (u128) fmix (1, -2, 3.75, 4, 5, -6, 7, 8, 9, (i128) 1 << 80));
  p ("fstruct", fstruct (gs));
  struct S rs = fretstruct (K1);
  p ("fret.v", rs.v);
  p ("fret.w", (u128) rs.w);
  printf ("fret %d %d\n", rs.c, rs.s);
  u128 a[5] = {1, K1, K2, (u128) -1, 77};
  p ("fptr", fptr (a, 5));
  p ("fact30", fact (30));
  to_dec (fact (34), buf);
  printf ("fact34 %s\n", buf);
  to_dec (~(u128) 0, buf);
  printf ("max %s\n", buf);
  printf ("narrow %d %g\n", fnarrow ((int) (K1 + 1)), fdbl ((double) K1));
  /* compound assignment, mixed types */
  u128 x = 10;
  x += 5;
  p ("x+=", x);
  x *= K1;
  p ("x*=", x);
  x -= (u128) -3;
  p ("x-=", x);
  x /= 7;
  p ("x/=", x);
  x %= 1000003;
  p ("x%=", x);
  x <<= 100;
  p ("x<<=", x);
  x >>= 3;
  p ("x>>=", x);
  x |= 1;
  x ^= K2;
  x &= ~(u128) 6;
  p ("x|^&", x);
  int sh = 77;
  x = K1;
  x <<= sh;
  p ("x<<=var", x);
  x >>= sh;
  p ("x>>=var", x);
  i128 y = -1000;
  y >>= 3;
  p ("y>>=", (u128) y);
  y /= -3;
  p ("y/=", (u128) y);
  int n = 7;
  n += K1;
  printf ("n+= %d\n", n);
  n = 100;
  n *= (i128) -3;
  printf ("n*= %d\n", n);
  unsigned char uc = 200;
  uc += (u128) 100;
  printf ("uc+= %d\n", uc);
  double d = 1.5;
  d += K1;
  printf ("d+= %.17g\n", d);
  d = 3.5e30;
  u128 xd = 2;
  xd *= d;
  p ("xd*=", xd);
  xd += 0.75;
  p ("xd+=0.75", xd);
  float f = 2.5f;
  f *= (i128) -4;
  printf ("f*= %g\n", f);
  char *cp = buf;
  cp += (u128) 3;
  printf ("cp+= %d\n", (int) (cp - buf));
  x = 5;
  p ("x++", x++);
  p ("x", x);
  p ("++x", ++x);
  p ("x--", x--);
  p ("--x", --x);
  x = 0;
  x--;
  p ("0--", x);
  x++;
  p ("++", x);
  y = (i128) 1 << 64;
  y--;
  p ("y--", (u128) y);
  /* conditionals */
  u128 big = (u128) 1 << 64;
  p ("cond1", big ? K1 : 3);
  p ("cond2", 0 ? K1 : 3);
  printf ("cond3 %.17g\n", big ? 1.5 : (double) K1);
  printf ("cond4 %.17g\n", !big ? 1.5 : K1);
  i128 neg = -5;
  p ("cond5", neg < 0 ? (u128) neg : 7u);
  int cnt = 0;
  for (u128 i = (u128) 1 << 64; i != ((u128) 1 << 64) + 10; i++) cnt++;
  printf ("loop %d\n", cnt);
  u128 w = 5;
  while (w--) cnt++;
  printf ("while %d\n", cnt);
  if (big) cnt += 100;
  if (!big) cnt += 1000;
  if (big && (big >> 64)) cnt += 10000;
  if ((big & 1) || 0) cnt += 100000;
  printf ("if %d\n", cnt);
  /* pointers, memory */
  u128 *pp = &g_u;
  *pp += 1;
  p ("g_u", g_u);
  g_i = -g_u;
  p ("g_i", (u128) g_i);
  u128 m;
  memcpy (&m, &K1, sizeof m);
  p ("memcpy", m);
  uint64_t halves[2];
  memcpy (halves, &K1, 16);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  printf ("halves %016llx %016llx\n", (unsigned long long) halves[0],
          (unsigned long long) halves[1]);
#else
  printf ("halves %016llx %016llx\n", (unsigned long long) halves[1],
          (unsigned long long) halves[0]);
#endif
  /* _Generic, statement expression, compound literal, comma */
  printf ("gen %d %d %d\n", _Generic (K1, u128: 1, i128: 2, default: 3),
          _Generic ((i128) 1, u128: 1, i128: 2, default: 3),
          _Generic (K1 + 1, u128: 1, default: 3));
  p ("stmt", ({
       u128 t = K1;
       t * 3;
     }));
  p ("clit", (u128[]){99, 1}[0] + (u128[2]){K2}[0]);
  p ("comma", (cnt++, K1 + 1));
  /* bool and narrow conversions */
  _Bool b1 = big, b2 = (u128) 0;
  printf ("bool %d %d\n", b1, b2);
  long long ll = (i128) -5;
  unsigned long long ull = K1;
  int ii = K1;
  short ss = K1;
  printf ("narrowing %lld %llu %d %d\n", ll, ull, ii, ss);
  double dd[] = {0.0,    1.0,    0.5,    1.9999, 18446744073709551616.0, 1e20,
                 3.4e38, 1.7e38, 9.2e18, 9.3e18, 1.8446744073709552e19};
  for (int i = 0; i < 11; i++) {
    sprintf (buf, "dbl2u%d", i);
    p (buf, (u128) dd[i]);
    sprintf (buf, "dbl2i%d", i);
    p (buf, (u128) (i128) -dd[i]);
  }
  long double ld = 0x1.fffffffffffffp+110L; /* exact in any host's long double */
  p ("ld2u", (u128) ld);
  p ("ld2i", (u128) (i128) -ld);
  float ff = 1.5e20f;
  p ("f2u", (u128) ff);
  u128 fv[] = {0,
               1,
               ((u128) 1 << 64) + 1,
               ((u128) 1 << 64) + 0x800,
               ((u128) 1 << 64) + 0x801,
               ((u128) 1 << 64) + 0xfff,
               (((u128) 1 << 53) + 1) << 64,
               ~(u128) 0,
               ((u128) 1 << 127) + 1,
               (u128) 0x1fffffffffffffULL << 75 | 0xffff};
  for (int i = 0; i < 10; i++)
    printf ("u2f%d %.17g %.9g %.17g %.17g\n", i, (double) fv[i], (float) (fv[i] >> 1),
            (double) (long double) (fv[i] >> 75), (double) (i128) fv[i]);
  return 0;
}
