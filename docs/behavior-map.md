# Behavior-difference map — every Tomba! 2 INTENTIONAL divergence from recomp_path (managed by tools/behavior.py)

Durable ledger of SANCTIONED deviations from the byte-exact reference. Primary axis = GUEST-MEMORY AFFECT (how much canon guest state a deviation touches). One `## ` block per
deviation, grouped by affect. `tools/behavior.py` = view · `... <words>` = search · `... check` = gate (a canon-affecting change must be SBS-suppressed).

This ledger describes the implemented Tomba! 2 target (`SCUS_944.54`). Tomba! 1 (`SCUS_942.36`) has no implementation entries here:
it already runs at 60 fps, so its future enhancement scope is widescreen only and must not acquire the
`fps60` interpolation path or interpolation-only prerequisites below.

**By affect:** 5 none · 2 non-canon · 2 full
**By status:** 1 verified · 5 implemented · 2 planned · 1 reverted

---

### **affect: none** — pure host-side overlay, writes NO guest memory. Any guest write is a BUG (SBS catches it).

## native-vram-upload
- **class:** pc_render
- **affect:** none
- **status:** implemented
- **original:** FUN_80081218 enqueues the rect into the guest's GsSortObject ring at 0x800A5AC8, DMA'd later as a GP0 0xA0 packet
- **altered:** Asset::uploadImage writes the rect straight into native VRAM and does NOT enqueue; the later ring flush no-ops over an empty ring
- **guard:** Reached ONLY by native callers that call the method directly. 0x80081218 is deliberately NOT in the override registry, so every guest/substrate caller still runs the recompiled body and still performs the ring enqueue — which is what keeps SBS byte-exact. Registering the address would break that by construction.
- **owner:** game/core/asset.cpp Asset::uploadImage
- **notes:** Recorded 2026-07-29 after the address surfaced high on a recdep sweep and looked like an unwired-native free win. It is not one; the banner in asset.cpp and the call site in beh_seaside_prox_substate.cpp both say so now.

## pc-render
- **class:** pc_render
- **affect:** none
- **status:** verified
- **flag:** default (pc_render is the default renderer; PSXPORT_RENDER_PSX=1 selects the substrate psx_render instead)
- **original:** PSX GTE compose + OT walk + GP0 packets draw the picture
- **altered:** native float matrices + real depth buffer draw from scene state in its own pass
- **guard:** READ-ONLY OVERLAY: reads guest RAM + engine classes, writes ONLY host memory. Any guest write is a bug (SBS catches it). DisplayPassGuard enforces the scope.
- **owner:** game/render/*
- **notes:** parity-map n/a class (never writes guest RAM). See docs/render-arch.md, CLAUDE.md "Render — reimplement, dont transcribe".

## widescreen
- **class:** widescreen
- **affect:** none
- **status:** implemented
- **flag:** aspect (psxport_settings.ini / F1 overlay): 0=4:3, 1=16:9, 2=21:9, 3=Auto
- **original:** 4:3 FOV
- **altered:** genuinely wider FOV — engine shifts projection center OFX to nw/2 (not a present stretch); culled edge nodes re-included read-only
- **guard:** forced OFF under PSXPORT_ORACLE and in SBS legs (run 4:3), so the byte-exact reference is untouched
- **owner:** game/render/widescreen_margin_quad.cpp; runtime/recomp/mods.c
- **notes:** guest-READ-ONLY re-include of culled nodes (docs/findings render "Widescreen margin renderer").

## fps60
- **class:** fps60
- **affect:** none
- **status:** implemented
- **flag:** 60fps (psxport_settings.ini / F1 overlay)
- **original:** game renders at its native guest frame rate (~30fps)
- **altered:** interpolated in-between frames on the actor-transform tier, one frame behind
- **guard:** interpolation layer only — no guest re-run, no guest writes; reads dbg_node object identity and lerps host transforms
- **owner:** external/psxport/runtime/recomp/fps60.{h,cpp} (input-lerp re-run: sceneCam/projObj/bgScroll chokes + tier1Render)
- **notes:** verify per-object via preseqobj / fps60chk (docs/findings render); endpoint gate PSXPORT_FPS60_TFORCE=0|1 pixel-diffs interp vs the adjacent real frame.

## ires
- **class:** ires
- **affect:** none
- **status:** implemented
- **flag:** ires (psxport_settings.ini / F1 overlay): 0=Auto,1=1x,2=X2,3=X3,4=X4
- **original:** fixed 1024x512 VRAM render
- **altered:** 3D-world band renders to a VRAM*i target then box-filter downsamples back; 2D always native res
- **guard:** VRAM-space 2D ops + all readback/SBS stay on the fixed texture, untouched; scaled target is host-only
- **owner:** runtime/recomp/gpu_gpu.cpp (render_geom)
- **notes:** internal-resolution scale; ires_downsample.frag coverage-gated (bug #55).

---

### **affect: non-canon** — writes guest memory only to reach the SAME end-state faster. Must byte-match recomp_path at every rendezvous; SBS runs the faithful branch.

## synchronous-loads
- **class:** loading
- **affect:** non-canon
- **status:** implemented
- **flag:** none — this is the product execution policy, not an optional mode
- **original:** FUN_80044BD4 parks a spawned task across fields and, for flag 2, advances the loading counter and calls FUN_8007FD54
- **altered:** native FUN_80044BD4 drains the owned task before returning, preserves the authored flag-dependent RNG stamp, and emits zero wait ticks/loading services
- **guard:** `test_synchronous_task_wait` drives the shipping completion seam for flags 1/2/3; the default live route reached flag 2 and 3 with zero service ticks and reached free roam
- **owner:** psxport `SynchronousTaskWait`; game call sites use `PcScheduler::completeSyncWait`
- **notes:** the generated body remains the explicit substrate oracle. `PSXPORT_PC_SKIP` is retired; there is no second product cadence.

---

### **affect: full** — DELIBERATELY changes canon guest state. MUST be force-suppressed under PSXPORT_ORACLE / SBS (`guard` required) so byte-compares stay clean by construction.

## expanded-load-range
- **class:** pc_enh
- **affect:** full
- **status:** planned
- **flag:** cfg_enh("expanded-load-range") via PSXPORT_ENH=<name,name|all>
- **original:** objects load/unload within the guest engine range window
- **altered:** expanded object load/unload range window (more objects resident)
- **guard:** force-suppressed under PSXPORT_ORACLE / SBS in cfg.c so byte-compares stay enhancement-free
- **owner:** -
- **notes:** PLANNED. Canon guest-state change. Name is the PSXPORT_ENH token; register in docs/config.md when landed.

## faster-transitions
- **class:** pc_enh
- **affect:** full
- **status:** planned
- **flag:** cfg_enh("faster-transitions") via PSXPORT_ENH=<name,name|all>
- **original:** fade / area-transition ramps advance at the guest rate
- **altered:** faster fade/transition ramps
- **guard:** force-suppressed under PSXPORT_ORACLE / SBS in cfg.c so byte-compares stay enhancement-free
- **owner:** -
- **notes:** PLANNED. Canon guest-state change. Register the PSXPORT_ENH token in docs/config.md when landed.
