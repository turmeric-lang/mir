/* Reading a union member back through an array member of the same union is
   the type pun C11 6.5.2.3 allows (footnote 95).  c2mir gave a scalar member
   access the union's alias but an array element its element type's, so the
   generator saw `u.ld = x` and `u.words[0]` as independent and read the words
   from before the store.  The interpreter ignores aliases, so only generated
   code was wrong: MIR's own put_ldouble (mir.c) wrote every long double as 0
   when c2mir compiled it.  Byte values are printed only for double and float,
   whose layout is IEEE on every host; long double is checked by round trip.  */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

union dw {
  double d;
  uint32_t w[2];
};

union fw {
  float f;
  unsigned char b[4];
};

union ldw {
  long double ld;
  uint64_t u[2];
};

struct holder {
  int tag;
  union {
    double d;
    struct {
      uint16_t h[4];
    } s;
  } u;
};

static uint64_t dwords (double d) {
  union dw u;

  u.w[0] = u.w[1] = 0;
  u.d = d;
  return (uint64_t) u.w[0] | (uint64_t) u.w[1] << 32;
}

static unsigned fbytes (float f) {
  union fw u;

  memset (u.b, 0, sizeof (u.b));
  u.f = f;
  return u.b[0] + u.b[1] + u.b[2] + u.b[3];
}

static double from_words (uint32_t lo, uint32_t hi) {
  union dw u;

  u.d = 0.0;
  u.w[0] = lo;
  u.w[1] = hi;
  return u.d;
}

static int ld_round_trip (long double x) {
  union ldw a, b;

  a.u[0] = a.u[1] = 0;
  b.u[0] = b.u[1] = 0;
  a.ld = x;
  b.u[0] = a.u[0];
  b.u[1] = a.u[1];
  return b.ld == x;
}

static unsigned nested (struct holder *p, double d) {
  p->u.s.h[0] = p->u.s.h[1] = p->u.s.h[2] = p->u.s.h[3] = 0;
  p->u.d = d;
  return (unsigned) p->u.s.h[0] ^ p->u.s.h[1] ^ p->u.s.h[2] ^ p->u.s.h[3];
}

int main (void) {
  struct holder h;
  uint64_t w = dwords (1.5);

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
  w = w >> 32 | w << 32;
#endif
  printf ("double %016llx\n", (unsigned long long) w);
  printf ("float %u\n", fbytes (-2.75f));
  printf ("words %.17g\n", from_words (0, 0x40590000)); /* 100.0 on little endian */
  printf ("ld %d %d %d\n", ld_round_trip (18446744073709551616.0L), ld_round_trip (-0.1L),
          ld_round_trip (3.0L));
  printf ("nested %u\n", nested (&h, 2.5));
  return 0;
}
