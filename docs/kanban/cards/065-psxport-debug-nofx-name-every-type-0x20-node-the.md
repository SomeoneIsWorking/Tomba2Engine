---
id: 65
title: PSXPORT_DEBUG=nofx: name every type-0x20 node the native walk has no producer for
status: todo
labels: [render, tooling]
created: 2026-07-28
updated: 2026-07-28
---

Added 2026-07-28. A type-0x20 node draws through the render fn at node+0x18, and Render::fieldObjectsRender dispatches those through a hard-coded WHITELIST of fn addresses (render_walk.cpp). Anything not on it falls through and this walk draws nothing for it. Until now, finding those meant a STATIC census over the callers of one shared writer (kanban #15) — which only sees effects that use that particular writer, and cannot tell you which ones a given scene actually reaches. This turns it into a log line, per scene, for any writer.

WORDING MATTERS, and the first draft of this log got it wrong: a line here means 'the NATIVE walk has no whitelist entry for this fn', NOT 'this effect is blank'. The substrate walk still runs underneath, and a producer SCOPE on the fn (fx_mesh.cpp's FX_CONTROLLER_SCOPE family) can catch its writer calls from there. So each line is a producer-gap CANDIDATE to confirm with the fxmesh/fxsprite channels, not a finding on its own.

FIRST RUN, three scenes — and it already names things the static census could not:
  bucket-softlock:      0x8002AB5C  0x8013CDD4  0x8002BC9C  0x8013E08C  0x800288AC  0x80033080  0x8002801C  0x8002ECD8
  weapon-impact-bucket: 0x8002AB5C  0x8013CDD4  0x80033080  0x8002B3A4  0x8002A834
  hut-entry-alt:        0x8002AB5C  0x8013CDD4
Of those, 0x8013E08C / 0x8002801C / 0x8002ECD8 / 0x8002B3A4 never appeared in the #15 writer census at all — they use a different emitter. 0x8013CDD4 shows up in EVERY scene (it is WidescreenMarginQuad::emit, owned but not whitelisted here) and 0x8002AB5C is the terrain fn (owned via terrainRenderAll, reached another way) — two examples of why a line is a candidate and not a verdict.

THE HEADLINE: 0x80033080 appears in BOTH field scenes and has NO native owner. kanban #15 identifies exactly that address as the IMPACT effect's controller ('FUN_80033080 = { FUN_80027E5C(); FUN_800288AC(node); }'). So the impact effect's own node render fn is not on the whitelist, while #15's 2026-07-23 fix scoped its CALLEE. That is a concrete, addressable lead for the user's 'impact still missing' report — chase it next.
