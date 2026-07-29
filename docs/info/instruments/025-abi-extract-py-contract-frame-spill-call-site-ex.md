---
id: I025
kind: instrument
status: trusted
created: 2026-07-29
---

## Instrument

abi_extract.py --contract (frame/spill/call-site extractor) — and port_check.py, which shares its locator

## Validated by

**THIS ENTRY PREVIOUSLY DISTRUSTED THE TOOL. THAT WAS WRONG AND IS RETRACTED (2026-07-29, same day).**

The claim was: `--contract` reports `frame_size=0 / 0 spills / 0 call sites` for functions that have a
frame, when the entry block ends in an unconditional goto, because its reachability pass drops the
real body. The worked example was 0x80092E3C.

That is exactly backwards. What the recompiler emits for `gen_func_80092E3C` is a 24-line function
ending at `return;`, followed by **74 lines of the NEXT function's body** as unreachable trailing text
inside the same C function — the folded-sibling / shared-epilogue artifact. `abi_extract` analysed the
REAL function correctly (frameless, no calls) and reported the tail honestly as
`unreachable_block_count = 13`. `port_check` compared against that correct model and was right to FAIL.

The tool documents this behaviour in a ~20-line comment above its dead-code exclusion
(external/psxport/tools/abi_extract.py:869-885): "Recompiled bodies routinely carry an unreachable
trailing fall-through into the NEXT sibling function after their real `return;`… The recompiler
sometimes FOLDS several contiguous guest functions into ONE gen_func_ body… Only the function reached
at THIS entry is live; the rest are dead siblings."

**The real defect was in MY port**, and the lesson generalises: `port_gen` transcribes the WHOLE C
body, dead sibling tail included, so a port_gen draft of a folded body contains a foreign function's
code after its `return;`. It is unreachable, so behaviour and SBS are unaffected — but the body is
inflated with another function's code and port_check FAILS on the store-sequence comparison.

RULE, and it is the opposite of what this entry used to say: **when port_check FAILs a port_gen body,
suspect the BODY first, not the tool.** Check whether the gen body has a bare `return;` before its
closing brace with more statements after it; if so, trim the port at that `return;`.

WHAT MISLED ME: I diffed my integrated body against the gen body and got "98 statements, 0
differences", and read that as proof the port was right. It only proved port_gen had copied faithfully
— including the 74 lines that are not part of this function. A diff against the wrong extent cannot
detect a wrong extent.

## Known failure modes

- None established. Both cases where this tool was accused (0x80092E3C, 0x8008913C) were the accuser's
  error.
