---
id: I026
kind: instrument
status: trusted
created: 2026-07-29
---

## Instrument

recdep histogram vs SBS ovhit coverage — DIFFERENT EXECUTION REGIMES

## Validated by

The recdep histogram is measured on a STANDALONE run, which uses the default pc_skip=true. The SBS gate forces pc_skip=false on BOTH cores (sbs.cpp:1984-1985). Those exercise different paths, so a histogram rank does NOT predict SBS coverage. Measured 2026-07-29 on the same 6000-frame replay: 0x800931C0 standalone native=12001 / SBS native=1; 0x80040558 standalone 17878 / SBS 82544. Same binary, same replay, same frame count — the only difference is the pc_skip regime. CONSEQUENCE: a target picked off the histogram for being hot may be nearly unreachable under the byte-exact gate (and vice versa), so 'ranked #1 by recdep' and 'well covered by the SBS gate' are independent properties. Check ovhit under SBS before claiming a port is well gated, and do not explain a low SBS count as a short window without testing that.

## Known failure modes

(none recorded yet)
