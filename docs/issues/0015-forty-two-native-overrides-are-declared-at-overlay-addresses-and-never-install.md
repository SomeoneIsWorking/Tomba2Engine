---
id: 15
title: 42 native overrides are declared at overlay addresses with the resident form and never install
status: open
symptom: bindResident counted every unreachable declaration as anonymous "inactive"; 42 of 254 declarations are at overlay addresses declared with declareOverride, so they can never install and their native behaviour is absent from the product. 32 of the 42 are now converted to the identity-scoped form; 10 remain (4 contested, 6 whose owning image the extent heuristic cannot decide)
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

## Fixed so far

| address | name | image |
|---|---|---|
| `0x8018FBCC` | `CardMenu::cardFrame` | CRD |
| `0x801365C4` | `ov_ropeStrip` | A00 |

## The remaining 42 (HISTORICAL — 32 converted 2026-09-27, 10 remain)

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

## MEASURED 2026-09-27: 32 of the 42 are converted; 10 remain. The source scan was under-reporting.

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

### The 32 converted addresses and their images

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

### What the ten remaining need

**The 4 contested need their own provenance, not more image arithmetic.** No image argument can pick
between two different bodies, and the guest address is identical in both:

| address | owner | images holding DIFFERENT bodies there |
|---|---|---|
| `0x8010AF60` | `sopBeatAdvanceWalk` | SOP 70 words, A0F 70 words |
| `0x8010B078` | `sopBeatAdvanceNarration` | A00 137 words, A0F 71, SOP 41 |
| `0x8010B44C` | `sopIntroEffectSpawn` | A0E 42 words, SOP 19 |
| `0x8010BEAC` | `beh_orbit_spark_effect` | A0E 73 words, SOP 42 |

The file's own provenance says SOP for all four (`sop_intro_events.cpp` is transcribed from
`ram_sop.bin`, the SOP RAM dump, and the three `sop`-named ones were RE'd from it), which is
consistent but is inference from the source rather than from the image. What would decide it: for each
address, take the callers of that address IN EACH CONTESTING IMAGE and check which image's own call
sites can reach it — a function that no code in an image calls cannot be that image's definition.
A0E and A0F both also exist as field overlays, so the deciding evidence is whether A0E/A0F contain a
`jal`/`j` to the address, and SOP does. `tools/disasm_overlay.py <image> ... --base=0x80108F9C` is the
instrument; nothing needs new code.

**The 6 "no owner" addresses are a FALSE NEGATIVE OF THE TOOL'S FUNCTION SHAPE, not missing images.**
`body_at` requires a stack-allocating prologue (`addiu sp,sp,-N`) at the entry. A MIPS LEAF never
touches `$sp`, and a split prologue allocates after a leading `lui`/`lh` pair, so both read as "no
image owns this address". Disassembly of A00 shows a real function at all six:

| address | owner | first instruction | shape |
|---|---|---|---|
| `0x80124328` | `ReleaseTriggerMotion::xSweepCycle` | `move $a2,$a0; lbu $v1,6($a2)` | leaf jump-table dispatcher on `node[6]`, table at `0x80109B30` |
| `0x80127420` | `beh_arm_countdown_if_linked_ready` | `lw $v0,0x10($a0); lbu $v0,0x5e($v0)` | leaf; returns the parent's `+0x5E` byte, else stamps `0x14` into `+0x40`/`+0x4A` and `+0x05`=1 |
| `0x801274BC` | `beh_distance_band_predicate` | `lh $v0,0x60($a0); sll $v0,$v0,2` | leaf; table-indexed band comparison |
| `0x80138A64` | `AssemblyCompanion::endCamHoldAndRearmOnStroke` | `lh $v0,0x6a($a0)` | leaf; decrements `+0x40`, clears a global byte and `+0x6A` |
| `0x80140544` | `ActorZonedAttacker::typeInit` | `lui $v0,0x800f; lh $v0,-0x2f68($v0)` | SPLIT prologue: frame allocated at `+8`, two instructions in |
| `0x80145C78` | `ActorZonedAttacker::zoneClassify` | `slti $v0,$a0,4` | leaf; the {0,1,2} band classifier |

The same false negative already appears in declarations that were shipping and installed all along:
`OverlayGt3Gt4::gt3` (`0x801465EC`), `gt4` (`0x801467BC`) and `TileGridLayer::scrollStep` (`0x8011534C`)
all report no stack prologue in A00 and install regardless, because the product's rule is name+range+
identity and does not ask about function shape. `--census` now separates the two questions and prints
`stack_prologue_at_entry` as a SHAPE note, not a failure.

**What would decide them: fix `body_at` to accept an entry that is not a stack allocation, then re-run.**
The cheapest correct rule is "the address is a function entry if a stack-allocating prologue starts
there OR the preceding word pair is a `jr $ra` epilogue and its delay slot" — all six sit immediately
after such a pair. Once that lands, all six become determinate A00 and convert as one batch of six.
Doing it as a tool fix FIRST, rather than converting on a disassembly read, is what keeps the same
evidence standard for the last ten as for the first thirty-two.

### The remaining refusal

`bindResident`'s UNREACHABLE report stays a named, counted, non-fatal log line. It becomes fatal when
the list is empty. It should NOT become fatal at 10: a fatal abort on ten unconverted declarations takes
down a product that currently runs, for declarations whose owning image is genuinely still in
question, and issue 0015's whole point is that guessing an image is worse than not installing. The
gate that should hold the line in the meantime is `tools/overlay_owner_map.py --census`, which exits
non-zero when a declaration names an image that cannot install it — that is a build-time refusal over
code, not a runtime abort over gameplay.

## Why this is not an abort yet

An unreachable declaration is always a mistake and `bindResident` should refuse it. It does not,
because making it fatal today takes down a product that currently runs, for declarations whose owning
images are not yet established. The refusal lands with the last conversion.

## Next step

1. ~~Establish each address's owning image from the overlay load map, not by inference from
   neighbours.~~ **DONE for all 42**, by `tools/overlay_owner_map.py --unreachable`, after the scan
   was fixed to resolve named address constants (it had been reporting 38, not 42).
2. ~~Convert the determinate owners in batches by image, verifying the install count rises by exactly
   the number converted.~~ **DONE: 32 converted**, SOP 3 then A00 29, each batch's count checked.
3. **OPEN, and now the whole of the remaining work:** each of the 32 converted owners needs its own
   picture check. An override that finally installs is a behaviour change, not a no-op, and three
   conversion risks are already visible in the sources without a run:
   * `ActorMeleeEngage::doIt` (`0x80112188`) and `beh_id_routed_offset_point` (`0x80122BF4`) are
     transcribed from decompiled C, and both files record that they are NOT byte-exact — one lists
     a wrong-argument-class fix, the other says its byte-exact A/B gate has not been run. These two
     were wrong in a way nothing could see while they never installed.
   * `waterJetControllerTap` (`0x8013D454`) only RAISES a host scope and calls the original, and the
     scope is what makes the guest-GTE water-jet fallback present packets at all. Its file banner
     calls it deliberate fallback debt. A picture check should show the jet appearing; if it does
     not, this conversion is the reason.
   * The rest claim byte-exact transcriptions with mirrored guest frames; for them the check is that
     nothing MOVES, which is a weaker but still real requirement.
4. Decide the 4 contested by per-image caller reachability, and fix `body_at`'s prologue rule to
   release the 6 leaf/split-prologue addresses. Both are described above.
5. When the list is empty, make the report the abort it should be.

## Related

Issue 0014 — the dropped chrome groups that led here. Issue 0013 — the user's save-menu report that
led there.
