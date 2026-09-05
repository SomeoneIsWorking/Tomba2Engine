---
id: 5
title: Direct runtime drops the movie DMA3 completion callback
status: resolved
symptom: After nine genuine INT1 stream callbacks the direct runtime consumes DMA3 completion without dispatching StCdInterrupt's registered callback
state_items: S004
tags: tomba1,streaming,xa,audio,frame-loop
created: 2026-08-27
updated: 2026-08-27
---

## Symptom

The isolated real-disc product advances through nine genuine INT1 callbacks from controller LBA
57830 through 57838, then the active movie task remains inside `StGetNext` and the watchdog expires.
Presents 540 and 541 are both entirely black. No guest-VSync violation occurs.

## Current evidence

The title-owned host field now advances logic-only SPU/XA before pumping the controller. That service
is active: XA `rd` advances from 0 through 2015, 4031, and 5044. There is no `ring FULL` report and the
ring holds only about 3,020 unread frames when callbacks cease. Capacity/backpressure therefore did
not stop the drive.

The run exposes a separate ownership error before the first controller-routed audio sector. ReadS
starts both the pull-driven XA stream and the CDC drive with `XaState::push_mode == 0`. Because the
title correctly advances audio before pumping disc, the pull side independently scans and decodes
audio at LBAs 57837, 57845, and 57853. Only when the CDC later reaches its first audio sector does
`route_audio_to_spu` switch to push mode. Two owners therefore read the same stream during startup.
This is proven incorrect, but it is not what stops the controller at 57839; no cursor reset or retry
is justified.

PID 3195350 captured the decisive controller transition. The first nine callbacks acknowledge their
INT1 and re-arm the next sector. The tenth controller event latches sector 57839 with
`drive_event_armed=0`, `drive_deadline_ticks=0`, `following_sector_ready=1`, `bfrd=0`, an empty FIFO,
and current INT1. The pump enters `StCdInterrupt`, but that body exits before calling `CdReady`, so
the response remains current and the guest never raises BFRD to install the following sector.

The reason is the preceding successful callback. `StSetStream` calls the linked `DMACallback` entry
at `0x80067E84` through `0x80064D20`, registering channel 3 callback `0x80066D80`. After the ninth
INT1, `StCdInterrupt` finishes an eight-sector ring span, starts DMA3, and sets its in-flight guard at
`0x8001CA08` to one. DMA3 completes synchronously and owes channel 3 delivery. The shared IRQ owner,
however, resolves DMA callbacks only through the legacy `GameConfig::dmaCallbackTable`. A direct
runtime has no `GameConfig`, so it consumes that completion with callback zero. `0x80066D80` never
promotes the ring slot or clears the guard; all later `StCdInterrupt` calls take the guard's immediate
return before `CdReady`. The proper fix is a typed direct-runtime DMA callback owner, not polling,
forcing BFRD, or resetting a cursor.

## Falsified cause

The prior hypothesis was that normal-field starvation filled the XA ring after roughly eight sectors.
PID 3166391 falsified it: audio consumption runs, the ring remains mostly empty, and the product still
stops at the same callback count.

## Evidence

- `scratch/logs/tomba1-audio-field-isolated.console.log`: exact XA read-cursor movement, pull-side
  decode LBAs, nine reason-1 callbacks, controller LBA endpoint, watchdog stack, and absence of any
  ring-full or guest-VSync violation.
- `scratch/logs/tomba1-controller-field-isolated.console.log`: exact before/after field state. Nine
  successful callbacks advance and re-arm through 57838; sector 57839 then remains latched with
  INT1/following ready, no deadline, BFRD zero, and no further `CdReady` entry.
- Generated `StSetStream`, `StCdInterrupt`, and DMA completion callback bodies at `0x80066C14`,
  `0x800670D0`, and `0x80066D80`: measured channel/callback registration, in-flight guard set, and
  completion-side guard clear.
- `scratch/screenshots/present_540.ppm` and `present_541.ppm`: fresh player-facing captures, each 0 of
  691,200 non-black pixels and independently inspected as entirely black.
- `tests/test_stream_field_turn.cpp`: shipping field sequence advances audio exactly once before CD,
  plus the absent-layout and inactive-stream refusal paths.

## Resolution

The shared typed direct-runtime DMA registry/IRQ selector and title-owned adapter now compile
together. Tomba! 1 binds linked entry `0x80067E84`, exchanges the typed channel entry, returns the
prior callback, and preserves the measured DICR channel/master enable write. Focused coverage proves
first registration, replacement, channel isolation, and DICR arming; the isolated-framework Clang
product and 10/10 focused tests pass. The shared change is landed as psxport `fb08d30f`, the title pin
records that exact commit, and the isolated Clang build records the same clean framework provenance.
PID 3376435 ran that exact build. The direct registry dispatches callback `0x80066D80`, the in-flight
guard returns to zero, genuine INT1/CdReady delivery continues through at least LBA 58739, and the
MDEC-out callback at `0x8001F4D4` chains repeatedly. Present 600 is a clean decoded “Whoopee Camp”
logo frame. Issue 0005 is resolved. The first later failure is separately recorded as issue 0006;
title screen, gameplay, and widescreen remain unverified.
