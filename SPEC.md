# AnyPS5 (fork): spec for 1.0

**Approval:** 2026-10-26, the owner, in the Project Manager session's question tool: "Both (Recommended)" (AnyPS5 normalized like the other forks: spec = upstream plan + our delta, R18). A changed Must or Won't sets this back to "not yet".

## 1.0 in one sentence

One game the owner owns relinks with our build of AnyPS5 and runs natively on Linux (SteamOS on the handheld), the result is recorded in our compatibility table, and our build and tests are sound after every upstream sync.

## The spec is upstream's plan plus our delta

- **Upstream plan:** boykopovar/AnyPS5's README (status and compatibility), [docs/dev/ARCHITECTURE.md](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/ARCHITECTURE.md), [docs/dev/PROGRESS.md](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/PROGRESS.md) (library counts) and [docs/dev/TechnicalDebt.md](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/TechnicalDebt.md). Upstream's own compatibility list is one game, on Windows.
- **Our delta:** the goals and standing rules in the pinned issue #63: try games on Linux. Our 100 commits on `main` that upstream lacks are system-library fixes and additions under `core/libs/prx/` (libc regex, signals, threads and conditions, stdio and wide-character edge cases, libSceLibcInternal) with their tests, and the sync follow-ups.

| Delta | Upstream? |
|---|---|
| Running and recording games on Linux | keep on our fork |
| System-library fixes under core/libs/prx/ | keep on our fork: upstream's policy forbids AI-written contributions, so nothing is pushed, commented or opened there |
| Filling more of the system-library gap beyond what a tested game needs | keep on our fork, and held: the owner decides before it starts (#63) |

## Musts

| # | Must | Done looks like | Check | Issues |
|---|---|---|---|---|
| M1 | Our build works on the handheld | The release.yml recipe builds the relinker and libraries on SteamOS (in a container), and ctest passes | `cmake --build build`, `ctest --test-dir build --output-on-failure` | #61 |
| M2 | One game runs on Linux | A game relinked from the owner's own decrypted dump runs in SteamOS Desktop Mode, with the output logged | The run log in the issue (error lines only, no game data) | #62 |
| M3 | It is recorded | The compatibility table has a row: game, ID, release, machine class, result | README "Compatibility" section | #62 |
| M4 | Upstream syncs keep our tests | Each sync merges upstream with conflicts resolved, every dropped fork check logged and approved, and the build and tests pass on Linux | The sync PR's notes and CI | #65, #66 |
| M5 | No game files, ever | Nothing from a dump, firmware, system libraries or keys is in the repo, a build or an issue | Review of every PR | #63 rules |

## Won'ts (after 1.0)

Windows runs (upstream covers them), more games beyond the first, and the wider system-library work held for the owner's decision. Nothing is ever sent to the upstream repo.

## Platforms

Linux is the 1.0 target (SteamOS handheld, AMD). Windows builds need MinGW-w64 on a Windows machine; the relinker-only build runs on macOS, including Apple silicon (upstream README). R04: see `docs/code-map.md`.
