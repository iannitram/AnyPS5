Stage: Dev Loop
Since: 2026-10-26
Kind: fork (upstream: boykopovar/AnyPS5)
Spec: SPEC.md   Roadmap: PIPELINE.md#roadmap (fork delta), upstream README and docs/dev/PROGRESS.md / TechnicalDebt.md   Code map: docs/code-map.md
Platforms: linux (baseline and Product beta target), windows (existing runtime; companion parity deferred), mac (relinker and portable tooling only)
Product decision: Approved 2026-11-01 (#87): native game library, compatibility passport/rollback, bounded session diagnosis, repeatable test lab and input/accessibility studio. Implementation #88-#104, all planned; #66 remains first, then contracts, diagnostics and one test route. Final stage #104 is the running-build design pass. The original #61/#62/#65/#66 baseline remains required. No new module or skeleton is built by the planning update.
Notes: Integration branch `main`. The 2026-10-26 sync branches are merged (e7c0343f5); the last recorded upstream gap was about 2,060 commits on 2026-10-28. #66 (upstream-sync, Hard) stays first for implementation and requires Linux build/test evidence before merge. Nothing is pushed, commented or opened upstream. The closed unmerged tech-debt PRs #56-#59 are rebuilt as #67-#70 (Medium, blocked by #66); their old commits remain under archive tags. Agent rules: AGENTS.fork.md. Game runs need my own dump outside every repo.

Stages: Proposal -> Spec -> Roadmap -> Dev Loop -> Prototype / Beta.
What each stage means and who moves a project: the pipeline repo (private), PIPELINE.md.
The Project Manager updates this file when the stage changes and adds a CHANGELOG line.

# Roadmap

I approved this product direction on 2026-11-01 (#87). The existing Linux 1.0 baseline remains intact; the expanded Product beta joins five workflows into one companion. All new modules below are planned, not implemented.

## Delivery order

An issue is ready only when every "Blocked by" dependency is resolved. #66 remains first for implementation. Planning documents may proceed while it is open; the existing fork stays at Dev Loop. No feature implementation or skeleton is claimed by this roadmap update.

| Stage | Slice | Issue | Depends on |
|---|---|---|---|
| 0. Linux baseline | Build environment; first owned-title run and recorded result; sync/test preservation | #61, #62, #65, #66 | Existing issue dependencies |
| 1. Shared contracts | G0: companion skeleton and versioned installation/session/evidence records | #88 | #66 |
| 2. Session diagnosis | D1: bounded timeline and activity signals | #89 | #88 |
| 2. Session diagnosis | D2: evidence-backed failure classification | #90 | #89 |
| 2. Session diagnosis | D3: report preview, redaction and local export | #91 | #90 |
| 3. Repeatable testing | T1: route schema and checkpoint/input validation | #92 | #88 |
| 3. Repeatable testing | T2: runner with checkpoint waits, timeout and descendant cleanup | #93 | #92, #89 |
| 3. Repeatable testing | T3: comparable A/B route results and frame-pacing report | #94 | #93 |
| 4. Game library | L1: audited, atomic installation import | #95 | #88, #90 |
| 4. Game library | L2: persistent library and local desktop/fullscreen shell | #96 | #95, #91 |
| 4. Game library | L3: Play/Stop/Remove and session integration | #97 | #96, #93 |
| 5. Compatibility passport | C1: provenance-bearing milestone evidence | #98 | #88, #94 |
| 5. Compatibility passport | C2: title passport and qualified update guidance | #99 | #98, #96 |
| 5. Compatibility passport | C3: side-by-side update trial, save backup and rollback | #100 | #97, #98 |
| 6. Input/accessibility studio | I1: persistent per-game profiles and safe sharing | #101 | #88 |
| 6. Input/accessibility studio | I2: live input diagram and calibration | #102 | #101, #96 |
| 6. Input/accessibility studio | I3: hold/toggle alternatives and required-action check | #103 | #102 |
| 7. Final design pass | UX1: running-build audit, separate finding issues and final acceptance | #104 | #91, #94, #97, #99, #100, #103, #61, #62, #65 |

Diagnostics and one useful repeatable test route are the first product increments. Independent profile/contracts work can follow its dependencies without waiting for later stages. A first drawn frame, static import audit or library-function count never marks gameplay verified.

## Scope and reuse

| Approved change | Existing foundation | New product behavior |
|---|---|---|
| Game library | audit_dump, unwrap_self, import_audit, relinker CLI and browser converter | Persistent local installations, lifecycle control and controller-friendly shell |
| Compatibility passport | Session provenance, test observations and version/hash-checked patch tooling | Honest milestone evidence, setup matching, isolated updates and protected rollback |
| Session diagnosis | FailureReport, call tracing, VideoOut/pad telemetry and performance traces | Bounded event timeline, supported explanations and previewed sanitized reports |
| Repeatable test lab | pad_input, pad_timeline, trace_digest and performance_compare | Checkpoint-aware routes, lifecycle control and A/B comparisons with uncertainty |
| Input/accessibility studio | Native libScePad mappings and browser input mapper | Persistent profiles, faithful live preview/calibration and safe alternative actions |

The [code map](docs/code-map.md#product-module-plan) records exact seams and ownership. Implementation issues update their own status there in the same PR. Existing foundation code is present; its integration and the new product views are not.

## Platform gates

Linux is the first installed product acceptance target. Windows keeps its existing runtime/relinker tests and receives portable contract coverage; dedicated companion launch/packaging qualification is deferred until the Linux journey works. macOS keeps the relinker-only build and portable tooling tests; runtime launch controls must explain that limitation.

Each slice declares its tested platform. CI for public changes uses GitHub-hosted runners; GPU claims require the actual device evidence required by AGENTS.md. No new runtime dependency, remote service or shared system installation is assumed here.

## Design and release gates

The last stage is the pipeline design pass on the running companion using the shared approved system. The pass covers library/import, title passport/update, diagnosis/report preview, route comparison and input calibration, including their empty/loading/error/edge states and keyboard/controller paths. Findings become sized design-pass issues; Product beta waits for required fixes and feature PRs to merge.

The original Linux milestone must also be demonstrated with an owned title, including a documented save/load observation for any passport claiming those milestones. A partial or unknown result is useful evidence but does not satisfy a claimed pass.

## Deferred

Windows companion parity, macOS runtime, remote aggregation/blocker prioritization, additional title qualification and deterministic replay are outside this milestone. Wider system-library work stays held under #63. No new peer code or third-party package is adopted by this planning change.
