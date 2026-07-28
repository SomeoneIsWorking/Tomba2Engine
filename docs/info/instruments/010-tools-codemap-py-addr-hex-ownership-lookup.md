---
id: I010
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/codemap.py --addr <hex> ownership lookup

## Validated by

Fed it a case that MUST differ: 0x8007D594 is installed via overrides::install and ran 963x on replays/bugs/bucket-softlock.pad. Before the fix --addr said 'NO native owner found'; after load_installs() it names DialogBoxSm::step at game/ui/dialog_box_sm.cpp:24. Totals moved 777->779 owned / 735->737 LIVE, --dup-installs still 0. ROOT CAUSE of the lie: the parser keyed on the retired ov.register_(0xADDR,"sym",fn) idiom, which matches ZERO sites in the tree, while 24 overrides::install sites were invisible. Any pre-2026-07-28 'NO native owner found' verdict is suspect — re-run it.

## Known failure modes

(none recorded yet)
