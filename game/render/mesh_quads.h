// game/render/mesh_quads.h — MeshQuads: the host-side (float/int, NO GTE) builders for the 1.3.12
// object rotation matrices the engine's effect renderers compose before they submit a packed mesh.
//
// The guest builds these with libgte-class leaves that write a MATRIX into scratchpad — Math::rotmat
// (FUN_80085480), Math::rotY / rotZ (FUN_80084EB0 / FUN_80085050), the matrix multiply FUN_80084250 and
// Math::matColScale (FUN_80084520). A pc_render producer may not use those: they WRITE GUEST MEMORY, and
// the display pass is a read-only overlay. These are the same computations done in host registers off the
// same guest LUT, so a producer can build the effect's own transform, hand it to projComposeObjectHost,
// and interpolate with the rest of the frame.
#pragma once
#include <stdint.h>
struct Core;

// The mesh writer's own PER-QUAD ORDERING DECISION, which every caller of FUN_80027768 gets but which
// cannot be evaluated without the caller's OWN sort-bias argument (a2). The writer averages the four
// projected depths with AVSZ4, adds a2, compresses the result into an ordering-table bucket and DROPS
// the quad when that bucket falls outside the linkable range; the bias is also what lets an effect that
// spawns just behind the thing it hit still paint in front of it.
//
// It is opt-in rather than always-on because a2 is a per-CALLER fact: only a producer that has RE'd its
// own controller's call may claim to know it. `known == false` means "this caller's bias is not RE'd",
// and the walk then draws every record with no ordering bias — which is exactly what the callers written
// before this struct existed did, so they are unchanged by its introduction.
struct MeshOtBias {
  bool    known = false;
  int32_t bias  = 0;      // the s16 the controller hands the writer as its sort-bias argument
};

class MeshQuads {
public:
  // The engine's packed sin/cos LUT at 0x800A6490 (word = cos<<16 | sin), read the way Math::rotmat and
  // the rotpair kernel read it: index = |angle| & 0xFFF, sin negated for a negative angle.
  static void trig(Core* c, int32_t angle, int* sinOut, int* cosOut);

  // Math::rotmat (FUN_80085480) element math on three Euler angles, into a 1.3.12 3x3.
  static void rotmat(Core* c, int16_t ax, int16_t ay, int16_t az, int32_t M[3][3]);

  // Math::rotY / Math::rotZ applied to the IDENTITY — the "identity then rotate one axis" pair every
  // one of these effect renderers builds as its second matrix. (rotY is the rows-0/2 variant with the
  // flipped sin sign; rotZ is the rows-0/1 variant.)
  static void rotY(Core* c, int16_t angle, int32_t M[3][3]);
  static void rotZ(Core* c, int16_t angle, int32_t M[3][3]);

  // Read a guest MATRIX's 3x3 (row-major s16 at +0..+16) — the base transform when an effect inherits
  // its owner's matrix block instead of building one (NodeXform::copyMatrixBlock, FUN_80051B34).
  static void fromGuest(Core* c, uint32_t matPtr, int32_t M[3][3]);

  // out = colScale(A · B): the matrix multiply (FUN_80084250, >>12 with the MVMVA IR clamp) followed by
  // Math::matColScale's per-COLUMN factors, delivered as the float 1.3.12 rotation projComposeObjectHost
  // expects.
  static void composeScaled(const int32_t A[3][3], const int32_t B[3][3], const int32_t colScale[3],
                            float out[3][3]);
};
