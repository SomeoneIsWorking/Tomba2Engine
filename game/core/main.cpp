// game/core/main.cpp — the Tomba!2 process entry point.
//
// main() composes the process-lifetime TombaRuntime, generated-substrate registry, and framework
// machine. Game-specific behavior is reached through inheritance; the framework provides no main().
#include "cfg.h"
#include "core.h"
#include "dualcore.h" // class DualCore — NATIVE-render vs PSX-render RAM divergence harness
#include "fs_util.h"  // Fs::exists — MAIN.EXE presence probe for self-provisioning below
#include "game.h"
#include "platform_hle.h" // class PlatformHle — HW-sync HLE table (VSync/CdSync/MDEC/ChangeThread)
#include "recomp_register.h"
#include "sbs.h" // class Sbs — the PSXPORT_SBS live-two-core divergence debugger
#include "tomba_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C subsystems (compiled as C) reached across the boundary — declare with C linkage.
extern "C" {
void watchdog_init(void);
void mdec_init(void);
void spu_init(void);
}

void load_exe(const char *path, Core *c); // runtime/recomp/boot.cpp (framework)

int main(int argc, char **argv) {
  // Installation precedes every Game: Core snapshots the runtime and its bounded compatibility views.
  static tomba::TombaRuntime runtime;
  psxport_install_game(runtime);
  tomba_install_recomp(); // install the generated-substrate seam (main_dispatch/overlay table/setters)
  const char *path = argc > 1 ? argv[1] : "scratch/bin/tomba2/MAIN.EXE";
  Game *game = new Game(); // the whole machine (owns the Core + every subsystem's state — no globals)
  Core *c = &game->core;   // the CPU/RAM handle threaded through the interp (2 MB RAM lives in Game)
  // Self-provision MAIN.EXE: anyone with just a CHD (drop-in *.chd in the repo root, or
  // PSXPORT_TOMBA2_DISC / .env) can run the binary directly — no prior ./run.sh extraction step.
  if (!Fs::exists(path)) {
    cfg_logw("boot", "%s missing — extracting from disc", path);
    if (!disc_extract_file(&game->disc, "\\MAIN.EXE", path)) {
      cfg_loge("boot",
               "extraction failed: provide a disc (PSXPORT_TOMBA2_DISC, .env, or a *.chd in the working directory) or "
               "run ./run.sh");
      return 1;
    }
  }
  // The product has one execution policy: owned waits and I/O finish synchronously. The generated
  // substrate remains available through PSXPORT_ORACLE / the comparison harness, not a second user
  // launch mode with different cadence.
  c->game->gpu_vk.tritest(); // PSXPORT_VK_TRITEST=1: GPU triangle-rasterizer self-test, then exit
  watchdog_init();           // PSXPORT_WATCHDOG=<sec>: abort+backtrace if a frame stalls
  load_exe(path, c);
  void games_tomba2_init(void);
  void card_overrides_init(Game *);
  void threads_init(Core *);
  void threads_register_overrides(void);
  void gte_init(void);
  gte_init();                        // GTE (COP2) coprocessor, lifted from Beetle
  mdec_init();                       // MDEC video decoder (FMV), lifted from Beetle
  spu_init();                        // SPU audio core, lifted from Beetle
  game->spu_audio.init();            // SDL audio output sink (PSXPORT_NOAUDIO to disable)
  game->gpu.gpu_native_init();       // native GPU renderer (parses the game's GP0 stream)
  game->cd.overridesInit();          // native CD: drive-ready + by-LBA read (S3)
  games_tomba2_init();               // Tomba2 per-game overrides (vblank pacing)
  game->platform_hle.initBuiltins(); // HW sync/wait stalls -> native non-stall (VSync/CdSync/MDEC)
  c->game->pad.overridesInit();      // native controller input (per-VBlank pad read override)
  card_overrides_init(game);         // native memory card (synchronous file-backed libcard I/O)
  threads_init(c);                   // native BIOS threads (ucontext); main = slot 0
  threads_register_overrides();
  c->r[4] = 1;
  c->r[5] = 0; // a0=argc-ish, a1=argv (BIOS sets these; minimal)

  // PSXPORT_DUALCORE: NATIVE-render vs PSX-render guest-RAM divergence harness (dualcore.cpp). Creates its
  // own Game instances, so the primary `c`/`game` here is left unused.
  if (cfg_on("PSXPORT_DUALCORE")) {
    DualCore dc;
    dc.run(path);
    return 0;
  }
  // PSXPORT_SELFTEST: headless TDD regression (selftest.cpp). Owns its own Game instance.
  if (cfg_str("PSXPORT_SELFTEST")) {
    int selftest_run(const char *exe_path);
    return selftest_run(path);
  }
  // PSXPORT_SBS: LIVE side-by-side two-core divergence debugger (sbs.cpp). Owns its own Game instances.
  if (cfg_on("PSXPORT_SBS") || cfg_str("PSXPORT_SBS_MODE")) {
    Sbs::run(path);
    return 0;
  }
  // Plain (no-harness) path: THIS `game` is the one actually driven, so register its overrides into the
  // registry here (only on this path — the harnesses above register on their own Games via dc_boot_init;
  // registering unconditionally would corrupt `ovhit`'s target selection — see docs/findings/animation.md).
  c->runtime->registerOverrides(*game); // ALL override clusters — game/core/register_overrides.cpp
  game->stub.run(path);                 // stub draws SCEA, then hands off to native MAIN boot
  cfg_logi("boot", "boot stub returned");
  return 0;
}
