/* GCC `__attribute__ ((aligned (N)))` on a struct member raises that
   member's alignment, as `_Alignas (N)` does, and so the struct's layout.
   c2mir parsed the attribute and dropped it, and libc headers erased it
   first anyway: glibc's <sys/cdefs.h> does `#define __attribute__(xyz)` for
   a compiler that is neither gcc nor clang.  glibc's aarch64 mcontext_t
   carries `__reserved[4096] __attribute__ ((__aligned__ (16)))`, so
   ucontext_t came out 4544 bytes / align 8 against gcc's 4560 / 16, and
   getcontext/swapcontext wrote past the end of a c2mir-allocated one.
   Expected layouts are gcc's and clang's on every LP64 target. */
#include <stddef.h>

struct plain {
  char c;
  unsigned char r[8] __attribute__ ((__aligned__ (16)));
};

/* a run of specifiers is one attribute list */
struct run {
  char c;
  int x __attribute__ ((__unused__)) __attribute__ ((aligned (32)));
};

/* a smaller N lowers nothing (that takes `packed`) */
struct smaller {
  char c;
  long l __attribute__ ((aligned (1)));
};

/* glibc's idiom, verbatim, must not erase the attribute */
#if !(defined __GNUC__ || defined __clang__)
#define __attribute__(xyz) /* Ignore */
#endif
struct libc_style {
  unsigned long long a;
  unsigned char reserved[64] __attribute__ ((__aligned__ (16)));
};

extern void noret (void) __attribute__ ((__noreturn__)) __attribute__ ((__nothrow__));

int main (void) {
  if (sizeof (struct plain) != 32 || _Alignof (struct plain) != 16) return 1;
  if (offsetof (struct plain, r) != 16) return 2;
  if (sizeof (struct run) != 64 || _Alignof (struct run) != 32) return 3;
  if (offsetof (struct run, x) != 32) return 4;
  if (offsetof (struct smaller, l) != sizeof (long)) return 5;
  if (sizeof (struct libc_style) != 80 || _Alignof (struct libc_style) != 16) return 6;
  if (offsetof (struct libc_style, reserved) != 16) return 7;
  return 0;
}
