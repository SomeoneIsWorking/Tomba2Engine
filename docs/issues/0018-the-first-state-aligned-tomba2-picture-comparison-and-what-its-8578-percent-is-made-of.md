---
id: 18
title: Tomba! 2's first state-aligned picture comparison, and what its 85.78% is made of
status: open
symptom: with `picture_decisive` declared and every declared range agreeing byte for byte, a settled free-roam frame differs from the console reference in 61488/71680 pixels (85.78%). Mean luminance and per-channel means match, so it is not brightness, colour or quantisation; a ~3px camera offset explains only part of it
state_items: S005
tags: oracle,picture,instrument,guard
created: 2026-09-19
---

## Until today this title produced NO picture comparison at all

`tools/picture_oracle.py --bios ../SCPH1001.BIN`, before psxport `96c208cc`:

```
[picture] game_stage: REFUSED — the reference picture at game_stage is blank (0/71680 non-black)
[picture] field:      REFUSED — the product picture at field is blank (0/71680 non-black)
[picture] free_roam:  REFUSED — the product picture at free_roam is blank (0/71680 non-black)
```

Three dead ends. Not a match, not a difference — nothing. The cause was in the framework and is
fixed there: a checkpoint predicate is guest STATE, and this title enters those states long before
it draws, while the product's host-file I/O arrives far ahead of the reference's emulated disc. The
product had the complete title screen at game frame 27 while the reference was still black at 302.
Each core is now advanced, bounded and with no input, until it presents.

## What the run says now

```
[picture] console: game_stage after 302 game frames   (presented after 80 more)
[picture] native:  game_stage after 27 game frames
[picture] game_stage: REFUSED — not at the same guest state:
                      state_machine (+6: native 00 console 01), pad.current (+1: native 40 console 00)
[picture] field:      REFUSED — not at the same guest state: player.motion (+2: native A7 console 00)
[picture] free_roam:  17664/71680 pixels differ (24.64%), 206/280 tiles touched — spread
[picture]   worst tiles (0,64):256, (16,64):256, (128,64):256, (272,64):252
```

Two named diagnoses and one number. That is the instrument working.

## But the number cannot be read yet, and the reason is here, not in the framework

`compare.py`'s picture guard blocks on the title's `picture_decisive` when it declares one, and
falls back to the RAM `decisive` set when it does not. **This title declares none.** So free_roam
was admitted on `task0.entry`, `state_machine`, `area_index`, `player.position`, `player.motion` and
the three pad words agreeing — and on nothing else.

Not in that set: the camera, and the screen fade. Both frames at free_roam are the same scene — same
tree, same silhouette, same layout — at different fade brightness, and the worst tiles are a whole
row at y=64. That is the signature of a phase difference, not a rendering defect.

Spyro 1 hit this exact wall and answered it: `picture_decisive = <declared decisive> + camera +
dragon_cutscene`, on the measured grounds that a camera difference cannot change whether the game
behaves and moves every pixel (it framed a cutscene 25px left and 20px down, and 87% of pixels
differed). This title needs the same declaration, plus whatever owns the fade.

## Why this is not "just tighten the guard"

Tightening it will most likely turn free_roam into a third state refusal, and that is the correct
outcome: a refusal that names the fade is worth more than 24.64%, which today ranks nothing. The
work is to declare the ranges and then find a checkpoint where they agree — a settled moment in the
area, past the fade — so the title gets a picture number that means something.

## Next

1. Add `picture_decisive` to `tools/oracle_tomba2.py`: the declared decisive ranges plus the camera
   and the fade/transition phase. Name the guest addresses from the binary, not by guess.
2. Re-run and expect free_roam to refuse by name.
3. Move or add a checkpoint at a settled in-area moment both cores can reach, and compare there.

Related: spyro issue 0126 is the same fault in the other direction — a declared `picture_decisive`
that still does not carry the intro phase, so its `playing` number also ranks nothing.


---

## UPDATE 2026-09-19: the set was declared, and it moved the question somewhere else

`picture_decisive` is now declared in `tools/oracle_tomba2.py`: the existing decisive ranges plus
`camera.mode` (CAM_OBJ+0x64, the 18-entry mode dispatch), `camera.angles` (CAM_OBJ+0x6C, the three
angles that place the look point), `fade.sequencer` (CAM_OBJ+0x02) and `fade.level` (CAM_OBJ+0x68).
The camera and the fade sequencer share a base because the guest overloads that one block: the camera
driver 0x8006EC44 hardcodes it, and the fade sequencer 0x8010957C is handed the same address as its
node.

The camera's world position is deliberately NOT in the set. It is assembled into the scratchpad
(0x1F8000D2/D6/DA, `docs/areas.md`), and the console reference exposes main RAM only.

### It fires, which is the part that had to be proved

```
game_stage: REFUSED — state_machine (+6), pad.current (+1), camera.angles (+0: native 00 console D6)
field:      REFUSED — player.motion (+2), camera.mode (+0: native 07 console 00)
```

Both new camera ranges named a real divergence at a checkpoint, so neither is an ornament.

The fade pair needs the opposite care, and the framework was changed to make it visible
(psxport 4c10ce32 — the report used to print only what DIFFERED, so an agreeing range and a range
that is never populated looked identical). With values printed:

| range | game_stage | field | free_roam | verdict |
|---|---|---|---|---|
| `fade.sequencer` | 0000 | 0000 | 0100 | live, and both cores agree |
| `fade.level` | 00000000 | 00000000 | 00000000 | **inert at all three** |
| `fade.field_ramp` | 00 | 00 | 00 | **inert at all three** |

`fade.level` is the ramp driven from `sm[0x4e]==0xb`, and none of these checkpoints is there
(free_roam is `sm[0x4e]==1`).

`fade.field_ramp` (TASK0+0x6E) was added afterwards because it, not the sequencer, is the fade that
runs on area entry and exit: `Engine::fieldRun` case 9 arms it to 31 and case 10 counts it down to 0,
driving the leaf at 0x8007E9C8 with `(level * -8)` replicated into R/G/B
(`game/core/engine.cpp:1721-1738`). It reads 0 everywhere too.

**Two zeroes are not evidence until the address is proved**, because "no fade is running" and "this
range points at the wrong byte" produce the identical reading. The guest reaches this machine as
`sm = mem_r32(0x1F800138)` — a SCRATCHPAD pointer the console reference cannot read, so the range has
to be expressed against a fixed base. Asking the product directly (it can read the scratchpad), at
each checkpoint, through the same driver the comparison uses:

```
game_stage  sm ptr @0x1F800138 = 0x801FE000  == 0x801FE000   ramp@0x801FE06E=0
field       sm ptr @0x1F800138 = 0x801FE000  == 0x801FE000   ramp@0x801FE06E=0
free_roam   sm ptr @0x1F800138 = 0x801FE000  == 0x801FE000   ramp@0x801FE06E=0
```

The base is right. The zero means **no fade is running on either core at any of these checkpoints**.
Neither fade range has yet shown its other answer, so both remain unvalidated as instruments and will
only earn trust at a transition checkpoint — but the address itself is no longer in doubt.

### What free_roam actually says now

Under the stronger guard the number survived, and that is the finding. At free_roam **every declared
range agrees byte for byte** — `player.position`, `player.motion`, `state_machine`, `area_index`, all
three pad words, both camera ranges — and 388 bytes of `player.G` agree too. The only differing range
in the whole report is `task0.state`, the scheduler-phase one already excluded for a known and
documented reason.

So the premise this issue was filed on is **superseded**. The 24.64% is not admitted by a weak guard.
The two cores are in an identical guest state and the pictures still differ.

### What the difference is, measured

- Correctly aligned: a vertical-offset search over dy in [-20,+20] minimises at **dy=0** (24.64%),
  rising monotonically either side. Not a misregistration.
- The product presents 320x240 and the reference 320x224; the comparison uses the product's own
  `guest_scan` crop, so the compared region is 320x224 on both. The height difference is a separate
  display-mode question and is NOT what this number measures.
- Green-dominant and bidirectional: over the differing pixels, mean delta R +2.76 / G +10.54 /
  B +4.96, median G +16, and the product is DARKER at 2,934 of 17,664 of them. A uniform fade offset
  would move all three channels the same way.
- Of the differing pixels, 10,145 are "both draw, different colour", 5,807 "reference black, product
  draws something", 1,712 the reverse. Spread over the whole frame: only 1 of 224 rows is clean, and
  no row exceeds 250 differing.

**And both frames are nearly black.** Mean luminance: product 4.52, reference 2.96, out of 255. The
reference has **zero** pixels above luminance 64 and 3.34% above 32.

### The fade reading was wrong, and the measurement is what corrected it

An interim reading of this issue said the 24.64% was the two cores' fades sitting a step apart. That
is **falsified**: no fade is running on either core at free_roam, by the check above. The frames are
dark because the scene is a night beach, not because anything is fading it.

So with every declared range agreeing byte for byte and no fade active, the difference is about how
the two renderers draw the same scene from the same state. That is the first Tomba! 2 rendering
evidence of any kind — but see the next section before spending it.

### The caveat that still applies, and it is mine

`advance_to_presented` (psxport 96c208cc, added yesterday to unblock this title) advances each core to
the first frame that is not ENTIRELY black. That is by construction the darkest presented frame in the
sequence — the first frame of a fade-in. It converted "no comparison at all" into "a comparison", which
was the right move, but it lands the photograph at the one moment where the fade composite dominates
the frame and the scene barely exists.

That is why the delta is a broad low-amplitude brightness difference across the whole picture rather
than a located defect. And the product's frame-scoped fade colour is deliberately host memory, not
guest RAM (`game/render/screen_fade.h`), so the quantity most likely responsible is structurally
invisible to every range this guard can ever contain.

**Do not chase the 24.64% as a rendering defect until it is re-measured at a settled frame.** It is
currently a measurement of fade phase at maximum sensitivity.

### Next, replacing the original list

1. Add a settled free-roam checkpoint: `sm[0x4e]==1` plus the fade done, so the photograph is taken
   when the scene is actually lit. Name the fade-done condition from the binary — the sequencer at
   CAM_OBJ is not it, because it is inert here.
2. Re-measure free_roam there. Expect the number to drop sharply; whatever survives is the first
   Tomba! 2 rendering evidence worth the name.
3. Separately: the product presents 240 lines where the reference presents 224. That is its own
   question and deserves its own issue once someone has looked at which is right for this title.


## The settled comparison, which is the one that counts

The checkpoint frames above are taken at the first non-blank frame, which is the darkest presented
one. Running the title's own scripted route for 180 game frames past free_roam reaches a fully lit
scene, and the state guard did **not** refuse there — the two cores are still in the same declared
guest state:

```
free_roam:    17664/71680 (24.64%), 206/280 tiles
played-180f:  61488/71680 (85.78%), 280/280 tiles — spread
              worst tiles (288,0):256 (288,64):256 (304,0):255 (176,32):255
```

The number goes UP, not down, once there is a scene to compare. Both frames show the same hut, the
same tree, Tomba in the same place — this is not a wrong scene, it is the same scene drawn differently.

### What the 85.78% is NOT

| checked | measured | rules out |
|---|---|---|
| overall brightness | product 94.04, reference 94.48 mean luminance | a global fade/gamma difference |
| colour balance | mean delta R +0.15, G -0.89, B +0.35; all medians 0 | a systematic colour or channel shift |
| colour depth | both exactly 32 channel levels, 100% multiples of 8 | a 15-bit vs 24-bit quantisation difference |
| registration | best 2D offset dx=-3 dy=+1 takes 86.98% -> 73.81% | a pure translation; it explains part, not most |

The remaining difference is distributed and unbiased: 85.78% of pixels differ by at least one 5-bit
colour level, 50.15% by two or more, 17.69% by eight or more, mean max-channel delta 37.91. Every
tile is touched.

So: same geometry, same placement to within a few pixels, same overall light and colour — and broad
per-pixel disagreement everywhere. That is a shading/lighting/texturing question, and it is the first
Tomba! 2 rendering evidence that survives a real state guard.

### Next, superseding the list above

1. The ~3px camera offset (dx=-3, dy=+1) is its own defect and its own issue — the camera ranges
   agree in RAM, so the offset is introduced downstream of the state, in projection or viewport.
2. With registration resolved, re-measure; whatever remains is the shading question, and it needs a
   located comparison (one surface, one material) rather than a whole-frame percentage.
3. The product presents 240 lines where the reference presents 224. Separate question, own issue.
