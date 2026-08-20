---
id: 119
title: SBS oracle booted the wrong render path and cleared software VRAM during pane readback
status: done
labels: [oracle, tooling]
created: 2026-08-20
updated: 2026-08-21
---

The initial 472-second failure was per-Game service initialization, not an interpreter limitation:
the two SBS cores shared process-global CD/platform-HLE state. Per-Game services now boot both cores
with byte-identical RAM and scratchpad and let the interpreter leg reach the same bounded area-4
cold warp.

Two more false-oracle defects were exposed by the health-wheel comparison:

- `M_ORACLE` assigned B `RenderPath::Psx` before `dc_boot_init`, but boot reapplied the process-wide
  configured path (`Native`). The harness banner claimed `interp+softGPU` while B actually used the
  native renderer. Mode application now happens after both boots and on every step, and oracle mode
  refuses unless B reports `softGpu()`.
- Once B really used the software rasterizer, its pane was 0/76,800 non-black. The software GPU had
  drawn the frame into CPU VRAM, but `gpu_vk_render_readback` called `render_geom` with the default
  empty-batch policy, clearing the just-uploaded picture. The readback now reuses the shipping
  `vram_backdrop_is_picture` policy with both inputs: `GameConfig::preserveVramBackdrop` and
  `GpuState::sw_path()`.

Final bounded evidence: boot RAM+scratchpad identical, game-owned area-4 warp at f300, A/B panes at
f560, clean f562 exit, and software B 76,800/76,800 non-black with an empty native geometry batch.
The harness also reports its limit honestly: only 248/484 owned addresses executed in this run, and
post-warp game state is not byte-identical (including a one-tick field-state skew). A visual A/B diff
therefore identifies rendering leads but is not automatically a same-state pixel proof.
