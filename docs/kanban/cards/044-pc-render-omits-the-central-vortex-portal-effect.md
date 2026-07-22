---
id: 44
title: pc_render omits the central vortex/portal effect in area 15
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the corrected area sweep. Area 15 (golden temple-block room): psx_render draws a dark swirling vortex/portal effect in the centre of the room (plus a ceiling difference); pc_render does not. Same foreground geometry state on both legs (scratch/screenshots/warpsweep/s15_{pc,psx}.png, >8/255 = 39948). A short-lived/animated effect layer with no native producer since the break-first rebuild — same class as the torch flame (#12) and score popup (#18). RE its emitter and own it with a tap; do NOT stamp/tag.
