/* What follows head.c: it uses the state head.c left behind. */
#define UNDEFINED_YET 11

struct still2 { /* pack(2) is still in effect */
  char c;
  int i;
};
#pragma pack(pop)
struct back4 { /* pack(4) again */
  unsigned int a;
  unsigned long long b;
};
#pragma pack(pop)
struct natural {
  unsigned int a;
  unsigned long long b;
};

int printf (const char *, ...);
#define CHECK(c) \
  do { \
    if (!(c)) { \
      printf ("prepro-split: %s failed\n", #c); \
      return 1; \
    } \
  } while (0)

int main (void) {
  int CAT (x, y) = 5;
  const char *s = XSTR (TWICE (q));
  CHECK (sizeof (struct p4) == 12);
  CHECK (sizeof (struct p2) == 6);
  CHECK (sizeof (struct still2) == 6);
  CHECK (sizeof (struct back4) == 12);
  CHECK (sizeof (struct natural) == 2 * sizeof (unsigned long long));
  CHECK (TWICE (3) == 6 && -NEG == 1 && neg_neg == 1);
  CHECK (s[0] == '(' && s[1] == '(' && s[2] == 'q');
  CHECK (xy == 5 && SUM (2, 3) == 5 && NOARGS () == 7 && PAREN == 3);
  CHECK (LATER == 11 && u8_val == 200);
  CHECK (CMD == 2 && __STDC_NO_VLA__ == 2 EMPTY);
#ifdef GONE
  CHECK (0);
#endif
#ifdef __MIRC_MUSTTAIL__
  CHECK (0);
#endif
#ifndef SELF
  CHECK (0);
#endif
  return 0;
}
