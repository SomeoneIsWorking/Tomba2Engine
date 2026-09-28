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

## The bound's red case, MEASURED — by lowering the bound, not by inventing a spin

A guest that never reaches its yield must fail **loudly and in bounded time**, with the state that
hit the bound. That was asserted by the code and not shown by any run. It is now shown, on the real
long call rather than a synthetic one, by temporarily setting `kMaxBudgetResumesPerCall = 1` and
running the product against the operator's disc (`PSXPORT_TOMBA1_DISC`, 400 fields). The bound of 1
is deliberate and is the whole point: the decompress needs about seven fields, so it cannot pass a
bound of 1, and the run therefore exercises the failure path on genuine work.

    [tomba1-frame] guest task 2 used its whole field budget at 0x8005B78C (564484 cycles)
                  and resumes on field 1 of 1 for this call
    [tomba1-frame:error] guest task 2 spent 2 consecutive display fields without reaching its
                  cooperative yield; last resume 0x8005B78C after 564490 cycles — the task record
                  was re-armed 1 time beyond the 1 allowed for one call, so this is a guest call
                  that makes no progress, not one that is merely long
    [tomba1-frame:error] guest task 2 exited with budget-exhausted at 0x8005B78C after 564490 of
                  564480 budget cycles: cycle budget exhausted
    [tomba1-frame:error]   guest-regs zero=0x00000000 at=0x800A0000 v0=0x00000011
    [tomba1-frame:error]   guest-regs v1=0x800B9526 a0=0x800B957E a1=0x800B967D
    [tomba1-frame:error]   guest-regs a2=0x000000A7 a3=0xFFFFFFFF t0=0x8009E424
    ... 32 registers in all, three per line, named by PSX ABI number

So the property holds: **the driver stops, names the resume address, and prints the whole register
file, instead of resuming forever.** The process ends through the abort path (exit 139), which is
the intended "loud" ending rather than a hang.

**The registers are consistent with the decompress and not with a spin**, which is worth stating
because it is the difference between "the bound works" and "the bound works on a real call":
`at = 0x800A0000` is a RAM base, `a0`/`a1`/`v1` (`0x800B957E`, `0x800B967D`, `0x800B9526`) are three
RAM pointers, and `a2 = 0xA7` is a small count — a routine copying decoded output, holding three
pointers into the output area. A spin would show one loop counter and a constant frame.

**What this does NOT show:** that the shipping bound of 32 is the right number. It shows the bound is
*reachable, enforced, and reported*. Whether 32 is correctly derived from the seven-field measurement
is still the arithmetic in the section above, and 32 remains unexercised by a run because the real
call finishes in about seven.


## Not claimed

Nothing here claims a boot beyond the decompress is correct. The bound makes the call complete; what
the title does after it is a separate question, tracked by the title's own state items.
