/* Long doubles must survive calls and lazily generated block boundaries.
   AAPCS64 preserves only the low 64 bits of v8-v15 across a call, so on
   aarch64 Linux (128-bit long double) one kept in v8-v15 lost its upper half
   whenever native code ran in between -- a C callee, or the lazy generation
   hooks of -el and -eb.  f keeps more long doubles live across a branch than
   the other vector registers hold; main keeps want live across the first,
   lazily generated, call of f. */
static int flag;
static long double vals[30];

static long double f (int c) {
  long double x0 = vals[0];
  long double x1 = vals[1];
  long double x2 = vals[2];
  long double x3 = vals[3];
  long double x4 = vals[4];
  long double x5 = vals[5];
  long double x6 = vals[6];
  long double x7 = vals[7];
  long double x8 = vals[8];
  long double x9 = vals[9];
  long double x10 = vals[10];
  long double x11 = vals[11];
  long double x12 = vals[12];
  long double x13 = vals[13];
  long double x14 = vals[14];
  long double x15 = vals[15];
  long double x16 = vals[16];
  long double x17 = vals[17];
  long double x18 = vals[18];
  long double x19 = vals[19];
  long double x20 = vals[20];
  long double x21 = vals[21];
  long double x22 = vals[22];
  long double x23 = vals[23];
  long double x24 = vals[24];
  long double x25 = vals[25];
  long double x26 = vals[26];
  long double x27 = vals[27];
  long double x28 = vals[28];
  long double x29 = vals[29];

  if (c) flag++;
  return x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12 + x13 + x14 + x15 + x16
         + x17 + x18 + x19 + x20 + x21 + x22 + x23 + x24 + x25 + x26 + x27 + x28 + x29;
}

int main (void) {
  long double want = 0;

  for (int i = 0; i < 30; i++) {
    vals[i] = (i + 1) * 1.25L;
    want += vals[i];
  }
  return f (1) == want ? 0 : 1;
}
