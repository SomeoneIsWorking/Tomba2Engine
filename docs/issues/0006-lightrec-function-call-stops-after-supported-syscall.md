---
id: 6
title: Lightrec function call stops after a supported syscall during Tomba boot
status: resolved
symptom: TombaRuntime::bootInit aborts before its first native frame because a completed HLE syscall is reported as an incomplete guest function call
state_items: S001
tags: tomba2,dynarec,lightrec,syscall,boot
created: 2026-09-05
updated: 2026-09-05
---

## Reproduction and evidence

The authenticated `SCUS_944.54` product, built against PSXPort
`639e3630af3af9ed519bffa7da53c229c689b4d1` and Lightrec
`b764c4c9f4bc425a56bfc4c32333ff8200ce8ab9`, opens the silent headless renderer,
completes native CRT0, then refuses `bootInit` at resume PC `0x800808A8` after
21,884 guest cycles: `interrupt-or-exception`, detail `syscall`.

The authenticated resident image contains `addiu a0,zero,2; syscall; jr ra; nop`
at `0x800808A0`. This is the supported ExitCriticalSection selector. Its syscall
is at `0x800808A4`, followed by the ordinary guest return at `0x800808A8`.
The run stops before the first requested native frame; logo/movie presentation
is not gameplay or dynarec performance evidence.

Reproduce from the repository root after building the product, with the user's
disc already configured:

```sh
PSXPORT_VK_HEADLESS=1 PSXPORT_VK_WINDOW=0 PSXPORT_NOAUDIO=1 SDL_AUDIODRIVER=dummy \
PSXPORT_NATIVE_FRAMES=1 PSXPORT_WATCHDOG=3 \
build/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
```

## Owning cause and required fix

PSXPort's `LightrecExecutor::executeWithBoundary` handles `LIGHTREC_EXIT_SYSCALL`
through `handleSyscall`, applies the resume PC, and always returns
`InterruptOrException`. Its function-call mode therefore stops at a supported
internal service boundary instead of continuing to the guest return.
`dispatchGuestWithArgumentsToReturn` correctly refuses that incomplete result.

The shared executor must distinguish its explicit checkpoint mode from bounded
function-call completion, preserve the existing CP0/service transition, and
resume supported syscalls within the original execution budget. Unsupported
selectors, delay-slot exceptions, faults, and budget exhaustion must remain
explicit refusals or typed exits. The existing shared syscall evidence is in
PSXPort issue 0024 and claim C036; this is not permission to bypass syscall
instructions or add a title-address workaround.

After the shared fix, rerun this exact checkpoint and continue issue 0005's
resident/overlay discriminator. Representative gameplay remains open.

## Resolution

PSXPort `eb5f23a8b3506f8853b3cfadcedc024cd90818a0` and Lightrec
`b1457137c31cedff5f440d59da29401d021ba2da` pass the same real-image checkpoint:
native CRT0 and the complete init prefix return, START.BIN is loaded, and the
first native frame reaches DEMO (`0x801062E4`) before a clean frame-budget exit.
No title-address bypass or interpreter selector was added. This resolves the
supported-syscall continuation failure, not the remaining representative
gameplay or image-qualified override gates.
