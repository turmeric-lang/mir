/* A function with no return -- it only leaves through exit -- inlined into
   its caller: the inliner looked for the callee's last ret to splice the
   result through, found none, and asserted (a release build crashed).  Such
   a function is no longer inlined.  */
#include <stdlib.h>
#include <stdio.h>
static long spin (long n) {
again:;
  long t = n + 1;
  if (t > 1000) { printf ("%ld\n", t); exit (0); }
  n = t;
  goto again;
}
int main (void) {
  printf ("%ld\n", spin (0));
  return 1;
}
