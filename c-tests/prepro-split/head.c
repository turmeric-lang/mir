/* The part of a translation unit c2m -E -fdump-defines preprocesses once:
   prepro-split.sh checks that [this file's -E output][rest.c] compiles and
   runs as [this file][rest.c] does. */
#pragma pack(push, 4)
struct p4 {
  unsigned int a;
  unsigned long long b;
};
#pragma pack(push, 2) /* still in effect, under a 4, at the end of this part */
struct p2 {
  char c;
  int i;
};

#define TWICE(x) ((x) + (x))
#define NEG -1
#define STR(x) #x
#define XSTR(x) STR (x)
#define CAT(a, b) a##b
#define SUM(...) sum (__VA_ARGS__)
#define NOARGS() 7
#define EMPTY
#define PAREN (3)
#define SELF SELF
#define LATER UNDEFINED_YET
#define GONE 1
#undef GONE

#undef CMD /* from the command line */
#define CMD 2
#undef __MIRC_MUSTTAIL__ /* predefined, and only undefined */
#undef __STDC_NO_VLA__   /* predefined, and redefined */
#define __STDC_NO_VLA__ 2

typedef unsigned char u8; /* an identifier starting with u8 */
static u8 u8_val = 200;
static int sum (int a, int b) { return a + b; }
static int neg_neg = -NEG; /* `- -1`, not `--1` */
