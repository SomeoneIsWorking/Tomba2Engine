// panel.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_8004FFB4.
// ORACLE: gen_func_8004FFB4 (../../generated/shard_2.c:6001-6115)
// PORT_GEN: 0x8004FFB4 ../../generated/shard_2.c:6001-6115
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "ui/panel.h"
#include "core.h"
#include "override_registry.h"

void rec_dispatch(Core*, uint32_t);  // overlay_router.cpp — shared dispatch choke point

void Panel::fillQuad(Core* c) {
    c->r[3] = (uint32_t)32780u << 16;
    c->r[8] = c->mem_r32((c->r[3] + (uint32_t)-2748));
    c->r[2] = c->r[8] + (uint32_t)40;
    c->mem_w32((c->r[3] + (uint32_t)-2748), c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)44;
    c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
    c->r[2] = c->r[6] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[3] = c->r[6] + c->r[0]; if (_t) goto L_8004FFE4; }
    c->r[2] = c->r[0] + (uint32_t)46;
    c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
  L_8004FFE4:;
    c->r[2] = c->r[6] & 64u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[6] & 31u; if (_t) goto L_80050000; }
    c->r[2] = c->r[2] + (uint32_t)496;
    c->r[2] = c->r[2] << 6;
    c->r[2] = c->r[2] | 63u; goto L_8005000C;
  L_80050000:;
    c->r[2] = c->r[2] + (uint32_t)496;
    c->r[2] = c->r[2] << 6;
    c->r[2] = c->r[2] | 62u;
  L_8005000C:;
    c->mem_w16((c->r[8] + (uint32_t)14), (uint16_t)c->r[2]);
    c->r[2] = c->r[3] & 32u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)64; if (_t) goto L_8005002C; }
    c->mem_w8((c->r[8] + (uint32_t)6), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)5), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)4), (uint8_t)c->r[2]); goto L_8005003C;
  L_8005002C:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)7));
    c->r[2] = c->r[2] | 1u;
    c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
  L_8005003C:;
    c->r[2] = c->r[0] + (uint32_t)95;
    c->mem_w16((c->r[8] + (uint32_t)22), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)0));
    c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)2));
    c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)0));
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)4));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)2));
    c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)0));
    c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)2));
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)6));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)0));
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)4));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)2));
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)6));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)(c->r[5] < (uint32_t)5);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_80050174; }
    c->r[2] = c->r[2] + (uint32_t)23084;
    c->r[3] = c->r[5] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x800500ECu: goto L_800500EC; case 0x80050108u: goto L_80050108; case 0x80050124u: goto L_80050124; case 0x80050134u: goto L_80050134; case 0x80050144u: goto L_80050144; default: rec_dispatch(c, c->r[2]); return; } }
  L_800500EC:;
    c->r[4] = c->r[0] + (uint32_t)192;
    c->r[2] = c->r[0] + (uint32_t)136;
    c->r[3] = c->r[0] + (uint32_t)200;
    c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)144; goto L_8005015C;
  L_80050108:;
    c->r[4] = c->r[0] + (uint32_t)240;
    c->r[2] = c->r[0] + (uint32_t)136;
    c->r[3] = c->r[0] + (uint32_t)248;
    c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)144; goto L_8005015C;
  L_80050124:;
    c->r[4] = c->r[0] + (uint32_t)208;
    c->r[2] = c->r[0] + (uint32_t)137;
    c->r[3] = c->r[0] + (uint32_t)216; goto L_80050150;
  L_80050134:;
    c->r[4] = c->r[0] + (uint32_t)224;
    c->r[2] = c->r[0] + (uint32_t)137;
    c->r[3] = c->r[0] + (uint32_t)232; goto L_80050150;
  L_80050144:;
    c->r[4] = c->r[0] + (uint32_t)216;
    c->r[2] = c->r[0] + (uint32_t)136;
    c->r[3] = c->r[0] + (uint32_t)223;
  L_80050150:;
    c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)143;
  L_8005015C:;
    c->mem_w8((c->r[8] + (uint32_t)12), (uint8_t)c->r[4]);
    c->mem_w8((c->r[8] + (uint32_t)20), (uint8_t)c->r[3]);
    c->mem_w8((c->r[8] + (uint32_t)28), (uint8_t)c->r[4]);
    c->mem_w8((c->r[8] + (uint32_t)29), (uint8_t)c->r[2]);
    c->mem_w8((c->r[8] + (uint32_t)36), (uint8_t)c->r[3]);
    c->mem_w8((c->r[8] + (uint32_t)37), (uint8_t)c->r[2]);
  L_80050174:;
    c->r[2] = (uint32_t)32783u << 16;
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)-10040));
    c->r[2] = c->r[7] << 2;
    c->r[4] = c->r[4] + c->r[2];
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)0));
    c->r[3] = (uint32_t)2304u << 16;
    c->r[2] = c->r[2] | c->r[3];
    c->mem_w32((c->r[8] + (uint32_t)0), c->r[2]);
    c->mem_w32((c->r[4] + (uint32_t)0), c->r[8]); return;
    return;
}

static void ov_panel_fill_quad(Core* c) { Panel::fillQuad(c); }

void panel_fill_install() {
  extern void gen_func_8004FFB4(Core*);
  extern void shard_set_override(uint32_t, void (*)(Core*));
  overrides::install(0x8004FFB4u, "Panel::fillQuad", ov_panel_fill_quad,
                     gen_func_8004FFB4, shard_set_override);
}
