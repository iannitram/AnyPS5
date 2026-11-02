# Code map: AnyPS5 (fork)

- Repo: iannitram/AnyPS5, a fork of boykopovar/AnyPS5 (C++20 / CMake, GPL-2.0-only).
- Kind: static relinker plus reimplemented PS5 system libraries (no emulation). It converts a PS5 executable into a native Linux ELF, Windows PE or macOS Mach-O that calls native implementations of the system libraries.
- Integration branch: `main` (merge commits only). Existing-code snapshot: 369f2ac474 (2026-10-28), whose last code change is merge `e7c0343f5` (2026-10-26). The 2026-11-01 product plan (#87) below describes planned fork work, not present code.
- Upstream: `boykopovar/AnyPS5` (`upstream/main`). This fork does not post to it.

## Layout

| Path | What it owns |
|---|---|
| `core/relinker/` | The relinker tool. `main.cpp` runs the steps; `cli/` arguments; `io/` readers and writers; `domain/` shared types; `codegen/` `--to-intel` lowering of AMD-only instructions; `elfpatcher/` Linux ELF, Windows PE and macOS Mach-O output (shared entry stubs and layout builders, per-OS patchers and guest-module writers); `relinker/` import reading, NID filtering, game patches (`src/patching/GamePatch.cpp`), conversion pipeline and its tests; `wasm/` browser build. |
| `core/libs/prx/` | One directory per PS5 system library (about 370, named `libSce*`, plus `libc`, `libkernel`). Each builds as a shared library whose exports are later renamed to NIDs. Largest: `libc`, `libkernel` (Pthread, File, System, Module/DynamicLoader), `libSceAgc*` (GPU command buffers), `libSceLibcInternal`, `libScePosixForWebKit`. |
| `core/libs/nid/` | `nid_patcher`: computes NIDs (SHA-1 of the name) and rewrites exports in ELF, PE and Mach-O; Python tests for layout, hash tables, symbol versions. |
| `core/libs/allocator/` | Windows mimalloc import-order fix (`PeImportFirst`) and runtime heap. |
| `core/libs/tests/` | Native runtime tests for the libraries (about 735 files), registered with ctest. |
| `core/libs/{HitLog,SceShaders,SceTypes}.hpp` | Shared headers for the library layer. |
| `core/shader/recompiler/` | RDNA shader recompiler to SPIR-V: `RdnaDecoder/`, `ControlFlow/`, `IntermediateRepresentation/`, `Optimization/`, `Translation/`, `SpirvBackend/`, disk cache (`ShaderDiskCache`, `ShaderCachePack`), `tests/`. |
| `core/Decoder/{Jpeg,Png}/` | Image decoders used by the libraries (JPEG via libjpeg-turbo). |
| `3rdparty/` | Git submodules (see Open-source pieces). |
| `cmake/` | Build helpers: `SystemDeps`, `Opus`, `Jxr`, `Tracy`, `ZArchive`, Windows exit shims. |
| `demo/web/` | Browser converter front end (`app.mjs`, `archive.mjs`, `batch.mjs`, `index.html`) with Node tests in `demo/web/tests/`. |
| `tools/` | Python and shell dev tools with tests in `tools/tests/`: `check_conventions.py`, `check_compile.py`, `check_main_sync.py`, `check_skips.py`, `check_test_registration.py`, `compile_slot.sh`, `pr_overlap.py`, `pr_labels.py`, `select_tests.py`, `stubgen.py`, `progress.py` and `progress_history.py`, `performance_compare.py`, `trace_digest.py`, `symbolize_samples.py`, `package_release.py`, `prepare_release_notes.py`, `build_web.py`, `test_web.py`, `unwrap_self.py`, `nid_names.py`, `import_audit.py`, `audit_dump.py`, `configure_seed.py`, `env_reference.py`, `debt_split.py`, `suggest_patches.py`, `pad_input.py`, `pad_timeline.py`, `run_job_manager_reference.py`, `rdna_isa.txt`, `input-mapper/` (Node app, own CI workflow), `hw-oracle/` (Python and C hardware-oracle runner). |
| `docs/user/` | `USAGE.md`, `INPUT_MAPPING.md`. |
| `docs/dev/` | `ARCHITECTURE.md`, `BUILD.md`, `CONVENTIONS.md`, `ENVIRONMENT.md`, `GAME_TESTING.md`, `HW_ORACLE.md`, `PROGRESS.md`, `TRACING.md`, `TechnicalDebt.md`, and `debt/<library>.md` (about 250 known-gap files). |
| `AGENTS.md`, `CONTRIBUTING.md` | Contribution rules (Conventional Commits, no code comments, throw rather than fake success). |
| `.github/` | Workflows, issue and PR templates, labeler. |

## How it fits together

```mermaid
flowchart LR
    elf["PS5 ELF + sce_module/*"] --> rl["core/relinker"]
    rl --> app["native app (ELF / PE / Mach-O)"]
    prx["core/libs/prx/*"] --> nid["core/libs/nid: nid_patcher"] --> libs["libs/*.prx"]
    app -- "loader binds imports by NID" --> libs
    libs --> sh["core/shader/recompiler (GPU path)"]
    libs --> sdl["SDL2, Vulkan, ffmpeg-core"]
```

- Entry point of the tool: `core/relinker/main.cpp`. Order: optional `--to-intel` lowering, `RelinkerPipeline` (read imports by NID, check syscalls, drop unused NIDs, build the dynamic section), `GuestModuleBuilder` for bundled modules, then `LinuxElfPatcher` / `WindowsPePatcher` / macOS image writer. Game patch files (a TOML subset) are applied to the bytes first.
- Runtime: the converted program imports functions by NID; the OS loader binds them to the patched `.prx` shared libraries from `core/libs/prx`. Guest code allocates from a guest arena (`libc/src/GuestHeap.cpp`).
- Graphics: `libSceAgc*` builds command buffers, the shader recompiler produces SPIR-V, and Vulkan presents through SDL2.
- Unsupported behavior throws `std::runtime_error`; unimplemented exports call `NotImplemented_nid_no_patch`.
- Full detail: `docs/dev/ARCHITECTURE.md`.

## Where our work is

`git log upstream/main..HEAD` shows about 102 non-merge commits (46 fix, 44 feat, the rest docs, tests and pipeline files) plus many merge commits; `git diff upstream/main...HEAD` is 100 files, +13,647 / -40. Our merge-base with upstream is `18ef6be0c`; upstream was about 2,060 commits (merges included) ahead at this snapshot, so a `sync/upstream-*` branch is normally in flight.

| Area touched | Share of changed files |
|---|---|
| `core/libs/tests/` (new tests for our library work) | about 37% |
| `core/libs/prx/libc/src/` (exceptions, regex limits, libm and complex exports, GuestHeap-adjacent fixes) | about 37% |
| `core/libs/prx/libSceLibcInternal/` (glob, descriptor printf, file tree walk, error reports) | about 6% |
| `core/libs/prx/libkernel/` (per-thread pending signals, setitimer/SIGALRM, pthread/posix, file, system) | about 9% |
| `core/libs/prx/libScePosixForWebKit/` | about 3% |
| `docs/dev/debt/`, `TechnicalDebt.md` | about 3% |

The remaining files are the fork's own docs (`PIPELINE.md`, `SPEC.md`, `AGENTS.fork.md`, `CHANGELOG.md`, this map). Nothing under `core/relinker`, `core/shader`, `demo` or `tools` differs from upstream in our commits. All of our work is POSIX/libc-layer library behavior and its tests.

## Product module plan

This is the approved fork delta from #87. Every module below is **planned**; existing tools are foundations, not completed companion features. Paths under tools/companion/ are proposed ownership boundaries established by #88. I keep the relinker and native runtime independent of the companion.

| Module | Proposed owner / existing seam | Status | Issues |
|---|---|---|---|
| Session contracts | tools/companion/contracts.py; tools/tests/test_companion_contracts.py | planned | #88 |
| Diagnostics | tools/companion/{recorder,diagnosis,report}.py; existing FailureReport/VideoOut/tracing | planned | #89-#91 |
| Test lab | tools/companion/{routes,runner,compare}.py; existing pad_timeline/trace_digest/performance_compare | planned | #92-#94 |
| Game library | tools/companion/{install,library,service}.py and ui/; existing audit_dump/import_audit/relinker CLI | planned | #95-#97 |
| Compatibility passport | tools/companion/{compatibility,versions}.py and ui/; existing version/hash-checked patch tooling | planned | #98-#100 |
| Input studio | tools/companion/profiles.py and ui/; existing tools/input-mapper and native libScePad | planned | #101-#103 |
| Design pass | Running companion views using the shared approved design system | planned | #104 |

### Session contracts

G0 (#88) creates the importable skeleton and v1 installation/session/evidence contracts. Installation records identify source locally, generated output, title/content version, executable/library revisions and patch/config fingerprints. Session records add process identity, monotonic events, hardware/driver/OS, instrumentation and cache mode. Evidence records carry source, method, timestamp and explicit unknown/partial/dropped states. An allowlisted export view excludes private paths and arbitrary data.

Other modules consume these records rather than scraping each other's UI text. Parsing rejects unsupported versions; unused scaffold operations fail explicitly. The scaffold does not implement a macOS runtime.

### Diagnostics

D1 (#89) adapts existing failure reports, call traces and VideoOut/pad/performance telemetry into bounded session events. D2 (#90) maps evidence to a category, supporting event IDs and safe next actions. Unknown causes and inactivity remain unknown; only direct evidence establishes an unsupported operation or explicit failure.

D3 (#91) builds a previewable allowlisted report. No arbitrary file collection, game bytes, memory dumps, artwork, credentials or unreviewed uploads. Absolute paths remain local. The UI and test lab consume the same diagnosis.

### Test lab

T1 (#92) defines versioned routes: initial-state description, bounded input sequence, checkpoint predicates and timeouts. It adapts tools/pad_timeline.py and tools/pad_input.py; current injected-input telemetry and wall-time scheduling limitations must be represented, not hidden.

T2 (#93) owns process lifecycle, descendant cleanup and checkpoint orchestration. The library reuses it for Play/Stop. T3 (#94) joins session/milestone records to tools/trace_digest.py and tools/performance_compare.py. Matching requires content, route, initial conditions, hardware/config and instrumentation/cache context; incomparable runs cannot support a speed verdict. This is repeatable testing, not deterministic replay.

### Game library

L1 (#95) uses tools/audit_dump.py, tools/import_audit.py, tools/unwrap_self.py and core/relinker/cli/ for source inspection and conversion. Source remains untouched; staging output becomes an installation only through an atomic manifest commit. Canceled/failed/partial imports are never Ready.

L2 (#96) owns the persistent library and UI/backend boundary. Packaging/toolkit selection is part of that issue, using existing dependency/license rules. A web-backed shell requires authenticated local IPC or loopback-only service with origin checks. The browser-only converter cannot launch a native game; reuse its patterns without implying it already supplies a desktop library.

L3 (#97) binds manifests and per-game profiles to the shared lifecycle manager, diagnostics and UI. Removal is confined to generated installation paths and preserves originals, saves and settings. The companion may exit while the converted program remains a native OS process.

### Compatibility passport

C1 (#98) stores observed boot/menu/gameplay/save/load milestones with session provenance. Static import audit, function counts and drawn frames remain different evidence types; manual assertions carry that label. Reports from different title versions/builds/devices cannot silently merge into a universal compatibility status.

C2 (#99) presents provenance, matching differences, conflicts and staleness; an update claims a blocker fix only with a reviewed fix-to-failure link. C3 (#100) stages side-by-side versions and verifies save/config backups before a protected trial, sharing L1's atomic installation path. Version selection and save restoration are separate operations; unknown writable roots or corrupt backups block the protected flow.

### Input studio

I1 (#101) persists validated named per-game profiles and safe mappings-only sharing. Native libScePad defaults and tools/input-mapper/{mapping,validation}.js are the contract reference, including alternate bindings and conflicts.

I2 (#102) provides physical/mapped input preview and calibration through native transforms or shared contract tests. Unsupported live reload is shown as restart-required. I3 (#103) owns generic hold/toggle state and reset on focus/device/profile changes, plus an explicit required-action checklist. Existing shake/touch substitutes do not imply arbitrary gesture support.

### Design pass

UX1 (#104) is the last stage. It audits the actual installed Linux build against the pipeline's shared approved design system, creates sized design-pass findings and closes required gaps before Product beta. It covers every major view and loading/empty/error/edge state, keyboard/controller navigation, visible focus, readable text and reduced motion.

### Product data flow

```mermaid
flowchart LR
    source["Local owned dump"] --> install["Audit and atomic import"]
    install --> manifest["Installation manifest"]
    manifest --> library["Library / native launch"]
    route["Route and input profile"] --> runner["Shared process / checkpoint runner"]
    library --> runner
    runner --> native["Converted native process"]
    native --> events["Bounded session events"]
    events --> diagnosis["Diagnosis / report preview"]
    events --> compare["A/B test comparison"]
    compare --> evidence["Compatibility evidence"]
    evidence --> passport["Title passport"]
    manifest --> versions["Isolated versions / verified local backups"]
    versions --> library
```

### Product platform coverage

| Boundary | Linux / SteamOS | Windows | macOS |
|---|---|---|---|
| Contracts, route validation, diagnosis, report/evidence/profile serialization | Required | Portable tests | Portable tests |
| Import and relinker integration | First product target | Preserve existing target support; product packaging deferred | Relinker-only; no Play promise |
| Native lifecycle, calibration and rollback | Real installed-build acceptance | Dedicated companion qualification after Linux | Unsupported runtime, explained in UI |
| Shell and final design pass | Desktop/fullscreen keyboard/controller journey | No parity claim before qualification | No native-game UI acceptance claim |

Each implementation PR updates its completed slice here from planned to present with tests/platforms and links remaining work. Do not mark an entire feature present after only its schema or UI stub lands.

## Build and test

Commands as `docs/dev/BUILD.md` states them:

```sh
# relinker only (no submodules; runs on Linux, Windows, macOS incl. Apple silicon)
cmake -S . -B build-relinker -G Ninja -DCMAKE_BUILD_TYPE=Release -DANYPS5_RELINKER_ONLY=ON -DBUILD_TESTING=ON
cmake --build build-relinker --parallel
ctest --test-dir build-relinker --output-on-failure

# full build
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel
cmake --build build --target libs --parallel   # libraries are EXCLUDE_FROM_ALL
```

Needs CMake 3.22.1+, Ninja, C++20, x86-64. Before pushing: `python3 tools/check_conventions.py --base origin/main`; before a PR: `python3 tools/pr_overlap.py --branch <branch>`. Web converter: `python3 tools/test_web.py --build build-wasm`.

CI (`.github/workflows/build.yml`): `eligibility` (decides which jobs a change reaches), `relinker` (matrix of hosts), `relinker-asan`, `register-context`, `write-watch`, `linux` (self-hosted runner labeled `anyps5`), `windows` (windows-latest, WinLibs GCC). Other workflows: `web-tests`, `windows-msys2`, `release`, `pages`, progress reports, label and PR-intake automation, `conventions`.

## Mac / PC / Linux

- Linux x86-64: the primary full target; self-hosted CI builds and tests it. R04: this repo is a Linux-check repo, so a Linux machine must build and verify before a sync PR merges.
- Windows x86-64: full build with MinGW-w64 GCC (POSIX threads); CI job `windows` and `windows-msys2`.
- macOS: only the relinker-only build is supported (`-DANYPS5_RELINKER_ONLY=ON`); the runtime libraries are not built there, and `Aps5StartGuest` has no macOS implementation. CI covers macOS in the relinker matrix (macos-latest clang++, plus x86_64), and the `register-context` and `write-watch` jobs run on macOS runners (macos-15, macos-15-intel).
- Platform-specific code lives in `core/relinker/elfpatcher/src/{general,linux,windows,macos}` and in `cmake/` Windows shims.

## Open-source pieces

Submodules in `.gitmodules`: SDL2 (window, input, audio), Vulkan-Headers, SPIRV-Headers, SPIRV-Tools and glslang (shader validation/compile), VulkanMemoryAllocator, ffmpeg-core (KytyPS5 fork, media decode), opus, ogg, vorbis, vgmstream, LibAtrac9 (audio codecs), freetype, stb, libjpeg-turbo, jxrlib, mbedtls, zstd, ZArchive, nghttp2, libusb-cmake, mimalloc (optional Windows allocator), tracy (profiling).
