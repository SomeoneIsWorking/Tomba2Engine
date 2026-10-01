---
id: kanban-031
title: fps60: interpolated frames and real frames appear to be built differently
status: open
labels: [bug, render, fps60]
---


USER 2026-07-22: 'it still feels like lerp frames and regular frames are created differently but that should never be the case'. This is the standing fps60 architecture directive stated as a symptom: ONE render path for both frame kinds, the interpolated one running a frame behind on lerped actor transforms. If the two kinds diverge structurally then that single defect is a candidate root for FOUR open cards at once — #16 sign-text jitter, #17 barrel flicker, #20 black screen when paused, #23 roof flames not lerping — which is why it is worth settling before working any of them individually. Do not fix them one at a time until this is answered: establish first whether a real frame and an interpolated frame produce the same RqItem set modulo the lerped transforms, and if not, WHERE they part company.
