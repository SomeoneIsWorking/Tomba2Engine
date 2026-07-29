// game/ai/cull_substate_native.cpp — see cull_substate_native.h. Body is port_gen output, verbatim.
#include "core.h"
#include "game.h"
#include "cull_substate_native.h"
#include "override_registry.h"
#include "ov_a00_decls.h"

// ORACLE: ov_a00_gen_80133550
void CullSubstateLeaves::tickChildEulerZSwing(Core* c) {
    c->r[5] = c->r[4] + c->r[0];
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[5] + (uint32_t)120));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80133578; }
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80133590; }
     goto L_801335EC;
  L_80133578:;
    c->r[3] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + c->r[4]; goto L_801335A8;
  L_80133590:;
    c->r[3] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] - c->r[4];
  L_801335A8:;
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + (uint32_t)-8;
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)20));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < -32);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801335EC; }
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->mem_w16((c->r[5] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
  L_801335EC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[5] + (uint32_t)43));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80133608; }
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)192));
    c->mem_w16((c->r[5] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
  L_80133608:;
     return;
    return;
}

void CullSubstateLeaves::registerOverrides(Game*) {
  engine_set_override_a00(0x80133550u, &CullSubstateLeaves::tickChildEulerZSwing, ov_a00_gen_80133550);
}
