---
id: 4
title: Dual-view advertises a PSX pane but the SDL_GPU target selector is inert
status: open
symptom: PSXPORT_DUALVIEW=1 logs side-by-side native | PSX render, but the presented picture contains only the native target because gpu_vk_select_target is a no-op and gpu_vk_target_count always returns zero
state_items: S002
tags: render,dualview,psxport,shared-backend
created: 2026-08-27
updated: 2026-08-27
---

## Root cause

The shared SDL_GPU backend currently implements `gpu_vk_select_target(int)` as a no-op and returns
zero from `gpu_vk_target_count(int)`. Tomba! 2's measured rewind/re-render transaction therefore
executes, but both passes address the same native target and there is no PSX pane to compose.

## What was tried / dead ends

A 300-frame `PSXPORT_DUALVIEW=1` run reached gameplay without crashing, but its 960x720
`present_280` remained the native picture only. The successful frame transaction does not prove the
advertised side-by-side output.

## Resolution

TombaRuntime now exits 1 at boot with the exact shared-backend reason instead of advertising output it
cannot produce. The post-extraction refusal run (`scratch/logs/tomba2-extracted-dualview-refusal.log`)
exited 1 before the frame loop, while a rebuilt default native/wide/fps60 run still reached free-roam
at frame 216, reconciled 620/620 fences with zero dropped layers, and exited 0. The issue remains open
until the shared backend owns multiple targets and composition; at that point the title refusal can
be removed and the side-by-side pixels must be checked live.
