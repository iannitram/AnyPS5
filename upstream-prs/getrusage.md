**Title:** fix(libkernel): fail getrusage with EINVAL and EFAULT instead of throwing

### What
`getrusage` threw `std::invalid_argument` for a `who` other than `RUSAGE_SELF` (0), `RUSAGE_CHILDREN` (-1) or `RUSAGE_THREAD` (1), and for a null `usage`. FreeBSD 11 returns an error for both cases ([kern_resource.c](https://github.com/freebsd/freebsd-src/blob/stable/11/sys/kern/kern_resource.c)):
- `kern_getrusage` returns `EINVAL` for any other `who`, before anything is copied.
- `sys_getrusage` then gets `EFAULT` from `copyout` for a null buffer.

The function now sets `errno` to `EINVAL` or `EFAULT` and returns -1, checking in that order, and leaves the buffer untouched on failure. The Windows and Linux paths after the checks are unchanged.

### Tested
Linux (Ubuntu, GCC), full Release build on current `main` with `BUILD_TESTING=ON`:
- `guest_getrusage` passes. Its new assertions cover `who` = 2 and -2 (22), a null buffer for 0, 1 and -1 (14), the EINVAL-before-EFAULT order, and an untouched buffer. They fail on `main` with "getrusage: unsupported who".
- Full `ctest`: 954/959 pass. The 5 failures are environmental: the 4 `guest_datagram_*` tests (UDP is blocked in the container) and `guest_named_shm_concurrency` (it runs as root).
- Not tested on Windows; the Windows path is unchanged apart from the earlier checks.

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
