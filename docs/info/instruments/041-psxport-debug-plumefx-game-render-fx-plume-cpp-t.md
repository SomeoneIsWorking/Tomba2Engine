---
id: I041
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

PSXPORT_DEBUG=plumefx (game/render/fx_plume.cpp) — the four-copy radial plume producer's own channel

## Validated by

Validated against BOTH classes rather than reasoned about. POSITIVE: on replays/bugs/bucket-softlock.pad it prints 24 lines over f252-f263, each naming node/subtype/script frame/mesh/bias/angles/position and quads>0 plus the screen box the call emitted into, and those exact frames are where the A/B pixel gate finds 675-3267 changed pixels (claim C039) — 100% / 99.72% / 100% of which fall INSIDE the box this channel predicted, which is what makes the line checkable rather than merely present. NEGATIVE: the SAME binary on TEN of the 17 replays prints ZERO lines, and the producer-disabled leg prints zero on every replay — so the channel can produce the other answer. Its silences are deliberately distinguishable and each one has its denominator: 'no mesh loop' = the node carries no script or table (the guest emits nothing either), 'mesh=00000000' = the script frame selects no mesh, 'quads=0' = every record failed the writer's own ordering reject, and NO LINE AT ALL = fieldObjectsRender never reached the node. BLIND SPOT, stated: it sits behind fieldObjectsRender's `node+1 == 0` visibility skip, so it is structurally blind to a plume node the walk skips for invisibility — the same blind spot instrument I018 (nofx) carries, for the same reason. It is NOT inside a native override, so §0.3 of the unported-render inventory does not apply: it fires normally under PSXPORT_GATE=1 (measured — 24 lines under exactly that flag).

## Known failure modes

(none recorded yet)
