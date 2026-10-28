/* A _Generic controlling expression undergoes lvalue conversion and nothing
   else (C11 6.5.1.1p2, C17 DR 481): qualifiers drop and arrays and functions
   decay, but the integer promotions do not happen.  c2mir promoted it, so a
   short or a char selected the int association.  The promotion also hid that
   c2mir typed a character constant char rather than int (C11 6.4.4.4p10), so
   sizeof ('a') was 1.  gcc and clang print the expected output.  */
#include <stdio.h>

static int f (void) { return 0; }

int main (void) {
  short s = 0;
  signed char sc = 0;
  unsigned char uc = 0;
  const int ci = 0;
  int a[3];

  printf ("%d %d %d\n", _Generic (s, short: 1, int: 2, default: 0),
          _Generic (sc, signed char: 1, int: 2, default: 0),
          _Generic (uc, unsigned char: 1, int: 2, default: 0));
  printf ("%d %d %d\n", _Generic ((short) 0, short: 1, int: 2, default: 0),
          _Generic ((short *) 0, short *: 1, default: 0), _Generic (ci, int: 1, default: 0));
  printf ("%d %d\n", _Generic (a, int *: 1, default: 0), _Generic (f, int (*) (void): 1, default: 0));
  /* A character constant is an int, and an arithmetic result is promoted. */
  printf ("%d %d\n", _Generic ('a', int: 1, char: 2, default: 0), _Generic (s + s, int: 1, default: 0));
  printf ("%d %d\n", sizeof ('a') == sizeof (int), (char) -1 < 0 ? '\xff' == -1 : '\xff' == 255);
  return 0;
}
