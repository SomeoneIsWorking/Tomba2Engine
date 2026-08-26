# Tomba! 1 RE frontier

Statuses follow the workspace RE-frontier vocabulary. No hacks are present.

## Faithful boot-to-picture spine

### T1-00 — Select and measure the target executable
- status: re-partial
- deps:
- evidence: C001/I001. A local file named `SCUS_942.36` is 559,104 bytes with SHA-1 `81cbc79f0230aeb4252e058039f47ac95a777f5a`; its PS-X header reports entry `0x8006B58C`, text `[0x80010000,0x80098000)`, and initial SP `0x801FFFF0`. The shipping verifier compares 15 facts and its real-data selftest detects an altered byte.
- where: `executable.json`; `tools/verify_executable.py`; private input outside the tracked tree
- gap: The selected disc's `SYSTEM.CNF`, executable path, and extraction have not been verified, so this is not yet a disc-proven title identity.

### T1-01 — Provision the selected disc reproducibly
- status: todo
- deps: T1-00
- evidence: Not started.
- where: future title-local provisioner; user-supplied disc; `scratch/bin/tomba1/`
- gap: Resolve CLI > `PSXPORT_TOMBA1_DISC` > `.env` > one root CHD, verify `SYSTEM.CNF`, extract, and run the shipping identity verifier before publishing bytes.

### T1-02 — Recover crt0 and startup semantics
- status: todo
- deps: T1-01
- evidence: Not started.
- where: future binary-backed verifier and `game/core/` immutable facts
- gap: The PS-X header entry is not semantic evidence for BSS, heap, libc, or game-main ownership. Derive and cross-check the complete startup group.

### T1-03 — Establish a deterministic independent-oracle boundary
- status: todo
- deps: T1-02
- evidence: Not started.
- where: future title harness target using psxport's retained independent reference engine
- gap: Run the selected executable to its first real boundary twice in both engines; prove determinism, full compared-state denominator, a forced mismatch, and a too-short refusal.

### T1-04 — Emit and compare the first generated boundary
- status: todo
- deps: T1-03
- evidence: Not started.
- where: future `titles/tomba1/generated/`, title-local seed manifest, and boundary runner
- gap: Emit from executable-backed roots only, retain the generated body, and compare shipping generated execution with the independent oracle at the measured boundary.

### T1-05 — Boot the actual product to a visible frame
- status: todo
- deps: T1-04
- evidence: Not started.
- where: future `game/app/`, `game/core/`, and `tomba1_port`
- gap: Build and run the real headless product from user-supplied assets, capture visible output, and exercise input. Unit tests or framework smoke alone do not satisfy this step.

## Widescreen spine

### T1-06 — Identify the game's projection owner
- status: todo
- deps: T1-05
- evidence: Not started.
- where: future binary-backed RE and `game/render/`
- gap: Identify the game state and code that submit projection centre/focal length. GTE output, OT nodes, and GP0 packets are diagnostic evidence, not picture inputs.

### T1-07 — Identify visibility and 2D layout owners
- status: todo
- deps: T1-06
- evidence: Not started.
- where: future binary-backed RE and `game/render/`
- gap: Separate visual culling from gameplay activation, and classify left/right/centre-authored interface layout before changing wide behavior.

### T1-08 — Implement and verify true widescreen
- status: todo
- deps: T1-07
- evidence: Not started.
- where: future title-owned projection/layout modules plus psxport's shared non-temporal presentation contract
- gap: Preserve the 4:3 path and prove additional correctly projected content, visibility, edge coverage, and anchored UI on the actual running product.

## Explicitly excluded enhancement work

### T1-X1 — Interpolation/lerp
- status: skip-by-design
- deps:
- evidence: User scope explicitly excludes interpolation/lerp for Tomba! 1; widescreen is its only rendering enhancement.
- where: no title owner
- gap: None. Do not add transform interpolation or intermediate-frame synthesis.

### T1-X2 — Temporal frame history
- status: skip-by-design
- deps:
- evidence: User scope and T1-X1; no interpolation consumer exists.
- where: shared neutral current-frame presentation only
- gap: None. Do not add previous/current capture or presentation-history state.

### T1-X3 — Title-native renderer, producers, or depth
- status: skip-by-design
- deps:
- evidence: User scope excludes the native-renderer program for Tomba! 1.
- where: shared psxport renderer remains the platform owner; no title module
- gap: None. Widescreen must not be used to smuggle in a native-renderer prerequisite.

### T1-X4 — 60fps and native-rendering options
- status: skip-by-design
- deps:
- evidence: User scope requires these unsupported modes to be absent rather than exposed as disabled compatibility options. `enhancement_scope.json` records the contract and the title-isolation test scans source/CMake for option registration tokens.
- where: no title owner and no configuration key
- gap: None. Do not add hidden, experimental, or off-by-default switches.
