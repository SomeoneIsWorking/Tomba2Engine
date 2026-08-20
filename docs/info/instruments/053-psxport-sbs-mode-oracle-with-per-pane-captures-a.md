---
id: I053
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

PSXPORT_SBS_MODE=oracle with per-pane captures and bounded EXIT_FRAME

## Validated by

OTHER ANSWER before per-Game service initialization: 6 boot RAM diffs plus repeated
CdlSync/CdlSetloc/CdlPause timeouts and no pane capture for 472s. SAME ANSWER after repair: boot
RAM+scratchpad identical, both legs controllable, same game-owned area-4 cold warp f300, pane pairs
written, and clean bounded exit. A second positive control caught a false banner: applying B's PSX
path before `dc_boot_init` left it Native; post-boot application plus a `softGpu()` refusal makes that
state observable. A third OTHER ANSWER caught the real software pane at 0/76,800 non-black because
readback cleared its empty native batch; reusing `vram_backdrop_is_picture(cfg backdrop, sw_path)`
produces 76,800/76,800 non-black at f560. On the health wheel, that B pane falsified the shader-only
diagnosis: translating the native producer's complete AddPrim-LIFO order improved exact agreement on
opaque red-gradient CLUT pixels from 0/340 to 293/340. The remaining 47 are a localized, disclosed
sprite-raster residual, so the instrument demonstrated both answers rather than merely going uniform.
After the independent production shader gate corrected fragment-centre UV phase, fresh f560/f561
captures reach 340/340 exact palette-mask pixels at both frames.

## Known failure modes

B independently executes guest bodies but both legs share psxport hardware models; this is not an
external console/emulator oracle. A pane pair is also not automatically same-state: the area-4 run
executes only 248/484 owned addresses and reports post-warp RAM/scratchpad differences, including a
one-tick field-state skew. Attribute pixel differences only after checking those denominators and
visible dynamic content (B has guest snow that A's current native producers omit).
