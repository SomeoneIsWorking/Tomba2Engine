---
id: 4
title: Widescreen-from-boot corrupts objects (flower/gem, attack weapon)
status: done
labels: [render]
created: 2026-07-17
updated: 2026-07-17
---

ROOT CAUSE: gpu_blank_display blanked VRAM cols [320,nw) at display Y when wide = the texture ATLAS, not framebuffer -> zeroed object textures/CLUTs. Only when STARTED wide (loader runs wide over resident atlas); 4:3-then-widen spared it. FIX: blank only native 320 (psxport 08f05d0f). Verified: gem restored + title margins clean.
