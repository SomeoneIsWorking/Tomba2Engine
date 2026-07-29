---
id: I025
kind: instrument
status: trusted
created: 2026-07-29
---

## Instrument

abi_extract.py --contract (frame/spill/call-site extractor)

## Validated by

DISTRUSTED AS OF 2026-07-29 — it silently reports frame_size=0 / 0 spills / 0 call sites for functions that HAVE a frame. Trigger: the entry block ends in an unconditional goto (an early-return path). Its reachability pass then marks everything after as dead. Worked example: 0x80092E3C reports 'frame_size = 0 (no sp descent — leaf/no-frame function)', 0 spills, 0 call sites and unreachable_block_count = 13, while gen_func_80092E3C (generated/shard_5.c:15682) descends sp by 40, spills r16/r17/r31 at +24/+28/+32 and calls func_8009A170. port_check inherits the same wrong oracle model and FAILS a byte-identical port because of it. VALIDATE ANY --contract OUTPUT of 0 AGAINST THE GEN BODY: grep the body for 'r\[29\]' and for 'func_' calls before believing 'leaf/no-frame'. A NON-zero frame_size has not been shown wrong.

## Known failure modes

(none recorded yet)
