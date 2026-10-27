# Rules for agents in this fork (our pipeline)

Upstream owns AGENTS.md, CONTRIBUTING.md and docs/dev/: read them (CONVENTIONS.md and TechnicalDebt.md first), never edit them for pipeline reasons. The full pipeline and rules live in the pipeline repo (private); this is the short copy. The goal and standing rules are the pinned issue #63.

1. **Find the stage** in `PIPELINE.md`. The spec is `SPEC.md`, the code map `docs/code-map.md`.
2. **Build on `main`**, one branch per change (`fix/<slug>` or `feat/<slug>`); open PRs against `main`.
3. **Work from an issue.** Take one that is not `in-progress` and not blocked; add `in-progress` before you start. #66 (upstream sync) goes first.
4. **Linux first.** Say which machine class you built and ran on; never overwrite the release install with our build.
5. **Check that it builds** (`cmake --build build`, then `ctest`), then open the PR. CI and a review do the rest.
6. **Label the PR** `by:<machine>` and `model:<model>`, and write `Closes #N`.
7. **A failed job** gets a comment on the issue and the issue moves up one size.
8. **No game files, ever** (#63). No private details in code, commits or PR text. Commit as the no-reply identity. GPL-2.0 only.
9. **Upstream:** its policy forbids AI-written contributions. Never push, comment or open anything on boykopovar/AnyPS5, and do not link its issues from ours.
