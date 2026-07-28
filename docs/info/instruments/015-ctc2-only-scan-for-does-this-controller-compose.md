---
id: I015
kind: instrument
status: DISTRUSTED
created: 2026-07-28
distrusted_on: 2026-07-28
---

## Instrument

ctc2-only scan for 'does this controller compose its own GTE transform'

## Validated by

FALSE-NEGATIVE PRONE — do not use alone. It answers 'does this function write CR0-7' by looking for direct ctc2 instructions, and therefore MISSES every controller that composes through the libgte leaves instead: SetRotMatrix 0x80084660 (-> CR0-4) and SetTransMatrix 0x80084690 (-> CR5-7), typically after matMul 0x80084110 / Math::matColScale 0x80084520. Caught 2026-07-28: 0x8002F36C was EXCLUDED from the kanban #15 controller batch on this scan's verdict ('writes NO GTE control register, so it inherits its caller transform') and that verdict was wrong — it composes via libgte and then calls the mesh writer, so it was scope-ready all along. Same pattern on 0x8013E08C. A correct census tests BOTH forms: direct ctc2 to CR0-7, OR a call to 0x80084660/0x80084690. The nine controllers the scan judged SCOPE-READY are unaffected (they write ctc2 directly and were re-audited).

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-28

Misses libgte-composed transforms (SetRotMatrix/SetTransMatrix); produced a wrong exclusion for 0x8002F36C. Test both forms.

> Every result this instrument produced is suspect until it is re-validated.
