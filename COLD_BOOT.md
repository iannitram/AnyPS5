You're picking up my AnyPS5 project. My fork is iannitram/AnyPS5 and upstream is boykopovar/AnyPS5, a C++20 tool that relinks PS5 executables to native Linux/Windows.

1. Set up:
   - Add the fork with push access (add_repo iannitram/AnyPS5).
   - `git remote add upstream https://github.com/boykopovar/AnyPS5` and fetch upstream main.
   - `git fetch origin project-notes && git worktree add <scratchpad>/notes project-notes`.
2. Read `STATE.md` on that branch first, then `ROUTINES.md`. They have:
   - the current state;
   - the hard rules (never push, comment or open anything on boykopovar/AnyPS5; merge commits only on the fork; no code comments);
   - the build environment;
   - the decisions I've already made.
3. Then report back briefly:
   - Check that fork main is still at 7c8936d7 and has no open PRs or issues. If anything changed, say what.
   - Check whether the two nightly routines exist (list_triggers). The notes say they disappeared on 2026-10-12. Ask me before recreating them from ROUTINES.md.
   - Check whether I've opened upstream PRs for `upstream/getrusage-errors` or `upstream/libc-longdouble-errno` yet, and their status (read-only).
   - Count how far upstream/main is ahead of fork main.
4. Wait for my go-ahead before doing more. Standing permissions:
   - Review, fix and merge routine PRs on the fork after an adversarial review.
   - Sync upstream into the fork with a merge PR (watch the sinhf/asinl duplicate).
   - Keep `project-notes` updated as you work (commit straight to that branch). Notes never go on main.
