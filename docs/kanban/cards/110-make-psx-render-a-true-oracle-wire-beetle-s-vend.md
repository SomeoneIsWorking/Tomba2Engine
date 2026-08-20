---
id: 110
title: Make psx_render a TRUE oracle: wire beetle's vendored GPU rasterizer (not PsyCross)
status: todo
labels: [render, oracle]
created: 2026-08-20
updated: 2026-08-20
---

USER 2026-08-20: 'Try to setup a true oracle like maybe https://github.com/OpenDriver2/PsyCross works better ... Because there are other oracle issues too ... PsyCross or beetle'. Prompted by the item menu rendering TOO DARK on the guest path while pc_render is correct (screenshot: scratch/screenshots/live/menu_dark_now.png, live session, PSXPORT_RENDER_PATH=gte, 1100 textured + 148 semi tris). Also: kanban #106 (the bogus strip below the cutscene bars, fixed) was likewise an 'oracle' bug that was really OURS.

THE PREMISE IS CORRECT AND THE REASON IS CONCRETE. The 'oracle' is not a verified reference at all — psx_render's rasterizer is OUR OWN code. Meanwhile the REAL, widely-validated PSX GPU is ALREADY VENDORED IN THIS TREE AND IS NOT COMPILED:

    external/psxport/vendor/beetle-psx/mednafen/psx/gpu.c            96,159 B   NOT built
                                                    gpu_polygon.c    68,852 B   NOT built
                                                    gpu_polygon_sub.c 13,609 B  NOT built
                                                    gpu_sprite.c     28,753 B   NOT built
                                                    gpu_line.c       14,087 B   NOT built
                                                    gpu_common.h     49,842 B   NOT built

while external/psxport/cmake/psxport.cmake:103-109 DOES compile beetle's gte.c, mdec.c and spu.c, each behind a thin adapter (runtime/recomp/gte_beetle.cpp, mdec_beetle.c, spu_beetle.cpp). So the pattern for 'use beetle's implementation of a PSX subsystem' is already established three times over; the GPU is the one subsystem where we kept our own and then called it the oracle.

That is exactly the class of error CLAUDE.md warns about at the architecture level ('core B IS NOT AN INDEPENDENT ORACLE ... both cores are OUR code, so a shared wrong assumption reads as SUCCESS'). A too-dark menu is what a wrong texture-modulation or semi-transparency rule looks like, and those are precisely the rules beetle's gpu_polygon.c/gpu_sprite.c implement bit-exactly (PSX modulates as texel*colour/128, and has four distinct semi-transparency modes) and that a hand-written rasterizer gets subtly wrong.

RECOMMENDATION: BEETLE, NOT PSYCROSS — and the difference is what each one IS, not how good it is.
  * beetle-psx is an EMULATOR. Its GPU consumes real GP0/GP1 command words and produces VRAM, which is exactly the interface psx_render already has. It is validated across the whole PSX library, it is already vendored here as a committed GPL-2 fork, and three of its subsystems are already wired the same way.
  * PsyCross is a REIMPLEMENTATION OF SONY'S SDK (libgpu/libgte/libgs) for PC, from the OpenDriver2 project, rendering through OpenGL. It is not an emulator and does not execute PSX code — you call it as a C library. Two consequences: (a) it is a THIRD reimplementation of the same spec, validated principally against one game (Driver 2), so using it as an oracle means trusting someone else's approximation instead of our own — the shared-wrong-assumption problem moved, not solved; (b) it would not slot in, because our substrate reaches the GPU through RECOMPILED MIPS writing GP0/GP1, not through libgpu C calls, so there is no seam for it. Its OpenGL rasterizer also cannot answer 'what does the hardware put in VRAM'.

PLAN, following the gte/mdec/spu precedent exactly:
  1. Add beetle's gpu*.c to psxport.cmake beside gte.c/mdec.c/spu.c.
  2. Write runtime/recomp/gpu_beetle.cpp, the adapter, mirroring gte_beetle.cpp's shape: feed it the GP0/GP1 word stream psx_render already decodes, and read back its VRAM as the presented surface.
  3. Make it the psx_render backend. KEEP the existing software rasterizer reachable, because the FIRST use of the new one is to diff the two and find out which of our reported 'oracle bugs' were ours all along.
  4. GATE IT ON THE OTHER ANSWER before trusting it (CLAUDE.md: an instrument is trusted only once it has shown the other answer): re-run this menu. If beetle's GPU renders the panel at full brightness and ours renders it dark, the instrument is live AND this bug is diagnosed in one step. If both are dark, the darkness is in the guest's own packets and the menu bug is somewhere else entirely.

WATCH OUT — PGXP and the parallel-psx renderer also live in that vendor tree; take the SOFTWARE rasterizer path only. And note the standing workspace lesson: a beetle GTE commit once broke PSXPORT_ORACLE=1 in every 3D scene for two days. Pin what is verified and re-verify the ports that consume it.

RELATED: external/psxport/docs/plans/oracle-against-beetle.md already exists and plans a FULL beetle oracle (whole-machine lockstep), whose hard part is that we HLE the BIOS and beetle executes it. THIS card is the much smaller, independent step: just the GPU, at a seam that already exists, with no BIOS problem at all. Do this one first — it settles the render-side 'oracle bugs' without waiting on the whole-machine plan.

**2026-08-20:** **2026-08-20 — PsyCross EVALUATED ON THE ACTUAL SOURCE, as the USER asked. Verdict: not usable as the oracle, but useful for something else.**

Cloned github.com/OpenDriver2/PsyCross (MIT, 1.2 MB) to scratch/eval/PsyCross and read it rather than arguing from the README.

ONE THING I SAID EARLIER WAS WRONG, and it is worth correcting: I claimed there is no seam for it because our substrate reaches the GPU through recompiled MIPS rather than libgpu C calls. PsyCross DOES have a packet-level seam — src/gpu/PsyX_GPU.cpp carries ParsePrimitivesLinkedList(u_long* p, int singlePrimitive) and ParsePrimitive(P_TAG*), i.e. it walks an ORDERING TABLE of P_TAG packets, which is exactly the structure our guest builds and our own drawOTag walks. Handing it our OT would work.

WHAT STILL RULES IT OUT, and this is the part that decides it:
  * ONE BACKEND, AND IT IS OPENGL. src/render/ contains exactly glad.c and PsyX_render.cpp. A search for a software rasterizer across the whole tree returns NOTHING. VRAM is a GL TEXTURE (g_vramTexture, g_vramTexturesDouble[2], g_glVRAMFramebuffer), not a 16-bit CPU array.
  * So the question this oracle exists to answer — 'given these commands, what does the hardware put in VRAM?' — it cannot answer natively. You would read back a GL texture and get OPENGL's rasterization: its fill rules, its interpolation precision, its texture sampling, without PSX dithering or the 15-bit colour semantics unless separately emulated. That is an approximation of the hardware, not a reference for it.
  * It also needs a live GL context (plus SDL2 + OpenAL-soft as build deps). Our measurement runs are headless by default; requiring a GPU context to consult the oracle makes it unavailable exactly where it is most used.
  * And it is an SDK COMPATIBILITY layer ('95% compatibility'), developed against Driver 2. Adopting it as the reference means trusting a third reimplementation validated against one game — the shared-wrong-assumption problem relocated, not solved.

BEETLE, by contrast, is a bit-exact software rasterizer producing a real uint16 VRAM array with no GL context, is already vendored in this tree, and is now wired (psxport 7dbee607).

WHERE PSYCROSS IS GENUINELY WORTH HAVING, and it is not nothing — it is MIT, so unlike the AGPL mmx4 decomp there is no lifting restriction:
  * As a READABLE REFERENCE FOR SDK SEMANTICS when RE'ing our own producers: what DrawOTag / DrawPrim / PutDrawEnv are supposed to do, in plain C, is often faster to consult than re-deriving from disassembly.
  * Its PGXP-Z work ('Precise GTE Vertex Cache with modern 3D hardware perspective transform and Z-buffer support') addresses the SAME problem as our PGXP-lite subpixel cache and native per-vertex depth. That is a directly comparable prior solution to a problem we are actively working on.
Keep the clone under scratch/eval for reading. Do not vendor it.

**2026-08-20:** **2026-08-20 — WIRED AND CALIBRATING. psxport 7dbee607 + 35a8650f.**

The beetle GPU oracle is built, linked, running and instrumented. Enable with PSXPORT_GPU_BEETLE=1; `debug gpubeetle` reports the per-frame comparison; PSXPORT_GPU_BEETLE_DUMP=<frame> writes BOTH VRAMs as .ppm.

THREE BUGS, EACH OF WHICH PRODUCED A SILENTLY BLACK ORACLE, all caught by the trust gate rather than by reading code — which is the whole argument for building the gate first:
  1. GP0 words do not draw. They queue into a 0x20-deep FIFO that drains only inside GPU_Update. 7,145,349 words were fed without ever calling Update, so everything past the first 32 was dropped.
  2. GPU_Update clamps its draw budget to 2*EventCycles, and mdec_beetle.c legitimately pins that SHARED global to 0x7FFFFFFF — which overflows int32 to -2, clamping draw time negative forever. The adapter now lends a sane horizon for the call and hands the global back.
  3. GPU.espec is dereferenced unconditionally by the per-scanline display walk: a null spec is an immediate SIGSEGV (measured at gpu.c:2145). It now gets a real surface + LineWidths sink whose contents we never read.

THEN THE REAL FINDING, and the oracle earned its keep here. The first calibrated comparison showed ours and beetle holding COMPLETELY DIFFERENT VRAM. The tell was a number that could not be explained: at frame 4 our VRAM held 97,540 non-black pixels after FOURTEEN GP0 words — impossible through the command port. Cause: GpuState::gpu_native_load_image writes *vram(...) directly from guest RAM and never touches gpu_gp0, so every native texture/framebuffer upload was invisible to the oracle. Now teed as a real 0xA0 transfer.

    differing pixels  65.87%  ->  3.82%

and the residual is ISOLATED, which is what makes it readable:
    every texture/atlas region (y >= 256)      0.00% differ
    framebuffer at (0,256)                     0.00% differ
    framebuffer at (0,0), the one drawn into   72.19% differ

Exact agreement on all uploaded data AND on one whole framebuffer means the plumbing is right and what is left is RASTERIZATION. Visibly (scratch/screenshots/fb0_ours_vs_beetle.png): beetle draws the text, the separator lines, the button legend and the bottom help panel, but NOT the wood panel background or the item icons. Ours draws all of it.

DO NOT yet read this as 'our rasterizer is right and beetle is wrong'. A third writer of the same shape as gpu_native_load_image would produce exactly this residual, and one such writer has already been found today. The next step is to establish whether the missing primitives were ever FED to beetle: instrument the count of primitives beetle actually rasterizes (it has the command-length table and the FIFO, so it can count what we cannot from the tee side) and compare it against our own prim count for the same frame, with denominators. Only once feeding is proven complete does a difference become a verdict.

WHEN IT IS: re-run the #111 three-way capture. That card is this one's acceptance test.

**2026-08-20:** 2026-08-20 — CALIBRATED AND TRUSTED. psxport 4bdc2ab6 + beetle-psx d52545c0.

The step this card asked for is done, and it changed the answer. The feed was NOT complete, and both losses looked exactly like a rasterizer disagreement.

The census lives INSIDE beetle (vendor/beetle-psx/mednafen/psx/psxport_gpu_census.h) because command boundaries only exist there: the tee side sees a flat word stream, while the per-opcode length table and the FIFO are in gpu.c. Every counter is a denominator or a named loss channel.

Measured at the item menu, f1120, psx_render:
    before   7,077,093 words accepted / 84,335 DROPPED · 117,804 starved · 5,454 unknown opcodes · 10.58% VRAM differing
    after        2,561 words accepted /      0 dropped ·       0 starved ·     0 unknown         ·  0.00% VRAM differing

ONE ROOT CAUSE BEHIND ALL THREE LOSSES. beetle models the GPU draw-time budget so a real CPU can be stalled against a real GPU. The oracle has no CPU to stall, no timing to be accurate about and no scanout being raced, so every unit of budget it declines to spend is pure loss — ProcessFIFO returns early, the FIFO backs up, GPU_WriteCB silently DISCARDS words, and a dropped word shifts every following word, which is what turned vertex data into "unknown opcodes" (last seen 0xFC — a Y coordinate read as a command). psxport_gpu_grant_drawtime() removes the model for the oracle rather than tuning a clock constant around it.

FOUR MORE COUNTERS WERE LYING, all of them mine, all in the direction of "the feed is broken":
  * s_prims is zeroed thirty lines ABOVE the report that reads it, so "our prims" was structurally 0 every frame — in a scene full of geometry.
  * s_frame++ also ran before the report, so the frame label was one late and PSXPORT_GPU_BEETLE_DUMP wrote the PREVIOUS frame's VRAM.
  * a 4-point polygon's continuation packet counted as a second primitive: 195 quads read as 390 polys.
  * opcode 0x00 has an all-NULL rasteriser matrix BY DESIGN, so every frame reported 55 phantom "accepted then not rasterised" losses.

FEED NOW PROVEN COMPLETE at f1120: ours drew 368 prim(s), beetle dispatched 368 (poly 195 + sprite 173, quad-cont 195). dropped 0 · starved 0 · unknown 0 · null-func 0 · fifo queued 0.

THE POSITIVE CONTROL SHIPS WITH THE GATE. PSXPORT_GPU_BEETLE_SELFTEST=1 shifts every primitive beetle draws 1px right — unconditional, unlike a dither or blend perturbation, which depend on the game setting a texture-page bit and could therefore pass by being inert. The verdict line scores every frame and is SKIPPED, audibly, on frames that drew nothing, because a control that cannot bite must not report FAIL. Measured over the item-menu replay: 1,692 PASS, 0 FAIL. At f1120 the control gives 2.32% differing against 0.00% without it.

VERDICT, and it is now a measurement rather than an assertion: at f1120, given the guest's command stream, our psx_render rasterizer writes EXACTLY what real PSX hardware writes. Zero differing pixels out of 524,288.

WHAT THAT DOES TO #111. It moves "the menu is too dark on gte/psx" OFF the rasterizer. VRAM is correct and 61.3% non-black, so the fault is either upstream (the guest path issues different COMMANDS than the native path) or downstream (scanout / display area / presentation). NOT YET DISTINGUISHED — that is #111's next measurement, and the two are told apart by comparing the command stream between paths, not by looking at pixels.

STILL OPEN: the earlier 3.82% residual with its (0,0) framebuffer at 72.19% was measured on the BROKEN feed and must be re-measured before anything is concluded from it. The screenshots scratch/screenshots/fb0_ours_vs_beetle.png date from that run and are void.
