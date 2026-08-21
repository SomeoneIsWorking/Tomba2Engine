---
id: I056
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

./run.sh default launcher via tools/run.py — current Tomba! 2 project target, disc/recomp/build identity, and resume contract

## Validated by

2026-08-21: two consecutive real no-argument `PSXPORT_NOWINDOW=1` launches at framework/pin
`eb2465b2` resolved the configured disc, reported recomp 2026-08-12.1 current, built target
`tomba2_port` without any Generating/Building/Linking output, and execed the native port. After the
final watchdog commit, a bounded run at framework/pin `be381503` opened `MOVIE/LOGO.STR` and advanced
through frame 600 without a watchdog timeout; only the external bound's SIGTERM produced a watchdog
interrupt report. `psxport_sync.py --check` and six launcher selftests passed. Negative controls:
`CC=gcc CXX=g++` refused before disc/build with exit 1, and the missing-disc unit case refused. The
same shipping target's `PSXPORT_GPU_SELFTEST=1` gate passed texture phase 20/20 and semi-texture
equations 16/16.

2026-08-21 repin control: a real zero-argument launch at the preliminary framework/pin `9f1bb927`
resolved the configured disc, detected changed recompiler inputs, re-emitted MAIN.EXE plus all
overlays, built the current `tomba2_port`, launched it, and exited cleanly at the explicit SBS f80
bound. The definitive `692b9b20` pin then passed the same zero-argument path after a clean Clang
22.1.8 rebuild: it reported the final framework commit in-band, found the recomp substrate current,
built/launched the shipping target, and exited cleanly at f80. Evidence:
`scratch/logs/repin_692_default_launcher.log`.

## Known failure modes

(none recorded yet)
