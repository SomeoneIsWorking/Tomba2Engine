---
id: 7
title: One guest LZ77 glyph decompress needs more display fields than one, and the frame driver used to abort on it
status: fixed
symptom: The title stops on ExecutionExitReason::BudgetExhausted instead of completing its boot decompress
state_items: S004
tags: tomba1,execution,budget,frame-driver
created: 2026-09-29
updated: 2026-09-29
---

## Symptom

Tomba! 1's frame driver called `std::abort()` on `ExecutionExitReason::BudgetExhausted`. The abort
looked like a guard against a runaway guest, and it was reported as one. It was not: the guest
function that hit the budget was doing legitimate work that could not fit in one field.

## Measured cause

Tomba! 1's boot runs one LZ77 text/glyph decompress at `0x8003EF50` (`GuestGlyphStreamDecode`).

- **Output size: 286,720 bytes.**
- **Reached inside one field's budget: 45,200 bytes.**
- **One field's budget: 564,480 cycles.**

45,200 of 286,720 is 15.8% of the work per field, so the call needs

    286,720 / 45,200 = 6.34  ->  at least SEVEN fields

and that is a LOWER bound computed from work already known to be linear in the emitted byte count,
not a guess about how the decompressor batches. So the frame driver was aborting a call that
legitimately spans seven display fields, and "abort on budget exhaustion" was the cause of the stop
rather than protection from one.

## Fix

`Tomba1FrameDriver::resumeAcrossField` resumes the task slot on the following field instead of
aborting, bounded by `kMaxBudgetResumesPerCall = 32`.

**The bound is derived, not tuned.** 32 is roughly four times the longest measured call (seven
fields), which is the headroom a diagnostic wants, and far below the ~2,000 fields a real spin loop
would need before it could be mistaken for progress. **A spin therefore still fails** — and when it
does it fails with the exited task's register file and its resume address, through
`reportGuestExit`, instead of as an unexplained hang. That is the property the bound exists to keep:
without it, "resume until it finishes" is indistinguishable from outside a guest that will never
finish.

`task.budgetResumes` is cleared by every cooperative yield and by every fresh task start, so the
counter measures one **unbroken guest call**. Accumulating it across a long-lived task slot would
produce a count describing the slot's lifetime, which is not the quantity the bound is about.

## Evidence

- Guest entry `0x8003EF50` (`GuestGlyphStreamDecode`), reached during boot before the title frame.
- 286,720 / 45,200 / 564,480 measured on the shipping dynarec product; these are the three numbers
  the bound's four-times headroom is computed from.
- Gate: `ctest --test-dir build`, **41 of 41 passed** at psxport pin `2832a959`.

**Not yet shown: the bound's own red case.** The register report on a wedged guest is reachable and
compiled, but no run has yet been made to hit `kMaxBudgetResumesPerCall`, so "a spin still fails
with the register file" is a property of the code path and not a measured outcome. The mutation that
would settle it — making `resumeAcrossField` unconditional and confirming the title then fails at
the bound instead of hanging — has not been run, and is the next thing to do on this issue.

## Not claimed

Nothing here claims a boot beyond the decompress is correct. The bound makes the call complete; what
the title does after it is a separate question, tracked by the title's own state items.
