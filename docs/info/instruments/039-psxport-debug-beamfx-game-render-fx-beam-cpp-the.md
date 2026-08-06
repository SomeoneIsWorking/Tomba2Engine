---
id: I039
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

PSXPORT_DEBUG=beamfx (game/render/fx_beam.cpp + the SUMMARY line in Render::fieldObjectsRender) — the beam-layer producer census

## Validated by

Validated against BOTH classes, not reasoned about. POSITIVE: 52/42/28/28 producer calls on weapon-impact-bucket / save-sign-softlock / seesaw-weight / walk-dust-puff, each line carrying the node, its kind/uvIdx, both endpoints, the half-extent and the emitted quad's screen bbox — and the bbox it predicts is where the A/B pixel delta actually lands (f652: 84 px in x[153,179] y[120,125] inside the predicted [145.9,117.3]..[188.8,129.3]). NEGATIVE: 0 calls on the other 13 replays, and the SUMMARY line carries the denominator (objListWalk4 live nodes inspected / routed to FUN_8003B704) so a silent run says WHICH of 'nothing asked for it' and 'the producer declined' happened. It also reports emitted=1 with a degenerate span at f650, where the pixel delta is legitimately 0 — the instrument distinguishes 'emitted nothing' from 'emitted a zero-area quad'. Placement matters: an earlier version of this probe lived in Render::objListWalk4 and read 0 everywhere, because that is an OVERRIDE and PSXPORT_GATE=1 runs override gen bodies (see instrument note on the walk/quadrtpt channels).

## Known failure modes

(none recorded yet)
