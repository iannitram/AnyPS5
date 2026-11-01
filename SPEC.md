# AnyPS5 (fork): spec

**Approval:** I approved the five product proposals on 2026-11-01: "I like it. Update the spec, road and code maps, then build the issues according to our template". The decision is recorded in [#87](https://github.com/iannitram/AnyPS5/issues/87). This extends the 2026-10-26 Linux milestone; it does not claim that any new feature is built.

## Milestones

**Linux 1.0 baseline:** One game I own relinks with my build and runs natively on Linux (SteamOS), its result is recorded, and upstream syncs preserve the fork's tests.

**Product beta:** On Linux I can import that game into a persistent library, launch it, understand a failed session, compare a repeatable test route between builds, inspect its compatibility evidence, trial and roll back an update with protected saves, and configure accessible controls without editing an INI.

The baseline remains the prerequisite for calling the expanded product ready. The five approved changes are the native game library, compatibility passport, session diagnosis, repeatable test lab and input/accessibility studio. Delivery order is the [roadmap](PIPELINE.md#roadmap); module ownership is [docs/code-map.md](docs/code-map.md).

## Upstream plan and my delta

I build on the upstream relinker, native system-library implementations, browser converter, input mapper, shader caches, overlay and tracing. Their current behavior is described by [README](README.md), [architecture](docs/dev/ARCHITECTURE.md), [usage](docs/user/USAGE.md), [progress](docs/dev/PROGRESS.md) and [technical debt](docs/dev/TechnicalDebt.md). Function counts and successful conversion do not establish game compatibility.

| Delta | Disposition |
|---|---|
| Run and record games on Linux; preserve fork fixes and tests across syncs | Keep on this fork (#61, #62, #65, #66) |
| Connect existing tools into the five approved product workflows | Keep on this fork (#88-#104) |
| General system-library work beyond a tested game's needs | Held for a separate decision (#63) |

Nothing is pushed, commented or opened upstream. The approved product work does not authorize bypassing unsupported behavior or returning invented success from a system function.

## Musts

| ID | What I can do and see | Check | Issues |
|---|---|---|---|
| M1 | Build the relinker and libraries on SteamOS without replacing the release install | Build and ctest output from the Linux machine class | #61 |
| M2 | Run one owned title natively and record its actual outcome | Local run; compatibility row includes title/version, build, machine class and result | #62 |
| M3 | Keep fork test coverage through an upstream sync | Sync PR records every removed check and Linux validation | #65, #66 |
| M4 | Import into a persistent library, see audit/readiness findings and use Play/Stop/Remove | Real Linux import-to-Play journey; failed/canceled import is never Ready; source and user data survive removal | #95-#97 |
| M5 | Read a bounded session timeline and an evidence-backed diagnosis; preview/export a support report | Normal exit, startup crash, inactivity and overflow fixtures; sanitized report inspection | #88-#91 |
| M6 | Run a checkpoint-aware route against two builds and see progress/failure/frame-pacing differences | One local owned-title route plus synthetic lifecycle tests; incompatible captures cannot claim a speed verdict | #92-#94 |
| M7 | Inspect a title passport showing observed milestones, unknowns, provenance and environment differences | Unknown/conflicting/stale reports remain visible; static audit never implies gameplay or saving | #98, #99 |
| M8 | Trial an isolated build and return to the previous installation with protected save/config data | Verified local backup and rollback tests, including interrupted update and corrupt backup | #100 |
| M9 | Save per-game input profiles, see live mapped input and configure supported accessible alternatives | Profile round-trip, physical-controller calibration and keyboard/controller walkthrough | #101-#103 |
| M10 | Use a coherent, accessible desktop/fullscreen interface | Final design pass on the running build; required findings resolved before Product beta | #104 |
| M11 | Keep game data and secrets out of repositories, issues and support exports | Allowlist/redaction tests and review; all dumps, saves, artwork and test captures remain local | #63, #91 |

## Experience

The library emphasizes the selected game, Play, readiness and last-session result. Artwork is optional and local. The title page links the passport, settings and installed versions; diagnostics and the test lab share the same session evidence.

Every view covers loading, empty, error and edge states. Keyboard and controller navigation, visible focus, readable type, reduced motion and non-color status cues are required. The final design pass reuses the pipeline's shared approved design system; this spec does not invent a replacement palette or toolkit.

## Implementation decisions

- A companion manages local installations and launches the converted native process; it is not an emulator and does not become a required execution layer.
- Reuse the audit, relinker, trace, controller and comparison tools behind explicit versioned contracts. Planned module paths and ownership are in the code map.
- Store source dumps, generated installations, sessions, saves, settings and backups separately. Imports and version switches commit atomically; installations identify exact executable/library/patch/config provenance.
- Keep evidence-backed milestones distinct from manual assertions and predictions. Unknown, missing, conflicting and dropped evidence remain explicit.
- Diagnostic collection is bounded and configurable. Export is an explicit local action with a field/file preview; no automatic remote publication. Reports exclude game files, assets, memory captures, credentials and private paths.
- Existing controller timelines use wall time. Checkpoints improve orchestration but do not create deterministic replay or arbitrary save-state support.
- A protected update trial requires a stopped title and verified backups of known writable roots. Save restoration is a separate previewed action; newer saves are not assumed backward compatible.
- UI/backend packaging is settled in #96 using GPL-compatible dependencies under the existing submodule rules. A web-backed shell must use authenticated local IPC or a loopback service with origin checks.
- The module skeleton and shared contracts are the first implementation slice (#88). All new modules remain planned until implemented and verified.

## Platforms

| Platform | Commitment |
|---|---|
| Linux x86-64 / SteamOS | Baseline and Product beta target; AMD first, isolated build/install, real device verification |
| Windows x86-64 | Preserve existing relinker/runtime behavior; run portable contract tests; companion packaging and full product qualification follow Linux |
| macOS | Preserve relinker-only support and portable data/tool tests; no native game-runtime or companion Play promise |

## Won'ts

For these milestones I do not promise every game runs, deterministic replay, emulator save states, arbitrary motion/touch emulation, an online compatibility service, automatic public reports, game-file distribution, a new macOS runtime, or Windows product parity. Wider system-library filling remains held. Existing patches and diagnostics cannot silently relax correctness checks.

## Testing decisions

Synthetic public fixtures cover contracts, failure classification, report leakage, atomic import, process descendants, route timeouts, evidence matching, rollback and input state transitions. Real-title acceptance uses my local owned dump outside every repository and publishes only permitted error lines and observations. Performance claims retain same-machine, same-scene trace digests and comparable instrumentation/cache conditions. Native GPU changes retain the focused baseline and validation-layer checks in AGENTS.md.
