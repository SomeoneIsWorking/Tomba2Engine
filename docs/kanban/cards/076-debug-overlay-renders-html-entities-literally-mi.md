---
id: 76
title: Debug overlay renders HTML entities literally: "&middot;" instead of "·" in the video/world readouts
status: done
labels: [ui,bug]
created: 2026-08-06
updated: 2026-08-06
---

USER screenshot 2026-08-06:
    render 1398x720 &middot; window 1536x790 &middot; internal 3x
    pos X 13029 Y -2872 Z 7161 &middot; stage GAME (0x8010637C)

CAUSE (two defects, one confusion — "RmlUi speaks HTML entities"):
1. RmlUi decodes ONLY the four XML predefined named entities plus numeric character references.
   StringUtilities::DecodeRml (vendor/rmlui/Source/Core/StringUtilities.cpp) handles &lt; &gt; &amp;
   &quot; and &#NNN;/&#xHH;. HTML4 names are not in it, and ElementText::BuildToken second decoder
   knows only lt/gt/amp/quot/nbsp. Unknown names pass through and render literally. This is upstream
   RmlUi design (RML is XML-ish, not HTML), so the fix is ours, NOT a patch to the vendored lib.
2. rmlui_overlay.cpp snprintf-ed each readout and handed it to SetInnerRML, which PARSES ITS
   ARGUMENT AS MARKUP. Numbers and a stage name are DATA. That is why an entity could appear at all,
   and it also silently broke the "only rewrite when changed" guard: GetInnerRML() returns
   EncodeRml(text), so for any string containing & < > " the comparison was ALWAYS unequal and the
   element was reparsed + relaid-out every frame.

SCOPE MEASURED, not guessed: 45 entity references across the shipped asset corpus + the overlay C++.
24 numeric (all fine). 21 NAMED — 16 &middot; + 2 &mdash; in assets/rml/menu.rml, 3 &middot; in
rmlui_overlay.cpp. EVERY named entity in the corpus was one RmlUi cannot decode; the user saw 2 of 21.
menu.rml is byte-identical in all three game trees (md5 365c3e9d...), so all three shipped it.

FIX (framework, coord/patches/rmlui-overlay.diff):
* NEW runtime/recomp/rml_text.{h,cpp}: rml_text_markup() — the one DATA->markup boundary, delegating
  to RmlUi own EncodeRml so it stays the exact inverse of the parser. All 5 SetInnerRML sites in the
  overlay collapse into one set_text() helper.
* Readouts compose PLAIN TEXT with a real U+00B7 (RML_TEXT_SEP), never an entity.
* assets/rml/menu.rml: the 18 named entities become numeric refs, matching the 24 the file already used.
* Regression gate: tests/test_rml_text_encoding.cpp lints the whole asset corpus for entities RmlUi
  cannot decode (prints its denominator, self-tests that it fires) and asserts the overlay holds
  exactly ONE raw inner-RML call site.

EVIDENCE — headless, PSXPORT_DEBUG=rmlui, REPL `menu on`:
  BEFORE (user screenshot):  "render 1398x720 &middot; window 1536x790 &middot; internal 3x"
  AFTER  (measured):         "render 960x720 · window 960x720 · internal 3x"
                             "pos X 0 Y 0 Z -1750 · stage DEMO (0x801062E4)"
NEGATIVE CONTROL: RmlUi own DecodeRml, the function the document parser applies to text, still turns
the OLD string into one containing the literal "&middot;" (test_old_separator_still_reproduces_the_bug).
Change guard now fires: 1 write per readout across 10 frames with the menu open.
