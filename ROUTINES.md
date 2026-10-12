# Routine prompts (last version set on 2026-10-12)

Both routines: create_new_session_on_fire=true, no connectors, environment = the AnyPS5 cloud environment. Recreate with create_trigger if they are missing.

## AnyPS5 nightly big fix: `CRON_TZ=America/Denver 46 3 * * *` (was trig_01WqU6ZD8CgaHdDuBDXQ4TVv)

````text
Nightly AnyPS5 BIG-FIX run. You work on the user's fork `iannitram/anyps5` (GitHub: https://github.com/iannitram/AnyPS5), a fork of upstream `boykopovar/AnyPS5` (https://github.com/boykopovar/AnyPS5): a C++20 tool that relinks PS5 executables to native Linux/Windows with reimplemented system prx libraries and a shader recompiler. A separate small-fixes routine runs later the same night; your job is one substantial fix, and you keep searching until you find one.

## Budget (hard limits, the user pays for every token)
- Note the time at the start (`date`). Hard stop at 120 minutes from the start: wherever you are, push what is complete and tested, then write the summary.
- One substantial item. A second only if the first is pushed within 60 minutes.
- If the same build or test failure survives 2 fix attempts, stop working on that approach; land the tested subset if there is one (step 3), otherwise mark the item skipped and go back to searching.
- Don't run the full `ctest` suite: run the tests for the area you touched (`ctest -R <pattern>`). Build only the targets you need where possible.
- Don't spawn subagents. Don't re-read large files you already read; use grep and line ranges.

## 1. Set up
- If iannitram/AnyPS5 isn't already checked out with push access, call `mcp__claude-code-remote__add_repo` (owner `iannitram`, repo `AnyPS5`, access `push`) and follow its instructions. GitHub MCP tools are `mcp__github__*` (load with ToolSearch). If you can't get push access, stop and report the exact error.
- `git remote add upstream https://github.com/boykopovar/AnyPS5 && git fetch upstream main`.
- The full build needs `sudo apt-get install -y libasound2-dev libx11-dev libxext-dev` and `git submodule update --init --recursive --depth 1` (only if you will do a full build).
- Read CONTRIBUTING.md, docs/dev/CONVENTIONS.md, docs/dev/BUILD.md and .github/pull_request_template.md before writing code. These rules are binding: no code comments except technical-debt entries, unimplemented paths throw via `NotImplemented_nid_no_patch(__func__)`, general behaviour (not one title's needs), Conventional Commits, one topic per PR, shader semantics changes cite their exact source (ISA section, LLVM, ACO or Mesa file).

## Quality rules (lessons from reviewing earlier runs; binding)
- Test expectations come from the reference, never from your implementation. Every expected value in a test must be what the cited source (FreeBSD stable/11 code, the C/POSIX/Itanium spec) produces. Where practical, compile the FreeBSD reference function standalone in a scratch dir and compare against it for edge cases (NaN/inf/out-of-range, empty and null inputs, maximum sizes). Two earlier PRs shipped tests that asserted the implementation's own wrong output.
- Null pointers and invalid inputs: match the reference's return/errno where it defines one; throw only where the reference has undefined behaviour. Check size arithmetic for overflow and zero sizes.
- Thread safety: hold the object's lock across the whole read-and-copy, as FreeBSD does.
- Windows hosts: guest varargs and va_list follow the existing SysV patterns (`__builtin_sysv_va_list`/`__builtin_sysv_va_start`, see core/libs/tests/GuestStandardStreams.cpp); guest wchar_t is 16-bit; never call host wide-char or POSIX-only functions directly.
- Don't report fake state: a function that can't honestly answer (e.g. a pending-signal set that is never tracked) throws or gets a TechnicalDebt entry; it doesn't return a plausible empty value.
- Known gap: blocked guest signals are currently dropped rather than left pending (core/libs/prx/libkernel Signals.cpp Dispatch). Don't build new signal features on top of that assumption; fixing it is itself a valid big-fix item.
- When you change behaviour another function relies on (e.g. a new return value), check every caller and related export (`git grep`) still handles it.
- No code comments at all, not even a one-line `// FreeBSD 11 struct ...` header; explanations belong in the TechnicalDebt entry or the PR body. `tools/check_conventions.py` drops its comment findings whenever docs/dev/TechnicalDebt.md is in the diff, so check yourself: `git diff upstream/main -U0 -- '*.cpp' '*.hpp' '*.h' | grep -E '^\+\s*(//|/\*)'` must print nothing.
- Tests must be deterministic and clean up after themselves: never assert on the characters of random output (mkstemps names can contain `X`), use unique temporary names, and release every file, segment, queue or thread a test creates. A leaked object made an earlier test fail once a limit was enforced.
- Linux and FreeBSD errno numbers differ above 34: translate any host errno that can reach the guest.
- Before pushing, re-read your full diff adversarially for these issues.

## 2. Search for a substantial item until you find one
Keep searching, source by source, until you have a candidate that meets the bar below; don't give up after a first pass. Only the 120-minute hard stop ends the search. Search in this order:
1. Open issues labeled `wishlist-big` on iannitram/anyps5 (oldest first).
2. `docs/dev/TechnicalDebt.md` on `upstream/main`: entries describing throwing, partial or missing functionality whose semantics a reference settles.
3. The code: `git grep -n NotImplemented_nid_no_patch upstream/main -- core/libs/prx` for clusters of throwing exports in one library, and check whether shadPS4 (https://github.com/shadps4-emu/shadPS4), KytyPS5, SharpProspero or FreeBSD source implements them.
4. Incomplete subsystems in `core/libs/prx` or gaps in `core/relinker` that docs/dev/ARCHITECTURE.md or the code shows clearly.
The bar: one coherent topic with real scope (e.g. a group of related exports in one library, completing a partial subsystem, a relinker gap with a clear spec); single topic per CONTRIBUTING.md; resolvable without PS5 hardware or a GPU; testable here; semantics settled by a citable source. Avoid silent stubs (they exist on purpose), anything whose only open question is unverified console behaviour, and new third-party dependencies.
Skip a candidate if: a PR on iannitram/anyps5 (open, closed or merged; branches `techdebt/...`, `bigfix/...`, `bugfix/...` or `fix/...`) already addresses it; an open issue labeled `techdebt-skip` names it; `git log upstream/main` shows it already done; or an open upstream PR implements the same functions (fetch https://github.com/boykopovar/AnyPS5/pulls?q=is%3Apr+is%3Aopen+<function-name>; if you can't check, say so in the PR body).

## 3. Implement
- Branch from `upstream/main`, named `bigfix/<short-slug>`. If it needs code that exists only on the fork's main, branch from `origin/main` instead and say `Depends on:` in the PR.
- Implement the topic fully where you can and update `docs/dev/TechnicalDebt.md` in the same PR: remove the entry if fully resolved, or narrow it to what remains.
- If the item is bigger than the budget, land a coherent, fully tested subset that stands alone, narrow the entry to what remains, and list what's left in the PR body. Don't push half-done code.
- Validate: relinker-only changes use the relinker-only build (docs/dev/BUILD.md#relinker-only). Library/shader changes need the full configure (`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON`), then build the affected targets and run the relevant tests. Add tests for the new behaviour, and show they fail before the change and pass after. Always run `python3 tools/check_conventions.py --base upstream/main`.
- Push only when the build, relevant tests and the conventions check pass. If the item can't be resolved at all, open an issue on iannitram/anyps5 labeled `techdebt-skip` that quotes it and says why, then return to step 2.

## 4. Push and open the PR
- Push the branch to origin and open a ready-for-review PR against iannitram/anyps5 `main`. Fill in the upstream template's sections (What / Tested / Checklist). What quotes the TechnicalDebt entry or describes the gap, cites the sources that settle the behaviour, and if relevant suggests how to split it for upstream review. Tested names the OS (Linux container), exactly what was built and run, and where each test's expected values came from. Checklist line `AI-assisted: yes`; `Depends on:` as appropriate. Write `Closes #N` if it came from an issue.
- Never push to, comment on or open PRs or issues in boykopovar/AnyPS5; the user sends work upstream themselves.

## 5. Summary
End with a short summary: the item taken (quoted briefly), where you found it, the PR link, what's left, the test results, and the elapsed time. Don't subscribe to PR activity, watch PRs, or schedule follow-ups: once the summary is written, the run is over. 
````

## AnyPS5 nightly small fixes: `CRON_TZ=America/Denver 16 6 * * *` (was trig_01SubU7QA9f6PkNqfVah3jvs)

````text
Nightly AnyPS5 SMALL-FIXES run. You work on the user's fork `iannitram/anyps5` (GitHub: https://github.com/iannitram/AnyPS5), a fork of upstream `boykopovar/AnyPS5` (https://github.com/boykopovar/AnyPS5): a C++20 tool that relinks PS5 executables to native Linux/Windows with reimplemented system prx libraries and a shader recompiler. A big-fix routine ran earlier tonight (branches `bigfix/...`); don't touch what it worked on. Every run ends with at least one submitted PR unless the time limit hits.

## Budget (hard limits, the user pays for every token)
- Note the time at the start (`date`). Don't start a new item more than 60 minutes after the start; finish the one in progress. Hard stop at 90 minutes: push what is complete and tested, then write the summary.
- At most 3 items.
- Per item: if the same build or test failure survives 2 fix attempts, abandon the item (step 3's skip rule) and move on.
- Don't run the full `ctest` suite: run the tests for the area you touched (`ctest -R <pattern>`). Build only the targets you need where possible.
- Don't spawn subagents. Don't re-read large files you already read; use grep and line ranges.

## 1. Set up
- If iannitram/AnyPS5 isn't already checked out with push access, call `mcp__claude-code-remote__add_repo` (owner `iannitram`, repo `AnyPS5`, access `push`) and follow its instructions. GitHub MCP tools are `mcp__github__*` (load with ToolSearch). If you can't get push access, stop and report the exact error.
- `git remote add upstream https://github.com/boykopovar/AnyPS5 && git fetch upstream main`.
- The full build needs `sudo apt-get install -y libasound2-dev libx11-dev libxext-dev` (install up front only if you will do a full build) and `git submodule update --init --recursive --depth 1` (once per run, only if needed).
- Read CONTRIBUTING.md, docs/dev/CONVENTIONS.md, docs/dev/BUILD.md and .github/pull_request_template.md before writing code. These rules are binding: no code comments except technical-debt entries, unimplemented paths throw via `NotImplemented_nid_no_patch(__func__)`, general behaviour (not one title's needs), Conventional Commits, one topic per PR, shader semantics changes cite their exact source (ISA section, LLVM, ACO or Mesa file).

## Quality rules (lessons from reviewing earlier runs; binding)
- Test expectations come from the reference, never from your implementation. Every expected value in a test must be what the cited source (FreeBSD stable/11 code, the C/POSIX spec) produces. Where practical, compile the FreeBSD reference function standalone in a scratch dir and compare against it for edge cases (NaN/inf/out-of-range, empty and null inputs, maximum sizes). Two earlier PRs shipped tests that asserted the implementation's own wrong output.
- Null pointers and invalid inputs: match the reference's return/errno where it defines one; throw only where the reference has undefined behaviour. Check size arithmetic for overflow and zero sizes.
- Thread safety: hold the object's lock across the whole read-and-copy, as FreeBSD does.
- Windows hosts: guest varargs and va_list follow the existing SysV patterns (`__builtin_sysv_va_list`/`__builtin_sysv_va_start`, see core/libs/tests/GuestStandardStreams.cpp); guest wchar_t is 16-bit; never call host wide-char or POSIX-only functions directly.
- Don't report fake state: a function that can't honestly answer throws or gets a TechnicalDebt entry; it doesn't return a plausible empty value.
- Known gap: blocked guest signals are currently dropped rather than left pending (core/libs/prx/libkernel Signals.cpp Dispatch); don't build on that assumption.
- When you change behaviour another function relies on (e.g. a new return value), check every caller and related export (`git grep`) still handles it.
- No code comments at all, not even a one-line `// FreeBSD 11 struct ...` header; explanations belong in the TechnicalDebt entry or the PR body. `tools/check_conventions.py` drops its comment findings whenever docs/dev/TechnicalDebt.md is in the diff, so check yourself: `git diff upstream/main -U0 -- '*.cpp' '*.hpp' '*.h' | grep -E '^\+\s*(//|/\*)'` must print nothing.
- Tests must be deterministic and clean up after themselves: never assert on the characters of random output (mkstemps names can contain `X`), use unique temporary names, and release every file, segment, queue or thread a test creates. A leaked object made an earlier test fail once a limit was enforced.
- Linux and FreeBSD errno numbers differ above 34: translate any host errno that can reach the guest.
- Before pushing, re-read your full diff adversarially for these issues.

## 2. Pick work from docs/dev/TechnicalDebt.md (spend at most ~15 minutes here)
- Read `docs/dev/TechnicalDebt.md` on `upstream/main`. If there are open issues labeled `wishlist` on iannitram/anyps5, take those first; otherwise pick from this file.
- Most entries say console behaviour is "unverified": you cannot settle those without PS5 hardware, so skip them. Pick entries you can resolve completely in this container, with an exact, citable source, for example:
  - a throwing or partial path whose correct behaviour is fixed by a cited reference (FreeBSD libc/libthr/msun, glibc, POSIX, shadPS4, KytyPS5, SharpProspero, VitaSDK) where the entry or code already points at it;
  - a `Build` section item (toolchain or CMake problem) you can fix and verify here;
  - a missing case in an otherwise-implemented function whose semantics the referenced source settles.
  Prefer small, self-contained items in `core/libs/prx` or `core/relinker`. Avoid silent stubs (they exist on purpose), anything needing a GPU or hardware measurement, and anything needing a new third-party dependency.
- Skip an entry if: a PR on iannitram/anyps5 (open, closed or merged; branches `techdebt/...`, `bigfix/...`, `bugfix/...` or `fix/...`) already addresses it or touches the same functions; an open issue on iannitram/anyps5 labeled `techdebt-skip` names it; `git log upstream/main` shows it already changed; or an open upstream PR already implements the same functions (check by fetching https://github.com/boykopovar/AnyPS5/pulls?q=is%3Apr+is%3Aopen+<function-name>; if you can't check, say so in the PR body).

## 2b. If no TechnicalDebt item qualifies: bug review until you find one
- Review the code for real correctness bugs until you find at least one you can prove. Start with recently changed code (`git log upstream/main --since="14 days ago" --stat -- core/`) and code the fork added (`git diff --stat upstream/main origin/main -- core/`), then widen to `core/libs/prx` and `core/relinker`. Look for the issues in the quality rules above: wrong errno/return codes against the cited reference, missing null or bounds checks, overflow and off-by-one, races, Windows ABI mistakes, and behaviour that contradicts the TechnicalDebt entry or the reference the code cites.
- A finding counts only when you can prove it: a test or reproduction that fails. Discard hunches and style issues. Check that no open upstream PR already fixes it.
- Fix it on a branch named `bugfix/<short-slug>` (from `upstream/main`, or from `origin/main` if the bug is in fork-only code) and submit it like any other item (steps 3–4), with the failing-then-passing test as evidence. Keep reviewing until you have one, or the 90-minute hard stop hits.

## 3. Implement each item (separately)
- Branch from `upstream/main`, named `techdebt/<short-slug>` (or `bugfix/<short-slug>` from step 2b).
- Make the minimal change that resolves the item, and update `docs/dev/TechnicalDebt.md` in the same PR when the item came from it: remove the entry if fully resolved, or narrow it to what is still unverified.
- Validate: relinker-only changes use the relinker-only build (docs/dev/BUILD.md#relinker-only). Library/shader changes need the full configure (`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON`), then build the affected targets and run the relevant tests. Add or extend a test, and show it fails before the fix and passes after. Always run `python3 tools/check_conventions.py --base upstream/main`.
- Push only when the build, relevant tests and the conventions check pass. If an item turns out not to be resolvable (needs hardware, the source is ambiguous, too large, or it hit the 2-attempt limit), don't push half-done code: open an issue on iannitram/anyps5 labeled `techdebt-skip` that quotes it and says why, so future runs skip it, and move on.

## 4. Push and open the PR
- Push the branch to origin and open a ready-for-review PR against iannitram/anyps5 `main`. Fill in the upstream template's sections (What / Tested / Checklist). What quotes the TechnicalDebt entry (or describes the bug and how it was found) and cites the source that settles the behaviour. Tested names the OS (Linux container), exactly what was built and run, and where each test's expected values came from. Checklist line `AI-assisted: yes`; `Depends on:` as appropriate. Write `Closes #N` if it came from a wishlist issue.
- Never push to, comment on or open PRs or issues in boykopovar/AnyPS5; the user sends work upstream themselves.

## 5. Summary
End with a short summary: each item taken (quoted briefly, and whether it came from TechnicalDebt or the bug review), the PR link or why it was skipped, the test results, and the elapsed time. Don't subscribe to PR activity, watch PRs, or schedule follow-ups: once the summary is written, the run is over. 

````
