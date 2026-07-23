---
id: 55
title: Stunned monsters missing their spinning stars
status: todo
labels: [render, bug]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue55_stunned_stars_missing.png
---

USER 2026-07-23 with a side-by-side capture: a STUNNED monster should have SPINNING STARS orbiting its head. The reference side clearly shows the yellow star sprites above/around the stunned enemy; under pc_render they are absent entirely.

SAME CLASS as the torch flame (#12), dust (#39), non-seaside backdrops (#42), minimap (#43), vortex (#44) and the bucket pole (#54) — an effect the guest submits that has NO native producer under pc_render, so it is simply not drawn. Several of those are now fixed and give a proven recipe.

WHY THIS ONE IS PROBABLY CHEAP: the stars are almost certainly the SCALED-SPRITE effect family already ported for the torch flame — game/render/fx_sprite.cpp (guest FUN_80027A4C, the 8-byte-record writer) dispatched from the type-0x20 branch of Render::fieldObjectsRender. That producer draws world-anchored scaled sprites from an effect node's own state. If the stun-star effect is another type-0x20 node whose render fn is one of that family's emitters, it may need only a dispatch entry rather than a new producer. CHECK THAT FIRST — `debug otattr` while a monster is stunned will attribute the star packets to their emitting fn AND caller; compare that fn against the ones fx_sprite/fx_dust/fx_vortex already handle. Note the vortex (#44) turned out to be a FOURTH emitter family in that same type-0x20 branch, so a fifth is plausible.

REPRO: needs a stunned enemy. Stun comes from hitting a monster with the weapon (see #15's weapon-impact work, which used replays/bugs/weapon-impact-bucket.pad to land a swing). No existing replay is known to stun an enemy — the seaside starting route has no reachable enemy (established during #22: walking left dead-ends at the cliff, right at the bucket obstacle). So this likely needs EITHER a warp to an area with enemies (settled recipe: newgame; run 3000; warp N; run 600 WITH PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad — note `run`, not `skip`, per the #43/#44 agent) OR a fresh capture cut from the user's live session with padrec save, the way house-on-the-point.pad was.

VERIFY the psx reference from a SECOND run with PSXPORT_RENDER_PSX=1 at the same frame index — never a bare mid-scene `renderpsx` toggle (kanban #41: it is honoured per scene entry and silently returns a pc frame).

PORT IT, DO NOT TAP IT (USER directive): read the effect node's own state, project with projComposeCamera(sceneCam) so it lerps at fps60, emit drawWorldQuad with has_xyf=1. game/render/fx_sprite.cpp and fx_dust.cpp are the templates.
