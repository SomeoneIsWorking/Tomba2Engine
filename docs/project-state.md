# Project state

## Comparison baseline

The external baseline is the unmodified PlayStation releases of *Tomba!* and *Tomba! 2* on original
hardware or a trusted emulator. The immediate migration baseline is this repository's pre-migration
native/offline-translated hybrid, whose remaining guest code was emitted offline and compiled into each product.
The intended products retain title-native ownership while replacing that generated execution with
runtime translation by `psxport` Lightrec.

This inventory covers both titles without inferring one title's state from the other. Epic intent is
in `project-goals.md`, migration order in `migration.md`, atomic work in `issues/`, and placement
in `codemap.md`.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Tomba! 2 reaches representative gameplay as a native/Lightrec product | blocked | — | G001 |
| S002 | Tomba! 2 behavior is independently compared against the original | partial | S001 | G001 |
| S003 | Tomba! 2 game behavior is owned by readable native subsystems | partial | S001, S002 | G001 |
| S004 | Tomba! 2 picture is produced completely from game-owned scene state | partial | S001, S003 | G002 |
| S005 | Tomba! 2 true widescreen covers world visibility and 2D layout | partial | S004 | G002 |
| S006 | Tomba! 2 interpolation covers moving camera, objects, and effects | partial | S004 | G002 |
| S007 | Tomba! 2 accepts native player input through representative gameplay | partial | S001, S003 | G001 |
| S008 | Tomba! 1 selected executable and disc provenance are established | verified | — | G003 |
| S009 | Tomba! 1 identity, isolation, and independent startup evidence exist | verified | S008 | G003 |
| S010 | Tomba! 1 reaches representative gameplay as a native/Lightrec product | missing | S008, S009 | G003 |
| S011 | Tomba! 1 true widescreen works in the actual product | missing | S010 | G004 |
| S012 | Tomba! 1 and Tomba! 2 game-engine implementations are isolated | verified | — | G003 |
| S013 | Tomba! 1 exposes widescreen only and no unrelated enhancement modes | verified | S012 | G004 |
| S014 | Tomba! 2 sound effects and music work throughout the game | partial | S001, S003 | G001 |
| S015 | Tomba! 2 saves, reloads, and survives a full restart | partial | S001, S003 | G001 |
| S016 | Tomba! 2 movies play correctly | partial | S001, S003 | G001 |
| S017 | Tomba! 2 area and scene transitions work throughout the game | partial | S001, S003 | G001 |
| S018 | Both titles have removed their offline guest-source product paths | verified | — | G001, G003 |
| S019 | Linux x86-64 asset-free native/Lightrec CI builds and tests the repository | partial | — | G001, G003 |
| S020 | Windows x86-64 native/Lightrec CI builds and tests the repository | missing | — | G001, G003 |
| S021 | macOS x86-64 and arm64 native/Lightrec CI builds and tests the repository | missing | — | G001, G003 |
| S022 | Android arm64-v8a native/Lightrec CI assembles and tests the repository | missing | — | G001, G003 |

## Current focus

S001 is the current focus. The break-first removal is complete and the shared per-`Core` Lightrec
executor is pinned at psxport `eb5f23a8b3506f8853b3cfadcedc024cd90818a0`. Issue 0005 must now
prove one resident and one colliding-overlay override plus scoped original calls through the shipping
dispatcher. Tomba! 2 then regains its recorded free-roam frontier and passes representative gameplay.
Tomba! 1 remains deferred until that complete gate. Issue 0006's supported-syscall
continuation is resolved: the real-image product crosses native initialization
and completes its first native frame at the DEMO stage.
Issue 0007 now blocks the second native frame at the GPU queue call boundary.

## Capability details

### S001 — Tomba! 2 native/Lightrec product: blocked

Recorded pre-migration evidence establishes a real target frontier: GAME at frame 25, free-roam at
frame 216, 620/620 presentation fences, coherent native-render captures, and a fatal guest-VSync
boundary at `0x80085900`. The title-owned `TombaFrameDriver` composes input, timing/event delivery,
game tasks, render submission, diagnostics, and exactly one presentation fence.

Missing capability: prove that all remaining guest instructions route through the shared Lightrec
executor with nonzero translated execution, image-generation invalidation, a resident native/original
call, and a colliding-overlay native/original call. Existing binary evidence identifies `0x801113B4`
in A03 and A0B with different entry shapes. Then reach the recorded frontier and pass representative
interactive gameplay with no guest instructions at source or interpreter-first gameplay selector.
Bounded fallback telemetry must name each reason, remain below the declared threshold, and be excluded
from gameplay-conformance and performance evidence.
Issue 0005 is the first discriminator.
Resident declarations are now restricted to the explicit resident token and text
range captured at the two boot-load boundaries. Exact-content authentication and
overlay-specific activation/binding remain missing; a generation token is not an
authenticated title identity.

Observed startup: issue 0006 records the supported-syscall fix and a clean
real-image one-frame run through the boot prefix, START.BIN load, and DEMO stage.
After the complete generated-body removal, its normal-exit shared telemetry
reports 174 executor calls, 18,012 executed blocks and 128,794 executed
instructions, with zero fallback blocks/instructions
and zero entries for every reported fallback/refusal reason. The declared bound
is one fallback block per execution. This report does not expose translated-block
creation, cache-hit/miss, or invalidation totals, so those remain unmeasured.
This checkpoint does not establish gameplay conformance. Issue 0007 records the
Blocker: the second frame exhausts its guest-call budget inside native
`gpuDmaQueueSync`, at reported guest PC `0x80044E54`.

### S002 — Independent Tomba! 2 comparison: partial

The old same-project byte comparator could detect a forced difference but was not an independent
oracle. Issue 0004 also records that the advertised dual-view PSX pane is refused because the shared
SDL_GPU backend does not yet own multiple targets.

Gap: compare the Lightrec product's relevant state and devices against a trusted emulator, hardware,
binary evidence, or a diagnostic interpreter run across representative gameplay. A green
same-implementation comparison or a boot-only result is insufficient.

### S003 — Native Tomba! 2 behavior ownership: partial

The root `game/` tree owns substantial engine, world, player, scene, UI, audio, render, input, and
finite-frame behavior. Address and behavioral evidence remains in `docs/code-map.md`,
`docs/port-map.md`, `docs/findings/`, and `docs/info/`.

The provenance audit removed 24 generated-output C++ files and the remaining
generated-only methods in mixed actor, collision, and substate owners. Renaming
register transcripts had not made them independent native implementations.
Their registrations are absent; remaining native callers enter shared guest
dispatch. Independently authored render/game owners remain, including the typed
child-oscillator loop and terrain-snap implementation. The source verifier guards
the exact audited output paths and removed-owner entry points.

Gap: known unowned game behavior remains. Native ownership grows through readable title subsystems
and image-aware runtime overrides; missing ownership is executed by Lightrec, not by emitted guest
functions.

### S004 — Tomba! 2 game-state picture: partial

The native renderer and many scene/UI/effect producers draw from owned game state. A recorded
620-frame run attributed 552,424 primitives across 13 re-earned native producer rows while leaving
4,589 undeclared native primitives.

Gap: `docs/unported-render-inventory.md` still records missing or unverified layers. Re-establish
the renderer on the Lightrec product and complete producer attribution without reading the picture
back from GTE, ordering-table, or GP0 output.

### S005 — Tomba! 2 widescreen: partial

Recorded controlled runs produced a 960-pixel-wide gameplay picture with additional world content.
The title compositor preserves the authored central title picture while filling the side
canvas, and issue 0003 records its focused evidence.

Gap: matched 4:3/wide controls plus representative scene, culling,
edge-visibility, and HUD-anchor coverage remain. These gates must ultimately run on the Lightrec
product.

### S006 — Tomba! 2 interpolation: partial

The title owns prior/current camera, object, backdrop, and effect presentation state. Recorded still
captures show coherent output, but still images do not prove temporal smoothness.

Gap: representative moving-camera, object, and effect cadence remains unverified, including every
layer still named stepped, snapped, cold, or unverified in the render inventory. Final proof belongs
to the Lightrec product.

### S007 — Tomba! 2 input: partial

The native input subsystem and deterministic replay/control surfaces exist and can reach controlled
game flow.

Gap: no current native/Lightrec product run demonstrates player-driven representative gameplay.

### S008 — Tomba! 1 executable/disc provenance: verified

Evidence: C001–C003 and I001–I002 establish the measured `SCUS_942.36`: 559,104 bytes, SHA-1
`81cbc79f0230aeb4252e058039f47ac95a777f5a`, PS-X entry `0x8006B58C`, text
`[0x80010000,0x80098000)`, and selected-disc `SYSTEM.CNF` agreement before publication.
The altered-byte and malformed/wrong-disc cases produce the opposite answer.

### S009 — Tomba! 1 identity/isolation/startup evidence: verified

Evidence: the title-local checks establish executable identity, disc selection, engine isolation, and a
deterministic 35-field CRT0 boundary with forced-mismatch and too-short negatives. The recorded
startup facts include BSS `[0x8009AFB0,0x800A3348)`, SP `0x801FFFF8`, heap base
`0x800A3348`, heap size `0x15C8B0`, gp `0x80097FA8`, first `A(39h)` wrapper
`0x8006B70C`, and game main `0x800163B0`.

### S010 — Tomba! 1 native/Lightrec product: missing

Recorded pre-migration execution reaches visible SCEA and Whoopee Camp presentation, advances past
the resolved DMA3 callback loss beyond LBA 58739, and preserves title-local native frame, CD, DMA,
and projection owners. The next known boundary is issue 0006: public `CdSync` `0x800648C8`
forwards to internal `0x80065470`, whose timeout reaches fatal guest VSync with return address
`0x800654A4`. DMA registration `0x80067E84` installs callback `0x80066D80`.

Missing capability: after Tomba! 2 completes, reproduce the 35-field boundary through Lightrec,
route the six engine-neutral overrides and scoped originals by complete image identity, cross the
recorded CD/movie boundary, reach the title screen, demonstrate input, and pass representative
interactive gameplay with no guest instructions at source or interpreter-first gameplay selector. Automatic
fallback must be bounded, reason-counted, and below the declared threshold; its coverage is excluded
from conformance and performance evidence.

### S011 — Tomba! 1 widescreen: missing

Grounded projection facts are preserved: initialization publishes centre `(160,112)`, `H=544`,
and 320x224 display rectangles; only one later resident caller reasserts `H`.

Missing capability: after the Lightrec gameplay gate, identify visibility, wide draw-buffer, and 2D
layout owners and prove additional correctly projected content with controlled 4:3 comparison.

### S012 — Cross-title engine isolation: verified

Evidence: Tomba! 1 owners live below `titles/tomba1/`; its composition imports no root `game/`
source; the isolation checker rejects cross-title includes/tokens and source files above the
1,200-line structural boundary with positive and negative controls.

### S013 — Tomba! 1 widescreen-only scope: verified

Evidence: `titles/tomba1/enhancement_scope.json` contains only `{"widescreen": true}`; the title
surface rejects interpolation, temporal history, native rendering, native producers/depth, and 60fps
options.

### S014 — Tomba! 2 audio: partial

Native audio and CD/XA owners exist with focused evidence.

Gap: sound effects and music have not been observed throughout representative gameplay on the
Lightrec product.

### S015 — Tomba! 2 saves and restart: partial

Save and menu owners exist in the native engine.

Gap: no native/Lightrec product run proves save, process restart, and successful reload end to end.

### S016 — Tomba! 2 movies: partial

FMV and CD owners exist with focused pre-migration coverage.

Gap: representative movies have not been observed through the native/Lightrec product.

### S017 — Tomba! 2 transitions: partial

Scene and transition owners exist, and recorded flow includes title-to-gameplay transition.

Gap: representative area and scene transitions across the game remain unverified on the
native/Lightrec product.

### S018 — Guest-source-path retirement: verified

Evidence: both emitted guest-source trees are absent; the root and Tomba! 1 generator entry points,
emission-only seeds, offline registries, guest-source CMake manifests, and generation-only
selftests were removed together. Both launchers provision user-supplied executable/overlay inputs
without emitting guest source, and both CMake graphs build native/Lightrec product executables.
Tomba! 2 currently validates input sizes only; exact-content authentication remains missing in
issue 0005 and is not implied by successful provisioning or runtime image-generation binding.
Representative gameplay through Lightrec remains unverified. The only permitted alternate execution
is the shared bounded automatic fallback described in `migration.md`.

### S019 — Linux x86-64 CI: partial

The asset-free workflow checks out full history without persisted credentials, restores exact
psxport and Lightrec revisions, configures with Clang and Ninja, builds the native product and test
targets, and runs the repository CTest gate. It has an explicit timeout and needs no game assets.

Local evidence: clean checkouts of psxport `eb5f23a8b3506f8853b3cfadcedc024cd90818a0`
and Lightrec `b1457137c31cedff5f440d59da29401d021ba2da` configure with Clang and Ninja;
both title products link. The canonical Python
entry point consumes PSXPort's shared consumer verifier and its linked-product
execution-boundary discriminator. CI builds the pinned maintained GNU lightning
revision instead of relying on an unqualified system library.
The final combined local gate passes 21/21 CTests, including the production native
catalog's stale-image, rebind, and scoped-original-call regression. C++ policy
checks 395 first-party files and clang-tidy checks all 254 compile-backed C++
translation units. Both linked products pass the execution-boundary discriminator;
these asset-free checks do not qualify either title's gameplay.
The combined gate includes the resident-token restriction and generated-body
removal. The production catalog test checks an unrelated image colliding at the
same address; the retired-boundary selftest rejects exact audited output paths
and removed registrations without treating historical evidence as executable code.

Gap: the workflow has not yet completed successfully on a hosted Linux runner at this revision.

### S020 — Windows x86-64 CI: missing

Required capability: a hosted Windows x86-64 job that builds and tests the real native/dynarec product.

No Windows product composition or hosted build exists. The current CMake graph and psxport
presentation dependencies have not been qualified for Windows, so a source-only policy job would not
be Windows product evidence.

### S021 — macOS desktop CI: missing

Required capability: hosted x86-64 and Apple Silicon jobs that exercise the real native/dynarec product.

No hosted macOS x86-64 or Apple Silicon product build exists. In particular, executable-memory
publication, instruction-cache coherence, ABI transitions, SDL presentation, and representative
Lightrec execution have not been qualified on macOS arm64.

### S022 — Android arm64-v8a CI: missing

Required capability: a hosted Android arm64-v8a job that assembles and inspects the real APK.

There is no Android application, Gradle/NDK composition, shared `android-port` consumption, touch
layer, SAF setup flow, or arm64-v8a Lightrec product test in this repository. Android CI must assemble
and inspect a real APK; a desktop cross-compile or metadata check would not establish support.
