---
id: 49
title: area 21 early-phase sky gradient was absent
status: done
labels: [render]
created: 2026-07-23
updated: 2026-08-22
---

**2026-08-22:** Fixed for the reached early phase. Generated `ov_a0l_gen_8010BE30` plus live state
(`bgstate=21`, `variant=1`, `phase=1`) show this branch calls `0x8010BB64` and returns; it does not
continue into the tilemap loop. The helper emits four gouraud `POLY_G` bands across x[0,320], with
guest colour words `0x00AC0606`, `0x00EA9898`, and `0x00390000`; their Y origin is derived from raw
signed camera pitch at `0x1F8000F0`.

`Render::area21SkyGradientRender` rebuilds those bands at `RQ_BACKGROUND`, with a game-owned temporal
pitch capture so both 60 Hz presents use the same producer. Same-binary `native area21-sky` ON/OFF
changed 53,907/76,800 pixels (53,842 >8/255); both legs advanced 3,601 frames and exited 0. Census:
2,256 native prims/282 frames, while an oracle run independently observes four guest prims/frame.
Captures: `scratch/screenshots/area21_sky_{on,off}.png` and `area21_sky_off_on_diff.png`.

Fresh 2026-08-24 visual gate, one binary (`fec4e570…`, build id
`edaa13a-dirty+psxport-d2266f4b`): native ON, native OFF, and the boot-time PSX-render reference all
reach frame 3615 with identical guest state (`bg=21`, `variant=1`, `phase=1`, pitch `-175`). ON/OFF
again changes 53,907/76,800 pixels (53,842 above 8/255). The native ON picture is coherent and close
to the aligned PSX reference; OFF loses the background. Native ON vs PSX reference still differs by
26,853 pixels (20,094 above 8/255), so this is **draw-verified, not pixel parity**. The independent
`PSXPORT_ORACLE=1` path enters GAME 11 frames later; its phase-1 capture is therefore useful only as an
unaligned visual reference, not a parity comparison.

Fresh captures and logs:

- `scratch/screenshots/area21_fresh_native_{on,off}_f3615.png`
- `scratch/screenshots/area21_fresh_psx_reference.png`
- `scratch/screenshots/area21_fresh_native_off_on_diff.png`
- `scratch/screenshots/area21_fresh_native_vs_psx_reference_diff.png`
- `scratch/screenshots/area21_fresh_oracle_{exact_recipe,settled20}.png`
- `scratch/logs/area21_fresh_{native_on,native_off,psx_reference,oracle}_20260824.log`

The tilemap loop belongs to another variant/phase branch. It remains excluded and unclaimed until a
capture visibly reaches it. Re-admitting the tilemap into this early-phase repro was the measured dead
end: it worsened the frame from 61,375 to 64,650 pixels above 8/255.
