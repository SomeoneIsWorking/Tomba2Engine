---
id: 18
title: The picture comparison has no `picture_decisive` set, so Tomba! 2's first picture number cannot be read
status: open
symptom: free_roam now reports 17664/71680 pixels (24.64%) differing — the first picture number this title has ever produced — but `tools/oracle_tomba2.py` declares no `picture_decisive`, so the guard that let the number through is the RAM set, which contains no camera and no fade phase. The two frames are the same scene at different fade levels
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
