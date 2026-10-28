/* End-to-end check of the wasm32 MIR target: compile C with c2mir, link it
   in interpreter mode, and call it -- including calls out to libc. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <setjmp.h>

#include "mir.h"
#include "mir-dlist.h"
#include "c2mir.h"

static const char *src_ptr;
/* libc headers: c2mir ships only the compiler-provided ones, so the
   embedder serves the rest (README "System headers are the embedder's
   job").  run.sh passes Emscripten's own musl sysroot, read straight off
   the host disk under -sNODERAWFS, which is what a real consumer parses. */
static const char *include_dirs[1];
static size_t include_dirs_num;

static int string_getc (void *data) {
  (void) data;
  if (*src_ptr == '\0') return EOF;
  return *src_ptr++;
}

static void __attribute__ ((noreturn)) err (MIR_error_type_t t, const char *fmt, ...) {
  (void) t;
  va_list ap;
  va_start (ap, fmt);
  fprintf (stderr, "MIR ERROR: ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (1);
}

/* Native code that calls interpreted functions through pointers it was
   handed: each is a genuine call_indirect, which a thunk address only
   survives if it is a real wasm table entry.  host_slot hands interpreted
   code the address of a callback slot it then writes itself, as a runtime's
   struct field is filled in, so the pointer never crosses a call;
   host_call_i64 receives it as a 64-bit integer, as a language that passes
   function values in an int64_t does. */
static int (*stored_fn) (int);
static int (**host_slot (void)) (int) { return &stored_fn; }
static int host_call_stored (int x) { return stored_fn (x); }
static int host_call_i64 (long long fp, int x) { return ((int (*) (int)) (intptr_t) fp) (x); }
static double host_call_d (double (*fn) (double), double x) { return fn (x); }
static long long host_call_ll (long long (*fn) (long long, long long), long long a, long long b) {
  return fn (a, b);
}

static char setjmp_stand_in;

static int run (const char *code, const char *entry, int64_t *out) {
  MIR_context_t ctx = MIR_init ();
  struct c2mir_options ops;
  MIR_module_t m;
  MIR_item_t f, target = NULL;
  MIR_val_t res;

  MIR_set_error_func (ctx, err);
  c2mir_init (ctx);
  memset (&ops, 0, sizeof (ops));
  ops.message_file = stderr;
  ops.include_dirs = include_dirs;
  ops.include_dirs_num = include_dirs_num;
  src_ptr = code;

  if (!c2mir_compile (ctx, &ops, string_getc, NULL, "<test>", NULL)) {
    fprintf (stderr, "compile failed\n");
    return 0;
  }
  for (m = DLIST_HEAD (MIR_module_t, *MIR_get_module_list (ctx)); m != NULL;
       m = DLIST_NEXT (MIR_module_t, m)) {
    for (f = DLIST_HEAD (MIR_item_t, m->items); f != NULL; f = DLIST_NEXT (MIR_item_t, f))
      if (f->item_type == MIR_func_item && strcmp (f->u.func->name, entry) == 0) target = f;
    MIR_load_module (ctx, m);
  }
  if (target == NULL) {
    fprintf (stderr, "entry %s not found\n", entry);
    return 0;
  }
  MIR_load_external (ctx, "printf", (void *) printf);
  MIR_load_external (ctx, "puts", (void *) puts);
  MIR_load_external (ctx, "strlen", (void *) strlen);
  MIR_load_external (ctx, "sqrt", (void *) sqrt);
  MIR_load_external (ctx, "abs", (void *) abs);
  MIR_load_external (ctx, "vsnprintf", (void *) vsnprintf);
  MIR_load_external (ctx, "snprintf", (void *) snprintf);
  MIR_load_external (ctx, "qsort", (void *) qsort);
  MIR_load_external (ctx, "host_slot", (void *) host_slot);
  MIR_load_external (ctx, "host_call_stored", (void *) host_call_stored);
  MIR_load_external (ctx, "host_call_i64", (void *) host_call_i64);
  MIR_load_external (ctx, "host_call_d", (void *) host_call_d);
  MIR_load_external (ctx, "host_call_ll", (void *) host_call_ll);
  /* setjmp's address cannot be taken on wasm (emcc: "Indirect use of setjmp
     is not supported"), so it resolves to any non-NULL stand-in and the
     interpreter calls setjmp itself; longjmp's address is an ordinary one. */
  MIR_load_external (ctx, "setjmp", (void *) &setjmp_stand_in);
  MIR_load_external (ctx, "longjmp", (void *) longjmp);
  /* musl's <math.h> defines static inline isless & co. over these; c2mir
     keeps unused static inline functions, so their imports must resolve even
     when nothing calls them. */
  MIR_load_external (ctx, "__fpclassify", (void *) __fpclassify);
  MIR_load_external (ctx, "__fpclassifyf", (void *) __fpclassifyf);
  MIR_load_external (ctx, "__fpclassifyl", (void *) __fpclassifyl);
  MIR_link (ctx, MIR_set_interp_interface, NULL);
  MIR_interp (ctx, target, &res, 0);
  *out = res.i;
  /* Finished, so a context's thunks and table entries are released: an
     embedder like a web page runs program after program in one module. */
  c2mir_finish (ctx);
  MIR_finish (ctx);
  return 1;
}

#define CHECK(label, code, entry, expect)                                       \
  do {                                                                          \
    int64_t r = 0;                                                              \
    printf ("--- %s ---\n", label);                                             \
    if (!run (code, entry, &r)) {                                               \
      printf ("  FAIL (did not run)\n");                                        \
      fails++;                                                                  \
    } else if (r != (expect)) {                                                 \
      printf ("  FAIL got %lld want %lld\n", (long long) r, (long long) (expect)); \
      fails++;                                                                  \
    } else {                                                                    \
      printf ("  ok (returned %lld)\n", (long long) r);                         \
    }                                                                           \
  } while (0)

int main (int argc, char **argv) {
  int fails = 0;

  if (argc > 1) {
    include_dirs[0] = argv[1];
    include_dirs_num = 1;
  }

  CHECK ("pure arithmetic (no FFI)", "int t (void) { int a = 6, b = 7; return a * b; }", "t", 42);

  CHECK ("printf with no varargs", "#include <stdio.h>\n"
                                   "int t (void) { return printf (\"hello\\n\"); }",
         "t", 6);

  CHECK ("printf %d", "#include <stdio.h>\n"
                      "int t (void) { return printf (\"int=%d\\n\", 42); }",
         "t", 7);

  CHECK ("printf %s and %c", "#include <stdio.h>\n"
                             "int t (void) { return printf (\"%s%c\\n\", \"str\", 33); }",
         "t", 5);

  CHECK ("printf %f (double vararg)", "#include <stdio.h>\n"
                                      "int t (void) { return printf (\"%.2f\\n\", 3.5); }",
         "t", 5);

  /* "7 x 2.5\n" is 8 characters */
  CHECK ("mixed varargs", "#include <stdio.h>\n"
                          "int t (void) { return printf (\"%d %s %.1f\\n\", 7, \"x\", 2.5); }",
         "t", 8);

  /* c2mir builds the vararg buffer at the call site, where the argument's
     real width is still known (README "Variadic calls"). */
  CHECK ("printf %lld", "#include <stdio.h>\n"
                                     "int t (void) { return printf (\"%lld\\n\", 12345678901ll); }",
         "t", 12);

  CHECK ("non-variadic ptr->int (strlen)", "#include <string.h>\n"
                                           "int t (void) { return strlen (\"abcde\"); }",
         "t", 5);

  CHECK ("double->double (sqrt)", "#include <math.h>\n"
                                  "int t (void) { return (int) sqrt (144.0); }",
         "t", 12);

  CHECK ("int->int (abs)", "#include <stdlib.h>\n"
                           "int t (void) { return abs (-17); }",
         "t", 17);

  CHECK ("ptr->int (puts)", "#include <stdio.h>\n"
                            "int t (void) { puts (\"via puts\"); return 1; }",
         "t", 1);

  CHECK ("call in a loop", "#include <stdio.h>\n"
                           "int t (void) { int i, n = 0; for (i = 0; i < 3; i++) n += printf (\"i=%d\\n\", i); return n; }",
         "t", 12);

  /* The predefined type macros must agree with clang's on wasm32: musl
     derives size_t/ptrdiff_t/intptr_t from __PTRDIFF_TYPE__ and typedefs
     wchar_t and wint_t from __WCHAR_TYPE__/__WINT_TYPE__, and the host
     functions an interpreted call reaches were compiled by clang. */
  CHECK ("musl headers parse together",
         "#include <stddef.h>\n#include <stdint.h>\n#include <stdarg.h>\n"
         "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n"
         "#include <wchar.h>\n#include <time.h>\n#include <errno.h>\n"
         "#include <setjmp.h>\n#include <unistd.h>\n#include <fcntl.h>\n"
         "#include <pthread.h>\n"
         "int t (void) { size_t n = strlen (\"abc\"); wchar_t w = L'x'; return (int) n + (w == 'x'); }",
         "t", 4);

  CHECK ("pointer-sized types are long, as clang's",
         "#include <stddef.h>\n#include <stdint.h>\n#include <sys/types.h>\n"
         "int t (void) {\n"
         "  return _Generic ((ptrdiff_t) 0, long: 1, default: 0)\n"
         "       + _Generic ((intptr_t) 0, long: 2, default: 0)\n"
         "       + _Generic ((uintptr_t) 0, unsigned long: 4, default: 0)\n"
         "       + _Generic ((size_t) 0, unsigned long: 8, default: 0)\n"
         "       + _Generic ((ssize_t) 0, long: 16, default: 0);\n"
         "}",
         "t", 31);

  /* Through pointer types: c2mir applies the integer promotions to a
     _Generic controlling expression, so (short) 0 would select int. */
  CHECK ("fast/least and wide-char types match clang's",
         "#include <stdint.h>\n#include <wchar.h>\n#include <uchar.h>\n"
         "int t (void) {\n"
         "  return _Generic ((int_fast16_t *) 0, short *: 1, default: 0)\n"
         "       + _Generic ((uint_fast16_t *) 0, unsigned short *: 2, default: 0)\n"
         "       + _Generic ((int_least8_t *) 0, signed char *: 4, default: 0)\n"
         "       + _Generic ((wchar_t *) 0, int *: 8, default: 0)\n"
         "       + _Generic ((wint_t *) 0, int *: 16, default: 0)\n"
         "       + (INT_FAST16_MAX == 32767) * 32 + (sizeof (char32_t) == 4) * 64;\n"
         "}",
         "t", 127);

  /* musl's <stdio.h> declares vprintf & co. over __isoc_va_list, which
     bits/alltypes.h typedefs from __builtin_va_list; c2mir's own <stdarg.h>
     must give va_list that same type.  (A variadic function defined in
     interpreted code cannot be called here -- README "Known limitations" --
     so this checks the types rather than calling vsnprintf.) */
  CHECK ("va_list from <stdarg.h> is musl's __isoc_va_list",
         "#include <stdarg.h>\n#include <stdio.h>\n"
         "int t (void) {\n"
         "  return _Generic ((va_list *) 0, __isoc_va_list *: 1, default: 0)\n"
         "       + _Generic (&vsnprintf, int (*) (char *, size_t, const char *, va_list): 2, default: 0);\n"
         "}",
         "t", 3);

  /* An interpreted function's address handed to native code.  qsort gets it
     as an argument, which worked before; the next two find it in memory or
     receive it as a long long, which trapped with "table index is out of
     bounds". */
  CHECK ("qsort with an interpreted comparator",
         "#include <stdlib.h>\n"
         "static int cmp (const void *a, const void *b) { return *(const int *) a - *(const int *) b; }\n"
         "int t (void) { int v[5] = {5, 3, 4, 1, 2}; qsort (v, 5, sizeof (int), cmp);\n"
         "  return v[0] * 10000 + v[1] * 1000 + v[2] * 100 + v[3] * 10 + v[4]; }",
         "t", 12345);

  CHECK ("native code calls a function pointer stored in its memory",
         "int (**host_slot (void)) (int); int host_call_stored (int);\n"
         "static int sq (int x) { return x * x; }\n"
         "int t (void) { *host_slot () = sq; return host_call_stored (7); }",
         "t", 49);

  CHECK ("native code calls a function pointer passed as long long",
         "#include <stdint.h>\n"
         "int host_call_i64 (long long, int);\n"
         "static int triple (int x) { return 3 * x; }\n"
         "int t (void) { return host_call_i64 ((long long) (intptr_t) triple, 14); }",
         "t", 42);

  CHECK ("a double through the table entry",
         "double host_call_d (double (*) (double), double);\n"
         "static double half (double x) { return x / 2; }\n"
         "int t (void) { return (int) (host_call_d (half, 7.1) * 100 + 0.5); }",
         "t", 355);

  CHECK ("long long arguments and result through the table entry",
         "long long host_call_ll (long long (*) (long long, long long), long long, long long);\n"
         "static long long sub (long long a, long long b) { return a - b; }\n"
         "int t (void) { return host_call_ll (sub, 5000000000ll, 4999999958ll) == 42; }",
         "t", 1);

  /* setjmp in interpreted code lands in the frame that called it: longjmp
     unwinds four interpreted activations (and the JS frames of each call
     between them) back to it, three times over. */
  CHECK ("setjmp and longjmp in interpreted code",
         "#include <setjmp.h>\n"
         "static jmp_buf jb;\n"
         "static int hits;\n"
         "static int depth (int n) { if (n == 0) longjmp (jb, ++hits); return depth (n - 1) + 1; }\n"
         "int t (void) { int r = setjmp (jb); if (r < 3) depth (4); return r * 10 + hits; }",
         "t", 33);

  /* One module, many contexts, as a page runs program after program.  Each
     context interns its foreign-call signatures and takes a thunk per
     function; before both were reclaimed, the 100-signature pool ran out
     after 50 of these runs and the 8192 thunks after about 200.  The libc
     calls are declared rather than included, which keeps 250 compiles
     quick under -O0 and SAFE_HEAP. */
  {
    static char prog[8192];
    int i, n = 0, bad = 0;
    int64_t r = 0;

    n += snprintf (prog + n, sizeof prog - n,
                   "int snprintf (char *, unsigned long, const char *, ...);\n"
                   "unsigned long strlen (const char *);\n"
                   "static int f0 (int x) { return x + 1; }\n");
    for (i = 1; i < 40; i++)
      n += snprintf (prog + n, sizeof prog - n, "static int f%d (int x) { return f%d (x) + 1; }\n",
                     i, i - 1);
    snprintf (prog + n, sizeof prog - n,
              "int t (void) { char b[16]; snprintf (b, sizeof b, \"%%d\", f39 (0));"
              " return (int) strlen (b) + f39 (0); }");
    printf ("--- 250 contexts in one module ---\n");
    for (i = 0; i < 250 && !bad; i++)
      if (!run (prog, "t", &r) || r != 42) bad = 1;
    if (bad) {
      printf ("  FAIL at run %d (got %lld want 42)\n", i, (long long) r);
      fails++;
    } else {
      printf ("  ok (250 runs)\n");
    }
  }

  printf ("\n=== %s ===\n", fails == 0 ? "ALL PASSED" : "FAILURES PRESENT");
  return fails != 0;
}
