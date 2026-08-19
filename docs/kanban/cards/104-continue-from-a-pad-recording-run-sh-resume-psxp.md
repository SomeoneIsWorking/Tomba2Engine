---
id: 104
title: Continue from a pad recording — ./run.sh --resume (PSXPORT_PAD_RESUME)
status: done
labels: [tooling]
created: 2026-08-19
updated: 2026-08-19
---

USER ask, 2026-08-19: "I would like a feature where I can continue from a pad recording instead of having to play all over again".

IMPLEMENTED. PSXPORT_PAD_RESUME=<path> (framework: pad_input.{h,cpp}, gpu_native.cpp, native_fmv.cpp, spu_audio.cpp) = the existing replay mechanism plus FAST-FORWARD until the recording is spent: pacer off, FMVs uncapped, rendered audio dropped instead of queued. All four consumers read ONE predicate, Pad::fastForwarding() (mResumeFf && rec_fc < rep_n), so speed, sound and control hand back on the same frame — there is no window where the player drives a fast-forwarding game. Recording stays ON during a resume, so the session you end becomes the next resume point (a plain PAD_REPLAY still suppresses it). Front door: ./run.sh --resume [file.pad]; with no path it snapshots scratch/bin/pad_session.pad to pad_resume.pad first, because launching rotates the live sink before the replay source is opened.

Deliberately a SECOND knob, not a flag on PAD_REPLAY: a gate/repro must run at real speed (the NOPACE lesson) and getting back to a spot wants all the speed there is. Which you meant is stated, never inferred. Both set = warn + use RESUME. A resume whose file fails to load refuses the fast-forward out loud instead of sprinting through a fresh boot.

GATE (bucket-softlock.pad, 1764 frames + free-run tail, headless): PAD_REPLAY 77.1s vs PAD_RESUME 32.6s; handover logged at pad frame 1764. Pacing PROVEN to resume rather than the run staying unpaced: extending the post-handover tail by 600 frames added 20.1s = 29.9 fps, the paced field rate. FF rate ~110 native fps (CPU-bound, 3.7x real time).

NOT VERIFIED: the audio drop with a real device open (every leg here was headless, where the SDL device is never opened) and the windowed path end to end — the user's first ./run.sh --resume is that test.
