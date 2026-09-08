---
id: 7
title: DrawSync runtime call exhausts its budget after the first native frame
status: resolved
symptom: The second Tomba native frame aborts inside gpuDmaQueueSync before the recorded gameplay frontier
state_items: S001
tags: tomba2,dynarec,lightrec,gpu,dispatch
created: 2026-09-05
updated: 2026-09-08
---

## Reached boundary

PSXPort `eb5f23a8b3506f8853b3cfadcedc024cd90818a0`, Lightrec
`b1457137c31cedff5f440d59da29401d021ba2da`, and the final title-native image
bindings complete native initialization and frame 0 at DEMO `0x801062E4`.
After the complete generated-body removal, a silent headless observation with
`PSXPORT_NATIVE_FRAMES=2` and no auto-input reproduces the next-frame failure:

```
gpuDmaQueueSync required a completed guest call, but execution exited as
budget-exhausted at 0x80044E54 after 564488 cycles: cycle budget exhausted
```

The reached native stack crosses `TombaFrameDriver::stepFrame`, the DrawSync
guest/native dispatch boundary, and `Render::gpuDmaQueueSync`. Its shared error
label did not distinguish timeout-arm/check/drain calls. The timeout arm at
`0x800834A0` is PlatformHle-owned; the reported PC lies inside the LZ
decompressor at `0x80044D8C`. Neither adding a GPU override nor increasing the
turn budget addresses the scheduler failure established below.

The title had a separate proven lifecycle defect: main bound native entries
before `BootStub` reloaded the executable and advanced its image generation.
Binding the resident declarations during `TombaRuntime::bootInit` restores them
after that reload. Following generated-body removal, 199 of 254 declarations
bind to the resident image, with 55 inactive nonresident addresses.
That restoration is covered by the production-catalog regression's stale-image
negative, idempotent rebind, and scoped original call through Lightrec. It changes
the reached stack to the native DrawSync owners but does not resolve this next
execution failure.

## Root cause and focused evidence

The shared `guest_run_coro_fiber_stanza` dispatched its guest task once, passed
`BudgetExhausted` to `completeOrPropagate`, and let the coroutine return. It then
deleted that unfinished task. `PcScheduler::step` continued and restored only
R3000 state; the pending `ExecutionControl` result survived. The next GPU timeout
arm completed its real host work, then `invokeNativeFunction` consumed that prior
result and attributed the LZ budget exit to the GPU call.

The dedicated `tomba_gpu_timeout_dispatch` target uses the production Tomba
configuration, `PlatformHle`, dispatcher, coroutine stanza and Lightrec. A finite
synthetic task calls a nested guest worker for one million iterations, so its
live return address differs from its outer task sentinel while budgets expire.
Before the fix, 3/20 assertions failed: the task was deleted (`state=0`), its
budget exit leaked, and the next real GPU arm returned `BudgetExhausted` at the
synthetic worker PC `0x8001103C` after 564,482 cycles. Direct/nested GPU calls and
an authored cooperative native yield already passed. The terminal-fault
subprocess also failed: an unmapped task returned exit 0 instead of stopping at
its scheduler owner.

The shared scheduler now preserves the original task-return boundary and exact
PC/register context across budget turns. It parks the coroutine only after
`ExecutionResult` and executor guards have been destroyed, because coroutine
cancellation uses `longjmp`. Only a real guest return or authored cancellation
ends the task. Unsupported task exits fail at the scheduler with their actual
reason and PC. Other typed task-to-host boundaries, including frame/thread yields
and process exit, remain unsupported host-loop lifecycle work. The budget path
changes only a still-running state 4 to runnable state 2; guest-authored state 0
ends the task, while sleep/restart states remain with the stanza/countdown owner.
An additional guest-written cancellation/sleep/restart discriminator failed 8/59
assertions before this state preservation correction and passes afterward.

Verified with Clang on 2026-09-08 after moving generic regressions into the
framework's normal test suite:

- Framework: `ctest --test-dir build/runtime-contract --output-on-failure --timeout 25 -R '^(test_guest_task_lifecycle|guest_task_unsupported_exit)$'`: 2/2 pass, 46/46 native assertions.
- Tomba: `ctest --test-dir build/final-pin --output-on-failure --timeout 25 -R '^tomba_gpu_timeout_dispatch$'`: 1/1 pass, 14/14 native assertions.
- Framework `tests/test_guest_task_lifecycle.cpp` owns exact completion after
  multiple budgets, nested PC/SP/return preservation, cooperative native
  yield/resume, cancellation/restart of a budget-suspended task, and guest-authored
  cancellation/sleep/restart before a budget boundary. Its synthetic configuration
  uses the layout constants still declared by the legacy countdown owner.
- Tomba retains production GPU binding, direct/nested/rebind/adjacent-address
  checks, and the budget-suspended-task to next-GPU-service discriminator.
- Framework execution reports 28 executor calls, 2,282,243 executed blocks and
  6,846,764 instructions; Tomba reports 1 call, 141,120 executed blocks and
  282,240 instructions. Both report zero interpreter fallback blocks/instructions.
- Framework `guest_task_unsupported_exit` refuses an unmapped guest task at its
  scheduler owner with `fault` and exact PC `0x80018000`.
- Touched C++ sources pass clang-tidy using their real compile databases.

## Integrated real-title confirmation

The final canonical title verifier passed 22/22 tests, complete title C++ policy,
and execution-boundary selftest/product checks against psxport
`a5a796521668cf078e150808cc1fc4616d1f31d6`:

```
CC=clang CXX=clang++ CMAKE_BUILD_PARALLEL_LEVEL=2 uv run --frozen python tools/verify_ci.py --build build/final-pin
```

The unchanged build performed zero compilations. One silent headless observation
of `build/final-pin/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE`, using the shared
`agent_environment` with `PSXPORT_NATIVE_FRAMES=2`, `PSXPORT_REPL=0`,
`PSXPORT_DEBUG=cam` and `PSXPORT_WATCHDOG=45`, exited 0 after 12.60 seconds. No
auto-input or FMV bypass was enabled. The renderer reported a headless native
SDL_GPU sink. Both completed frames were logged (`f0`, `f1`) at DEMO `0x801062E4`;
after frame 1, task 1 remained runnable (`state=2`). The native loop and boot stub
returned normally instead of attributing a task budget exit to the GPU.

Normal-exit telemetry reported 269 executor calls, 51,483 executed blocks and
426,137 instructions, with zero fallback blocks/instructions and zero entries
for every fallback/refusal reason. The issue's real second-frame failure is
resolved; this short startup observation does not restore the historical
free-roam frontier or establish gameplay conformance.

The direct observation omitted the launcher's `PSXPORT_ASSET_DIR` environment,
so the RmlUi overlay reported 0/4 resources present and no fonts/menu. This
limits the observation to the reached execution boundary; it does not qualify
UI resources, menu behavior, presentation fidelity or packaging. The shared
configuration audit also labeled `PSXPORT_VK_HEADLESS` unread, while the renderer
itself explicitly reported its headless sink. Issue 0005's authenticated
resident/overlay original-call discriminator remains open.
