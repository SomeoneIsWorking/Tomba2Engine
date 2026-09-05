---
id: 92
title: PSXPORT_ENH is ENV-ONLY and off the CVar ladder — cfg_enh() cannot satisfy the USER's CVars ruling, so the first pc_enh consumer had to duplicate its suppression
status: todo
labels: [psxport, config, debt, enhancements]
created: 2026-08-12
updated: 2026-08-12
---

Found 2026-08-12 while bootstrapping the megamanx4 tree, which is the workspace's FIRST real pc_enh consumer.

MEASURED: 'grep -rn cfg_enh( --include=*.cpp runtime/ ../Tomba2Engine/game ../spyro/game ../spider1/game' returns exactly ONE line — runtime/psx/cfg.cpp:197, the definition itself. **cfg_enh() has zero call sites anywhere in the framework or in any of the four game trees**, and both enhancement names registered in psxport's docs/config.md are marked 'planned'. So the pc_enh class has never been exercised; Mega Man X4 is the first thing that needs it, because that port IS three enhancements (widescreen, load removal, drop-in co-op) and nothing else.

THE DEFECT. cfg_enh reads lucent::config::text("PSXPORT_ENH") directly into a function-local seeded static. That puts it OFF the CVar ladder entirely: no Value (settings-file) layer, no Runtime (REPL) layer, no appearance in the CVar registry dump, and no row in the environment audit. The USER's standing ruling is 'use cvars not cfg_str' — and cfg_enh structurally cannot satisfy it.

THE CONSEQUENCE, already paid: megamanx4/game/core/enhancements.cpp declares its three knobs as psx::config::BoolVar and REPRODUCES cfg_enh's suppression rule (cv_oracle, cfg_on("PSXPORT_SBS"), non-empty PSXPORT_SBS_MODE) rather than calling it. That duplication is deliberate and documented at the site with the reason, but it is duplication of the definition of 'what a byte-compare run IS' — and that is the worst thing to have two copies of. If the two ever diverge, one of them will fail to recognise an SBS variant, which means a contaminated compare that still looks clean. A fake green.

THE FIX IS UPSTREAM AND A GAME REPO MAY NOT MAKE IT (which is why it is here): migrate PSXPORT_ENH onto the CVar ladder, keeping the ORACLE/SBS suppression as an explicit resolve-time hook with its own log line, per psxport's own docs/config-migration.md 'Qualification 2'. Then DELETE the duplication in megamanx4/game/core/enhancements.cpp — and delete its HONEST NOTE ON THE DUPLICATION comment at the same time, rather than leaving a tombstone saying the duplication used to exist.

WHEN DOING IT, PRESERVE THE ONE THING THE X4 COPY DOES BETTER: it warns ONCE PER KNOB keyed on the CVar's own identity, not once globally. A run with two enhancements set must name BOTH, or the second reads as never having been asked for. cfg_enh's single seeded static cannot express that.
