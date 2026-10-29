/* An identifier that starts with `u8` but is not a u8"..." string literal
   must keep its spelling.  The lexer once replaced the `u` with a second `8`
   when backing out of the u8 literal prefix, so `u8x16` became `88x16`:
   consistently, so ordinary uses still agreed, but stringification, token
   pasting and external symbol names did not. */
#include <stdio.h>
#include <string.h>

#define STR(x) #x
#define XSTR(x) STR (x)
#define CAT(a, b) a##b

int u8 = 1;               /* `u8` alone */
int u8x16 = 2;            /* `u8` then letters and digits */
int CAT (s_, u8x) (void) { return 3; } /* a pasted operand starting `u8` */
int CAT (u8, y) = 4;      /* `u8 ## y` */

int main (void) {
  printf ("%s %s %s\n", STR (u8), STR (u8x16), XSTR (CAT (u8, z)));
  printf ("%d %d %d %d\n", u8, u8x16, s_u8x (), u8y);
  /* the literal prefix itself still works */
  const char *s = u8"ab";
  printf ("%d %d\n", (int) sizeof (u8"abc"), s[1]);
  return strcmp (STR (u8x16), "u8x16") != 0;
}
