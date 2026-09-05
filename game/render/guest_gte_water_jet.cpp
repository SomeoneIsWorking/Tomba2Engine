// game/render/guest_gte_water_jet.cpp — bounded logic-time fallback for the A00 water-jet mesh.
//
// USER AUTHORIZATION 2026-08-21: an unresolved graphic may render from the guest's actual GTE
// output, provided it is not interpolated. This module applies that policy to exactly one proven
// gap: FUN_8013D454's non-zero-mode water jet. The zero-mode sprite branch remains owned by
// Render::waterJetSpriteRender, and no other FUN_80027768 controller opts into this fallback.
//
// The controller and shared writer still run their untouched generated bodies. The writer produces
// one 13-word packet (tag + 12 GP0 words) per accepted gouraud-textured quad in the guest packet pool;
// those words contain the guest GTE's integer SXY results, authored colours/UVs, CLUT and texpage.
// While the one controller scope is live, the native renderer feeds only that newly-written packet
// span through the existing GP0 decoder. The decoder joins each packet vertex address to ProjPrim's
// GTE-captured depth and queues an integer RQ_WORLD item (`has_xyf == 0`). Fps60 therefore presents
// it verbatim from the logic-frame queue instead of re-running or interpolating it.
//
// FALLBACK DEBT: this is deliberately not a display-pass producer and does not prove the controller's
// node-state transform. Retire it when FUN_8013D454 has a true controller-state producer. Do not widen
// the scope to the other sixteen unresolved mesh controllers merely because they share the writer.
#include "core.h"
#include "game.h"
#include "guest_call.h"
#include "native_override_catalog.h"
#include "render_internal.h"

#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>

namespace {

constexpr uint32_t kWriterAddr = 0x80027768u;
constexpr uint32_t kControllerAddr = 0x8013D454u;
constexpr uint32_t kPacketPoolCursor = 0x800BF544u;
constexpr unsigned kGt4PacketWords = 12u;
constexpr uint32_t kGt4PacketBytes = (kGt4PacketWords + 1u) * sizeof(uint32_t);
constexpr unsigned kMaxPacketsPerCall = 256u; // FUN_80027768's record-walk guard.

class WaterJetScope {
public:
  explicit WaterJetScope(Core *core) : mCore(core), mOuter(sActive) {
    sActive = this;
  }
  ~WaterJetScope() {
    sActive = mOuter;
  }

  WaterJetScope(const WaterJetScope &) = delete;
  WaterJetScope &operator=(const WaterJetScope &) = delete;

  static bool activeFor(Core *core) {
    return sActive && sActive->mCore == core;
  }

private:
  Core *mCore;
  WaterJetScope *mOuter;
  static thread_local WaterJetScope *sActive;
};

thread_local WaterJetScope *WaterJetScope::sActive = nullptr;

[[noreturn]] void malformedSpan(Core *c, uint32_t before, uint32_t after, uint32_t packet, uint32_t detail) {
  lucent::error("gtefallback",
                "FUN_{:08X} water-jet packet span malformed: before={:08X} after={:08X} "
                "packet={:08X} detail={:08X}",
                kWriterAddr,
                before,
                after,
                packet,
                detail);
  std::abort();
}

struct PacketReplay {
  unsigned packets = 0;
  long depthHits = 0;
  long depthMisses = 0;
  long depthStale = 0;
  int x0 = 0x7FFFFFFF;
  int y0 = 0x7FFFFFFF;
  int x1 = -0x7FFFFFFF;
  int y1 = -0x7FFFFFFF;
};

[[noreturn]] void missingGuestDepth(const PacketReplay &replay) {
  lucent::error("gtefallback",
                "FUN_{:08X} water-jet packet depth mismatch: packets={} hits={} misses={} stale={}",
                kWriterAddr,
                replay.packets,
                replay.depthHits,
                replay.depthMisses,
                replay.depthStale);
  std::abort();
}

// A polygon's embedded texpage and CLUT are GP0 parser state, and the parser intentionally leaves
// them live for the next packet in an ordinary OT walk. This fallback replays a bounded subset of
// that walk out of context, so leaking either value would recolour unrelated native submissions
// later in the frame. Preserve only the material fields a GT4 packet mutates; primitive counters,
// depth/provenance, `s_seen3d`, and the queued item itself are the fallback's intended effects.
class GpuMaterialScope {
public:
  explicit GpuMaterialScope(GpuState &gpu)
      : mGpu(gpu), mTpX(gpu.s_tp_x), mTpY(gpu.s_tp_y), mMode(gpu.s_tp_mode), mBlend(gpu.s_tp_blend),
        mClutX(gpu.s_clut_x), mClutY(gpu.s_clut_y) {}
  ~GpuMaterialScope() {
    mGpu.s_tp_x = mTpX;
    mGpu.s_tp_y = mTpY;
    mGpu.s_tp_mode = mMode;
    mGpu.s_tp_blend = mBlend;
    mGpu.s_clut_x = mClutX;
    mGpu.s_clut_y = mClutY;
  }

  GpuMaterialScope(const GpuMaterialScope &) = delete;
  GpuMaterialScope &operator=(const GpuMaterialScope &) = delete;

private:
  GpuState &mGpu;
  int mTpX;
  int mTpY;
  int mMode;
  int mBlend;
  int mClutX;
  int mClutY;
};

PacketReplay replayGuestGt4Span(Core *c, uint32_t before, uint32_t after) {
  if ((before & 3u) || (after & 3u) || after < before || after - before > kGt4PacketBytes * kMaxPacketsPerCall) {
    malformedSpan(c, before, after, before, after - before);
  }

  PacketReplay result;
  const ProjPrim::Stats depthBefore = c->rsub.projprim.stats();
  GpuMaterialScope materialScope(c->game->gpu);
  for (uint32_t packet = before; packet < after; packet += kGt4PacketBytes) {
    const uint32_t tag = c->mem_r32(packet);
    const unsigned words = tag >> 24;
    const uint8_t op = static_cast<uint8_t>(c->mem_r32(packet + sizeof(uint32_t)) >> 24);
    if (words != kGt4PacketWords || (op != 0x3Cu && op != 0x3Eu) || packet + kGt4PacketBytes > after) {
      malformedSpan(c, before, after, packet, tag);
    }

    // GT4 packet XY words are 1/4/7/10. Report the exact integer footprint this fallback claims,
    // including the GPU drawing offset the existing decoder applies to those packet coordinates.
    constexpr uint32_t kXyWords[] = {1u, 4u, 7u, 10u};
    for (uint32_t word : kXyWords) {
      const uint32_t xy = c->mem_r32(packet + sizeof(uint32_t) * (word + 1u));
      const int x = static_cast<int16_t>(xy) + c->game->gpu.s_off_x;
      const int y = static_cast<int16_t>(xy >> 16) + c->game->gpu.s_off_y;
      result.x0 = x < result.x0 ? x : result.x0;
      result.y0 = y < result.y0 ? y : result.y0;
      result.x1 = x > result.x1 ? x : result.x1;
      result.y1 = y > result.y1 ? y : result.y1;
    }

    // Actual guest packet words and packet addresses. gpu_dma2_block stamps each source address, so
    // the existing GP0 decoder recovers the depths captured beside these exact GTE-produced vertices.
    c->game->gpu.gpu_dma2_block(c, packet + sizeof(uint32_t), static_cast<int>(kGt4PacketWords), /*to_gpu=*/1);
    result.packets++;
  }
  if (before + result.packets * kGt4PacketBytes != after) {
    malformedSpan(c, before, after, before + result.packets * kGt4PacketBytes, result.packets);
  }
  const ProjPrim::Stats depthAfter = c->rsub.projprim.stats();
  result.depthHits = depthAfter.hit - depthBefore.hit;
  result.depthMisses = depthAfter.miss - depthBefore.miss;
  result.depthStale = depthAfter.stale - depthBefore.stale;
  if (result.depthHits != static_cast<long>(result.packets) * 4L || result.depthMisses != 0 || result.depthStale != 0) {
    missingGuestDepth(result);
  }
  return result;
}

// FUN_80027768 — untouched guest packed-mesh writer plus one scoped packet-span replay. Calls made by
// the landed impact-plume, charge-starburst, terrain, and every unresolved controller run only the
// authenticated executable/overlay evidence because WaterJetScope is absent.
void waterJetWriterTap(Core *c) {
  const uint32_t model = c->r[4];
  const uint32_t clutRow = c->r[5];
  const int32_t sortBias = static_cast<int16_t>(c->r[6]);
  const uint8_t uScroll = static_cast<uint8_t>(c->r[7]);
  const uint32_t before = c->mem_r32(kPacketPoolCursor);

  psx::cpu::callOriginalToReturn(*c, 0x80027768u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);

  if (!WaterJetScope::activeFor(c) || c->rsub.mode.psxRender()) {
    return;
  }
  const uint32_t after = c->mem_r32(kPacketPoolCursor);
  const PacketReplay replay = replayGuestGt4Span(c, before, after);
  lucent::debug("gtefallback",
                "f{} ctrl={:08X} node={:08X} writer={:08X} model={:08X} clutrow={} bias={} "
                "uscroll={} pool=[{:08X},{:08X}) packets={} screen=[{},{}]..[{},{}] "
                "depth={}/{}/{} source=guest-gte interp=off",
                c->game->gpu.s_frame,
                kControllerAddr,
                cur_render_node(c),
                kWriterAddr,
                model,
                clutRow,
                sortBias,
                uScroll,
                before,
                after,
                replay.packets,
                replay.packets ? replay.x0 : 0,
                replay.packets ? replay.y0 : 0,
                replay.packets ? replay.x1 : 0,
                replay.packets ? replay.y1 : 0,
                replay.depthHits,
                replay.depthMisses,
                replay.depthStale);
}

// FUN_8013D454 — A00 water-jet controller. The authenticated executable/overlay evidence owns every guest write and
// raises this host-only scope only for the duration of its direct FUN_80027768 call.
void waterJetControllerTap(Core *c) {
  WaterJetScope scope(c);
  psx::cpu::callOriginalToReturn(*c, kControllerAddr, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
}

} // namespace

void guest_gte_water_jet_install() {
  tomba::native::declareOverride(kWriterAddr, "waterJetWriterTap", waterJetWriterTap);
  tomba::native::declareOverride(kControllerAddr, "waterJetControllerTap", waterJetControllerTap);
}
