# Tomba! 1 project state

This is the title-local factual capability inventory. It does not inherit any capability from Tomba! 2.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Selected executable and disc provenance are established | partial | — | G001 |
| S002 | Title evidence gates are integrated in the repository build | verified | S001 | G001 |
| S003 | Deterministic independent-oracle execution boundary exists | missing | S001, S002 | G001 |
| S004 | Actual Tomba! 1 product boots, renders, accepts input, and reaches gameplay | missing | S003 | G001 |
| S005 | True widescreen works in the running Tomba! 1 product | missing | S004 | G002 |
| S006 | Tomba! 1 remains engine-isolated and exposes only widescreen | verified | — | G001, G002 |

## Current focus

S001 is the current focus: prove selected-disc `SYSTEM.CNF` provenance and reproducible executable
extraction before deriving startup or publishing a runtime.

## Capability details

### S001 — Executable and disc provenance: partial

C001/I001 establish that one measured local `SCUS_942.36` agrees with 15 tracked filename, size,
SHA-1, region, and PS-X header facts; the shipping verifier rejects altered bytes and fields.

Gap: issue 1 records that the selected disc's `SYSTEM.CNF` and reproducible executable extraction are
not yet proven.

### S002 — Integrated evidence gates: verified

Evidence: the repository root includes `cmake/tomba1_port.cmake`; its scaffold depends on the real
shared `psxport` library, and the combined Clang CTest graph runs the identity selftest plus
positive/negative title-isolation checks without publishing a fake product.

### S003 — Independent execution boundary: missing

Missing capability: no measured crt0 contract, deterministic reference boundary, compared-state
denominator, forced mismatch, or generated execution boundary exists.

### S004 — Actual product: missing

Missing capability: no `tomba1_port` target, title runtime, launcher, visible product frame, or
input-driven gameplay exists.

### S005 — True widescreen: missing

Missing capability: no executable-grounded projection, visibility-culling, edge-coverage, or 2D
layout owner has been recovered or exercised in the running title.

### S006 — Engine and enhancement isolation: verified

Evidence: all title owners live below `titles/tomba1/`; the CMake fragment compiles no root `game/` or
`generated/` code; `enhancement_scope.json` contains only `{"widescreen": true}`; and the shipping
isolation check rejects cross-title imports, added unsupported modes, and source files above the
1,200-line boundary with positive and negative controls.
