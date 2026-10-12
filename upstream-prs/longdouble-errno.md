**Title:** fix(libc): keep errno in long double math, logb and remainder

### What
FreeBSD's msun reports math errors only through floating-point exceptions (`math_errhandling` is `MATH_ERREXCEPT`) and never writes `errno`. These exports call the host's libm, and on Linux glibc sets `errno` to `EDOM` or `ERANGE`. A guest that checks `errno` after an unrelated call then sees a stale math error.

The following now leave `errno` unchanged:
- `acosl`, `atanl`, `sinl`, `cosl`, `tanl`, `sinhl`, `coshl`, `tanhl`, `expl`, `exp2l`, `logl`, `log10l`, `powl` and `ldexpl`;
- `logb`, `logbf`, `remainder` and `remainderf`.

They do so through a small `KeepErrno` guard. It reads and writes `errno` through a `volatile int&`, because GCC treats `errno` as unchanged across math builtins and drops a plain save and restore. Results are unchanged.

The TechnicalDebt entries for `acosl` and for the other `long double` functions no longer say these functions set `errno`. They now say the other double and float functions in `Math.cpp` still do.

### Tested
Linux (Ubuntu, GCC), full Release build on current `main` with `BUILD_TESTING=ON`:
- `guest_math` passes. The new `CheckLongDoubleErrno` covers every wrapped function with an argument that makes glibc set `errno`: `acosl(2)`, `logl(-1)`, `logl(0)`, `log10l(0)`, `expl`/`exp2l`/`coshl`/`sinhl` overflow, `powl(0, -1)`, `powl(-2, 0.5)`, `sinl`/`cosl`/`tanl(inf)`, `remainder(1, 0)`, `remainder(inf, 1)`, `ldexpl(1, 100000)`. It fails on `main`.
- Full `ctest`: 954/959 pass. The 5 failures are environmental: the 4 `guest_datagram_*` tests (UDP is blocked in the container) and `guest_named_shm_concurrency` (it runs as root).
- Not tested on Windows; the change uses only standard C++.

### Checklist
- [x] Based on current `main`; no other open PR implements the same functions
- [x] One topic per PR; follow-ups go in a new PR
- [x] No comments in code except [technical debt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/CONVENTIONS.md)
- [x] Unimplemented paths throw (`NotImplemented_nid_no_patch`); silent stubs are listed in [TechnicalDebt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/TechnicalDebt.md#silent-stubs)
- [x] No notes, investigation `.md` files or images added to the repository (attach them to this PR)
- [x] New third-party code is a submodule built from source
- [x] Generic behaviour, not specific to one title
- [x] Depends on: none
- [x] Unblocks: none
- [x] AI-assisted: yes
