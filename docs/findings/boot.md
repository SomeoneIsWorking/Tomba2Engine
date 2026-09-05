# boot / crt0

## The BIOS heap was initialised with ZERO capacity — crt0 never set a1
- **symptom:** every BIOS `A(33h) malloc` in the guest would return NULL; `[hle] InitHeap(base=0x8010622C, size=0)`; nothing visibly broken, because nothing ever asked the heap for memory
- **status:** fixed psxport 726d10c9 (pinned here by 559aeca)
- **cause:** psxport's `crt0_setup` (`native_boot.cpp`, pre-726d10c9) set only `c->r[4]` before `typed runtime address dispatch(c, cfg->libcInit)`. Tomba!2's `libcInit` 0x80089860 is the BIOS **A(39h) `InitHeap(ptr, size)`** thunk (`addiu t2,0xA0; jr t2; addiu t1,0x39`) and the real crt0 computes the size into **a1** before the `jal` (`crt0_extract` reports `a1 live at the call: YES` for MAIN.EXE). `hle.cpp case 0x39` copies a1 straight into `Hle::heap_size`/`blk[0].size`, which `Hle::heapAlloc` treats as the arena's whole capacity — so the missing `c->r[5]` made the arena 0 bytes. Verify the old shape yourself: `git -C $PSX/psxport show 726d10c9^:runtime/psx/native_boot.cpp | sed -n '218,232p'` — `c->r[4]` is assigned, `c->r[5]` never is.
- **fix:** `runtime/psx/crt0_boot.h` now computes the whole plan (`crt0_plan`) and applies it (`crt0_apply`) including `r[5] = heapsz`, `crt0_verify.h::crt0_audit` re-derives the group from the guest's own instruction stream at every boot and refuses a confirmed disagreement, and `tools/crt0_extract` reports the same derivation over an executable through the same `crt0_scan`. Nothing in this repo changed except the pin + the now-explicit `.stackBias = {1, -8}` in `game/core/game_config.cpp`.
- **refs:** psxport 726d10c9 · external/psxport/runtime/psx/crt0_boot.h · external/psxport/runtime/psx/hle.cpp (case 0x39, heapInit, heapAlloc) · megamanx4/docs/issues/0005 (the issue that motivated the change) · docs/info/claims (C044, C045)

## "0 heap refusals" is not "the heap works" — Tomba!2 never mallocs during boot
- **symptom:** `Hle::heap_refused` reads 0 after a full boot, and a reader concludes the heap is healthy
- **status:** known-issue (a reading trap, not a defect)
- **cause:** the zero-capacity heap above was LATENT for exactly one reason: a 428-frame Tomba!2 boot (attract + newgame + 401 frames) made **zero** BIOS malloc calls, so nothing was ever refused. `heap_refused` counts REFUSALS, and 0 requests produce 0 refusals — the counter cannot distinguish a healthy arena from an arena of size 0 that nobody touched.
- **fix:** do not read `heap_refused == 0` as evidence about the heap. The statement that holds is "0 refusals of 0 requests". To make it evidence, first show a request happened — a later stage that does allocate is where a wrong `heap_size` would finally bite.
- **HOW to show it, because the first version of this note asserted "zero malloc calls" with no method** — and an unmethodded negative is the same defect one paragraph up. `heap_refused` cannot answer it and neither can the default log: a SUCCESSFUL `A(33h)` prints nothing. The instrument is **`PSXPORT_DEBUG=bios`**, which logs EVERY BIOS call before the switch (`Hle::dispatchBios`), so the allocation family is greppable and the zero has a denominator:

  ```sh
  PSXPORT_NOPACE=1 PSXPORT_DEBUG=bios python3 tools/gate.py boot --frames 400
  grep -c '\[bios\]' <log>                                  # denominator
  grep -o '\[bios\] A0:0x[0-9A-Fa-f]*' <log> | sort | uniq -c   # the A0-table census
  grep -c 'A0:0x33\|A0:0x37\|A0:0x38' <log>                 # malloc / calloc / realloc
  grep -n 'A0:0x39' <log>                                   # POSITIVE CONTROL — must be non-empty
  ```

  **The positive control is not optional**: `A0:0x39` (InitHeap) is known to happen on every boot, so if it is absent the channel never fired and the malloc zero is "I never looked". Measured 2026-08-12 on gate binary md5 `32fcc50002dd`: **294 `[bios]` lines**, A0 table = `0x44` ×2, `0x72` ×1, `0x70` ×1, `0x49` ×1, `0x39` ×1 (**6** A0 calls), malloc family **0**, and `A0:0x39(0x8010622C, 0x000F99D0, …)` present. Blind spot: one seaside/mode-0 boot only.
- **refs:** external/psxport/runtime/psx/hle.cpp (heap_refused, dispatchBios `bios` channel) · docs/info/claims C045 · scratch/logs/gate-boot-20260812-211111.log
