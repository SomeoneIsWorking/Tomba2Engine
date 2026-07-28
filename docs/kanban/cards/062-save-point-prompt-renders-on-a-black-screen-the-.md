---
id: 62
title: GAME OVER screen keeps drawing field geometry — a stray rope + garbled sprite over the black
status: todo
labels: [bug, render]
created: 2026-07-28
updated: 2026-07-28
evidence: docs/reference/issues/issue62_save_prompt_black_screen.png
---

FOUND 2026-07-28 by PLAYING the game (not by a gate): walked east from the bucket pickup, triggered the save sign, and the 'Save? / Yes / No' prompt came up over an almost entirely BLACK screen. The field — terrain, hut, trees, water — is gone. Only a rope/vine, a small garbled colourful sprite cluster near the text, and the prompt text render.

REPRO (deterministic, verified by re-running it from scratch):
  PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=5965 \
    PSXPORT_PAD_REPLAY=replays/bugs/save-prompt-black-screen.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
then vkshot at the end of the replay (~f12500). Reproduced byte-for-byte on a fresh process.

MEASURED AT THE LOCK-IN FRAME (f12887):
  - 'scene' (guest display list): the OT holds ONE entry, a FILL rgb=(0,0,0) 320x240 at (0,0).
    poly=0 rect=0 line=0 fill=1 vramcopy=0 upload=0 env=6. So the GUEST emits no field geometry
    at all in this state — the black is the guest's own full-screen fill, not a pc_render dropout.
  - 'vkstats': the native renderer still drew textured=1632 verts (544 tris) + semi=18 (6 tris) —
    that is the rope + sprite cluster + text visible in the shot.
  - stage=8010637C sm48=2 scene-active=2, cut-mode 0x1F800137=0 (not a cutscene).
So two candidate readings, and they are NOT the same bug:
  (a) the field is SUPPOSED to be drawn behind the prompt and something suspended it, or
  (b) black IS the guest's intent for this sub-mode and the 544 tris pc_render still draws are the
      spurious part.
Either way the picture is wrong: a save prompt over a black void with one floating rope is not
what the game shows.

REFERENCE COMPARE IS STILL OPEN and is the next step. It cannot be done by replaying this pad on
the gate leg — the pad is only valid on the pc_skip=true timeline (same trap as kanban #2), so the
oracle must be DRIVEN to this save sign by hand (PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 + debug-server
press/wait; the sign sits near x~12700 z~3968 in area 0, past the water pump — jump the seesaw). An
attempt was started and abandoned mid-drive; the oracle starts at x=3940 and gets blocked at the
pump (x=6000) and again at x=7922.

TOOLING NOTE worth keeping: driving via the debug server, a held button is NOT consumed by 'step'
while PAUSED — 270 stepped frames with 'press right' moved the player 125 units. Drive with
'play' + real-time waits instead; 'step' is for freezing a frame to inspect it.

**2026-07-28:** 2026-07-28 SAME SESSION — CORRECTION, MOST OF THIS CARD'S PREMISE IS WRONG. This is the GAME OVER / CONTINUE screen, not a broken save point, and it is NOT softlocked.

WHAT IT ACTUALLY IS: walking east I died, and the game entered its game-over flow — the state machine is FUN_80106478, driven off the GAME task at 0x801FE000 with the sub-state at +0x4C and the menu cursor at +0x4E. Case 3 draws the prompt via FUN_8007ED5C(cursor) and reads the PRESSED edge mask 0x800E7E68: 0x10 = Up (cursor--), 0x40 = Down (cursor++), 0x4000 = CROSS = confirm. The prompt strings live in one table at 0x80010A84 — 'Select Options', 'OK to quit game?', 'Continue', 'No', 'Yes', 'Save?', 'Quit game', 'Load data', 'Options' — i.e. the shared pause/game-over UI table, not a save-point string.

IT RESPONDS FINE. Measured live: +0x4C = 3 (prompt) with +0x4E = 1; one CROSS tap -> +0x4C = 4; another -> +0x4C = 2, and the game returned to the field with Tomba respawned at the starting hut. Down had already moved the cursor 0 -> 1 earlier. So the earlier 'no response to Circle/Cross/Start/Down' reading was wrong — this menu confirms on CROSS (keyboard K), and the screenshots I compared did not show the cursor change. The black background is the game-over screen's own; the guest OT holding exactly one black FILL is that screen, not a dropout.

RULED OUT along the way (do not re-walk): beh_scene_ui_trigger 0x800739AC forced to substrate (PSXPORT_BEH_SUBSTRATE=800739AC) reproduces identically, so it is not that handler; no '[sched] caught a GAME substate yield' anywhere in a full PSXPORT_DEBUG=sched run, so it is not the kanban #50 truncation class; no CD/overlay load happens at the prompt (all overlay reads are at boot frames 89-128), so no missing overlay; area 0x800BF870 = 0 and target 0x800BF871 = 0, so no stuck warp; the case-3 save globals (0x1F800136, 0x800BF890..) are all 0, so beh_scene_ui_trigger's own save path never ran.

WHAT IS STILL A REAL BUG, and all this card should now be: on that black game-over screen, pc_render still draws FIELD GEOMETRY that does not belong there — a rope/vine hanging from the top of the screen and a garbled multi-coloured sprite cluster beside the text. The guest emits ONE black FILL and nothing else, while vkstats reports textured=1632 verts (544 tris) + semi=18. The prompt text itself is native (the guest draws no glyphs), so some of those tris are legitimate; the rope and the sprite blob are not. Native producers are not being suppressed when the field stops being the scene. Evidence image and the repro replay are unchanged and still valid.

INSTRUMENT NOTE: provat is BLIND on this path — it reads PSX VRAM, which pc_render never writes, so every probe came back '<never written>' over pixels that are visibly lit.
