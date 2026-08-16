// game/render/queue_dispatch.h — THE GUEST'S OWN RENDER DISPATCH FOR THE THREE CULL RENDER QUEUES.
//
// WHY THIS EXISTS (kanban #77). `Render::fieldObjectsRender` walks three object heads. For HEADS[1]
// and HEADS[2] it applies the guest's own per-TYPE routing (the jump tables at 0x80014DB8 and
// 0x80015000) and draws only the types the guest routes to a mesh renderer. HEADS[0] (0x800FB168)
// had no such filter and was flushed WHOLESALE, because HEADS[0] has no render walk of its own —
// which is exactly the gap the user's "geometry vanilla does not show" report sits in.
//
// A HEADS[0] node reaches vanilla's picture through the CULL RENDER QUEUES, not through its list:
// the cull (FUN_8007712C's tail / FUN_8007703C) pushes a KEPT object onto ONE of three queues keyed
// by its CLASS byte (+0xC), and each queue has its OWN consumer walk with its OWN per-TYPE jump
// table keyed on the TYPE byte (+0xB). A node can be kept, marked visible and queued and STILL draw
// nothing, because its type indexes a NO-OP arm. That is the axis this header models.
//
//   class -> queue -> consumer walk            per-type table   entries  no-op arm
//     2,9 -> A      -> FUN_8003BB50 (walk1)    0x80014A70       144      0x8003BCD0
//     4   -> B      -> FUN_8003BCF4 (walk2)    0x80014CB0        33      0x8003BED8
//     5   -> C      -> FUN_8003BF00 (walk3)    0x80014D38        32      0x8003C028
//   any other class -> NO QUEUE: the cull marks it visible and NOTHING consumes it.
//
// The three walks are already owned natively in objlist_walk.cpp; this header is the READ-ONLY model
// of the same routing, for the display pass to ask "would the guest's own walk have drawn this node".
// The tables are read from GUEST MEMORY at the address above — never a baked-in copy — so the model
// cannot drift from the running game. (Dumped live 2026-08-06 in area 13 to enumerate the arms;
// scratch/logs/heads0/tbl_a13.log.)
#pragma once
#include <cstdint>
class Core;

class GuestQueueDispatch {
public:
  // Which cull render queue the object's CLASS byte (+0xC) routes it to — a read-only restatement
  // of the routing cull.cpp owns (Cull::enqueueByClass and the tail of Cull::performBaseCull).
  enum class Queue : uint8_t { None = 0, A, B, C };
  static Queue queueForClass(uint8_t objClass);

  // What the walk's per-type arm DOES with the node. One entry per distinct target address in the
  // three live tables — see queue_dispatch.cpp for the RE of every arm.
  enum class Arm : uint8_t {
    OutOfRange,    // type >= the walk's own bound: the walk skips the node entirely
    NoOp,          // the table's dedicated loop-continue arm — the guest draws NOTHING for this type
    Mesh,          // perObjRenderDispatch (FUN_8003CCA4) — the per-object render-command flush
    MeshThenFlash, // Mesh + the FUN_8002AE0C highlight tail (queue A types 0/15/64/79/128/143)
    TetherLine,    // FUN_80122974 — the MESH (when node[+1]==1) and then the rope/fishing line,
                   // plus the same highlight tail. The mesh flush lives in the callee, not the arm.
    Billboard1,    // billboardCompose1 (FUN_8003C2D4)
    Billboard2,    // billboardCompose2 (FUN_8003C464)
    PreComposed,   // FUN_8003C5F8 / FUN_8003C788 — pre-composed-matrix renderers
    CustomFn,      // the node's own render fn, via a vtable slot at node+0x18 or node+0x7C
    OverlayFn,     // an area-overlay renderer selected by the mode byte @0x800BF870
    ModeDispatch,  // queue C type 31: mode byte picks 0x8010FC70 or FUN_8004CC88
    Unknown        // a target the RE below does not name — treat as "cannot say", never as "draws"
  };

  // The arm the guest's own walk would take for this node, and the raw table target it came from.
  // `table` reads are plain guest reads of static jump-table data (a diagnostic and a routing model,
  // never a source of pixels).
  struct Route { Queue queue; uint8_t type; uint32_t target; Arm arm; };
  static Route routeFor(Core* c, uint32_t node);

  // ---- THE FRAME'S SUBMISSION LIST, readable at display time -----------------------------------
  // The trap this exists to close (measured 2026-08-06): the queue the CULL pushes onto is an
  // ACCUMULATOR pair (ptr 0x1F80013C / count 0x1F800144 for queue A). The first consumer walk of each
  // field frame SNAPSHOTS that pair into a second pair (ptr 0x1F800140 / count 0x1F800146) and then
  // RESETS the accumulator — so a display-pass read of the accumulator always answers "empty", which
  // is how an earlier instrument reported a false "queued by nobody, 45/45". The SNAPSHOT pair is the
  // list the walk actually consumed this frame and it survives to display time. Read THAT.
  //   queue  accumulator ptr/cnt      snapshot ptr/cnt        list head
  //     A    0x1F80013C / 0x1F800144  0x1F800140 / 0x1F800146  0x800F2410
  //     B    0x1F800148 / 0x1F800150  0x1F80014C / 0x1F800152  0x800F26C8
  //     C    0x1F800154 / 0x1F80015C  0x1F800158 / 0x1F80015E  0x800F2738
  // RE: the refresh block at the head of gen_func_8003BB50 / 8003BCF4 / 8003BF00 (and the native
  // mirrors in objlist_walk.cpp, which write the same six words).
  struct QueueSnapshot { uint32_t ptr; int count; };
  static QueueSnapshot snapshotOf(Core* c, Queue q);
  // Was this node on the frame's submission list for the queue its class routes it to?
  static bool submittedThisFrame(Core* c, uint32_t node, Queue q);
  // Total entries across the three snapshots — the DENOMINATOR every negative membership answer
  // must be reported with, so "not submitted" can never be confused with "nothing was submitted".
  static int submittedTotal(Core* c);

  // Does the guest's own dispatch flush this node's render commands (the display pass's mesh path)?
  //
  // Not answerable from the arm alone, which is what this used to assume ("ONLY the two mesh arms").
  // Arm::TetherLine's target 0x8003BC24 calls FUN_80122974, and that function's FIRST act is
  //     if (node[+1] == 1) FUN_8003CCA4(node);      // perObjRenderDispatch — the mesh flush
  // before it draws any tether. So a type-1 node reaches vanilla's picture as a MESH PLUS a line, and
  // reading the jump-table arm as the whole story silently dropped the mesh: the cliff fisherman's body
  // and rod (176 polys over 18 render commands) were never submitted while his fishing line kept
  // drawing every frame, which is exactly how the bug presented (kanban #95). Verified in the
  // recompiled substrate itself, generated/ov_a00_shard_1.c `ov_a00_gen_80122974`.
  //
  // Hence the node, not just the route: the guest's mesh call is CONDITIONAL, and on `== 1` precisely —
  // the walk's own entry gate only guarantees non-zero, so do not relax it to `!= 0`.
  static bool guestFlushesMesh(Core* c, uint32_t node, const Route& r);

  static const char* armName(Arm a);
  static char        queueName(Queue q);

private:
  struct Walk { uint32_t table; uint32_t typeBound; uint32_t noOpTarget; };
  static const Walk* walkFor(Queue q);
};
