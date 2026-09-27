---
id: 15
title: 42 native overrides are declared at overlay addresses with the resident form and never install
status: closed
symptom: bindResident counted every unreachable declaration as anonymous "inactive"; 42 of 254 declarations are at overlay addresses declared with declareOverride, so they can never install and their native behaviour is absent from the product. ALL 42 are now converted to the identity-scoped form (SOP 7, A00 45), bindResident aborts on the condition, and tools/overlay_owner_map.py --census is a ctest that refuses a declaration naming an image that cannot install it.
state_items: S004
tags: native-override,overlay,architecture
created: 2026-09-19
updated: 2026-09-27
---

## What is wrong

`declareOverride` records a RESIDENT declaration. `bindResident` installs only declarations whose
address is inside the resident text range, and **skipped everything else into an anonymous
`inactive` count**. `declareOverlayOverride(imageName, ...)` is the only form that can reach an
address in an overlay slot.

So a native owner written for an overlay address, declared with the resident form, compiles, links,
registers, reports nothing, and **never runs**. The class is invisible: the code exists, the codemap
calls it LIVE, and the guest body executes instead.

**42 of 254 declarations — 16.5% of the catalog — are in this state.**

## How it was found

Chasing issue 0014's dropped chrome groups. `CardMenu`'s scope never fired on any of 2,400 frames.
Its declaration was `declareOverride(0x8018FBCC, ...)`, an AREA-slot address — so the whole
card-menu producer (kanban #102) had been dead since the day it was written, which is why 26,290 of
the card pages' own chrome groups were being routed with no scope and dropped.

`ov_ropeStrip` (0x801365C4) is the second: the producer written for the user's reported missing
BRIDGE ROPES (kanban #103), declared resident at an A00 address, never installed. Its own file
banner says "overlay guest 0x801365C4".

Both are fixed. `bindResident` now names every remaining offender on every run with a total.

## Fixed

All 42, plus the two above. Every one is now a `declareOverlayOverride` keyed by (image, address).

| address | name | image | converted |
|---|---|---|---|
| `0x8018FBCC` | `CardMenu::cardFrame` | CRD | before this pass (issue 0014) |
| `0x801365C4` | `ov_ropeStrip` | A00 | before this pass |

Final census state: **SOP 7/7, A00 45/45** declared and in text range, **0** declarations that can
never install, **0** function-shape notes, and **0 of 199** resident-form declarations outside the
resident text range. 254 declarations in total (199 resident + 55 overlay) — the same 254 as when
the issue opened; every conversion re-keyed a declaration and none added or dropped one.

## The 42 (HISTORICAL LIST — all converted 2026-09-27)

Each needed evidence of WHICH overlay image owns that address before it could be converted — guessing
an image name would install an override against the wrong body, which is worse than not installing
it. The addresses cluster: `0x8010Axxx`/`0x8010Bxxx` around the stage-overlay base `0x80106228`,
and `0x8011xxxx`-`0x8014xxxx` in the AREA slot that A00/A0B and friends share, where
`docs/re-frontier.md` already records a known collision (`0x801113B4` in both A03 and A0B).

| address | name |
|---|---|
| `0x8010AF60` | `sopBeatAdvanceWalk` |
| `0x8010B078` | `sopBeatAdvanceNarration` |
| `0x8010B11C` | `sopOrbitPathStep` |
| `0x8010B2D4` | `sopIntroEffectTick` |
| `0x8010B44C` | `sopIntroEffectSpawn` |
| `0x8010B588` | `sopLiftedSubtick` |
| `0x8010BEAC` | `beh_orbit_spark_effect` |
| `0x8010E258` | `ActorObjectContact::resolveHitOrProximity` |
| `0x8010EA80` | `ActorBump::respondToContact` |
| `0x80112188` | `ActorMeleeEngage::doIt` |
| `0x80118B10` | `AssemblyRider::rideSlotAndReactToStroke` |
| `0x80122BF4` | `beh_id_routed_offset_point` |
| `0x80123E9C` | `ReleaseTriggerMotion::hoverBobCycle` |
| `0x801241BC` | `ReleaseTriggerMotion::leaderFollowSync` |
| `0x80124328` | `ReleaseTriggerMotion::xSweepCycle` |
| `0x801244E8` | `ReleaseTriggerMotion::driftReposition` |
| `0x801246B4` | `ReleaseTriggerMotion::arcSwoopMotion` |
| `0x801249D4` | `ReleaseTriggerMotion::doubleArcMotion` |
| `0x80124C6C` | `ReleaseTriggerMotion::circleOrbitMotion` |
| `0x80125FE0` | `TiltFollower::applyHalvedOwnerPartPitch` |
| `0x80127420` | `beh_arm_countdown_if_linked_ready` |
| `0x801274BC` | `beh_distance_band_predicate` |
| `0x80127510` | `beh_spawn_toy_child_type2` |
| `0x8012763C` | `beh_spawn_toy_child_type4` |
| `0x80127720` | `beh_spawn_toy_child_type5` |
| `0x801281B8` | `RopeSwing::swingTickAndBendSegments` |
| `0x8012D27C` | `SwaySchedule::advanceRateThenSway` |
| `0x801316CC` | `SubstateEdgeLeaves::tickChildOscillators` |
| `0x801360F4` | `Spawn::spawnQuadRecordChild` |
| `0x801389C8` | `AssemblyCompanion::composeRigAndApplyPartScales` |
| `0x80138A64` | `AssemblyCompanion::endCamHoldAndRearmOnStroke` |
| `0x80139838` | `Spawn::spawnSiblingAngleChild` |
| `0x8013A730` | `Spawn::spawnLiftPlatformChild` |
| `0x8013AC34` | `Spawn::spawnChildTrigChild` |
| `0x8013D454` | `waterJetControllerTap` |
| `0x8014047C` | `ActorZonedAttacker::gateCheck` |
| `0x80140544` | `ActorZonedAttacker::typeInit` |
| `0x801409C0` | `ActorZonedAttacker::pickAttackByRange` |
| `0x80143A00` | `ActorZonedAttacker::defaultSubStateMachine` |
| `0x80144928` | `ActorZonedAttacker::approachAndFace` |
| `0x80144B50` | `ActorZonedAttacker::idleTick` |
| `0x80145C78` | `ActorZonedAttacker::zoneClassify` |

## MEASURED 2026-09-27: the owning image is now established for 33 of 38, from the images themselves

`tools/overlay_owner_map.py` answers step 1 of the plan below without inferring anything from
neighbours. It authenticates every MODE image (SOP and A00..A0L) against the tracked manifest
`config/tomba2-images.json`, and because every MODE image loads at the same base, file offset X is
address BASE+X in whichever image is active. So it groups the images by the function body they actually
hold at the address, and **every image is accounted for on every address** -- those whose file ends
before the offset, those with no function prologue there, and those that hold a body are each named.

Of the 38 unreachable declarations this tool reported at the time (`bindResident` counted 42; see the
2026-09-27 section above for why the tool under-counted by four and what that changed):

| verdict | count | what it means |
|---|---|---|
| exactly one image holds a function there | **29** | the owning image is determinate; the conversion needs no judgement |
| more than one image, bodies differ | **4** | contested -- the owner's own provenance must decide |
| no image holds a function there | **5** | the address is not a MODE function at all, so the declaration itself is wrong |

The contested four, with the collisions made concrete:

| address | owner | images holding a body there |
|---|---|---|
| `0x8010AF60` | `sopBeatAdvanceWalk` | **SOP and A0F**, 70 words each, DIFFERENT bodies |
| `0x8010B078` | `sopBeatAdvanceNarration` | **A00, A0F and SOP** -- 137, 71 and 41 words |
| `0x8010B44C` | `sopIntroEffectSpawn` | contested |
| `0x8010BEAC` | `beh_orbit_spark_effect` | contested |

This is the same collision `0x801113B4` in A03 and A0B demonstrated before the tool existed, and it is
why the issue is explicit that guessing is worse than leaving an owner alone: a declaration bound to the
wrong image installs a native owner over a **different function body**. `0x8010AF60` is a 70-word
function in SOP and a *different* 70-word function in A0F.

The five with no owner anywhere are a DIFFERENT defect from the other 33 and must not be converted:
`0x80124328 ReleaseTriggerMotion::xSweepCycle`, `0x80127420 beh_arm_countdown_if_linked_ready`,
`0x801274BC beh_distance_band_predicate`, `0x80138A64 AssemblyCompanion::endCamHoldAndRearmOnStroke`,
`0x80140544 ActorZonedAttacker::typeInit`. The address is not a MODE function in any authenticated
image, so either the address is wrong or the body is resident rather than overlaid. Converting these
would bind a native owner to whatever happens to live there, which is the exact failure the issue
warns about.

## MEASURED 2026-09-27, SUPERSEDED BY THE TWO SECTIONS BELOW: 32 of the 42 converted, 10 remaining

The conversion used `declareOverlayOverride(image, address, name, function)`, which is identity-scoped
by construction: the ownership key is the (image name, address) PAIR, and `bindOverlay` re-checks the
name AND the loaded text range AND the image identity at the address before installing. A guest
address alone is not a key, because every MODE overlay loads at `0x80108F9C` and the same numeric
address is a different function in each image that holds one there.

**The instrument was wrong before the code was.** `tools/overlay_owner_map.py` matched declaration call
sites with a regex for a LITERAL hex address, so it could not see a declaration that names its address
through a constant. Four were invisible that way, and all four are real offenders the product itself
names: `0x80145C78 zoneClassify` and `0x8014047C gateCheck` and `0x801409C0 pickAttackByRange` through
`constexpr FN_*` in `actor_zoned_attacker.cpp`, and `0x8013D454 waterJetControllerTap` through
`kControllerAddr` in `guest_gte_water_jet.cpp`. The tool reported 38 unreachable; the product's own
`bindResident` refusal named 41. The scan now resolves file-scope and header `constexpr` address
constants, refuses a name that is ambiguous across translation units instead of guessing, and REFUSES
OUTRIGHT rather than skipping when a declaration's address token resolves to nothing — a scan that
cannot see a declaration must say so.

### What the fixed scan decided, and what was converted

    [owner] of 42 unreachable declarations:  32 determined, 4 contested, 6 no owner

| verdict | count | action |
|---|---|---|
| exactly one authenticated image holds a function there | **32** | converted to `declareOverlayOverride` |
| more than one image, bodies differ | **4** | left resident; see "what would decide them" |
| no stack-allocating prologue in any image | **6** | left resident; see "what would decide them" |

Converted, in two batches, each verified by the install count the product's own `bindOverlay` reaches
(`tools/overlay_owner_map.py --census`, cross-checked against a recorded product run of 2026-09-26
whose `bound overlay 'A00' ... installed=10` the census reproduced exactly before any edit):

| batch | image | converted | census `in_text_range` before → after | `bindResident` unreachable before → after |
|---|---|---|---|---|
| 1 | `SOP` | 3 | SOP 0 → 3 | 42 → 39 |
| 2 | `A00` | 29 | A00 10 → 39 | 39 → 10 |

Declaration totals are unchanged at 254 (209 resident + 45 overlay), so no declaration was added or
lost — only re-keyed. `bindOverlay` now also prints `declared_for_image` beside `installed`, and names
by count any declaration that names the image but could not bind: without that denominator, `installed`
alone cannot distinguish a correct count from a coincidence.

### The 32 converted addresses and their images (batch 1 SOP 3, batch 2 A00 29)

`SOP`: `0x8010B11C sopOrbitPathStep`, `0x8010B2D4 sopIntroEffectTick`, `0x8010B588 sopLiftedSubtick`.

`A00`: `0x8010E258 ActorObjectContact::resolveHitOrProximity`, `0x8010EA80 ActorBump::respondToContact`,
`0x80112188 ActorMeleeEngage::doIt`, `0x80118B10 AssemblyRider::rideSlotAndReactToStroke`,
`0x80122BF4 beh_id_routed_offset_point`, `0x80123E9C ReleaseTriggerMotion::hoverBobCycle`,
`0x801241BC ReleaseTriggerMotion::leaderFollowSync`, `0x801244E8 ReleaseTriggerMotion::driftReposition`,
`0x801246B4 ReleaseTriggerMotion::arcSwoopMotion`, `0x801249D4 ReleaseTriggerMotion::doubleArcMotion`,
`0x80124C6C ReleaseTriggerMotion::circleOrbitMotion`, `0x80125FE0 TiltFollower::applyHalvedOwnerPartPitch`,
`0x80127510 beh_spawn_toy_child_type2`, `0x8012763C beh_spawn_toy_child_type4`,
`0x80127720 beh_spawn_toy_child_type5`, `0x801281B8 RopeSwing::swingTickAndBendSegments`,
`0x8012D27C SwaySchedule::advanceRateThenSway`, `0x801316CC SubstateEdgeLeaves::tickChildOscillators`,
`0x801360F4 Spawn::spawnQuadRecordChild`, `0x801389C8 AssemblyCompanion::composeRigAndApplyPartScales`,
`0x80139838 Spawn::spawnSiblingAngleChild`, `0x8013A730 Spawn::spawnLiftPlatformChild`,
`0x8013AC34 Spawn::spawnChildTrigChild`, `0x8013D454 waterJetControllerTap`,
`0x8014047C ActorZonedAttacker::gateCheck`, `0x801409C0 ActorZonedAttacker::pickAttackByRange`,
`0x80143A00 ActorZonedAttacker::defaultSubStateMachine`, `0x80144928 ActorZonedAttacker::approachAndFace`,
`0x80144B50 ActorZonedAttacker::idleTick`.

Eleven of these already carried the evidence in the file's own banner and were still declared
resident — `tilt_follower.cpp`, `rope_swing.cpp`, `sway_schedule.cpp` and `actor_bump.cpp` each say in
prose that the body is A00-only and registers with A00. The prose was right; the call was not.

### The last 10 converted addresses and their images (batch 3 SOP 4, batch 4 A00 6)

Batch 3 — the four contested, all **SOP** on caller reachability (the argument is above):
`0x8010AF60 sopBeatAdvanceWalk`, `0x8010B078 sopBeatAdvanceNarration`,
`0x8010B44C sopIntroEffectSpawn`, `0x8010BEAC beh_orbit_spark_effect`.

Batch 4 — the six the tool's entry rule could not place, all **A00** off the tool's own output:
`0x80124328 ReleaseTriggerMotion::xSweepCycle`, `0x80127420 beh_arm_countdown_if_linked_ready`,
`0x801274BC beh_distance_band_predicate`,
`0x80138A64 AssemblyCompanion::endCamHoldAndRearmOnStroke`,
`0x80140544 ActorZonedAttacker::typeInit`, `0x80145C78 ActorZonedAttacker::zoneClassify`.

### MEASURED 2026-09-27: the four contested, decided by CALLER REACHABILITY per image

The rule applied: **a function that no code in an image CALLS cannot be that image's definition.**
That is a fact about the image, not an inference from a neighbour or a source comment, and it is the
same standard the first thirty-two were held to. One refinement was needed and it mattered:

> A BRANCH is not a CALL. These overlays share epilogue/tail blocks across functions, so a `beq` can
> land mid-body of a function that starts earlier. A05, A06 and A0A each have a conditional branch
> aimed at one of these addresses; in all three the target is a **shared tail, not an entry** (A05
> `0x8010AF60` = `lw $ra,0x28($sp)` .. `jr $ra`; A06 `0x8010B44C` = `bne $a0,$s3,0x8010b474`; A0A
> `0x8010BEAC` = `lui $v0,0x1f80`). Counting those as callers would have invented reachability in
> three images. Only `jal`/`j` and a `jalr` over a formed constant count.

| address | owner | evidence in each image | owner |
|---|---|---|---|
| `0x8010AF60` | `sopBeatAdvanceWalk` | **SOP**: literal data word at `0x8010CA7C`, an op-0x3E call-fnptr slot in the pilot's cutscene SCRIPT — the `0x18`-stride `{u32 op/arg, u32 fnptr, u32 op/arg, u32}` table at `0x8010CA60..0x8010CAC8`, whose sibling op-0x3E entries are `0x8010AE9C` and `0x8010B498`. **A0F** (the other body holder) references the address NOWHERE: no `jal`/`j`, no `jalr` over a formed constant, no literal word. | **SOP** |
| `0x8010B078` | `sopBeatAdvanceNarration` | **SOP**: literal data word at `0x8010CA94`, the next op-0x3E slot in that same script table. **A00** (137 words) and **A0F** (71) reference it nowhere. | **SOP** |
| `0x8010B44C` | `sopIntroEffectSpawn` | **SOP**: a DIRECT CALL — `jal 0x8010b44c` at `0x8010BB60`, in a real instruction stream (delay slot `sb $v1,5($s0)`). **A0E** references it nowhere. | **SOP** |
| `0x8010BEAC` | `beh_orbit_spark_effect` | **No MODE image calls it.** See below — this is a real finding, not a gap in the scan. | **SOP** |

`0x8010AF60`/`0x8010B078` in SOP sit in a well-formed script table, and the contesting images hold
something else at that same offset: A0F's `0x8010CA60` region is a different instruction stream
(`0x00021042 srl $v0,$s0,0x10`), and A00's is another (`0x9642002E sh $v0,0x2c($s2)`). Neither is a
script table, so neither can be the image the script belongs to.

**`0x8010BEAC` is reached by no overlay at all, and that is the honest answer to the test.** Its single
xref is a raw 4-byte DATA reference in a **MAIN.EXE-RESIDENT** function-pointer table, not a call in
any overlay. (The file banner had put this table at `0x800A22B8`; the real address is `0x800A2AB8`,
0x800 too low. Corrected in `game/ai/sop_intro_events.cpp`.) The table holds the **consecutive pair**
`0x8010BEAC`, `0x8010BF54`:

* In **SOP** the pair is two consecutive DISTINCT functions. `0x8010BEAC` restores `ra` and returns at
  `0x8010BF4C`; `0x8010BF54` opens a different one (`addiu $sp,$sp,-0x30; sw $s0,0x18($sp); move $s0,$a0`).
* In **A0E**, which holds a different 73-word body at `0x8010BEAC`, that body runs **straight through**
  `0x8010BF54`, where the instruction is `lw $v0,($s0)` part-way through a 12-byte-stride loop. The
  two table entries would resolve to one function and then to its middle.

Only SOP makes the resident table coherent, so SOP owns the address. Because the table is resident and
the address is an overlay-slot address, the entry only has its intended meaning while SOP is the loaded
MODE image; **who installs it on a node's `+0x1C` is still untraced.**

All four agree with the file's own provenance (transcribed from `ram_sop.bin`) — but here that
agreement is a confirmation, not the evidence.

### MEASURED 2026-09-27: the six were a false negative of the tool's FUNCTION SHAPE, and the tool is fixed

`body_at` required a stack-allocating prologue (`addiu sp,sp,-N`) at the entry. A MIPS LEAF never
touches `$sp`, and a split prologue allocates after a leading `lui`/`lh` pair, so both read as "no
image owns this address". **The tool now recognises a second entry shape**: the address is an entry
when a `jr $ra` epilogue PAIR sits 8 bytes before it, with its delay slot in between. All six sit
exactly there, in A00 and in no other image. Measured, per address:

| address | owner | first instruction | shape | A00 body |
|---|---|---|---|---|
| `0x80124328` | `ReleaseTriggerMotion::xSweepCycle` | `move $a2,$a0; lbu $v1,6($a2)` | leaf jump-table dispatcher on `node[6]` | 48 words |
| `0x80127420` | `beh_arm_countdown_if_linked_ready` | `lw $v0,0x10($a0); lbu $v0,0x5e($v0)` | leaf | 12 words |
| `0x801274BC` | `beh_distance_band_predicate` | `lh $v0,0x60($a0); sll $v0,$v0,2` | leaf; table-indexed band comparison | 18 words |
| `0x80138A64` | `AssemblyCompanion::endCamHoldAndRearmOnStroke` | `lh $v0,0x6a($a0)` | leaf | 40 words |
| `0x80140544` | `ActorZonedAttacker::typeInit` | `lui $v0,0x800f; lh $v0,-0x2f68($v0)` | SPLIT prologue: frame allocated at `+8`, two instructions in | 104 words |
| `0x80145C78` | `ActorZonedAttacker::zoneClassify` | `slti $v0,$a0,4` | leaf; the {0,1,2} band classifier | 8 words |

On the tool's output all six became determinate A00 and converted as one batch. The same fix also
resolved the three declarations that had been shipping and installing all along —
`OverlayGt3Gt4::gt3` (`0x801465EC`), `gt4` (`0x801467BC`) and `TileGridLayer::scrollStep`
(`0x8011534C`) — so the census's function-shape note list is now **0** and every A00 declaration reads
`image_holds_the_function` at its full count.

#### The widened rule needed a second guard, and finding it changed an existing verdict

Widening the entry rule introduced a false positive, caught by diffing the old and new holder sets for
all 45 overlay declarations: `0x801241BC ReleaseTriggerMotion::leaderFollowSync`, already converted to
A00, began reading as **contested** — A01 appeared to hold a function there too.

It does not. In A01 the `jr $ra` 8 bytes back is an **early return inside a function that continues
past it**: the code at `0x801241BC` is `lh $v0,0x2e($v1)` / `slti` / `beqz $v0, 0x801241a0` — a loop
body whose branch goes back *below* its own entry, into the earlier function's own `jr $ra`. So the
rule now **refuses** a shape-2 entry whose body contains a conditional branch targeting below the
entry. That is sound rather than a special case: branches are PC-relative, and `jal`/`j` are
unconditional transfers to a callee or tail, so a well-formed function body cannot contain a
conditional branch below its own entry. With the guard, the diff over all 45 declarations is exactly
the three long-shipping shape notes and **nothing else** — zero declarations went determinate →
contested.

Worth recording: A00 *does* hold a real function at `0x801241BC` and calls it with
`jal 0x801241bc` at `0x80124F58`, so that conversion was never wrong. What the fix corrected is the
CLAIM that it was uncontested.

### The refusal is now the abort

`bindResident` **aborts** when any resident-form declaration falls outside the resident text range.
It was non-fatal only while conversions were outstanding, when an abort would have taken down a
product that runs; the list reached zero with the last ten converted, and the census agreed
(`0 declaration(s) that can never install`, `0 of 199 resident-form declarations ... can NEVER
install`) before the abort was made fatal. When the two ever disagree, the census is wrong, not the
abort.

The two are deliberately different kinds of refusal. `tools/overlay_owner_map.py --census` (ctest
`tomba_overlay_owner_census`) cross-checks every declaration against the authenticated MODE images and
exits 1 on a mismatch: a refusal over CODE, with no product running. `bindResident` is the backstop
for what the census cannot see — a resident-form declaration at an address no MODE image covers,
including AREA-slot addresses — and it has its own death test
(`tomba_native_override_unreachable_abort`, ctest 11), which forks and asserts the child died by
`SIGABRT`. An untested fatal path is one nobody dares change.

Both refusals were shown the other answer, not just the green one:

* the census **exits 1** on a real mismatch — an A00-owned address re-keyed to `SOP` reports
  `outside SOP's loaded text range [80108F9C, 8010D498)`, and the ctest reports `Failed`;
* the death test **fails** with the abort removed, naming all three of its checks.

## Why this was not an abort until now

An unreachable declaration is always a mistake and `bindResident` should refuse it. It did not, because
making it fatal while conversions were outstanding would take down a product that runs, for
declarations whose owning images were genuinely still in question — and the whole point of this issue
is that guessing an image is worse than not installing. Both preconditions are now met: the list is
empty, and the census independently agrees. The abort landed with the last conversion.

## Next step

1. ~~Establish each address's owning image from the overlay load map, not by inference from
   neighbours.~~ **DONE for all 42**, by `tools/overlay_owner_map.py --unreachable`, after the scan
   was fixed to resolve named address constants (it had been reporting 38, not 42).
2. ~~Convert the determinate owners in batches by image, verifying the install count rises by exactly
   the number converted.~~ **DONE: 42 converted**, in four batches — SOP 3, A00 29, SOP 4, A00 6 —
   each batch's `declared` / `in_text_range` / unreachable counts checked to move by exactly the
   number converted. Final state: SOP 7/7, A00 45/45, 0 of 199 resident-form declarations unreachable,
   254 declarations total (199 resident + 55 overlay), unchanged throughout — only re-keyed.
3. ~~Decide the 4 contested by per-image caller reachability.~~ **DONE, all SOP.** Three by call site
   or script fnptr entry; `0x8010BEAC` by the coherence of the resident handler table's consecutive
   `0x8010BEAC`/`0x8010BF54` pair. Details above.
4. ~~Fix `body_at`'s prologue rule to release the 6 leaf/split-prologue addresses.~~ **DONE as a
   tool fix first**, then converted on the tool's output, so the last six met the same evidence
   standard as the first thirty-two. Widening it also required a back-edge guard; without it one
   already-converted address read as contested on a false positive.
5. ~~When the list is empty, make the report the abort it should be.~~ **DONE**, with a death test.
6. **OPEN, and now the whole of the remaining work: the picture checks.** Every converted owner is a
   behaviour change, not a no-op — these bodies were written against decompiled C and had never
   executed. `tools/verify_authentic_overlay_collision.py` and
   `tools/verify_authentic_resident_override.py` are green but cover `0x801113B4` (A03/A0B) and a
   resident address, **not** any address converted here. The risks already visible in the sources
   without a run:
   * `ActorMeleeEngage::doIt` (`0x80112188`) and `beh_id_routed_offset_point` (`0x80122BF4`) are
     transcribed from decompiled C, and both files record that they are NOT byte-exact — one lists
     a wrong-argument-class fix, the other says its byte-exact A/B gate has not been run. These two
     were wrong in a way nothing could see while they never installed.
   * `waterJetControllerTap` (`0x8013D454`) only RAISES a host scope and calls the original, and the
     scope is what makes the guest-GTE water-jet fallback present packets at all. Its file banner
     calls it deliberate fallback debt. A picture check should show the jet appearing; if it does
     not, this conversion is the reason.
   * `CardMenu::cardFrame` (CRD, `0x8018FBCC`) is the one that motivated the issue: it should show
     the card-menu chrome groups that 0014 found being dropped.
   * The rest claim byte-exact transcriptions with mirrored guest frames; for them the check is that
     nothing MOVES, which is a weaker but still real requirement.
   * The ten converted here carry the same class of risk as the thirty-two before them, and the four
     SOP ones run inside the intro cutscene, where the picture checkpoints in issue 0020/0021 are
     already known to photograph it before its fade finishes.

## Related

Issue 0014 — the dropped chrome groups that led here. Issue 0013 — the user's save-menu report that
led there.
