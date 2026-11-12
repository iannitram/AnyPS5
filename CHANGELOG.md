# Changelog (our fork)

Newest first. One line per change of stage, release or decision that the next session needs.

- 2026-11-12 Audit: #131 (remquo) fixed in PR #151; #144 filed for a new upstream sync (f57c971, 6,126 commits behind, trial merge aborted on 8 conflicts, waiting on my `std::unexpected` decision). #147 and #148 (from the title-runs peer) feed T3 (#94) and T2 (#93).
- 2026-11-11 Roadmap map: the roadmap now opens with an Updated date and a features block of eleven feature ids (the baseline, upstream sync, runtime fixes, CI and tests, and the seven product stages), so every issue can carry one feature label and a 1.0 or After 1.0 milestone.
- 2026-11-11 Cleared repo-lint (R01): added CLAUDE.md pointing at AGENTS.fork.md, and reshaped the PIPELINE.md Spec/Roadmap/Code map line so the code map entry is read. Remaining findings are absolute paths in upstream-owned docs/dev files, left alone.
- 2026-11-06 Upstream synced into main (4921718de2).
- 2026-11-02 #66 (upstream sync) closed.
- 2026-11-02 G0 (#88) done: tools/companion/contracts.py and tools/tests/test_companion_contracts.py (the CLI operations are still stubs).
- 2026-11-01 I approved the five product proposals (#87): native library, compatibility passport with protected rollback, session diagnosis, repeatable test lab and input/accessibility studio. SPEC.md preserves the Linux baseline and defines Product beta; PIPELINE.md contains the roadmap and the code map links implementation #88-#104, starting after #66 and ending with the design pass. Modules remain planned; stage stays Dev Loop. The roadmap stays in PIPELINE.md because the upstream conventions checker refuses a new ROADMAP.md notes file.
- 2026-10-26 Normalized to pipeline v2 at Dev Loop: SPEC.md (upstream plan + our delta, approved by the owner), PIPELINE.md, AGENTS.fork.md. Next upstream sync filed as #66 (545 commits).
- 2026-10-26 Upstream synced into main (sync/upstream-2026-10-26 and -26b, merged at e7c0343f5); review of the conflict resolution is #65. Code map added (docs/code-map.md).
- 2026-10-18 Tech-debt PRs #56-#59 opened (libSceZlib, umtx SEM2, mutexattr protocol, _Stoul base).
