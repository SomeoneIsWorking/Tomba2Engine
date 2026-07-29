// game/render/ui_ft4_layout.cpp — see ui_ft4_layout.h. Body is port_gen output, verbatim.
#include "core.h"
#include "game.h"
#include "ui_ft4_layout.h"
#include "override_registry.h"
#include "rec_decls.h"

extern void func_8007E620(Core*);

// ORACLE: gen_func_8007E2F8
void UiFt4Layout::plainQuadVerts(Core* c) {
    c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
    c->mem_w32((c->r[8] + (uint32_t)8), c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
    c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
    c->r[2] = c->r[2] << 24;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
    c->r[3] = c->r[3] + c->r[2];
    c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
    c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)10));
    c->r[2] = c->r[2] << 24;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
    c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[3]);
    c->r[3] = c->r[3] + c->r[5];
    c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)10));
    c->r[5] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
    c->r[2] = c->r[2] + c->r[6];
    c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
    c->r[4] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)16));
    c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[3]);
    c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[5]);
    c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[2]);
    c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[4]); func_8007E620(c); return;
    return;
}

void UiFt4Layout::registerOverrides(Game*) {
  engine_set_override_main(0x8007E2F8u, &UiFt4Layout::plainQuadVerts, gen_func_8007E2F8);
}
