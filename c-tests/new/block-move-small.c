/* A copy of at most four words of an aggregate -- a word being its alignment,
   capped at 8 bytes -- is emitted as word loads and stores instead of a call
   to memcpy.  Every size either side of that limit for each alignment, a
   floating-point payload, and every copying context: assignment, by-value
   argument and return, both conditional-expression paths, a statement
   expression, and an exact self-overlap through pointers.  */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

struct q1 { int64_t a; };             /* 8, align 8: 1 word */
struct q2 { int64_t a, b; };          /* 16: 2 words */
struct q3 { int64_t a, b, c; };       /* 24: 3 words */
struct q4 { int64_t a, b, c, d; };    /* 32: 4 words */
struct q5 { int64_t a, b, c, d, e; }; /* 40: memcpy */
struct d2 { double x, y; };           /* 16, floating point */
struct w2 { int32_t a, b; };          /* 8, align 4: 2 words */
struct w3 { int32_t a, b, c; };       /* 12: 3 words */
struct w4 { int32_t a, b, c, d; };    /* 16: 4 words */
struct w5 { int32_t a, b, c, d, e; }; /* 20: memcpy */
struct h3 { int16_t a, b, c; };       /* 6, align 2: 3 words */
struct h4 { int16_t a, b, c, d; };    /* 8: 4 words */
struct h5 { int16_t a, b, c, d, e; }; /* 10: memcpy */
struct b6 { char s[6]; };             /* 6, align 1: memcpy */

#define DEF(T)                                                                  \
  static void fill_##T (struct T *p, int seed) {                                \
    unsigned char *b = (unsigned char *) p;                                     \
    for (size_t i = 0; i < sizeof *p; i++)                                      \
      b[i] = (unsigned char) (seed * 31 + i * 7 + 1);                           \
  }                                                                             \
  static unsigned long sum_##T (const struct T *p) {                            \
    const unsigned char *b = (const unsigned char *) p;                         \
    unsigned long s = 0;                                                        \
    for (size_t i = 0; i < sizeof *p; i++) s = s * 131 + b[i];                  \
    return s;                                                                   \
  }                                                                             \
  static struct T id_##T (struct T v) { return v; }                             \
  static unsigned long check_##T (int seed) {                                   \
    struct T a, b, c, d, e, g, *pa = &a, *pb = &a;                              \
    fill_##T (&a, seed);                                                        \
    fill_##T (&b, seed + 100);                                                  \
    b = a;                                     /* assignment */                 \
    c = id_##T (b);                            /* by-value argument, return */  \
    d = (seed & 1) ? c : b;                    /* conditional, into a dest */   \
    g = id_##T ((seed & 2) ? d : c);           /* conditional, as a value */    \
    e = ({ struct T t = g; t; });              /* statement expression */       \
    *pb = *pa;                                 /* exact overlap */              \
    return sum_##T (&a) == sum_##T (&e) ? sum_##T (&e) : 0;                     \
  }

DEF (q1) DEF (q2) DEF (q3) DEF (q4) DEF (q5) DEF (d2)
DEF (w2) DEF (w3) DEF (w4) DEF (w5) DEF (h3) DEF (h4) DEF (h5) DEF (b6)

#define RUN(T)                                                                  \
  printf ("%s size %d align %d: %lu %lu\n", #T, (int) sizeof (struct T),       \
          (int) _Alignof (struct T), check_##T (1), check_##T (2))

int main (void) {
  RUN (q1); RUN (q2); RUN (q3); RUN (q4); RUN (q5); RUN (d2);
  RUN (w2); RUN (w3); RUN (w4); RUN (w5); RUN (h3); RUN (h4); RUN (h5); RUN (b6);
  return 0;
}
