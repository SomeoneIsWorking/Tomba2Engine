---
id: 15
title: 42 native overrides are declared at overlay addresses with the resident form and never install
status: open
symptom: bindResident counted every unreachable declaration as anonymous "inactive"; 42 of 254 declarations are at overlay addresses declared with declareOverride, so they can never install and their native behaviour is absent from the product
state_items: S004
tags: native-override,overlay,architecture
created: 2026-09-19
updated: 2026-09-19
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

## The remaining 42

Each needs evidence of WHICH overlay image owns that address before it can be converted — guessing
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

## Why this is not an abort yet

An unreachable declaration is always a mistake and `bindResident` should refuse it. It does not,
because making it fatal today takes down a product that currently runs, for 42 declarations whose
owning images are not yet established. The refusal lands with the last conversion.

## Next step

1. Establish each address's owning image from the overlay load map, not by inference from
   neighbours. `activateOverlay` and the AREA-slot descriptor table are the ground truth.
2. Convert in batches by image, verifying after each batch that the install count rises by exactly
   the number converted — a conversion that does not raise it is an image name that does not match.
3. Each converted owner then needs its own picture check: an override that finally installs is a
   behaviour change, not a no-op.
4. When the list is empty, make the report the abort it should be.

## Related

Issue 0014 — the dropped chrome groups that led here. Issue 0013 — the user's save-menu report that
led there.
