# Tomba! 1 project state

## Comparison baseline

The external baseline is the USA PlayStation release `SCUS_942.36` on original hardware or a
trusted emulator. The immediate migration baseline is the recorded pre-migration native/offline-translated
hybrid. The intended product keeps title-native ownership and executes all remaining guest code
through `psxport` Lightrec.

This inventory is title-local and inherits no capability from Tomba! 2.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Selected executable and disc provenance are established | verified | — | G001 |
| S002 | Title identity and engine-isolation evidence exists | verified | S001 | G001 |
| S003 | The 35-field CRT0 boundary is independently established | verified | S001, S002 | G001 |
| S004 | Native/Lightrec product reaches representative Tomba! 1 gameplay | blocked | S003 | G001 |
| S005 | True widescreen works in the running Tomba! 1 product | missing | S004 | G002 |
| S006 | Tomba! 1 remains engine-isolated and exposes only widescreen | verified | — | G001, G002 |
| S007 | The Tomba! 1 offline guest-source product path is removed | verified | — | G001 |

## Current focus

S004 is the title-local focus and is **blocked on a framework seam, not on title work**. The boot
decompress that used to abort the product is fixed (issue 0007, measured 400 fields, exit 0, 0
fallback). The product now stops because the guest streams its own code module and nothing publishes
an image identity for it, and because the `CdRead` route does not invalidate Lightrec (issue 0008).
Both are `psxport` work. Once that seam exists, this title's own next steps are the guest module table
and the `0x8001EFE8` ring producer.

## Capability details

### S001 — Executable and disc provenance: verified

Evidence: C001–C003 and I001–I002 establish one selected USA disc whose root `SYSTEM.CNF` names
`SCUS_942.36`. The executable is 559,104 bytes with SHA-1
`81cbc79f0230aeb4252e058039f47ac95a777f5a`; its PS-X header reports entry `0x8006B58C`,
text `[0x80010000,0x80098000)`, and initial SP `0x801FFFF0`. The verifier compares 15 facts,
and altered bytes, malformed/multiple boot records, wrong targets, ambiguous input, and failed
publication all produce the opposite answer.

### S002 — Identity and isolation evidence: verified

Evidence: the title-local checks exercise executable/disc identity and reject Tomba! 2 source or address
leakage plus unsupported enhancement registrations. All Tomba! 1 owners live below this subtree and
the composition imports no root `game/` source.

This evidence remains valid input to the migration. Cold generation and guest-source product checks are
not continued as current gates.

### S003 — Independent CRT0 boundary: verified

Evidence: C004/I003 records two deterministic independent-oracle runs that agree on 35/35 target/PC/register
fields at the first `A(39h)` call after 42,140 steps. A forced `gp` mutation and a 100-step run
both produce the opposite answer.

The grounded startup facts are BSS `[0x8009AFB0,0x800A3348)`, SP `0x801FFFF8`, heap base
`0x800A3348`, heap size `0x15C8B0`, gp `0x80097FA8`, `A(39h)` wrapper `0x8006B70C`,
and game main `0x800163B0`. The Lightrec product must reproduce this boundary; no address-based
workflow is part of that future proof.

### S004 — Native/Lightrec gameplay: blocked

Recorded pre-migration evidence establishes a useful frontier. Title-local native owners cover the
three-task finite frame transaction, measured VBlank delivery, pad buffers, five reached libcd
entries, DMA callback registration, stream-field cadence, and projection leaves. Real-disc runs
reached coherent SCEA presentation, loaded `OPTSUB00` at `0x800E7388`, advanced the movie stream
beyond LBA 58739, and displayed a clean Whoopee Camp frame.

Issue 0005 records the resolved DMA3 cause: linked `DMACallback` `0x80067E84` registered channel-3
callback `0x80066D80`, which must clear the in-flight guard at `0x8001CA08`. Issue 0006 is the next
known boundary: public `CdSync` `0x800648C8` forwards to internal `0x80065470`, whose timeout
reaches fatal guest VSync with return address `0x800654A4`.

**The boot decompress is fixed (issue 0007) and the product is now stopped by a framework boundary
(issue 0008), not by a title defect.** `Tomba1FrameDriver` used to abort on
`ExecutionExitReason::BudgetExhausted`; the guest function at the budget was `0x8003EF50`, a bit-driven
LZ77 glyph decompress emitting 286,720 bytes that reached only 45,200 inside one field's 564,480-cycle
budget, so the call needs at least seven display fields. A bounded, reason-coded resume across fields
replaced the abort.

Measured on the shipping product after that fix: **400 fields, exit 0, 1,713,248 Lightrec blocks and
11,422,254 guest instructions across 3,624 executor calls with 0 fallback blocks.** The run then
faults at `0x800E7D5C` with `image identity lookup: claimed by none`.

**The exact gap, and why it is not the title's to close.** The guest streams its own code module
(`CdRead 7 sector(s) x 2048 from LBA 103311 -> 0x800E7388`, 14,336 bytes) and calls into it by a
direct `jal` from resident code at `0x80019A7C`; the call target is at offset `0x9D4` inside that
module. `ImageCatalog` has exactly two production `activate()` call sites in the whole tree — the
main-executable loader, and **Tomba! 2's own** scene code — and Tomba! 1 has none, because it has no
native scene. The same read also does not invalidate: `cd_read_stock_sync` calls
`notifyExecutableWrite` **0** times, and **2 of 2** call sites in `cd_override.cpp` are in
`cd_loadfile`, the route this title does not use. So this is a missing framework seam, and fixing it
here would mean a second owner of the invalidation rule.

**A sufficiency measurement, because "one fix finishes it" is the claim that gets made.** Publishing
the module identity experimentally (scratch build, measured, reverted) advances the title past the
fault and onto a **different** frontier: guest task 1 in a retry loop at `0x8001F2FC` calling
`0x8001EFE8`, whose retry counter reads **1** after 33 fields while the ring it drains
(`base 0x800D7188`, `index 0`, first halfword `0`) is empty. So the identity gap is enough to expose
the next blocker and is **not** enough to reach gameplay.

Live channel evidence, and an explicit non-claim: `titles/tomba1/tools/tomba1_live.py` presents
real frames (the count this connection saw ranged over 1, 6 and 15 across runs), accepts a Start tap,
and reads guest state back — and the captured screenshot, opened and measured, is 320x224 with
**0 of 71,680 pixels non-black**. Every presented frame is black. That is not a picture and not
gameplay.

Missing capability: the framework must let a title publish an image identity for a guest-streamed
module and must invalidate on the `CdRead` route; then this title must recover the guest's own module
table, and then the `0x8001EFE8` ring producer must be found. No gameplay claim is made and none is
available: the product presents black frames and stops after boot.

### S005 — True widescreen: missing

Grounded projection facts are preserved: `SetGeomOffset` is `0x80063A34`,
`SetGeomScreen` is `0x80063A54`, initialization `0x80016AF4` publishes centre
`(160,112)` and `H=544`, display construction at `0x80016C4C` uses 320x224 rectangles, and
only resident `0x8002D784` later reasserts `H`.

Missing capability: after S004, identify loaded-code projection contributions, visual-versus-gameplay
culling, wide draw-buffer placement, and authored 2D anchors; then prove additional correctly
projected content against a controlled 4:3 run.

### S006 — Engine and enhancement isolation: verified

Evidence: title owners live below `titles/tomba1/`; the title composition imports no root `game/`
source; `enhancement_scope.json` contains only `{"widescreen": true}`; and the isolation check
rejects cross-title imports, unsupported modes, and source files above the 1,200-line boundary with
positive and negative controls.

### S007 — Guest-source-path retirement: verified

Evidence: the title generator, emitted guest source, emission-only seeds, offline registry/build rules,
address-based tests, and generation-only selftest are absent. `tools/run.py` retains only
authenticated disc provisioning and the native/Lightrec product build. There is no offline-produced
fallback; only the shared bounded runtime fallback described in the root migration authority is
permitted.
