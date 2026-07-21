---
id: 9
title: pc_skip should skip the LOADING SCREEN entirely (not just its text)
status: todo
labels: [pc-skip, enhancement]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** USER 2026-07-22: with pc_skip ON the loading screen should be skipped as a STATE - the host file read is already complete, so the screen is a wait that does not exist. Clarified after a first attempt only stopped the 'Loading.....' string from drawing (game/ui/loading_text.cpp), which leaves the same screen up for the same duration, just blank - that change is REVERTED and the behavior-map entry loading-text-skip is marked reverted. The right fork is one state up: whatever drives the loading screen's lifetime should collapse under pc_skip, in the shape 'pc_skip ? skipState() : faithfulState()'. Starting point: LoadingText::draw (FUN_8007FD54, RE'd + port_check PASS) is the drawer - find its caller and fork THAT. It ran once in the bucket capture, so ovhit on that replay will name the caller.
