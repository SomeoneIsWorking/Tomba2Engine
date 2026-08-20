// game/render/queue_dispatch.cpp — the guest's cull-queue render dispatch, modelled read-only.
// Read queue_dispatch.h first; it carries the class->queue->table chain this file implements.
//
// RE PROVENANCE. Every arm below is named from the recompiler's own translation of the walk bodies
// (generated/, which is the recompiled MAIN.EXE, i.e. ground truth) — never from a guess:
//   queue A  gen_func_8003BB50   generated/shard_1.c:5942   table 0x80014A70, bound 144
//   queue B  gen_func_8003BCF4   generated/shard_2.c:3795   table 0x80014CB0, bound  33
//            + its shared tail gen_func_8003BED8, which IS the queue-B no-op ("continue the walk")
//   queue C  gen_func_8003BF00   generated/shard_6.c:4770   table 0x80014D38, bound  32
// and the ARM bodies the tables point at (all of them one to four instructions):
//   8003BC00/8003BDAC/8003BFAC  -> FUN_8003CCA4 perObjRenderDispatch          = the mesh flush
//   8003BC24                    -> FUN_80122974 tether line, + the flash tail
//   8003BC6C/8003BE84/8003BFBC  -> FUN_8003C2D4 billboardCompose1
//   8003BC7C/8003BEA4/8003BFCC  -> FUN_8003C464 billboardCompose2
//   8003BC8C/8003BFDC           -> FUN_8003C5F8 pre-composed-matrix renderer
//   8003BC9C/8003BFEC           -> FUN_8003C788 pre-composed-matrix renderer
//   8003BCAC/8003BEB4           -> billboardCompose1 then the node's vtable slot at +0x7C
//   8003BCB4/8003BEBC           -> the node's vtable slot at +0x7C
//   8003BCC0/8003BEC8           -> the node's vtable slot at +0x18 (the type-0x20 custom-render fn)
//   8003BE94                    -> vtable +0x7C then billboardCompose2
//   8003BDBC/8003BDF4/8003BE74  -> an AREA-OVERLAY renderer chosen by the mode byte @0x800BF870
//   8003BFFC                    -> mode byte picks 0x8010FC70 or FUN_8004CC88
//   8003BCD0 / 8003BED8 / 8003C028 -> the dedicated loop-continue arm: NOTHING IS DRAWN
//
// The live tables (dumped from the running game in area 13, scratch/logs/heads0/tbl_a13.log, and
// parsed by scratch/parse_tbl.py) hold ONLY these targets, so `Unknown` means the game data changed
// under us — it is reported, never silently treated as "draws" or "does not draw".
#include "queue_dispatch.h"
#include "core.h"

namespace {
// The cull's three render queues, as the walks read them (objlist_walk.cpp owns the walks themselves).
constexpr uint32_t QUEUE_A_TABLE = 0x80014A70u; // FUN_8003BB50
constexpr uint32_t QUEUE_B_TABLE = 0x80014CB0u; // FUN_8003BCF4
constexpr uint32_t QUEUE_C_TABLE = 0x80014D38u; // FUN_8003BF00
constexpr uint32_t QUEUE_A_TYPE_BOUND = 144u;
constexpr uint32_t QUEUE_B_TYPE_BOUND = 33u;
constexpr uint32_t QUEUE_C_TYPE_BOUND = 32u;
constexpr uint32_t QUEUE_A_NOOP = 0x8003BCD0u;
constexpr uint32_t QUEUE_B_NOOP = 0x8003BED8u;
constexpr uint32_t QUEUE_C_NOOP = 0x8003C028u;

constexpr uint32_t NODE_CLASS = 0x0Cu; // the cull's queue-routing key
constexpr uint32_t NODE_TYPE = 0x0Bu;  // the walks' per-type dispatch key

// The per-frame SNAPSHOT pair each consumer walk takes of its accumulator (see queue_dispatch.h).
// This is the list the walk consumes, and it is what survives to display time.
constexpr uint32_t QUEUE_A_SNAP_PTR = 0x1F800140u, QUEUE_A_SNAP_CNT = 0x1F800146u;
constexpr uint32_t QUEUE_B_SNAP_PTR = 0x1F80014Cu, QUEUE_B_SNAP_CNT = 0x1F800152u;
constexpr uint32_t QUEUE_C_SNAP_PTR = 0x1F800158u, QUEUE_C_SNAP_CNT = 0x1F80015Eu;
// The cull's own caps (Cull::performBaseCull) — a snapshot count above its cap means the counter is
// garbage, not that the list is longer, so it is clamped rather than trusted.
constexpr int QUEUE_A_CAP = 24, QUEUE_B_CAP = 40, QUEUE_C_CAP = 28;
} // namespace

const GuestQueueDispatch::Walk *GuestQueueDispatch::walkFor(Queue q) {
  static const Walk kA{QUEUE_A_TABLE, QUEUE_A_TYPE_BOUND, QUEUE_A_NOOP};
  static const Walk kB{QUEUE_B_TABLE, QUEUE_B_TYPE_BOUND, QUEUE_B_NOOP};
  static const Walk kC{QUEUE_C_TABLE, QUEUE_C_TYPE_BOUND, QUEUE_C_NOOP};
  switch (q) {
  case Queue::A:
    return &kA;
  case Queue::B:
    return &kB;
  case Queue::C:
    return &kC;
  default:
    return nullptr;
  }
}

// The routing itself is OWNED (and byte-exact) in cull.cpp — Cull::enqueueByClass and the identical
// tail of Cull::performBaseCull are the ONLY two places a node enters a render queue. This is a
// read-only restatement for the display pass, not a second port of either: any class outside the
// three below is marked visible and then consumed by nobody.
GuestQueueDispatch::Queue GuestQueueDispatch::queueForClass(uint8_t objClass) {
  switch (objClass) {
  case 2:
  case 9:
    return Queue::A;
  case 4:
    return Queue::B;
  case 5:
    return Queue::C;
  default:
    return Queue::None;
  }
}

GuestQueueDispatch::Route GuestQueueDispatch::routeFor(Core *c, uint32_t node) {
  Route r{};
  r.type = c->mem_r8(node + NODE_TYPE);
  r.queue = queueForClass(c->mem_r8(node + NODE_CLASS));
  const Walk *w = walkFor(r.queue);
  if (!w) {
    r.arm = Arm::OutOfRange;
    return r;
  } // no queue => no consumer at all
  if (r.type >= w->typeBound) {
    r.arm = Arm::OutOfRange;
    return r;
  }
  r.target = c->mem_r32(w->table + (uint32_t)r.type * 4u);
  if (r.target == w->noOpTarget) {
    r.arm = Arm::NoOp;
    return r;
  }
  switch (r.target) {
  case 0x8003BC00u:
    r.arm = Arm::MeshThenFlash;
    break;
  case 0x8003BDACu:
  case 0x8003BFACu:
    r.arm = Arm::Mesh;
    break;
  case 0x8003BC24u:
    r.arm = Arm::TetherLine;
    break;
  case 0x8003BC6Cu:
  case 0x8003BE84u:
  case 0x8003BFBCu:
    r.arm = Arm::Billboard1;
    break;
  case 0x8003BC7Cu:
  case 0x8003BEA4u:
  case 0x8003BFCCu:
    r.arm = Arm::Billboard2;
    break;
  case 0x8003BC8Cu:
  case 0x8003BC9Cu:
  case 0x8003BFDCu:
  case 0x8003BFECu:
    r.arm = Arm::PreComposed;
    break;
  case 0x8003BCACu:
  case 0x8003BCB4u:
  case 0x8003BCC0u:
  case 0x8003BEB4u:
  case 0x8003BEBCu:
  case 0x8003BE94u:
  case 0x8003BEC8u:
    r.arm = Arm::CustomFn;
    break;
  case 0x8003BDBCu:
  case 0x8003BDF4u:
  case 0x8003BE74u:
    r.arm = Arm::OverlayFn;
    break;
  case 0x8003BFFCu:
    r.arm = Arm::ModeDispatch;
    break;
  default:
    r.arm = Arm::Unknown;
    break;
  }
  return r;
}

GuestQueueDispatch::QueueSnapshot GuestQueueDispatch::snapshotOf(Core *c, Queue q) {
  uint32_t ptrAddr = 0, cntAddr = 0;
  int cap = 0;
  switch (q) {
  case Queue::A:
    ptrAddr = QUEUE_A_SNAP_PTR;
    cntAddr = QUEUE_A_SNAP_CNT;
    cap = QUEUE_A_CAP;
    break;
  case Queue::B:
    ptrAddr = QUEUE_B_SNAP_PTR;
    cntAddr = QUEUE_B_SNAP_CNT;
    cap = QUEUE_B_CAP;
    break;
  case Queue::C:
    ptrAddr = QUEUE_C_SNAP_PTR;
    cntAddr = QUEUE_C_SNAP_CNT;
    cap = QUEUE_C_CAP;
    break;
  default:
    return QueueSnapshot{0, 0};
  }
  int n = (int)c->mem_r16s(cntAddr);
  if (n < 0) {
    n = 0;
  }
  if (n > cap) {
    n = cap;
  }
  return QueueSnapshot{c->mem_r32(ptrAddr), n};
}

bool GuestQueueDispatch::submittedThisFrame(Core *c, uint32_t node, Queue q) {
  const QueueSnapshot s = snapshotOf(c, q);
  for (int i = 0; i < s.count; i++) {
    if (c->mem_r32(s.ptr + (uint32_t)i * 4u) == node) {
      return true;
    }
  }
  return false;
}

int GuestQueueDispatch::submittedTotal(Core *c) {
  return snapshotOf(c, Queue::A).count + snapshotOf(c, Queue::B).count + snapshotOf(c, Queue::C).count;
}

// The visibility byte the tether arm's callee tests before flushing the mesh (FUN_80122974's
// `lbu r2,1(r16)` / `addiu r18,1` / `bne r2,r18`). Same byte the walk's entry gate reads, but the gate
// only requires non-zero and this requires exactly 1.
static constexpr uint32_t kNodeVisible = 1u;
static constexpr uint32_t kNodeVisibleOff = 1u; // node + 1

bool GuestQueueDispatch::guestFlushesMesh(Core *c, uint32_t node, const Route &r) {
  if (r.arm == Arm::Mesh || r.arm == Arm::MeshThenFlash) {
    return true;
  }
  // The tether arm flushes the mesh too, from inside FUN_80122974 — see the header.
  if (r.arm == Arm::TetherLine) {
    return c && c->mem_r8(node + kNodeVisibleOff) == kNodeVisible;
  }
  return false;
}

const char *GuestQueueDispatch::armName(Arm a) {
  switch (a) {
  case Arm::OutOfRange:
    return "out-of-range";
  case Arm::NoOp:
    return "NO-OP";
  case Arm::Mesh:
    return "mesh";
  case Arm::MeshThenFlash:
    return "mesh+flash";
  case Arm::TetherLine:
    return "tether";
  case Arm::Billboard1:
    return "billboard1";
  case Arm::Billboard2:
    return "billboard2";
  case Arm::PreComposed:
    return "pre-composed";
  case Arm::CustomFn:
    return "custom-fn";
  case Arm::OverlayFn:
    return "overlay-fn";
  case Arm::ModeDispatch:
    return "mode-dispatch";
  default:
    return "UNKNOWN-TARGET";
  }
}

char GuestQueueDispatch::queueName(Queue q) {
  switch (q) {
  case Queue::A:
    return 'A';
  case Queue::B:
    return 'B';
  case Queue::C:
    return 'C';
  default:
    return '-';
  }
}
