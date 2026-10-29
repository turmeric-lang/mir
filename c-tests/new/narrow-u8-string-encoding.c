/* Narrow and u8 string literals: a source byte, \x or octal escape is one
   byte as written; only a \u / \U universal character name is UTF-8 encoded.
   c2mir once encoded every value above 0x7F in a u8 literal (u8"\xc3\xa9"
   came out four bytes, and so did u8"é" typed in UTF-8 source) and none in a
   narrow one ("é" came out as the single byte 0xe9). */
#include <stdio.h>

static void dump (const char *name, const char *s, int n) {
  printf ("%s %d:", name, n);
  for (int i = 0; i < n; i++) printf (" %02x", (unsigned char) s[i]);
  printf ("\n");
}

#define DUMP(name, lit) dump (name, lit, (int) sizeof (lit))

int main (void) {
  DUMP ("x", "\xc3\xa9");
  DUMP ("u8x", u8"\xc3\xa9");
  DUMP ("oct", "\303\251");
  DUMP ("u8oct", u8"\303\251");
  DUMP ("src", "é");
  DUMP ("u8src", u8"é");
  DUMP ("ucn", "é");
  DUMP ("u8ucn", u8"é");
  DUMP ("ucn3", "€");
  DUMP ("u8ucn4", u8"\U0001F600");
  return 0;
}
