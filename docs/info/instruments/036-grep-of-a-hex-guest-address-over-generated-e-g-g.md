---
id: I036
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

grep of a HEX guest address over generated/ (e.g. grep -r 800A5ADC generated/)

## Validated by

DO NOT TRUST IT FOR ABSENCE — it returns 0 hits for addresses the code demonstrably uses. The recompiler never emits a guest address as hex text: it emits the lui/ori pair in DECIMAL and split, so 0x80105EE8 appears as 'c->r[2] = (uint32_t)32784u << 16;' followed by '+ (uint32_t)24296'. MEASURED 2026-08-06 over 98 generated/*.c files, 1,165,755 lines: '105EE8', '0x80105EE8', '800A5ADC' and '0x800A5ADC' each score ZERO hits, while gen_func_8009A450 (generated/shard_2.c:13598-13611) provably reads AND writes 0x80105EE8 as an LCG seed, and '32784u << 16' alone occurs 95 times in that one shard. This nearly produced a false falsification of claim C018 in the same session — the grep said 'the seed is never touched' and the 14-line body said otherwise. USE INSTEAD: locate the gen symbol by name (gen_func_<ADDR> / ov_<area>_gen_<ADDR>), brace-match the body, and read it; or decompose the address as (addr>>16) and (addr&0xFFFF) in decimal and grep for those. NEGATIVE CONTROL RUN: the same body-reading method returns a POSITIVE for 0x80105EE8, so the method that replaces the grep is known to be able to say yes.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

Trustworthy for a POSITIVE (a hit is a real hit) but structurally blind for a NEGATIVE, which is how it is almost always used. It cannot report absence: 0 hits over 1,165,755 lines for four addresses the code provably uses, because the recompiler emits addresses as split DECIMAL lui/ori, never as hex text. Any past conclusion of the form 'address X never appears in generated/' that rests on this grep must be re-checked by reading the gen body.

> Every result this instrument produced is suspect until it is re-validated.
