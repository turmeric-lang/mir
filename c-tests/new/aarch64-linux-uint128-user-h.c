/* The aarch64 target header declared its __uint128_t stand-in only under
   __APPLE__, so on Linux aarch64 glibc's <sys/user.h> -- which <ucontext.h>
   reaches through <sys/procfs.h> -- failed to parse at
   `__uint128_t vregs[32];` ("syntax error on struct (expected
   '<declarator>')"), and so did every program including <ucontext.h>.  The
   stand-in must keep AAPCS64's size and alignment there too, so glibc's
   struct user_fpsimd_struct matches the platform compiler's layout.
   Elsewhere this test has nothing to check. */
#if defined(__aarch64__) && defined(__linux__)
#include <ucontext.h>
#include <sys/user.h>
#endif

int main (void) {
#if defined(__aarch64__) && defined(__linux__)
  if (sizeof (__uint128_t) != 16 || _Alignof (__uint128_t) != 16) return 1;
  /* __uint128_t vregs[32]; unsigned int fpsr, fpcr; -- 520, padded to 528 */
  if (sizeof (struct user_fpsimd_struct) != 528) return 2;
  if (_Alignof (struct user_fpsimd_struct) != 16) return 3;
#endif
  return 0;
}
