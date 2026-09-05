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
| S004 | Native/Lightrec product reaches representative Tomba! 1 gameplay | missing | S003 | G001 |
| S005 | True widescreen works in the running Tomba! 1 product | missing | S004 | G002 |
| S006 | Tomba! 1 remains engine-isolated and exposes only widescreen | verified | — | G001, G002 |
| S007 | The Tomba! 1 offline guest-source product path is removed | verified | — | G001 |

## Current focus

S004 is the title-local next capability but is deferred until Tomba! 2 has completed representative
gameplay through Lightrec. Tomba! 1's guest-source path is already removed. When this title resumes, first reproduce the
verified 35-field CRT0 boundary through Lightrec, then cross issue 0006's internal-`CdSync` boundary
and reach the title/input frontier before the representative-gameplay gate.

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

### S004 — Native/Lightrec gameplay: missing

Recorded pre-migration evidence establishes a useful frontier. Title-local native owners cover the
three-task finite frame transaction, measured VBlank delivery, pad buffers, five reached libcd
entries, DMA callback registration, stream-field cadence, and projection leaves. Real-disc runs
reached coherent SCEA presentation, loaded `OPTSUB00` at `0x800E7388`, advanced the movie stream
beyond LBA 58739, and displayed a clean Whoopee Camp frame.

Issue 0005 records the resolved DMA3 cause: linked `DMACallback` `0x80067E84` registered channel-3
callback `0x80066D80`, which must clear the in-flight guard at `0x8001CA08`. Issue 0006 is the next
known boundary: public `CdSync` `0x800648C8` forwards to internal `0x80065470`, whose timeout
reaches fatal guest VSync with return address `0x800654A4`.

Missing capability: prove the shared image-aware Lightrec executor reproduces S003, routes the six
engine-neutral native overrides plus scoped original calls, crosses issue 0006, reaches the title
screen, demonstrates input, and passes representative interactive gameplay. The product must execute
nonzero Lightrec blocks, invalidate overlay code correctly, and prove by link and selector inspection
that no guest instructions at source or interpreter-first gameplay selector is present. Automatic
fallback must be bounded, reason-counted, and below its declared threshold; its coverage is excluded
from gameplay conformance and performance evidence.

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
