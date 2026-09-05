---
id: I031
kind: instrument
status: trusted
created: 2026-07-30
---

## Instrument

PSXPORT_MIRROR_VERIFY=<addr> — per-invocation native-vs-substrate byte gate

## Validated by

Run against BOTH classes on 0x80086288, 400 frames / 800 invocations each. POSITIVE (correct port must pass): the wired LibapiIntr::runVblankCallbacks reports '[mirror-verify] 0x80086288 OK (pass #769)' and zero MISMATCH lines — scratch/logs/mv_86288.log. NEGATIVE (a defective port must FAIL): the SAME body with the r16/r17 GuestFrame spill offsets swapped (the exact defect claim C021 records as invisible without diffing the guest-visible behavior), built in an ISOLATED tree copy so the shared checkout was never touched, reports 'MISMATCH at invocation #1' and names the guest-stack bytes 0x801FFFD0..D7 with the s0 value 0x800AC430 transposed between sp+16 and sp+20 — scratch/logs/mv_negctl.log. So it catches a defect that changes NO observable game behaviour, on the first call. WHAT IT COMPARES (verify_harness.cpp strictCheck): the union of RAM bytes either leg touched, the full 1 KB scratchpad, registers v0/v1/s0-s7/gp/sp/fp/ra, and hi/lo. BLIND TO: r1/$at and all other caller-saved temps (t0-t9/a0-a3) — a port that omits an $at write passes, correctly, since nothing reads it across a call. ALSO BLIND TO code paths the run does not reach: on 0x80086288 the 8 callback slots at 0x800ABDC0 are all NULL in the boot+title window, so the indirect typed runtime address dispatch arm is NEVER exercised and 800 green passes say nothing about it. REFUSAL/SILENCE MODE: a gate that is never armed prints NOTHING at all, which is indistinguishable from a clean pass — always require a positive 'OK (pass #N)' line AND a non-zero PSXPORT_DEBUG=ovhit count for the address before reading silence as success.

## Known failure modes

(none recorded yet)
