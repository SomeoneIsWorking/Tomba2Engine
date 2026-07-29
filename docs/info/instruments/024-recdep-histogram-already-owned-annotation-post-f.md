---
id: I024
kind: instrument
status: trusted
created: 2026-07-29
---

## Instrument

recdep histogram — ALREADY-OWNED annotation (post-fix reading)

## Validated by

The recdep counter sits after overrides::dispatch but BEFORE func_X consults g_<mod>_override[], so PlatformHle-owned addresses appear at full call count while running native handlers. Registry-owned addresses never reach the counter, so THEY are correctly absent — the blind spot is PlatformHle only. Fixed in psxport d8ba3030: the dump annotates them. Validated on the case that must differ — 0x800834A0 (gpuTimeoutArm, 24152 calls, previously the #1/#2 entry) plus 0x8001CF2C/0x8001DC40/0x8001D2A8 now carry 'ALREADY OWNED by PlatformHle', genuine targets stay unmarked. READ ANY PRE-2026-07-29 recdep RESULT WITH THIS IN MIND: its top entries may include owned addresses.

## Known failure modes

(none recorded yet)
