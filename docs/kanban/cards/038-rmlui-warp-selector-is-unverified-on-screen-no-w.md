---
id: 38
title: RmlUi warp selector is unverified on screen — no window in the agent environment
status: todo
labels: [verification]
created: 2026-07-23
updated: 2026-07-23
---

The Debug-tab Area Warp rows (external/psxport e34d4593) are proven at the level below the UI: hooks resolve, build clean, the range guard fires with the right message, and warp 12 loads the ghost pig arena. What is NOT proven is the ROWS THEMSELVES — that they appear in the Debug pane, that Up/Down reaches them, that Left/Right steps the destination and renders '12 - Ghost Pig boss', that Enter on 'Warp There' arms it, and that the readout line updates. None of that could be exercised here: there is no window in this environment and Xvfb cannot host one (no DRI3, Vulkan never acquires a swapchain — established during kanban #20). USER verification needed: ESC to open, Debug tab. If a row misbehaves the suspects in order are (a) the new rows are select-buttons so RCSS tab-index auto should pick them up, (b) warp_area is special-cased BEFORE the generic Mods adjust path in THREE places — activateFocused and both arrow handlers — and a missed one would make the row inert in that direction only, (c) warp_readout is looked up by id and silently skipped when absent.
