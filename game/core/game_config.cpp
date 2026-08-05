// game_config.cpp — the Tomba!2-specific GameConfig instance (game_iface.h seam).
//
// Every value here is a MAIN.EXE guest-address literal that USED to be baked directly into the
// framework substrate (runtime/recomp/*). The framework now reads `c->cfg->field` in place of the
// literal; this file is where the game supplies them. Pure value-preservation: each number matches
// the framework literal it replaced EXACTLY (a wrong address silently breaks boot / diverges SBS).
//
// Installed once at the very top of main() (boot.cpp) via tomba_install_game_config(), before any
// Game/Core is constructed, so Core's ctor snapshots a non-null c->cfg.
//
// DESIGNATED INITIALIZERS, deliberately (converted 2026-07-29). This used to be a POSITIONAL
// initializer whose fields were labelled by `/* name */` comments — labels the compiler never
// checked. game_iface.h says outright that "GameConfig is initialised POSITIONALLY by every
// consumer, so inserting a field mid-struct silently shifts every value after it", and has been
// appending new fields at the ends of groups to work around exactly that. Appending is a discipline
// the framework has to remember on every change; a designator is checked on every build. With these
// every field is bound by NAME, so a framework insertion anywhere can no longer slide this game's
// guest addresses one slot sideways — and a renamed or removed field becomes a compile error rather
// than a wrong address that boots and diverges. Fields left unset are value-initialised to zero,
// which is the framework's documented "this game has no such primitive".
#include "game_iface.h"
#include "overlay_table.h"   // generated: REC_MAIN_LO/HI — the game owns this, not the framework
#include "game_ctx.h"

static const GameConfig g_tomba_config = {
  // --- crt0 / boot (native_boot.cpp crt0_setup, game_init) ---
  .bssZeroLo = 0x800be0d8u,
  .bssZeroHi = 0x80106228u,
  .stackTopBase = 0x800a3f88u,
  .stackTopBase2 = 0x800a3f8cu,
  .heapBase = 0x80106228u,
  .heapSizePtr = 0x800abef8u,
  .heapBasePtr = 0x800abef4u,
  .gp = 0x800be0d4u,
  .libcInit = 0x80089860u,
  .gameMain = 0x80050b08u,   // FUN_80050b08 (native-overridden game-main; comment-only literal)
  .crt0 = 0x800896e0u,   // FUN_800896E0 (native crt0; comment-only literal)

  // Recompiled MAIN .text range (physical, addr & 0x1FFFFFFF). Taken straight from the values our
  // own recompiler run emits into generated/overlay_table.h, which is included below so these can
  // never drift from the substrate they describe.
  .recMainLo = REC_MAIN_LO,
  .recMainHi = REC_MAIN_HI,

  // Name of the environment variable / .env key that points at THIS game's disc image. The disc
  // resolver in disc.c used to hardcode this string; it now reads it from here, so a second consumer
  // can set its own key instead of silently booting with no media.
  .discEnvVar = "PSXPORT_TOMBA2_DISC",

  // Boot intro movies, in play order. native_boot_run used to hardcode this path; it now reads it
  // from here so a second consumer can name its own (or none). Only LOGO.STR belongs at boot —
  // OP.STR is the front-end's, and playing it here too caused the "FMV repeats" bug.
  .bootFmv = {"MOVIE/LOGO.STR", nullptr, nullptr, nullptr},

  // --- per-frame OT / packet-pool dance (native_boot.cpp native_step_frame) ---
  .otRegionBase = 0x800e80a8u,
  .otRegionStride = 0x00002070u,
  .packetPoolBase = 0x800bfe68u,
  .packetPoolStride = 0x00014000u,
  .otBasePtr = 0x800ed8c8u,
  .dwellCounter = 0x800e809cu,
  .poolPtrCur = 0x800bf544u,
  .poolPtrLast = 0x800bf4f4u,
  .clearOtagR = 0x80081458u,
  .putDrawEnv = 0x800815d0u,
  .drawSync = 0x80080f6cu,
  .irqEventClasses = { 0xF2000003u, 0xF0000001u, 0xF0000009u },
  .dualviewRenderOrch = 0x8003f9a8u,
  .dualviewSubmit = 0x8010810cu,

  // --- scheduler task layout (scheduler.cpp, native_boot probes) ---
  .taskTableBase = 0x801fe000u,
  .taskSlotStride = 0x00000070u,
  .taskCount = 3u,             // up to 3 cooperative tasks (loop lives game-side, no framework literal)
  .curTaskPtr = 0x1f800138u,
  .stageStart = 0x8010649cu,
  .stageDemo = 0x801062e4u,
  .stageGame = 0x8010637cu,

  // --- overlay router slots (overlay_router.cpp slot_index) ---
  .overlaySlots = {
    { 0x80106228u, "STAGE" },   // START/DEMO/GAME
    { 0x80108f9cu, "MODE"  },   // SOP / A0* field area code
    { 0x8018a000u, "AREA"  },   // OPN; also raw area DATA
  },

  // --- CD chokepoints (cd_override.cpp) ---
  .cdInit = 0x8008b2d8u,   // CdInit handshake (registered by PlatformHle::initBuiltins, not cd_override)
  .cdCommand = 0x8008ac34u,
  .cdSync = 0x8008a6ecu,
  .cdReadPrim = 0x8008c1ecu,
  .cdFileLoad = 0x8001db8cu,
  .cdAsyncRead = 0x8001d940u,
  .voicePlay = 0x8001d2a8u,
  .voiceStop = 0x8001cf2cu,
  .lastSectorTracker = 0x800be0e0u,
  .cdInlineLoad = 0x8001dc40u,
  .cdCmdStream = 0x8001ce90u,
  .cdCallbackTable = { 0x800abfbcu, 0x800abfc0u, 0x800abf24u, 0x800abf28u },
  .cdCallbackFn = { 0x8009996cu, 0x80089994u, 0x800899bcu, 0x00000000u },

  // STOCK Sony libcd entry points — ALL ZERO FOR THIS GAME, and that is a statement, not a gap.
  // Tomba!2 does not read through stock libcd: it drives the drive through its own engine loader
  // (cdFileLoad / cdAsyncRead / cdInlineLoad / cdCmdStream above, all FUN_8001xxxx engine code), so
  // there is no CdRead/CdReadSync/CdSearchFile/CdGetSector call site to intercept and no libcd
  // ready-callback, last-position buffer or DMA-completion slot to inherit. A game that DOES use
  // stock libcd fills these in; leaving them zero here keeps every one of those handlers unarmed.
  .cdGetSector = 0u,
  .cdReadyCbPtr = 0u,
  .cdLastPosBuf = 0u,
  .cdReadStock = 0u,
  .cdReadSync = 0u,
  .cdSearchFile = 0u,
  .cdDmaDoneCbPtr = 0u,

  // --- pad driver (pad_input.cpp) ---
  .padSlot0Buf = 0x800bf4f8u,
  .padSlot1Buf = 0x800bf51au,
  .padDriverFn = 0x80003a4cu,   // FUN_80003A4C SIO pad read (inert: driver not in MAIN.EXE; no live register)
  .padSlotPtrTable = 0x0000aec8u,
  // Byte distance between consecutive slots' buffer pointers. This driver keeps a FLAT pointer
  // array, so 4. (0 would be read as 4 as well, but say it rather than lean on the fallback.)
  .padSlotPtrStride = 4u,

  // --- platform HLE: the PSX hardware-sync primitives (framework: sync_overrides.cpp) ---
  // These were hardcoded in the framework's initBuiltins() until 2026-07-28. They are facts about
  // MAIN.EXE, so they belong here — same move the seed set and recMainLo/recMainHi already made.
  // Values below are the ones the framework previously baked in, unchanged, so behaviour is identical.
  .hle = {
    // Two I/O / hardware-service windows, NEVER game logic. The guard on register_() keeps engine
    // FUN_xxxx out of the HLE table (those are owned top-down via the override registry):
    //   [0x8001C000,0x8001E000) the engine's CD/SPU I/O glue (libcd-wrapper readers, SPU-mix)
    //   [0x80080000,0x8009E000) the SCEI library text (libgpu/libetc/libcd/libgs/libmdec) plus the
    //                           kernel thread primitives at 0x80080xxx
    // Game/engine LOGIC lives at [0x8001E000,0x80082000) and in the overlays (0x8010xxxx+) — both
    // outside these windows, which is what makes the guard meaningful.
    .windowLo = {0x8001C000u, 0x80080000u},
    .windowHi = {0x8001E000u, 0x8009E000u},
    // Resident-code range for the guest-backtrace heuristic (physical; overlays sit above the main
    // text, so this is wider than recMainLo/recMainHi and must be stated explicitly).
    .codeScanLo = 0x00010000u, .codeScanHi = 0x00120000u,

    .decDctInSync = 0x8009CAECu,   // libmdec DecDCTinSync
    .decDctOutSync = 0x8009CB80u,   // libmdec DecDCToutSync
    .cdReadSync = 0x8008A96Cu,   // FUN_8008a96c(mode, result)
    .cdDataSync = 0x8008B4B8u,   // FUN_8008b4b8(mode)
    .cdInitHandshake = 0x8008B2D8u,   // low-level CdInit controller-ready handshake
    .gpuTimeoutArm = 0x800834A0u,   // FUN_800834a0 arm deadline
    .gpuTimeoutCheck = 0x800834D4u,   // FUN_800834d4 check (not timed out)
    .gpuTimeoutDeadlineVar = 0x800A5ADCu,
    .gpuTimeoutFlagVar = 0x800A5AE0u,
    .changeThread = 0x80080880u,   // ChangeThread — the universal yield/task-end primitive
    // VSync TRAP: correct FOR THIS PORT, whose PC-native frame loop owns all timing, so nothing may
    // reach libetc VSync in any mode. A port still running the guest's own loop on the substrate
    // would leave this zero and register a faithful VSync instead.
    .vsyncTrap = 0x80085900u,
  },

  // Does the guest's uploaded VRAM stay visible under the submitted primitives? NO for this port:
  // pc_render owns the frame and draws only what a native producer submitted, so anything left in
  // VRAM from the guest's own drawing is stale and the clear-to-black is correct. A port still
  // running the guest's drawing code sets this to 1 so its upload-only screens are not erased.
  .preserveVramBackdrop = 0u,

  // Vblanks one gpu_pace_frame call represents. 2 = the engine's 30fps base cadence.
  // Was read from scratchpad 0x1F800235 — this engine's OWN field, but a magic address in the
  // framework, and ordinary working memory for every other port (it silently mistimed Spyro and
  // Spider-Man). That fallback is deleted; state it here instead.
  // NOTE: if this engine legitimately VARIES that byte per frame (slowdown frames), a constant is
  // wrong and it needs a GameHooks callback, not this. Unmeasured — see psxport gpu_native.cpp.
  .paceQuota = 2u,
};

// The game's callback vtable — defined in game_hooks.cpp (thin impls reaching eng(c).*).
extern const GameHooks g_tomba_hooks;

void tomba_install_game_config() { psxport_install_game(&g_tomba_config, &g_tomba_hooks); }
