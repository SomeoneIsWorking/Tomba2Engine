# Tomba! 2 native/dynarec RE frontier

This is the ordered execution-evidence chain for the active title. It preserves verified behavioral
and address facts while replacing the removed offline-generated product with runtime Lightrec.
Tomba! 1 has its own frontier under `titles/tomba1/docs/re-frontier.md` and remains deferred until
Tomba! 2 representative gameplay is complete.

## Execution spine

### T2-00 — Preserve authenticated title and overlay identity
- status: re-partial
- deps:
- evidence: Existing binary/run evidence identifies `SCUS_944.54` and the recorded resident/overlay behaviors. C044 records an independently checked MAIN.EXE digest; address-collision evidence identifies `0x801113B4` in A03 and A0B with different entry shapes. The current provisioner checks sizes only.
- where: title provisioning metadata; binary-backed findings; user-supplied disc outside Git
- gap: Enforce exact-content identity in provisioning and product loading; historical binary evidence and runtime generation tokens are not an authenticated input manifest. See issue 0005.

### T2-01 — Execute authenticated images through psxport Lightrec
- status: re-partial
- deps: T2-00, T2-06
- evidence: The shipping Lightrec product completes a real-image native startup frame with nonzero translated execution and zero fallback; exact counters and the reached second-frame failure are recorded in project-state S001 and issue 0007. Linked products pass the execution-boundary discriminator.
- where: shared `external/psxport/` executor; title composition in `cmake/tomba2_port.cmake`
- gap: Complete authenticated overlay activation and representative gameplay, with measured invalidation and bounded fallback accounting; startup is not conformance. Diagnostic interpreter mode is not a product selector.

### T2-02 — Prove a resident native override and original call
- status: in-progress
- deps: T2-01
- evidence: Production catalog tests exercise resident generation binding, scoped original calls, and a different-image collision negative through Lightrec. Real startup reaches native owners; real resident original-call comparison remains open in issue 0005.
- where: title-native owner in `game/`; shared image-aware override/original-call dispatcher in `external/psxport/`
- gap: Exercise native dispatch, a scoped original guest-body call through Lightrec, normal return, and wrong-address/disabled-override negative cases.

### T2-03 — Prove colliding-overlay override and original calls
- status: todo
- deps: T2-02
- evidence: Binary evidence establishes that numeric address `0x801113B4` belongs to different entry shapes in A03 and A0B; address-only dispatch is therefore invalid.
- where: overlay-authentication owner and shared runtime dispatcher; title-owned override selected by complete image identity
- gap: Exercise the selected address under both authenticated overlays, show that only the matching native owner runs, call each original body through Lightrec, and prove a mismatched image cannot reuse the override or translated block.

### T2-04 — Re-establish the current boot-to-free-roam frontier
- status: todo
- deps: T2-03
- evidence: Recorded pre-migration runs reach GAME at frame 25 and free-roam at frame 216, complete 620/620 presentation fences, and preserve the fatal guest-VSync boundary at `0x80085900`.
- where: native title runtime and frame driver; shared Lightrec executor and PSX services
- gap: Reach the same frontier with all native owners active, nonzero Lightrec execution, relevant overlay invalidation, and no generated guest code or interpreter in the gameplay link/selector surfaces.

### T2-05 — Prove representative interactive gameplay
- status: todo
- deps: T2-04
- evidence: Existing boot and bounded free-roam captures are checkpoints, not representative gameplay conformance.
- where: shipping product plus independent emulator/hardware or separately built test oracle; deterministic gameplay scenario
- gap: Cover real player input, guest PC/registers and memory, interrupts/timing, relevant CD/GPU/SPU behavior, native override reachability, and declared frame-time/correctness budgets with positive and negative controls.

### T2-06 — Remove the Tomba! 2 static path
- status: re-verified
- deps: T2-00
- evidence: The generated tree, generator entry point, emission-only seeds, static registry and build inputs, and generated-symbol tests are absent. Provisioning retains authenticated executable and overlay bytes only.
- where: absent by design; enforced by title CMake and provisioning boundaries
- gap: None for removal. Fresh product build/launch evidence belongs to T2-01 through T2-05 and cannot fall back to the retired path.
