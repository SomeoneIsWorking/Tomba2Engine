---
id: C005
kind: claim
status: holds
created: 2026-08-27
tags: tomba1,frame-loop,vsync,scheduler
depends: game/core/frame_driver.cpp,game/core/stream_field_turn.cpp
---

## Claim

`SCUS_942.36` owns three cooperative task records and waits on an interrupt-mode VBlank event, while
its linked libetc VSync occupies `[0x80067C30,0x80067D78)`.

## Evidence

Ghidra decompilation and narrow disassembly covered main `0x800163B0`, scheduler `0x80017024`, yield
`0x800171D4`, restart `0x800172C4`, callback `0x80017374`, and VSync `0x80067C30`. They show three
records at `0x801FD800` with stride `0x70`, event class `0xF2000003`, spec `2`, mode `0x1000`, the
callback's two counter increments, and the VSync body's timeout extent.

Bounded product execution reached all three scheduler records and falsified the original assumption
that every runnable record persisted its entry at `+0x0C`: direct start `0x80017154` carries slot in
`a0` and entry in `a1`, while only restart `0x800172C4` writes the record entry. A title-local native
start owner now captures that ABI before the task is resumed. The same runs proved that libgpu helpers
`0x80061480`/`0x800614B4` use VSync only for their 240-field queue-timeout clock; native replacements
retain the measured deadline, spin limit, and reset writes without calling guest VSync.

A later bounded product run installed six native synchronization bindings without reaching the fatal
guest-VSync trap, opened the selected CHD, and completed reads of four sectors at LBA 96085, four at
102011, one at 102015, and another four at 96085. PRESENT captures 1, 2, 5, and 10 were black startup
frames; capture 30 contained 14,004 non-black pixels of 691,200 (2.03%) and was independently
inspected as a coherent centered SCEA presentation. This proves the boot/render/presentation-fence
frontier, not title-screen, input, or gameplay behavior.

Source inspection against psxport's established coroutine scheduler then found that the first
title-local driver preserved each coroutine's C stack but not its R3000 register file. The corrected
driver copies the native loop context into a fresh slot, installs the record's stack top at `r29`,
uses `0xDEAD0000` as the return sentinel, saves registers before yield, restores them before resume,
and restores the loop registers after every task handoff. This correction is Clang-built,
clang-formatted, clang-tidy-clean, and title-test-clean. A later real-disc run confirmed it advances
through the dynamically loaded `OPTSUB00` overlay and into the movie ReadS path.

The movie path busy-polls `StGetNext` and cannot return to the ordinary frame boundary while it
waits. Tomba! 1 therefore publishes the measured ready-callback slot at `0x80095FF0` and registers a
title-owned stream field at the shared 59.940 Hz NTSC cadence. That field advances logic-only SPU/XA
exactly once before the paced CD-stream pump; each ordinary native field reports completion to cancel
a duplicate timer turn. Shipping coverage proves the operation order and that the callback does
nothing when the runtime publishes no callback layout or when no continuous stream is active. Typed
shared CdRead ownership resolves issue 0002. The linked `CdReady` wrapper's guest VSync timeout is
title-owned natively; isolated execution passes that former fatal violation while keeping the VSync
trap intact. Positive/negative coverage proves exact INT1 consumption and a nonblocking
absent-response result. Shared issue 0004 is resolved: callbacks now require a current controller
INT1. PID 3166391 then advances through nine genuine callbacks to LBA 57838 with no guest VSync.
Its XA read cursor advances, falsifying ring capacity as the later stop, while pull-side decode before
the first CDC push reveals temporary dual drive ownership. PID 3195350 identifies the remaining
scheduling halt: linked `DMACallback` `0x80067E84` registers channel 3 callback `0x80066D80`, but the
direct runtime has no typed DMA callback owner, so the completion is consumed without clearing
`StCdInterrupt`'s in-flight guard at `0x8001CA08`. The following sector and INT1 remain latched at
57839 with BFRD zero. Issue 0005 owns that direct-runtime seam. The typed shared owner and title
adapter now compile together with focused registration/DICR coverage, but no post-fix real-product
run exists yet; black presents 540 and 541 are not render, gameplay, or widescreen success.

## What would falsify it

Raw disassembly or a running scheduler trace contradicts the task-record transitions, per-slot
register ownership, event tuple, callback counter writes, or VSync extent; or an inspected capture
shows that the reported PRESENT content is incoherent.
