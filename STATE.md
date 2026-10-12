# AnyPS5 fork: project state (updated 2026-10-12)

This orphan branch holds the project notes for the fork. It has no code and must never be merged into `main`. The repo's conventions check rejects notes files in the tree ("notes-file" rule).

## Repositories and hard rules

- Fork: `iannitram/AnyPS5`, the remote `origin`. Upstream: `boykopovar/AnyPS5`, the remote `upstream` (read-only for us).
- **Never push to, comment on, or open PRs or issues in boykopovar/AnyPS5.** The user sends work upstream themselves.
- Fork workflow:
  - Work happens on branches with PRs into fork `main`.
  - Merge with merge commits only. Never force-push and never rebase shared branches.
  - The user gave a standing green light for routine, bug, branch and PR fixes and for merging fork PRs after an adversarial review.
- Upstream PR rules for the user's own submissions:
  - A newcomer may have only 1 open PR until one merges; after that, 20.
  - The template has an "Unblocks:" checklist line, and reviewers prioritize PRs that unblock titles.
  - Fill in the What / Tested / Checklist sections and the `AI-assisted: yes` line.

## Current state

- Fork `main` is at `7c8936d7` (Merge #53 bigfix/regex).
  - PRs #1–#55 are all merged. There are no open PRs and no open issues.
- The last upstream sync into the fork was #55 (`sync/upstream-main`, merged at 15:14 UTC on 2026-10-12).
  - Since then upstream/main has moved a lot: 311 commits are on upstream and not on fork main, and 193 are on the fork and not upstream (2026-10-12 21:00 UTC). Most of the 193 are routine features that were never sent upstream.
  - **Known collision for the next sync:** upstream added its own `sinhf`/`asinl`, and the fork has them in `core/libs/prx/libc/src/MathSpecial.cpp`. Keep one copy (prefer upstream's unless it is wrong; a duplicate export fails the conventions check).
- Branches prepared for upstream (pushed to the fork, cut from upstream/main 2a5eca11; the user opens the PRs):
  1. `upstream/getrusage-errors` (b292f4f9). Open it **first**. `getrusage` returns EINVAL when `who` is not in {-1, 0, 1}, then EFAULT for a null pointer; adds the `guest_getrusage` test. Description: `upstream-prs/getrusage.md`.
  2. `upstream/libc-longdouble-errno` (4afba1f9 + 6d28ba12). **Squash before opening.** The 2nd commit restores a brace lost in a conflict.
     - `KeepErrno` keeps errno unchanged in acosl, atanl, sinl, cosl, tanl, sinhl, coshl, tanhl, expl, exp2l, logl, log10l, powl, ldexpl, logb, logbf, remainder and remainderf, as msun does.
     - Adds `CheckLongDoubleErrno` to the GuestMath test. Description: `upstream-prs/longdouble-errno.md`.
  - Both were built together on upstream main: 954/959 tests passed (only the 5 environmental failures below).
  - Before opening either, search upstream's open PRs for overlap.
- Later candidates for upstream (bigger, after the first ones merge):
  - The signal work: #2 per-thread sigmask and #30 pending signals, plus #19/#21/#42/#38.
  - Regex (#53).
  - SysV shm (#32) and mqueue (#31).
  - Each needs re-cutting onto upstream/main as one topic per PR.
- About 60 stale `techdebt/*`, `bigfix/*`, `bugfix/*` and `fix/*` branches remain on the fork; all are merged. Safe to delete if the user wants.

## Nightly routines

| Routine | Schedule | Last known id |
|---|---|---|
| AnyPS5 nightly big fix | `CRON_TZ=America/Denver 46 3 * * *` | trig_01WqU6ZD8CgaHdDuBDXQ4TVv |
| AnyPS5 nightly small fixes | `CRON_TZ=America/Denver 16 6 * * *` | trig_01SubU7QA9f6PkNqfVah3jvs |

- **Status at 21:04 UTC on 2026-10-12: neither id is found by `get_trigger`, and neither appears in `list_triggers` (even with include_completed).** They existed at 08:55 UTC. They were deleted or moved somewhere this account's API can't see. Confirm with the user before recreating them.
- The full prompts are in `ROUTINES.md`; recreate them with `create_trigger` (create_new_session_on_fire=true, no connectors).
- The fork's GitHub labels the routines use are `wishlist`, `wishlist-big` and `techdebt-skip`.
- Routine branches are named `bigfix/<slug>`, `techdebt/<slug>` and `bugfix/<slug>`. Review follow-ups go on `fix/<slug>`.

## Build and test environment (cloud container)

- `sudo apt-get install -y libasound2-dev libx11-dev libxext-dev`, then `git submodule update --init --recursive --depth 1`.
- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build build`
  - A full build takes a long time; build the affected targets and then run `ctest --test-dir build -R <pattern>`.
- A separate worktree of fork main makes a convenient build dir.
- Five tests always fail here and are environmental, not real:
  - `guest_datagram_recv`, `guest_datagram_recvfrom`, `guest_datagram_sceNetRecv` and `guest_datagram_sceNetRecvfrom`: UDP is blocked.
  - `guest_named_shm_concurrency`: runs as root.
- The full suite on the fork was 984/989 at the last integration run; on upstream main it is 954/959.
- Conventions: `python3 tools/check_conventions.py --base upstream/main`.
  - It skips comment findings when docs/dev/TechnicalDebt.md is in the diff, so check by hand that `git diff upstream/main -U0 -- '*.cpp' '*.hpp' '*.h' | grep -E '^\+\s*(//|/\*)'` prints nothing.
- CI conventions job: it reads the PR body's template checkboxes from the event payload.
  - A re-run reuses the old body; edit the PR body to trigger a fresh run.
- `remerge_debt.py` (on this branch) merges docs/dev/TechnicalDebt.md entry by entry when two branches both edited it.
  - Usage: `python3 remerge_debt.py <ours> <base> <theirs>`.
  - It stops when both sides changed the same entry; resolve those by hand.

## Code conventions and lessons (binding; also in the routine prompts)

- No code comments at all; explanations go in TechnicalDebt.md or the PR body.
- Unimplemented paths throw `NotImplemented_nid_no_patch(__func__)`. Silent stubs exist only where TechnicalDebt.md lists them.
- Implement the general behaviour, never one title's needs.
- Use Conventional Commits, one topic per PR.
- Reference semantics come from FreeBSD stable/11:
  - kern_sig.c, sysv_shm.c, uipc_mqueue.c;
  - libthr thr_attr.c/thr_join.c/thr_mutex.c;
  - tzcode asctime/strftime;
  - mktemp.c, ftok.c, nsdispatch, fmtmsg, regex;
  - msun for math.
- Test expectations come from the reference, never from our implementation. Compile the FreeBSD function standalone to compare edge cases.
- Guest errno uses FreeBSD numbers: translate host errno values above 34.
- To restore errno around a host libm call, use `volatile int& hostErrno = errno;`. GCC drops a plain save and restore.
- Windows hosts:
  - guest va_list uses SysV `__builtin_sysv_va_*`;
  - guest wchar_t is 16-bit;
  - never call host wide-char or POSIX-only functions directly.
- Hold the object's lock across the whole read-and-copy.
- Tests must be deterministic and must release every file, segment, queue and thread they create.

## Deliberate decisions (don't "fix" these)

- setitimer/ualarm/alarm throw NotImplemented when arming ITIMER_REAL with a guest SIGALRM handler installed. Delivery isn't possible yet; a review bot called this "back to stubs".
- pthread prioceiling is not range-checked to 0–31, because PS5 priorities are 256–767.
- Some libc paths write to host stderr (err/warn/perror family). This matches upstream's perror.
- The Getopt optind collision with glibc is solved by forward-declaring `struct FileStream;` instead of including FileStream.hpp.
