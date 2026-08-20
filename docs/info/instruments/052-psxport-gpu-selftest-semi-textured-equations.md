---
id: I052
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`PSXPORT_GPU_SELFTEST=1` production semi-texture shader/pipeline gate in `GpuVkState::tritest`

## Validated by

OTHER ANSWER: before the ABR0 destination-factor correction, the shipping SPIR-V path passed 14/16
cases and failed both ABR0/STP1 cases (`bg=1882: got 214C, expected 150B`; `bg=4298: got 4B5F,
expected 2A16`). SAME ANSWER after the correction: 16/16 cases pass across ABR0--3 x dark/bright
destination x STP1/STP0. The gate uses the production `draw_semi` queue, generated SPIR-V, render pass,
fixed-function blending, encode pass, and VRAM readback; its CPU side supplies only the PSX 5-bit
reference equations and compares the packed result.

## Known failure modes

This instrument validates the shared GPU's textured semi-blend semantics only. It cannot prove that
the guest or a native producer submitted the correct packet, nor can it serve as a whole-machine or
lockstep game oracle. It requires a working host Vulkan device and a rebuilt shader header after a
GLSL edit.
