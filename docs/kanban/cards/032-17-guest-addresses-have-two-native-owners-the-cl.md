---
id: 32
title: 17 guest addresses have two native owners — the class that caused #28
status: todo
labels: [workflow]
created: 2026-07-22
updated: 2026-07-22
---

tools/codemap.py --conflicts reports 18 cross-file duplicate-owned guest addresses; one of them (0x8004FFB4) caused #28 today — the dialog box drawing over its own text — and is now down to 17. This is a nasty class because NOTHING catches it automatically: a second owner does not fail the build, does not fail SBS (both owners write the same guest state, so the byte-compare stays green), and surfaces only as a picture bug or a subtle behavior change days later. codemap --conflicts is the only detector. Two things to do. (1) Triage the remaining 17 — each is either a deliberate pc_skip fork (those are supposed to live in ONE file, so a cross-file pair is never that), a stale ORPHAN transcription that should be deleted per no-tombstones, or a live duplicate like 0x8004FFB4. Known-suspicious from the listing: 0x8007FCC8 Panel::pushDialogBackdrop vs Render::optionsSolidBox, 0x80115598 Render::backdropRender vs TileGridLayer::emit, and a cluster of Render::options* vs leaf_* in game/core/field_owned_leaves.cpp. (2) Make it impossible to reintroduce: wire codemap --conflicts into a pre-commit check the way codemap.py check already flags coverage drift, so adding a second owner FAILS instead of shipping.
