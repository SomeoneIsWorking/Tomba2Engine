# 0023 — Tomba! 2's frame-rate lever is one guest byte, and the port now owns it

## What the lever is

Tomba! 2's engine paces itself with a vblank gate inside its main loop. The whole mechanism is
five instructions and two guest fields, both recovered from the authenticated resident image
(`SCUS_944.54` → `MAIN.EXE`, text `[0x80010000,0x800AE800)`, entry `0x800896E0`, load base and size
read from the PS-X EXE header rather than assumed):

The gate, `FUN_80050b08` (the StrPlayer main loop crt0 reaches by `jal` at `0x800896E0`):

```
80050C8C  A6C0809C  sh    zero,-0x7f64(s6)     ; DAT_800e809c  = 0    (s6 = 0x800F0000, u16)
80050CB0  0C01E22B  jal   0x800788AC          ; per-frame state update
80050CB8  0C014798  jal   0x80051E60          ; task scheduler -> builds the OT
80050CC0  0C0203DB  jal   0x80080F6C          ; DrawSync(0)
80050CC8  3C031F80  lui   v1,0x1f80
80050CCC  96C2809C  lhu   v0,-0x7f64(s6)      ; v0 = DAT_800e809c
80050CD0  90630235  lbu   v1,0x235(v1)        ; v1 = DAT_1f800235
80050CD4  00000000  nop
80050CD8  0043102B  sltu  v0,v0,v1           ; v0 = (counter < quota)
80050CDC  10400006  beq   v0,zero,0x80050CF8  ; no -> release the gate
80050CE0  3C04800F  lui   a0,0x800f
80050CE4  9482809C  lhu   v0,-0x7f64(a0)      ; spin: reload the counter
80050CE8  00000000  nop
80050CEC  0043102B  sltu  v0,v0,v1
80050CF0  1440FFFC  bne   v0,zero,0x80050CE4  ; yes -> keep spinning
80050CF4  00000000  nop
80050CF8  0C0141B4  jal   0x800506D0
80050D08  1072FFDC  beq   v1,s2,0x80050C7C     ; next pass
```

The counter's only incremental writer, libapi vsync-callback **slot 4**:

```
800506B4  3C03800F  lui   v1,0x800f
800506B8  6C628064  lhu   v0,-0x7f64(v1)      ; v0 = DAT_800e809c
800506C0  24420001  addiu v0,v0,1
800506C4  5C628064  sh    v0,-0x7f64(v1)      ; DAT_800e809c = v0 + 1
800506C8  03200008  jr    ra
```

and the quota's only store, in `FUN_80050a0c`:

```
80050A18  A440809C  sh    zero,-0x7f64(v0)     ; DAT_800e809c = 0
80050A1C  24020002  li    v0,0x2              ; addiu v0,zero,2
80050A20  A0620235  sb    v0,0x235(v1)        ; DAT_1f800235 = 2
```

## What sets each side, and when

`DAT_1F800235` (u8, the threshold) has **exactly two** references in the whole resident text: the
one store above, whose literal is compiled into the instruction word `0x24020002`, and the one load
at `0x80050CD0`. **The guest has no runtime 30/60 switch.** The value is a boot-time constant.

`DAT_800E809C` (u16, the counter) has **exactly six** references in the resident text:

| site | instruction | what |
|---|---|---|
| `0x80050A18` | `sh zero,-0x7f64(v0)` | zeroed at init |
| `0x80050C8C` | `sh zero,-0x7f64(s6)` | zeroed at the top of every pass |
| `0x80050CCC` | `lhu v0,-0x7f64(s6)` | the gate's first test |
| `0x80050CE4` | `lhu v0,-0x7f64(a0)` | the gate's spin reload |
| `0x800506B8` | `lhu v0,-0x7f64(v1)` | the incrementer reads it |
| `0x800506C4` | `sh v0,-0x7f64(v1)` | the incrementer writes `+1` |

Two independent measurements agree on those counts: Ghidra's reference database, and
`tools/frame_cadence_census.py` matching address-forming instruction words in the image (8
references, 0 unresolved candidates — 6 for the counter, 2 for the quota).

## What the wait does to the simulation cadence

**Nothing except limit the rate.** One full pass of per-frame work runs per gate release, and no
instruction in that pass reads the quota. The pass is: counter reset → `FUN_800788AC` → `FUN_80051E60`
→ `DrawSync(0)` → gate → `FUN_800506D0` → `PutDispEnv`/`PutDrawEnv`/`DrawOTag` → flip parity. The quota
does not scale, weight or gate any work; one logic frame per N vblanks is simply 60/N Hz of logic.
So **the guest's own 60 fps mode is this byte equal to 1**, and that is a far better place to reach
60 fps than anything host-side.

## The defect this found: two homes, no owner, and a store nothing could read

Before this change the retail literal `2` lived in three places and each of the two addresses in
two:

- `game/scene/startup.cpp` wrote `0x1F800235 = 2` as a transcription of the guest's `li`;
- `game/core/game_config.cpp` carried `.paceQuota = 2` for the framework's host pacer;
- `game/game_tomba2.cpp` read the byte back out of guest memory every logic frame to size the
  per-field audio loop — i.e. the port read its own decision back through RAM;
- and at the end of every logic frame it did `mem_w16(0x800E809C, mem_r8(0x1F800235))`, described
  in the comment as "satisfy the pacing dwell immediately".

That last store was unreachable by anything. The gate that would have read it does not execute in
this product (below), and the counter's only other reader is the vsync callback the port reaches
through its own sequencer tick. Nothing observed the value it was setting.

## The first version of the fix was wrong, and reading the image is what caught it

The obvious repair — have the port advance the counter once per display field, standing in for the
vsync callback the port does not raise — would have **counted every field twice**. Reading who
registers vsync-callback slot 4 rather than reasoning about it:

- the port parks `LibapiIntr::runVblankCallbacks` (`0x80086288`) into libsnd's user-callback slot
  `DAT_800AC430` (`game/audio/sequencer.h`);
- `FUN_800909C0`, the per-field tick the port dispenses, is `if (DAT_800ac430) (*DAT_800ac430)();`
  then `(*PTR_FUN_800ac42c)()` — so it *does* call it;
- `runVblankCallbacks` walks all 8 slots and calls every non-null one;
- and `DAT_800AC430`'s **only** reference in the resident image is that read — nothing in MAIN.EXE
  ever writes it, so the port really is the only writer and the callback chain really does run;
- slot 4 is filled by libsnd's `SsSetTickMode` (`FUN_80090750` calls `FUN_80085BB0` at
  `0x800908E4`; `FUN_80085BB0` forwards `(4, cb)` to `FUN_800862F4`, whose body is
  `(&DAT_800abdc0)[slot] = cb`).

So the guest's own incrementer runs `quota` times per logic frame in this product, through the port's
own tick. A port incrementing as well would have left the counter at 4 for a 2-field frame — and
would then have been checking a number it manufactured itself. The shipped `consumeVblank()` only
reads.

## Why this is not a `declareOverride`

`0x80050CC8..0x80050CF4` is a **label inside `FUN_80050b08`'s body**, reached by falling through,
never by `jal`/`jalr`. `psxport/AGENTS.md` makes that ineligible as an override key. The enclosing
`FUN_80050B08` is itself never dispatched here: psxport's `native_boot` calls
`GameRuntime::bootInit`, which runs `FUN_80050b08`'s init prefix and then hands iteration to
`TombaFrameDriver`. Declaring an override would declare something with no reachability. What the port
owns is the gate's **state** — the two fields the comparison reads.

## What was changed

- `game/core/frame_cadence.{h,cpp}` — new `tomba::FrameCadence`, the one owner. It holds the
  measured addresses and the retail value, publishes the decision into the guest's quota byte
  through the same single byte store the guest makes, performs the guest's own per-frame counter
  reset, observes (never writes) the counter, and reproduces the gate's `sltu` for reporting.
- `game/scene/startup.cpp` — `initFrameState` publishes instead of transcribing the literal.
- `game/game_tomba2.cpp` — the per-field loop takes its count from the owner; the unreachable
  counter store is gone.
- `game/core/frame_driver.cpp` — opens the cadence at the top of each logic frame and closes it at
  the bottom.
- `game/core/frame_diagnostics.cpp` — a per-frame `cadence` line with the frame's own field count,
  the guest's counter, the gate's verdict, and the running denominators.
- `game/core/game_config.cpp` — `paceQuota` and `dwellCounter` now read the owner's constants
  instead of repeating them.
- `tools/frame_cadence_census.py` + 3 ctest registrations + `tests/test_frame_cadence.cpp`.

## A second writer family, outside the resident image

Scanning all 28 provisioned overlays for the same address-forming idiom finds it in two of them, at
byte-identical-looking code (file offsets; **load bases not established**):

```
A0L.BIN  +0x0099AC  0xA062019C  sb  v0,0x19C(v1)   ; DAT_1f80019C = 2  (StrPlayer "swap")
A0L.BIN  +0x0099B8  0xA462809C  sh  v0,-0x7f64(v1) ; DAT_800e809c = 1
DEMO.BIN +0x001040  0xA062019C  sb  v0,0x19C(v1)
DEMO.BIN +0x00104C  0xA462809C  sh  v0,-0x7f64(v1)
```

each preceded by `lui v1,0x800F` and `li v0,1`. So the guest does have a per-frame cadence lever —
it is just not the quota byte. It **shortens an individual frame** by writing the counter to 1, which
releases the gate after one vblank instead of two, while setting the swap-mode byte so the loop takes
its `0x80050D00` `DAT_1f80019c == 2` arm. It never changes the rate. **Whether either body is reached
in this product is not measured.** This is the reason `consumeVblank()` is read-only: it leaves the
guest's one piece of authored cadence alone.

## Gap

- **The 60 fps mode is not implemented.** The owner holds the lever and reports it; setting the
  decision to 1 is a one-line change with a game-balance consequence (every per-frame constant,
  animation curve and scripted wait in the game was authored against a 2-field logic frame), so it
  is a decision for the operator, not a silent default. The interpolation path the workspace already
  records for this title (`RenderCapabilities::interpolatedNative()`, `PSXPORT_FPS60=1`) is a
  separate, presentation-level thing and is unaffected.
- **No live run evidence.** The product slot was held by another arm working a user-reported
  blocking Spyro defect, so the `cadence` diagnostic line and `endLogicFrame`'s verdict have not been
  observed in a real run. Every claim above is static, from the image, or from the unit test.
- **`endLogicFrame`'s error path is untested at runtime** for the same reason. Its quiet path is
  covered by `tests/test_frame_cadence.cpp`; the log line is not.
- The two overlay writer bodies' load bases are unestablished, so which image (and therefore whether
  the code is reachable) they belong to is open.
