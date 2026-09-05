---
id: C065
kind: claim
status: holds
created: 2026-08-26
tags:
depends: game/ui/font.cpp#Font::iconGlyphEmit, game/ui/icon_glyph_selftest.cpp#run_icon_glyph_selftest
---

## Claim

Font::iconGlyphEmit reproduces FUN_80078988 guest state exactly across the 98-case icon-token differential

## Evidence

PSXPORT_SELFTEST=iconglyph compared all 2 MB RAM, 1 KB scratchpad, r0-r31 and hi/lo against test-only reference execution of MAIN.EXE for empty/direct-letter/direct-digit/newline/table-miss/all 90 real table-hit cases plus both synthetic combining-mark variants: 0 mismatches; host opposite-answer control queued 1 direct glyph, 2 for each combining variant with the expected U=56/64 mark, and 0 for an unmapped token

## What would falsify it

Any change to Font::iconGlyphEmit, its icon packet helpers, the iconglyph selftest, or the authenticated executable/overlay evidence oracle requires rerunning PSXPORT_SELFTEST=iconglyph
