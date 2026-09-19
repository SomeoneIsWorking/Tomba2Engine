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
| S019 | Linux x86-64 asset-free native/Lightrec CI builds and tests the repository | verified | — | G001, G003 |
| S020 | Windows x86-64 native/Lightrec CI builds and tests the repository | missing | — | G001, G003 |
| S021 | macOS x86-64 and arm64 native/Lightrec CI builds and tests the repository | missing | — | G001, G003 |
| S022 | Android arm64-v8a native/Lightrec CI assembles and tests the repository | missing | — | G001, G003 |

## Current focus

S001 is the current focus. The break-first removal is complete and the shared per-`Core` Lightrec
executor is pinned at psxport `f5d32d97`, which is also the commit
`build/psxport_resolved.txt` records for the verified build (`tools/verify_ci.py`, 24 of 24 tests,
execution boundary clean, 2026-09-19). Issue 0005 must now
prove one resident and one colliding-overlay override plus scoped original calls through the shipping
dispatcher. Tomba! 2 then regains its recorded free-roam frontier and passes representative gameplay.
Tomba! 1 remains deferred until that complete gate. Issue 0006's supported-syscall
continuation is resolved: the real-image product crosses native initialization
and completes its first native frame at the DEMO stage.
Issue 0007 is resolved at its execution boundary: the integrated real-title run
completes both native frames with zero fallback. Representative gameplay remains
unverified; the direct observation does not qualify UI resources.

## Capability details

### S001 — Tomba! 2 native/Lightrec product: blocked

Recorded pre-migration evidence establishes a real target frontier: GAME at frame 25, free-roam at
frame 216, 620/620 presentation fences, coherent native-render captures, and a fatal guest-VSync
boundary at `0x80085900`. The title-owned `TombaFrameDriver` composes input, timing/event delivery,
game tasks, render submission, diagnostics, and exactly one presentation fence.

Missing capability: prove that all remaining guest instructions route through the shared Lightrec
executor with nonzero translated execution, image-generation invalidation, a resident native/original
call reached in gameplay, and a colliding-overlay native/original call reached in gameplay. Existing binary evidence identifies `0x801113B4`
in A03 and A0B with different entry shapes. Then reach the recorded frontier and pass representative
interactive gameplay with no guest instructions at source or interpreter-first gameplay selector.
Bounded fallback telemetry must name each reason, remain below the declared threshold, and be excluded
from gameplay-conformance and performance evidence.
Issue 0005 is the first discriminator.
Resident declarations are restricted to the explicit resident token and text range captured at the
two boot-load boundaries. MODE overlays now use the same generation-qualified binding contract;
the remaining gap is authenticating and exercising the complete title load route on real data.

On main `4fe4e20`, the canonical title verifier passed 22 tests and execution-boundary checks;
issue 0007 holds the two-frame real-image Lightrec/fallback denominators. Current work qualifies
native dispatch by resident, stage, MODE, and AREA image generations. Authentic local runs passed
the former DEMO frame-3 and SOP frame-27 identity faults, completed the intro, and loaded A00 at
GAME frame 113. The next run stopped at frame 115 on OPN code in the shared `0x8018A000`
AREA/data slot without an image generation. After OPN/CRD activation and retirement before
raw-data reuse, 25 focused catalog checks pass. An authentic 350-frame run with the corrected
stage task entry/`gp` loader bound OPN, crossed frame 115, reported free-roam at frame 216,
and exited with 1,688,706 translated
blocks and zero fallback blocks. The authentic A03/A0B native/original-call discriminator,
retail reach of the resident owner/original-call path, independent comparison, and representative
interactive gameplay remain open (issue 0005). The isolated authenticated resident-byte probe is
recorded in that issue. On 2026-09-17 `replays/scene-transitions/hut-entry-alt.pad` ran 900 frames
through the hut-interior transition under Lightrec once the object-list walker stopped overriding
labels inside its own body (`docs/findings/render.md`, walker section); the 400-frame boot gate and
the 24-test verifier pass on the same build. On 2026-09-18 all 22 recorded replays under `replays/`
ran 900 frames each once the resident indexed loader FUN_80045558 became the one native owner of
AREA-slot (0x8018A000) code-image residency, which the pause-menu card page reaches from guest code
(`replays/bugs/machinery-cutscene.pad` had faulted at the CRD slot browser with no active image).

### S002 — Independent Tomba! 2 comparison: partial

`tools/oracle_compare.py --frame-step 1` (2026-09-18) compares the Lightrec product against
PSXPort's Beetle full-console reference (authentic SCPH-1001, the same disc, an independent
implementation) at title-declared checkpoints: GAME stage, seaside field, free roam, then a known
per-frame schedule of walking left and right and jumping. 405/405 checkpoints match on every
decisive range — task-0 stage entry, the six state-machine halfwords, area index, Tomba's position
and motion, and the three pad words the game read — plus his whole 0x184-byte G block; 244 distinct
positions were compared. `--selftest` seeds one position byte and the comparator reports exactly
that range. The measured barrier, pad-delivery, and exclusion facts are in `docs/findings/tooling.md`
"Oracle comparison". Issue 0004 still records that the dual-view PSX pane is refused because the
shared SDL_GPU backend does not yet own multiple targets.

Re-measured 2026-09-18 on `shared/lightrec` `3fddb23` (psxport `fea6a7da`), which changes translated
code wherever a statically false branch carries a delay-slot load: the default 13-checkpoint run
matches on every decisive range with zero divergence, so the fix does not disturb this title's
conformance. The repository gate passes 24/24 on that pin.

Gap: the comparison covers main RAM only (no VRAM, SPU, or CD device state; the console cannot read
the scratchpad), one area, and ~400 frames of free roam. The console reaches free roam 29 frames
later than the product after an otherwise identical Start-skip of the opening cutscene; which side is
right is unmeasured.

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

On the Lightrec product (2026-09-18, `external/psxport/tools/port/looks_right.py --repository .
--replay replays/bugs/walk-dust-puff.pad --frames 520 --shot-at 250,400,500` with
`PSXPORT_AUTO_SKIP=1`): the 4:3 and 16:9 runs both reach 520 frames with no failure marks, the wide
frame differs from the 4:3 frame, and the seaside free-roam captures show a genuinely wider field of
view — the log fence and water on the left and a second ice block and the bridge on the right that
the 4:3 picture cannot see — at unchanged object proportions, with the in-game message box still
centered. With `aspect=1` and `fps60=1` the product still matches the Beetle console reference on
405/405 oracle checkpoints (S002), so presentation writes nothing into guest state.

That pass is now PROVEN to have run with the enhancements live, which it previously was not.
Until psxport `d37adcce` the `[wide] native picture:` line existed only in Spyro, so a Tomba! 2
oracle leg could show the settings file arriving and nothing at all about whether the picture
widened — and a settings file that is silently ignored passes every checkpoint. Four legs on
2026-09-19, each 405 checkpoints / 3,240 decisive range comparisons / **0 divergences**, complete:

| leg | product env | the product's own log |
|---|---|---|
| baseline | — | `aspect=3 wide_engine=1 native_width=320 render_width=320` |
| 60fps | `PSXPORT_FPS60=1` | `[fps60] TRUE per-object interpolated 60fps ON (source: env)` |
| widescreen | `PSXPORT_SETTINGS=<aspect=1>` | `aspect=1 wide_engine=1 native_width=320 render_width=428` |
| both | both of the above | both lines above, in one run |

The baseline is the negative control and it is not 4:3: this title's persisted settings carry
`aspect=3` (ASPECT_AUTO), which on a headless 4:3 sink resolves to `render_width=320` — no extra
coverage. So `wide_engine` alone does not answer "did it widen"; the 320 vs 428 render width does.
The comparator was shown the other answer first: `--selftest` seeded a byte at 0x800E7EAC and the
run DETECTED it.

This establishes that neither enhancement perturbs guest state the oracle observes. It is not
evidence that the extra horizontal pixels or the interpolated midpoint frames look right — the
comparator reads guest memory at checkpoints and never samples the extra presents. The capture
evidence above, and S006's tile classification, remain the only evidence about the pictures.

Menus were the first part of that gap to be closed, and closing it found a defect (issue #122 on the
kanban). Every opaque full-screen 2D page is authored 320 wide, so at 16:9 it covered only the 4:3
middle and the live field reappeared in both side margins behind a hard vertical edge: measured on the
pause/item menu (`replays/bugs/ingame-item-menu.pad` f1120), the in-game Select Options page
(`replays/bugs/ingame-options-page.pad` f1160) and the front-end one
(`replays/bugs/title-options-page.pad` f1027). Two call sites already carried a private copy of a
"pillarbox" quad, and the copy in `Render::optionsBackdrop` could never work: the framework spreads a
flat untextured fill on `RQ_BACKGROUND` only, and that page must draw on `RQ_OVERLAY` because it is
raised over a live field frame. `game/render/wide_page_fill.*` now owns that fill once for the title,
states the geometry in wide-final coordinates instead of relying on a material heuristic, and emits
nothing at 4:3. All three pages are solid to the canvas edges at 16:9; their 4:3 captures are
byte-identical to the pre-fix run; the in-game START page, which composites over the field on purpose,
still shows the full-width field at f1090; and the oracle still reports 405/405 checkpoints MATCH with
`aspect=1` and `fps60=1`, zero divergences (S002). The combined asset-free Clang gate passed 24/24
CTest cases, the C++ policy check over 412 first-party files, the execution-boundary scan and the
build-receipt pin check on psxport `18e8d184`.

Three more scene kinds were captured at 16:9 on 2026-09-19 and look right, so wide-edge culling is no
longer wholly unverified. The hut interior (`replays/scene-transitions/hut-entry-door-freeze.pad`
f1150) keeps its vertical field and widens horizontally: the left wall, a ceiling beam and the whole
vegetable barrel that 4:3 cuts off all become visible, at unchanged object proportions. The black
beyond the room's right side is authored, not a culling gap — it is black in the 4:3 capture too. The
cliff/village field (`replays/bugs/cliff-fisherman-missing.pad` f300) and the water-pump field
(`replays/bugs/weapon-charge-starburst.pad` f670) both fill the full canvas with coherent world
geometry to both edges. All three pass `reaches`, `widescreen` and `fps60`.

Three 2D scene kinds were captured on 2026-09-19 (`replays/bugs/ingame-item-menu.pad` f1110,
`title-options-page.pad` f1110, `ingame-options-page.pad` f1150), and they are the reason this item
is not closer to done than it was. All three reach their frames with no failure marks, and all three
draw their page at a **4:3 extent inside the 16:9 target** — measured drawn aspect 1.333 -> 1.335 for
both options pages and 1.420 -> 1.420 for the item menu, against 1.333 -> 1.784 for the 3D world
scene in the same run. On a real 16:9 display that is black pillars either side of every full-screen
page. Issue 0010 holds the measurements and the reason it must not be fixed by stretching.

Interpolation is clear of it: with `PSXPORT_FPS60=1` the real frames of that route are byte-identical
to the 4:3 leg at both f1000 (3D) and f1110 (menu) — 0 of 691,200 pixels differ — while the same run
emitted 615,338 interpolated prims over 1,091 extra presents.

Gap: the three 2D pages above do not widen (issue 0010). The memory-card pages, cutscenes, and HUD
anchors in every remaining scene kind are still uncaptured, and no area beyond these four has been
looked at. Note that `looks_right.py`'s `widescreen` check PASSED all three non-widening pages — it
asks only whether the PNGs differ — so earlier "widescreen PASS" lines in this document do not by
themselves establish that a scene gained coverage; only the `coverage` measurement does.

### S006 — Tomba! 2 interpolation: partial

The title owns prior/current camera, object, backdrop, and effect presentation state. Recorded still
captures show coherent output, but still images do not prove temporal smoothness.

On the Lightrec product (2026-09-18): the looks-right fps60 leg over the 520-frame free-roam walk
reports 492,600 interpolated prims across 505 extra presents (no duplicate frames), and
`external/psxport/tools/port/fps60_check.py` over 220 real/interp/real triples captured with `PSXPORT_DEBUG=fps60dump`
from frame 300 (moving camera, walking Tomba, water and effects) classifies the 16-pixel tiles where
anything moved as 75.0% BETWEEN (lerped), 0.2% STALE, and 2.2% AHEAD; the AHEAD tiles cluster on
Tomba's own sprite, whose walk-cycle frame flips cannot be blended and take the newer frame. RAM
parity with the console reference holds with fps60 on (S002).

A second scene kind is now dumped. The hut interior at 16:9 with fps60 on
(`replays/scene-transitions/hut-entry-door-freeze.pad`, `PSXPORT_FPS60_DUMP_FROM=1080`, 120 triples,
16-pixel tiles) classifies 87.3% of tiles STATIC, 12.0% BETWEEN, 0.4% STALE and 0.4% AHEAD — of the
6,185 tiles where anything moved, 94.2% are lerped, 3.0% stale and 2.8% ahead. That is materially
worse than the seaside field's stale share, and it is concentrated: tiles (112,160) and (144,160) are
stale in 21 of the 96 triples in which they moved, with (96,160), (80,96), (240,48) and (224,64)
next.

Re-measured 2026-09-19 on psxport `f5d32d97` at 16:9 with fps60 on, same replay and 120 triples:
86.7% STATIC, 12.8% BETWEEN, 0.2% STALE, 0.2% AHEAD. Of the 6,447 tiles where anything moved, 96.7%
are lerped, 1.6% stale and 1.7% ahead, against 94.2/3.0/2.8 before — the residual roughly halved.

That residual is now attributed rather than guessed. The `fps60seq` dump groups a captured frame by
entity node and reports each run's screen extent, so a stale tile can be looked up by coordinate.
All 105 stale and all 110 ahead tile occurrences are covered by a run, and every owner is a layer-1
reconstructed (TIER1) entity: **no verbatim producer owns a single moving tile in this scene**, so
the residual is not a missing producer. Crediting each tile to the smallest run covering it, which
credits an entity only where nothing smaller drew there:

| entity node | what it is | lerped | stale | ahead | snapped to an endpoint |
|---|---|---|---|---|---|
| `0x800FD850` | the hut room object | 392 | 28 | 35 | **13.8%** |
| `0x800FD958` | interior object, unidentified | 1,543 | 25 | 38 | 3.9% |
| `0x800E7E80` | Tomba's actor/view base | 1,240 | 22 | 14 | 2.8% |
| `0x800FDA60` | interior object, unidentified | 3,057 | 30 | 23 | 1.7% |

That table is a ranking of exposure, not of defects. `external/psxport/tools/port/fps60_check.py` now also measures, for
every tile it did not call STATIC, the best whole-pixel translation that aligns real frame N-1 onto
real frame N, so "the object is at an endpoint" can be separated from "the object did not move":

| verdict | tiles | mean shift | distribution |
|---|---|---|---|
| BETWEEN | 6,232 | 0.06 px | 0px 5,951 · 1px 222 · 2px 38 · 3px 9 · 4px 12 |
| STALE | 105 | 0.00 px | 0px 105 |
| AHEAD | 110 | 0.00 px | 0px 110 |

**Not one stale or ahead tile's content translated by a whole pixel.** A stale verdict requires the
two real frames to differ, so those tiles changed without moving a pixel: the change is sub-pixel,
and a t=0.5 sample of it quantises onto one side or the other. That is correct output, not a failed
lerp. The hut interior therefore has no attributable interpolation failure left at 16-pixel
granularity, and the room object leads the table above because it is the largest and slowest-moving
surface in the scene and so owns the most sub-pixel tiles — not because its camera input is wrong.
This supersedes the earlier reading of that 13.8%. Separately, across all 1,197 dumped fences no
entity's extent was ever identical to its previous frame's while the current frame's differed, so
nothing is frozen wholesale either.

The check is an instrument, not a rationalisation: fed a synthetic triple whose square really
translates 6 px with the interpolated frame drawn at the old position, it reports `2px:4` and names
those tiles the defect.

A third scene kind — the opening cutscene, where the flower creature swallows Tomba on the tree —
is the other answer, and it is the first measured interpolation failure in this title. Captured
2026-09-19 at 4:3 with fps60 on (`replays/bugs/cliff-fisherman-missing.pad`, fences 300-599, 299
triples, 16-pixel tiles): 24.1% STATIC, 72.3% BETWEEN, 0.4% STALE, 3.2% AHEAD. Unlike the hut, the
shift measurement does not clear it — **1,547 endpoint tiles translated a whole pixel or more and
were still drawn at an endpoint**.

**The two owner tables that stood here are withdrawn (2026-09-19).** They credited each moving tile
to the smallest `fps60seq` run covering it, and said 1,471 of the cutscene's 1,547 defects (95.1%)
and 1,328 of the FIELD route's 1,545 (86%) were layer-2 verbatim content. That attribution required
run extents and captured pixels to describe the same place, and they do not: a run's extent comes
from `RqItem` screen vertices in the renderer's own space while the capture is the VRAM display
region. This title's widest run spans 961 px against a 320 px frame.

Coverage reported 100% throughout and could not have done otherwise, because every frame holds a
screen-sized fill and a misaligned fill still covers every tile. The check that fails is whether
being inside a run predicts motion at all, with fills excluded: re-measured over 709 fences a tile
inside a specific run is **1.25x** as likely to have moved as one inside none, which is chance.
`fps60_check.py` now refuses the table rather than printing it (psxport 6a019811, psxport issue
0120).

What survives is everything measured from the images alone: the cutscene's 1,547 and the FIELD
route's 1,545 moved endpoint tiles are real, as is the hut interior's clean result. What producer
owns them is not established, and the conclusion that the cause is specifically LAYER-2 verbatim
content no longer has evidence behind it.

The split into two defects that followed from those tables — a large unported-producer one and a
smaller `0x800EDDA0` interpolation one with 184 moved endpoint tiles — went with them. Both figures
came from the same attribution.

The earlier note that the two coordinate spaces "coincide" here because every `gpu_shot` line
reports `320x240 @ 0,0` was wrong, and is worth keeping as a warning. A matching display origin only
means no OFFSET is needed; it says nothing about scale, and the scale does not match. Tomba! 2 needs
no offset and still scores 1.25x, which is why an identical origin looked like agreement and was
not. Capturing both channels in one run is still right, for the separate reason that a run and a
dump from different runs are not the same frames.

What Spyro 1 and Tomba! 2 have in common is now the interesting part, and it is established without
attribution. Forcing the interpolation factor to its previous endpoint (`PSXPORT_FPS60_TFORCE=0`)
over the same replay separates content that is interpolated from content that is not, using only the
captured images:

| | moved to the PREVIOUS endpoint | moved to the NEXT endpoint |
|---|---|---|
| t = 0.5 (product) | 255 | 2,259 |
| t = 0.0 (forced) | 28,308 | 1,811 |

28,053 tiles moved when `t` moved. At least **1,811 of the 2,259 forward-snapping tiles, 80%, did
not**, so they are not being interpolated at all: the in-between present draws them where the NEXT
real frame will show them, a whole frame early. That is the same result Spyro 1 gives, where the
invariant share is 99%. The remaining 448 are tiles that changed class between the two runs, which
is what a tile holding both interpolated and uninterpolated content does, so 80% is a floor rather
than an estimate.

This matters because it survives the withdrawn attribution. The mechanism is in
`Fps60::presentPass`: both presents of a fence run over the same captured queue and replace only
what a scene source owns, so anything with no native producer is drawn at the current update's
position in both. Closing it means reconstructing more producers. Which ones is the part that still
needs psxport issue 0120.

The field capture also answers what draws the sky here: nothing of ours. The backdrop, terrain and
scene-table sentinels (`0xFFFF0001`-`0xFFFF0003`) appear **zero times** in 1,731 dumped fences, so
the native backdrop tilemap producer never runs in this area and the whole top of the screen is
verbatim guest output. That is consistent with `Render::backdropTilemapDrawer` resolving false
outside the seaside and areas 10/11, and it means issue 0009's open Y-modulus question is not
reachable in this scene at all.

Gap: three scene kinds are measured, all from one replay. A 16-pixel tile cannot see an error
smaller than itself, so a sub-pixel residual would need a per-prim vertex comparison to bound. Which
producers own the measured defects is not established at all while psxport issue 0120 is open, so
the next step here is that issue rather than another capture; carrying an identity on
OT-walk-classified items is necessary but not sufficient, because the extents would still be in a
different space from the pixels. Running the forced-interpolation-factor experiment on this title
needs no attribution and would say whether its defect is the same forward snap Spyro 1 measured.
Issue 0009 remains open and now needs a scene where the native backdrop actually draws. Every layer
named stepped, snapped, cold, or unverified in the render inventory remains so.

### S007 — Tomba! 2 input: partial

The native input subsystem and deterministic replay/control surfaces exist and can reach controlled
game flow.

Under Lightrec the product walks Tomba left and right and jumps in the seaside field from held pad
input for 402 scheduled frames, and the independent reference agrees frame by frame (S002).

Gap: player-driven gameplay is demonstrated for one area and ~400 frames; other areas, longer
sessions, menus, and combat are not yet driven this way.

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
Tomba! 2 launcher provisioning has an exact-content manifest and staged validation
owner qualified against all 30 original-disc images (issue 0005). Runtime-load
authentication remains open and is not implied by runtime image-generation binding.
Representative gameplay through Lightrec remains unverified. The only permitted alternate execution
is the shared bounded automatic fallback described in `migration.md`.

### S019 — Linux x86-64 CI: verified

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

Evidence: hosted [run 33962696109](https://github.com/SomeoneIsWorking/Tomba2Engine/actions/runs/33962696109)
at `e897d1416771926628ee268e9ed1763c1d960055` passes its sole job,
`Linux x86-64 native/Lightrec contract`: GNU lightning passes 135/135 tests with
zero skips or failures, both title products build, and the repository passes all
21 CTests, including C++ policy and clang-tidy. The execution-boundary checker
passes its clean and three negative cases, then checks the linked products.
This verifies the Linux asset-free CI contract only; it does not qualify gameplay
or resolve the second-frame DrawSync failure tracked by S001 and issue 0007.

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
