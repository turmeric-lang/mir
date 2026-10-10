/* Every __int128 operator over a table of edge values, signed and unsigned,
   folded into one checksum per operator class.  c2mir lowers each operation to
   64-bit MIR (MIR has no wider integer), so the classes that need carries or
   long sequences -- multiplication, division, shifts by 64 or more, the
   conversions to and from floating point -- are where a slip would show.  The
   expected output is gcc's.  When a line differs, print the values instead of
   folding them to find the operands.  */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef unsigned __int128 u128;
typedef __int128 i128;

#define MK(hi, lo) (((u128) (hi) << 64) | (u128) (lo))
#define VALS                                                                                    \
  {                                                                                             \
    0, 1, 2, 3, 7, 10, 255, MK (0, 0x7fffffffffffffffULL), MK (0, 0x8000000000000000ULL),       \
      MK (0, 0xffffffffffffffffULL), MK (1, 0), MK (1, 1), MK (1, 0xffffffffffffffffULL),       \
      MK (0x7fffffffffffffffULL, 0xffffffffffffffffULL), MK (0x8000000000000000ULL, 0),         \
      MK (0x8000000000000000ULL, 5), MK (0xffffffffffffffffULL, 0xffffffffffffffffULL),         \
      MK (0xffffffffffffffffULL, 0xfffffffffffffffeULL), MK (0xffffffffffffffffULL, 0),         \
      MK (0x0123456789abcdefULL, 0xfedcba9876543210ULL),                                        \
      MK (0xdeadbeefcafebabeULL, 0x0badf00d12345678ULL), MK (0x54b40b1f852bda00ULL, 0),         \
      MK (0, 10000000000000000000ULL), MK (0x0000000c9f2c9cd0ULL, 0x4674edea40000000ULL),       \
      MK (3, 0x123456789ULL), MK (0x100000000ULL, 7), MK (0xffffffff00000000ULL, 0xffffffffULL) \
  }

static u128 static_vals[] = VALS; /* constant-folded */
#define NV (int) (sizeof (static_vals) / sizeof (static_vals[0]))

enum { NEG, NOT, CMP, CONV, FLT, SHL, SHR, SAR, ADD, SUB, MUL, BITS, DIV, MOD, SDIV, SMOD, NCLASS };
static const char *names[] = {"neg", "not", "cmp", "conv", "float", "shl", "shr",  "sar",
                              "add", "sub", "mul", "bits", "div",   "mod", "sdiv", "smod"};
static uint64_t sums[NCLASS];

static void mix (int cl, u128 v) {
  uint64_t h = sums[cl] ^ (uint64_t) v;
  h *= 0x100000001b3ULL;
  h ^= (uint64_t) (v >> 64);
  sums[cl] = h * 0x100000001b3ULL;
}

static void sweep (u128 *vals) {
  for (int k = 0; k < NCLASS; k++) sums[k] = 0xcbf29ce484222325ULL;
  for (int i = 0; i < NV; i++) {
    u128 a = vals[i];
    i128 sa = (i128) a;
    double d;
    float f;
    uint64_t bits;
    uint32_t fbits;

    mix (NEG, -a);
    mix (NEG, (u128) -sa);
    mix (NOT, ~a);
    mix (NOT, !a);
    mix (CONV, (_Bool) a);
    mix (CONV, (unsigned char) a);
    mix (CONV, (u128) (signed char) sa);
    mix (CONV, (unsigned short) a);
    mix (CONV, (u128) (short) sa);
    mix (CONV, (unsigned) a);
    mix (CONV, (u128) (int) sa);
    mix (CONV, (uint64_t) a);
    mix (CONV, (u128) (int64_t) sa);
    /* conversions to floating point round: check the bits.  Back to an
       integer, stay in range (anything else is undefined), and keep long
       double to 53 bits, which every host's long double holds exactly.  */
    d = (double) a;
    memcpy (&bits, &d, sizeof (bits));
    mix (FLT, bits);
    d = (double) sa;
    memcpy (&bits, &d, sizeof (bits));
    mix (FLT, bits);
    f = (float) (a >> 1);
    memcpy (&fbits, &f, sizeof (fbits));
    mix (FLT, fbits);
    f = (float) sa;
    memcpy (&fbits, &f, sizeof (fbits));
    mix (FLT, fbits);
    mix (FLT, (u128) (double) (a >> 1));
    mix (FLT, (u128) (i128) (double) (sa >> 1));
    mix (FLT, (u128) (float) (a >> 1));
    mix (FLT, (u128) (i128) (float) (sa >> 1));
    mix (FLT, (u128) (long double) (a >> 75));
    mix (FLT, (u128) (i128) (long double) (sa >> 75));
    for (int s = 0; s < 128; s++) {
      mix (SHL, a << s);
      mix (SHR, a >> s);
      mix (SAR, (u128) (sa >> s));
    }
    mix (SHL, (a << 0) ^ (a << 1) ^ (a << 63) ^ (a << 64) ^ (a << 65) ^ (a << 127));
    mix (SHR, (a >> 0) ^ (a >> 1) ^ (a >> 63) ^ (a >> 64) ^ (a >> 65) ^ (a >> 127));
    mix (SAR, (u128) ((sa >> 0) ^ (sa >> 1) ^ (sa >> 63) ^ (sa >> 64) ^ (sa >> 65) ^ (sa >> 127)));
    for (int j = 0; j < NV; j++) {
      u128 b = vals[j];
      i128 sb = (i128) b;
      mix (ADD, a + b);
      mix (SUB, a - b);
      mix (MUL, a * b);
      mix (MUL, (u128) (sa * sb));
      mix (BITS, a & b);
      mix (BITS, a | b);
      mix (BITS, a ^ b);
      mix (CMP, (a == b) | (a != b) << 1 | (a < b) << 2 | (a <= b) << 3 | (a > b) << 4
                  | (a >= b) << 5 | (sa < sb) << 6 | (sa <= sb) << 7 | (sa > sb) << 8
                  | (sa >= sb) << 9 | (a && b) << 10 | (a || b) << 11);
      if (b != 0) {
        mix (DIV, a / b);
        mix (MOD, a % b);
        if (sb != -1 || sa != (i128) MK (0x8000000000000000ULL, 0)) {
          mix (SDIV, (u128) (sa / sb));
          mix (SMOD, (u128) (sa % sb));
        }
      }
    }
  }
}

int main (void) {
  u128 vals[] = VALS; /* computed at run time */

  sweep (vals);
  for (int k = 0; k < NCLASS; k++) printf ("%s %016llx\n", names[k], (unsigned long long) sums[k]);
  sweep (static_vals);
  for (int k = 0; k < NCLASS; k++) printf ("%s %016llx\n", names[k], (unsigned long long) sums[k]);
  return 0;
}
