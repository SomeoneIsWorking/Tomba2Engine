---
id: 3
title: RmlUi not working
status: done
labels: [ui]
created: 2026-07-17
updated: 2026-07-17
---

USER report 2026-07-17: RmlUi (vendored UI lib, external/psxport/vendor/rmlui; built via tools/build_rmlui.sh) not working. Symptom unspecified — need repro: build failure vs runtime (overlay not rendering / crash)? Investigate after READMEs.

**2026-07-17:** ROOT CAUSE: repo split moved assets/rml/ into the psxport submodule, but RmlOverlay disk-loads fonts+menu.rml via cwd-relative "assets/rml/..."; game runs from repo root where ./assets/rml doesn't exist -> LoadFontFace/LoadDocument fail -> blank overlay. FIX: rml_asset() resolves paths against PSXPORT_ASSET_DIR (cfg_str, default cwd); run.sh sets it to external/psxport; documented in config.md. Verified: build clean + resolved paths exist. Windowed overlay eyeball = USER gate.
