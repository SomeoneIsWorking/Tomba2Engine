---
id: 7
title: DrawSync runtime call exhausts its budget after the first native frame
status: open
symptom: The second Tomba native frame aborts inside gpuDmaQueueSync before the recorded gameplay frontier
state_items: S001
tags: tomba2,dynarec,lightrec,gpu,dispatch
created: 2026-09-05
updated: 2026-09-05
---

## Reached boundary

PSXPort `eb5f23a8b3506f8853b3cfadcedc024cd90818a0`, Lightrec
`b1457137c31cedff5f440d59da29401d021ba2da`, and the final title-native image
bindings complete native initialization and frame 0 at DEMO `0x801062E4`.
After the complete generated-body removal, a silent headless observation with
`PSXPORT_NATIVE_FRAMES=2` and no auto-input reproduces the next-frame failure:

```
gpuDmaQueueSync required a completed guest call, but execution exited as
budget-exhausted at 0x80044E54 after 564488 cycles: cycle budget exhausted
```

The reached native stack crosses `TombaFrameDriver::stepFrame`, the DrawSync
guest/native dispatch boundary, and `Render::gpuDmaQueueSync`. The failing call site dispatches `0x800834A0`, the
GPU timeout arm already documented as PlatformHle-owned in claim C026. This is
not evidence that another native SDK body is missing, and increasing the budget
or bypassing the call would hide the cause.

The title had a separate proven lifecycle defect: main bound native entries
before `BootStub` reloaded the executable and advanced its image generation.
Binding the resident declarations during `TombaRuntime::bootInit` restores them
after that reload. Following generated-body removal, 199 of 254 declarations
bind to the resident image, with 55 inactive nonresident addresses.
That restoration is covered by the production-catalog regression's stale-image
negative, idempotent rebind, and scoped original call through Lightrec. It changes
the reached stack to the native DrawSync owners but does not resolve this next
execution failure.

## Remaining diagnosis

Compare the reached platform-service lookup, nested call/return addresses, and
CPU state at the timeout-arm boundary with the recorded native GPU contract and
the new executor. Establish why this requested service reaches the reported
guest PC instead of completing. The observation localizes a regression in the
migrated composition; it does not yet identify which owner causes it.

The watchdog fault handler reports signal 6; the process exits 139. The run
terminates before representative gameplay. No generated source or
interpreter substitute was enabled, and no fallback or gameplay claim follows
from this failed run. Keep issue 0005's real resident/overlay original-call
discriminator open as well.
