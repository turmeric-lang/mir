# wasm32 / Emscripten target

Lets MIR + c2mir be compiled to WebAssembly and run in a browser or under
node. WebAssembly cannot generate machine code at runtime, so the JIT
generator is not built and MIR runs in **interpreter mode only**.

## Building

```sh
emcmake cmake -B build-emcc -S .
cmake --build build-emcc
```

`EMSCRIPTEN` disables `mir-gen.c` and all native executables/tests; a wasm
consumer links `mir_static` into its own Emscripten build.

Consumers must build with:

```
-sALLOW_TABLE_GROWTH
```

plus `-sWASM_BIGINT` on an older Emscripten (current ones enable it by
default, and emcc 6 reports the flag as deprecated). Every function MIR
loads takes an entry in the wasm table (see "Function pointers" below), so
without table growth the first `MIR_load_module` fails with `failed to grow
table`. The JavaScript runtime pieces `mir-wasm.c` uses (`wasmTable`,
`UTF8ToString`, `stackSave`/`stackAlloc`/`stackRestore`) are declared with
`EM_JS_DEPS`, so a consumer no longer exports them; `run.sh` builds with no
`EXPORTED_RUNTIME_METHODS` at all to keep that true.

## How foreign calls work

`mir-interp.c` routes every call from interpreted code into a native
function through `_MIR_get_ff_call`, which on native targets JITs a thunk
that marshals arguments into registers. That is impossible here.

A wasm `call_indirect` is strictly type-checked — casting a function
pointer and calling it traps with `null function or function signature
mismatch` unless arity *and* types match exactly — so a table of
pre-generated C stubs would need one entry per signature (~`4^nargs`).

Instead `mir-wasm.c` trampolines through JavaScript. A wasm function
pointer is an index into the wasm table, and `wasmTable.get(ptr)` yields a
JS-callable function; JS can spread an argument list of arbitrary length
and type. `_MIR_get_ff_call` records the signature in a slot and returns
one of 100 pre-generated stubs bound to that slot index (the returned
pointer must be a plain `void (*) (void *, void *)`, so the signature has
to be baked into it). The slots are interned by signature across contexts:
each context asks again for the signatures it calls through, and before
they were shared a page running one program after another in the same
module exhausted the pool after a few dozen programs. Exhausting it raises a
MIR error.

### Exact-signature stubs

The trampoline costs a JS round trip with BigInt conversions on every call
(about 180 ns against 13 ns for an interpreted loop iteration), and it
leaves a JS frame on the stack, which Asyncify cannot unwind through. An
embedder whose interpreted code mostly calls into its own runtime knows that
runtime's signatures — a few dozen, not `4^nargs` — and can register a C stub
for each with `MIR_wasm_register_ff_stub` (`mir.h`). The key is the wasm
signature: the result's type code, then each argument's (`i` i32, `j` i64,
`f` f32, `d` f64, `v` for no result), with a variadic callee's va buffer as a
final `i`. The stub reads its arguments from 8-byte slots, does the exact
`call_indirect`, and writes the result:

```c
static void stub_jji (void *fn, const void *slots, void *retbuf) {
  int64_t a0, r; int32_t a1;
  memcpy (&a0, slots, 8);
  memcpy (&a1, (const char *) slots + 8, 4);
  r = ((int64_t (*) (int64_t, int32_t)) fn) (a0, a1);
  memcpy (retbuf, &r, 8);
}
MIR_wasm_register_ff_stub ("jji", stub_jji);
```

A call whose signature has a stub takes it; any other still trampolines.
Since the stub's `call_indirect` is checked, a callee whose real type differs
from what the interpreted code declared traps where the trampoline would
have coerced the arguments.

## setjmp

Natively the interpreter calls `setjmp` through its address, in `eval`'s own
frame, so that a `longjmp` lands in the activation that asked. emcc refuses
any indirect use of `setjmp` (`Indirect use of setjmp is not supported`), so
here the embedder resolves `setjmp` to any non-NULL stand-in;
`MIR_load_external` replaces it with a marker, and `eval` calls `setjmp` by
name. `longjmp`'s address is an ordinary one and goes through the FFI. A tail
call or jcall of `setjmp`, which has no frame to land in, aborts.

## Function pointers

A MIR function's address is its *thunk*, handed out when the module is
loaded and redirected when it is linked. Here a thunk is an entry in the
wasm function table: `_MIR_get_thunk` grows the table by one null entry and
returns its index, and `_MIR_redirect_thunk` fills it -- with a copy of a
native function's entry, or, for an interpreted function, with a small wasm
function that trampolines through JavaScript into `MIR_interp_arr`. So an
interpreted function's address is an ordinary wasm function pointer.
Interpreted code that calls it still skips the table: `wasm_ff_dispatch`
recognises a thunk index and re-enters the interpreter directly. Native code
does a genuine `call_indirect` and lands in the trampoline, however the
address reached it: as an argument (`qsort`'s comparator), stored in memory
by interpreted code (a callback field), or passed as a 64-bit integer.

A table entry has to be a wasm function, so the JS trampoline is wrapped in
a one-import module exporting it at the function's signature -- what
Emscripten's `addFunction` does -- with the compiled module cached per
signature and only instantiated per function. Every function a program
defines gets one at link time.

`MIR_finish` releases a context's thunks (`wasm_context_finish`, from
`code_finish`): their entries go back to null and the next context reuses
them, so the table and the 8192-thunk pool stay bounded across programs. A
native caller still holding a pointer into a finished context traps on a
null entry rather than running freed code.

Variadic calls use Emscripten's ABI, where `f(fixed..., ...)` lowers to
`f(fixed..., void *vararg_buffer)`. c2mir's wasm32 target builds that buffer
at the call site and emits a non-variadic call (`VA_BUF_TARGET_P`), because
that is the last point where each argument's real C type is known —
`mir-interp.c` derives variadic argument types from an operand's value_mode
and cannot tell `long long` from `int`. `wasm_ff_dispatch` still has a
fallback for any other producer of vararg MIR calls, which lays integers out
in 4-byte slots and therefore truncates a genuine `long long`.

## System headers are the embedder's job

wasm32 ships only compiler-provided headers, exactly like every other
target. It used to also bundle minimal `stdio.h`, `stdlib.h`, `string.h`
and `math.h`, on the grounds that a browser has no filesystem to read the
real ones from. That put the declarations in the compiler and the matching
symbol table in the embedder — two different repositories — and they
drifted, until the headers declared roughly twice as many functions as the
embedder actually supplied. The surplus compiled cleanly and then failed at
`MIR_link`.

Serve libc through `c2mir_options.include_dirs` instead. Emscripten's MEMFS
is readable by c2mir's ordinary `fopen`-based include search, so an embedder
can write headers there at startup and generate both those headers and its
`MIR_load_external` table from a single list, which makes drift impossible.

Note that `standard_includes` is consulted *before* `system_header_dirs`, so
anything the compiler ships under a given name shadows the embedder's copy.

Calling an undeclared function no longer passes silently: c2mir warns that
it is synthesising an `int f()` prototype. That is worth promoting to an
error, because the synthesised prototype takes no arguments and the call is
generated against it, so the arguments are never passed.

## Type model

ILP32: `int`, `long` and pointers are 4 bytes, `long long` is 8. Note this
is the only target where `long` is not 64-bit, so `size_t` and `ptrdiff_t`
are 32-bit too. `long double` is mapped to `double`.

## Testing

```sh
./wasm-tests/run.sh
```

Needs `emcc`, `em-config` and `node` on `PATH`; CI's `wasm` job runs it on the
SDK version pinned in `.github/workflows/test.yml`. Covers pure
interpretation, variadic and non-variadic libc calls, calls in a loop,
native code calling interpreted functions (through an argument, through
memory, as a `long long`, with `double` and `long long` signatures), 250
contexts in one module (past both the old signature pool and the thunk
pool), and the type model against Emscripten's own musl headers: the test serves c2mir
the SDK's `sysroot/include` (read off the host disk with `-sNODERAWFS`, the
headers a consumer would put in MEMFS) and checks that `size_t`, `ptrdiff_t`,
`intptr_t`, `wchar_t`, `wint_t`, the fast/least types and `va_list` come out
as the types clang gives them; `setjmp` and `longjmp` across interpreted
activations; and calls through registered exact-signature stubs. It is built with `-sSAFE_HEAP`, which traps a
read through address 0 that a plain module would silently see as zeroes.

## Known limitations

- **`%lld` works** for calls generated by c2mir, which builds the vararg
  buffer itself (see above). MIR calls made by any other producer still lose
  the width and truncate.
- **A variadic function *defined* in interpreted code cannot be called.** The
  call side is lowered to a buffer plus a non-variadic call; the definition
  side is not, so the callee has no parameter to receive the buffer and its
  `va_start` still expects the interpreter's own channel. Raises an explicit
  error rather than returning garbage. Passing a struct by value in a variadic
  tail is diagnosed for the same reason.
- **Native code cannot call an interpreted function that is variadic,
  returns more than one value, or takes more than 32 arguments.** Its table
  entry stays null (a native call traps), though interpreted code calls it
  normally.

  If you touch the trampoline: `sizeof (MIR_val_t)` is **16** here, not 8 —
  the union carries a host `long double` and Emscripten's is 128-bit — so the
  marshalling buffer's stride has to come from `sizeof` rather than being
  assumed.
- `long double` precision is that of `double`, so `%Lf` is not usable.
- Multiple return values are rejected.
