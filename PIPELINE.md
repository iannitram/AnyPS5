Stage: Dev Loop
Since: 2026-10-26
Kind: fork (upstream: boykopovar/AnyPS5)
Spec: SPEC.md   Roadmap: upstream README, docs/dev/PROGRESS.md and docs/dev/TechnicalDebt.md, plus our issues #61, #62, #65, #66   Code map: docs/code-map.md
Platforms: linux (1.0 target), windows, mac (relinker only)
Notes: Integration branch `main`. The 2026-10-26 sync branches are already merged into main (e7c0343f5); upstream has moved 545 commits since: #66 (upstream-sync, Hard) is taken first, prepared on a branch and built on Linux before it is merged. Upstream is AI-hostile: never push, comment or open anything there. Open PRs #56-#59 (tech-debt fixes from 2026-10-18) predate the sync and need a rebase before review. Agent rules for our pipeline: AGENTS.fork.md. Game runs need the owner's own dump (needs-owner-device).

Stages: Proposal -> Spec -> Roadmap -> Dev Loop -> Prototype / Beta.
What each stage means and who moves a project: the pipeline repo (private), PIPELINE.md.
The Project Manager updates this file when the stage changes and adds a CHANGELOG line.
