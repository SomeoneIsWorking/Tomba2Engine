# Boot SIGSEGV in `NativeDispatcher::invoke` — reproduces on psxport HEAD

**Kind:** bug
**Status:** fixed

## Symptom

Launching `tomba2_port` through the headless automation path aborted during boot, before any frame was
presented. The issue was filed against a signal 11 with this chain:

```
psx3cpu::cpu::NativeDispatcher::invoke(psx3cpu::cpu::NativeKey)
psx3cpu::cpu::dispatchGuestHostService(Core&, unsigned)
Engine::frameUpdate()
tomba::TombaFrameDriver::stepFrame(Core&, unsigned)
```

Driving the same binary through the control channel names the chain exactly and prints the guest
address the dispatcher was handed:

```
Sequencer::frameTick()
tomba::guest::dispatchJalToReturn<>()            address=1633820737  (0x61622041)
[executor:error] dispatchJalToReturn required a completed guest call, but execution
exited as fault at 0x61622041 after 0 cycles: ambiguous code-image identity
```

`0x61622041` is not code and is in no loaded image — it is an overlay word read as a function
pointer. The product then took the title's own abort on the fault, which is why the report reads
SIGSEGV/SIGABRT depending on the launch.

## Cause

`game/audio/libsnd_globals.h`, the sound driver's named globals, published every constant as a
CLUSTER OFFSET into the 0x80100000 libsnd cluster. Nine of them are not cluster members at all: they
are whole guest addresses the guest forms with its own `lui`/`addiu`. Expressed as offsets they
evaluate somewhere else entirely, and each one's own comment disagreed with its own value:

| constant | decimal in the header | address it means | what the tick actually reads |
|---|---|---|---|
| `kUserCallback` | `70448` → 0x8010ACC0 | 0x800AC430 | the tick descriptor's user-callback slot |
| `kSeqTickFn` | `70444` → 0x8010ACBC | 0x800AC42C | the tick descriptor's `*SsSeqCalled` slot |
| `kSeqPrepFn` | `46144` → 0x8010B440 | 0x800931C0 | SsSeqCalled's one-shot pre-loop leaf |
| `kSeqPtrArray` | `21680` → 0x801054B0 | 0x80104C30 | the per-sequence channel-table array |
| `kSeqReentryFlag` | `21668` → 0x801054AC | 0x80104C24 | SsSeqCalled's re-entry guard |
| `kSeqActiveMask` | `21672` → 0x801054A8 | 0x80104C28 | the per-sequence active bitmask |
| `kSeqCount` | `21712` → 0x801054D0 | 0x801054B0 | s16 sequence slots |
| `kSeqChanCount` | `21714` → 0x801054D2 | 0x801054B2 | s16 channels per sequence |
| `kHardwareVoiceActive` | `21694` → 0x801054BE | 0x800AC3F4 | the SPU's active-voice bitmask |

Three more were used at one call site as an address and at another as an offset, so no single value
could be right: `kPerVoiceTable` (0x801054CE offset / 0x801054D8 address), `kPerVoiceDispatchLo`
(0x801054F2 / 0x80105D10) and `kUnnamedWord23464` (0x80105BA8 / 0x80105D0C).

The header arrived in commit `09f705f` ("Make Tomba! 2's guest operations readable … behaviour-
preserving"), which replaced the pre-rename absolute `#define`s in `game/audio/sequencer.cpp` with
these names. It was not behaviour-preserving for them, and `game/game_tomba2.cpp` kept the boot
guard's own literal for the same slot, so the guard tested 0x800AC42C while the tick dispatched
0x8010ACBC — a guard reading a different word than the dispatch, which is not a guard.

Why the boot reached it at all: the port drives the tick wrapper once per display field
(`Engine::frameUpdate`) because it delivers no IRQ, so frame 0 is the FIRST tick of the run — and
0x8010ACBC is inside the MODE overlay slot, holding overlay bytes at that moment.

## Fix

`game/audio/libsnd_globals.h` now states the two kinds of address apart, and every constant's
comment names the address its value evaluates to:

* ABSOLUTE guest addresses — the nine above plus the four mask words, published as `0x...`
  addresses. The tick descriptor's three words (`kUserCallback`, `kSeqTickFn`, `kTickMode`) and the
  key-event scan's four (`kHardwareVoiceActive`, `kKeyScanPitchTable`, `kKeyScanMatch`,
  `kVolumeSnapshotScratch`) are here.
* CLUSTER OFFSETS — the immediates `voiceStateFlush` adds to `kBase`, kept as decimals with the
  address each evaluates to in its comment.

The four mask words were correct as offsets (0x80105BF0 == `kBase` + 23536) while also being used
whole; that coincidence is a trap, so they are absolute now and their `kBase +` call sites in
`game/audio/sequencer.cpp` use them whole.

The three split constants got the address each use site needs: `kKeyScanPitchTable`,
`kKeyScanMatch` and `kVolumeSnapshotScratch` for the address role, while `kPerVoiceTable`,
`kPerVoiceDispatchLo` and `kUnnamedWord23464` stay the offsets `voiceStateFlush` adds to `kBase`.

Call sites moved to the one owner:

* `game/audio/sequencer.cpp` — the tick wrapper, the key-event scan, the voice merge and
  `voiceStateFlush` now read the constant that matches their own kind of access.
* `game/game_tomba2.cpp` — the boot guard's `SEQ_FUNC_PTR` literal is gone; it reads
  `libsnd::kSeqTickFn`, the same word the wrapper dispatches.
* `game/core/frame_diagnostics.cpp` — the seq-debug line reports `libsnd::kTickMode` /
  `libsnd::kSeqTickFn` instead of two more literals.

## Evidence

Headless agent launch (`launch_environment.agent_environment`, `tools/shipping_settings.ini`, the
`.env` disc), `run 60` → `shot` → `run 120` → `shot` → `run 240` → `shot`:

```
[repl] shot (VK readback) -> scratch/boot/shots/boot60.png   [render path = native] guest_scan=224
[repl] shot (VK readback) -> scratch/boot/shots/boot180.png  [render path = native] guest_scan=224
[repl] shot (VK readback) -> scratch/boot/shots/boot420.png  [render path = native] guest_scan=224
```

Logo FMV → title screen, a real picture, and the run continues past frame 870. Fallback telemetry at
exit: `fallback_blocks=0 fallback_instructions=0`, i.e. everything ran through Lightrec.

The first three faults named in this issue are gone: 0x61622041 (frame 0), 0x0000B440
(`kSeqPrepFn`, frame 1) and the sequencer's own loop all now run. What remains at ~frame 870 is a
DIFFERENT abort with its own owner — the water-jet GTE depth mismatch (`FUN_80027768`, cards
kanban-120 / issue 0022), reached only because the run now gets that far.

## Open

The guest instruction words at `0x800909C8`..`0x800909CC` are `lui s0,0x800B` +
`addiu s0,s0,0xC430`, i.e. a base of **0x800BC430**, one page above the slots this fix uses. The
slots the fix uses are the ones the image itself backs: MAIN.EXE ships `0x80090BD0` (SsSeqCalled)
at `0x800AC42C`, and a live read shows `0x80086288` (`LibapiIntr::runVblankCallbacks`) at
`0x800AC430` — two function pointers at exactly the two slots a "call the user callback if
installed, then call SsSeqCalled" trampoline reads, with the unguarded one already holding code in
the image's own data. `0x800BC42C` instead holds table words (`0x03FF000C`) that nothing in the
image writes, so a trampoline reading it could not run. Reconciling the instruction stream with that
is open RE, not part of this fix.