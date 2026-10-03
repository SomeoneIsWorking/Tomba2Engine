# Boot SIGSEGV in `NativeDispatcher::invoke` — reproduces on psxport HEAD

**Kind:** bug
**Status:** open

## Symptom

Launching `tomba2_port` through the headless automation path aborts during boot, before any frame is
presented:

```
[watchdog] FAULT (signal): signal = 11
psx3cpu::cpu::NativeDispatcher::invoke(psx3cpu::cpu::NativeKey)
psx3cpu::cpu::dispatchGuestHostService(Core&, unsigned)
psx3cpu::cpu::dispatchGuest(...)
Engine::frameUpdate()
tomba::TombaFrameDriver::stepFrame(Core&, unsigned)
native_boot_run(Core*)
BootStub::run(char const*)
main
```

Exit code 139. It happens with `fps60=0` and `fps60=1` alike, so the temporal product is not involved.
The log also names a missing `PSXPORT_ASSET_DIR`; setting it to `external/psxport` does not change the
outcome.

## Not a framework regression

Checked against an exported psxport HEAD with none of the in-flight framework work applied
(`git archive HEAD` into `scratch/tomba-head`, the `vendor/beetle-psx` and `vendor/lucent` submodules
symlinked, configured with `-DPSXPORT_DIR=<export>`, built as `build-head`):

| binary | result |
|---|---|
| `build-head/bin/tomba2_port` (psxport HEAD) | rc=139, identical call chain |
| `build/bin/tomba2_port` (working framework) | rc=139, identical call chain |

So this is a Tomba issue, not a framework one, and it is not a regression from the
`InBetweenStrategy` seam change.

## Where to look

`NativeDispatcher::invoke` is reached from a guest HOST SERVICE during `native_boot_run`, so the crash
is in the boot-time dispatch path rather than in the game's own code. The reproduction is a direct
launch with the automation environment (`tools/drive_to_grab.py`); it needs the disc and the boot
inputs a windowed `./run.sh` provisions, and this configuration does not supply all of them.

## Acceptance

A direct headless launch reaches the first frame boundary. The 4:3 `fps60=0` real frame stays
byte-identical, which is the gate the temporal work re-checks on every change.
