---
id: I054
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

CTest tomba_cpp_policy — shared psxport C++ format, structure, and clang-tidy verifier over the real Tomba compile database

## Validated by

2026-08-21: clean Clang build passed 385/385 tracked first-party format/size files and clang-tidy 255/255 compile-backed first-party C++ TUs; negative control first rejected three seeded real diagnostics (copied std::function, inefficient string concatenation, integer-to-pointer trace reconstruction), then passed after fixes. GNU C++ configure independently refused with exit 1.

## Known failure modes

(none recorded yet)
