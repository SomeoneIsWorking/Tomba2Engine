#include "cfg.h"
#include "core.h"
#include "frame_loop_shell.h"
#include "game.h"
#include "hw_bind.h"
#include "render_mode.h"
#include "stream_field_turn.h"
#include "tomba1_runtime.h"

#include <cstdio>
#include <filesystem>
#include <lucent/log.h>
#include <memory>
#include <string_view>

extern "C" {
void mdec_init();
void spu_init();
void watchdog_init();
}

void gte_init();
void load_exe(const char *path, Core *core);

namespace {

constexpr const char *kDefaultExecutable = "scratch/bin/tomba1/SCUS_942.36";
constexpr const char *kDiscEnvironmentKey = "PSXPORT_TOMBA1_DISC";

bool isHelpRequest(int argc, char **argv) {
  return argc == 2 && (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help");
}

void printUsage() {
  std::puts("Usage: tomba1_port\n"
            "Launch the provisioned Tomba! USA product using PSXPORT_TOMBA1_DISC.\n"
            "\n"
            "Options:\n"
            "  -h, --help  Show this help and exit");
}

} // namespace

int main(int argc, char **argv) {
  if (isHelpRequest(argc, argv)) {
    printUsage();
    return 0;
  }
  if (argc != 1) {
    lucent::error("boot", "tomba1_port takes no executable override; provision the verified Tomba! USA disc");
    return 2;
  }
  if (!std::filesystem::is_regular_file(kDefaultExecutable)) {
    lucent::error("boot", "{} is absent; run titles/tomba1/tools/provision.py first", kDefaultExecutable);
    return 2;
  }
  if (!cfg_str(kDiscEnvironmentKey)) {
    lucent::error("boot",
                  "{} is unset; select the verified Tomba! USA disc explicitly (generic PSXPORT_DISC and drop-in "
                  "fallbacks are refused)",
                  kDiscEnvironmentKey);
    return 2;
  }

  static tomba1::Tomba1Runtime runtime;
  psxport_install_game(runtime);

  auto game = std::make_unique<Game>();
  game->disc.env_key = kDiscEnvironmentKey;
  Core *core = &game->core;
  tomba1::registerStreamFieldTurn(*core);
  watchdog_init();
  load_exe(kDefaultExecutable, core);
  gte_init();
  mdec_init();
  spu_init();
  gte_bind(core);
  core->rsub.projprim.bind(core);
  spu_bind(core);
  mdec_bind(core);
  xa_bind(core);
  game->spu_audio.init();
  game->gpu.gpu_native_init();
  game->pad.overridesInit();
  core->runtime->registerOverrides(*game);
  render_path_install(core);

  FrameLoopShell shell;
  shell.prepareProduct(*game);
  for (std::uint32_t frame = 0;; ++frame) {
    shell.step(*core, frame);
  }
}
