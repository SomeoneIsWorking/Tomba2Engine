---
id: 22
title: Health wheel native blend and AddPrim order differ from the PSX
status: done
labels: [render]
created: 2026-07-22
updated: 2026-08-21
evidence:
  - docs/reference/issues/issue22_health_wheel_reference.png
  - docs/reference/issues/issue22_health_wheel_reference_dark.png
  - scratch/screenshots/health_wheel_probe.png
  - scratch/screenshots/health_wheel_after.png
  - scratch/screenshots/oracle_health_fullorderfix_f560_A.ppm
  - scratch/screenshots/oracle_health_fullorderfix_f560_B.ppm
  - scratch/screenshots/oracle_health_uvphase_final_f560_A.ppm
  - scratch/screenshots/oracle_health_uvphase_final_f560_B.ppm
  - scratch/screenshots/oracle_health_uvphase_final_f561_A.ppm
  - scratch/screenshots/oracle_health_uvphase_final_f561_B.ppm
---

USER 2026-08-21: "Please do oracle compare for Tomba! 2 health indicator"

USER 2026-08-21: "True oracle, not the oracle renderer"

USER 2026-08-21: "Yes we don't have lockstep oracle, just find out why our port draws it wrong"

## Resolution

The health wheel was washed out because psxport's Vulkan semi-texture shader implemented ABR0 as
`F/2+B`, not the PSX equation `(F+B)>>1`. `trisemi_hw.frag` emitted a half-strength source but alpha
1, while its fixed-function pipeline uses source alpha as the destination coefficient. The shared
shader now emits destination coefficient 0.5 for ABR0/STP=1, 0 for opaque STP=0 texels, and 1 for
ABR1--3. The 5-bit source and integer AVG/ADD_FOURTH rules are quantized in that same shipping path.
No HUD-specific override, opaque backing, or guessed positioning adjustment was added.

The repaired true interpreter/software-GPU B pane then falsified the claim that blend math was the
whole defect. The native producer emitted `fieldHudItemRing` groups in guest call order, but the guest
helpers use `AddPrim`, which prepends every packet to one OT bucket; the final PSX draw order is the
reverse. `RenderQueue` appends and preserves submission order. The producer now submits its entire
group sequence in final guest draw order: fixed chrome calls reversed, both item-loop families
reversed, and each loop index reversed. Reversing only the three chrome calls fixed the centre numeral
but left the red wedges beneath the translucent halves, which positively controlled the full-function
fix.

The final 47-pixel lower-right residual was not producer geometry, UV, material, or another ordering
fault. A raw guest GP0 capture and the native queue contain the same winning FT4 byte-for-byte:
`xy={(39,47),(55,47),(39,31),(55,31)}`, `uv={(56,15),(72,15),(56,31),(72,31)}`, CLUT `(496,203)`,
tpage `0x0006`. The Vulkan shaders instead truncated a UV interpolated at fragment centres, whereas
the PSX affine rasterizer evaluates at native integer pixels. Decreasing slopes therefore selected
the preceding texel. The shared opaque, semi, and semi-cover shader paths now reconstruct the
integer-pixel UV at 1x and internal resolutions, then snap it to the PSX rasterizer's
12-fractional-bit grid before texel selection. No game-specific UV adjustment was added.

The retained bright and dark real-game references prove the wheel is intentionally
background-modulated; making it opaque would be wrong.

## Reproduction and packet evidence

`newgame; run 300; warp 4; run 600` presents the wheel around f914/f915. The actual owner is
`Render::fieldHudItemRing` (`0x80025934`): 3,159 primitives over 351 sampled frames, exactly nine per
frame. The two large halves are raw, semi-transparent ABR0 textured quads using texture page
`(384,0)` and CLUT `(496,203)`. That CLUT mixes STP and non-STP entries, so the same primitive must
blend blue/green texels and overwrite its red gradient texels opaquely.

The authored halves span x=8..32 and x=31..55. Their one-column overlap explains the visible centre
seam, which is also present in the real-game reference; it is not the washout cause.

## Falsifiers

- The production SPIR-V selftest renders through the shipping shader and fixed-function pipelines.
  Before the fix it passed 14/16 cases, failing only ABR0/STP=1 over both dark and bright
  destinations. After the fix it passes 16/16: ABR0--3 × dark/bright × STP1/STP0.
- In the live bright scene, 1,612/2,352 wheel-crop pixels changed. Mean RGB changed from
  `(155.03,180.10,217.05)` to `(95.68,125.41,189.58)`, removing the pale background wash.
- In the bounded true-SBS area-4 capture at f560, software B is 76,800/76,800 non-black. Before the
  AddPrim-order correction, native A mismatched all 340/340 pixels whose B value is exactly one of the
  wheel CLUT's nine opaque red-gradient entries. The full-function reversal reduces that to 47/340;
  the other 293 agree exactly. The full 56x42 crop changes 565/2,352 pixels from the pre-order A.
- Before the shared UV-phase correction, the residual is stable at f560/f561: 47/340 pixels in
  x=40..52,y=33..45. The production shipping-path phase test initially passes only 2/5 at 1x:
  positive X/Y pass while negative X/Y and a mixed non-unit slope fail. It now passes 20/20 across
  1x/3x, opaque/semi, positive/negative X/Y, and mixed non-unit slopes; the established 16/16
  semi-equation matrix remains green.
- Final true-SBS captures `oracle_health_uvphase_final_f{560,561}_{A,B}.ppm` are exact on the same
  nine-word wheel palette mask: **0/340 differing pixels at f560 and 0/340 at f561**.
- Falsify the landed parts if the semi pipeline factors, `trisemi_hw.frag`, 5-bit output encoder,
  shared `psx_uv.glsl` helper, captured wheel packet/CLUT material, or `fieldHudItemRing`'s AddPrim
  call sequence changes.

The GPU-only Beetle tee is not called a true oracle here: it consumes the port's GP0 stream and
cannot detect upstream state or packet-generation faults. Card #119 records the repaired
interpreter/software-GPU oracle path and its limits.
