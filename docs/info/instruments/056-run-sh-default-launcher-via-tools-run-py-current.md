---
id: I056
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

./run.sh default launcher via locked bootstrap.py and tools/run.py — current Tomba! 2 project target, dependency/disc/guest instruction path/build identity, and resume contract

## Validated by

2026-08-21: two consecutive real no-argument `PSXPORT_NOWINDOW=1` launches at framework/pin
`eb2465b2` resolved the configured disc, reported guest instruction path 2026-08-12.1 current, built target
`tomba2_port` without any Generating/Building/Linking output, and execed the native port. After the
final watchdog commit, a bounded run at framework/pin `be381503` opened `MOVIE/LOGO.STR` and advanced
through frame 600 without a watchdog timeout; only the external bound's SIGTERM produced a watchdog
interrupt report. `psxport_sync.py --check` and six launcher selftests passed. The historical
compiler-identity negative control was retired on 2026-08-24 because compatible GCC/AppleClang are
player-supported; the missing-disc unit case still refuses. The
same shipping target's `PSXPORT_GPU_SELFTEST=1` gate passed texture phase 20/20 and semi-texture
equations 16/16.

2026-08-21 repin control: a real zero-argument launch at the preliminary framework/pin `9f1bb927`
resolved the configured disc, detected changed recorded binary evidence inputs, re-emitted MAIN.EXE plus all
overlays, built the current `tomba2_port`, launched it, and exited cleanly at the explicit SBS f80
bound. The definitive `692b9b20` pin then passed the same zero-argument path after a clean Clang
22.1.8 rebuild: it reported the final framework commit in-band, found the guest instruction path substrate current,
built/launched the shipping target, and exited cleanly at f80. Evidence:
`scratch/logs/repin_692_default_launcher.log`.

2026-08-24 locked-launcher control (non-launching, because launcher work must not open the game): the
hermetic suite ran the real `run.sh` against a fake `uv` and observed exactly `run --frozen python
bootstrap.py` plus an argument containing spaces. Its mocked production flow selected GCC spellings
without any compiler-version query, configured with `BUILD_TESTING=OFF`, built only the prerequisite
`discdump` and shipping `tomba2_port` targets, propagated the active locked interpreter through
`Python3_EXECUTABLE`, and execed the expected native binary. Negative controls refuse a missing disc
and a missing SDL3_image module;
the Fedora case names the exact user-run `sudo dnf install SDL3_image-devel` remediation. An isolated
non-building CMake configuration then accepted GCC/G++ 16.2.1 and resolved the same locked
`.venv/bin/python`; the game/window was not launched.

## Known failure modes

(none recorded yet)
