// game/input/input.cpp — PC-native per-frame INPUT/controller-state subsystem.
// The per-frame input / controller-state processor (FUN_800931C0) — five phases over the global
// controller tables (ring buffer, presence accumulator, coherence window, channel flushes). Control
// flow + memory ops owned native; every sub-call (incl. the indirect fn-ptr globals) stays reachable by
// address via rec_dispatch. We mirror the gen 120-byte stack frame so sub-calls' frames align. Extracted
// verbatim from game_tomba2.cpp (one behavior, byte-identical) into its own module for PC-game code
// structure. The `pad931c0` diagnostic A/B gate (full RAM+scratchpad vs rec_super_call) is a REPL
// channel, unchanged.
#include "core.h"
#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
extern void gen_func_80092E3C(Core*);
extern void func_8009A170(Core*);
void rec_super_call(Core*, uint32_t);
void rec_dispatch(Core*, uint32_t);

// (removed 99 lines: the input_dispatch_931c0 draft for 0x800931C0 — REPLACED by Sequencer::voiceStateFlush
//  (game/audio/sequencer.cpp). It was filed in the WRONG SUBSYSTEM: the address neighbours the pad
//  leaves but the function is SPU voice state, and its sole caller is Sequencer. Four defects.)


// ================================================================================================
// FUN_80093650 — one-shot voice/channel-table init. Zeroes the SPU-voice allocation table + the
// controller-channel state block at base 0x80100000 (gen: `32784u<<16`) + ~21688..24000, clamps the
// active-voice cap (arg0/r4, an int8) to <=24, then per-voice seeds the record (gain 255, pan/env
// defaults) and fires the SPU key-off / channel-reset helpers. ready-FRAME: mirrors the gen 112-byte
// stack frame (spills r16..r21 + r31 at sp+80..104, marshals a per-voice struct at sp+16 for
// func_80099970). Installed by guest address into the ONE override registry; gen_func_80093650 is the
// oracle leg (SBS core B). Body byte-faithful to the gen (tools/port_check.py gates it).
// ================================================================================================
#include "override_registry.h"   // overrides::install — the one native-override registry
extern void shard_set_override(uint32_t, void (*)(Core*));
extern void gen_func_80093650(Core*);
void func_80099450(Core*);   // generated/shard_disp.c — SPU/voice subsystem reset
void func_80097760(Core*);   // generated/shard_disp.c
void func_80099970(Core*);   // generated/shard_disp.c — per-voice SPU key-off / envelope submit
void func_80094B50(Core*);   // generated/shard_disp.c — channel reset
void func_800931C0(Core*);   // generated/shard_disp.c — input processor prep

// ORACLE: gen_func_80093650
// PORT_GEN: 0x80093650 generated/shard_2.c:12938-13151
void Input::voiceTableInit(Core* c) {
  c->r[29] = c->r[29] + (uint32_t)-112;
  c->mem_w32((c->r[29] + (uint32_t)84), c->r[17]);
  c->r[17] = c->r[4] + c->r[0];
  c->r[4] = c->r[0] + c->r[0];
  c->mem_w32((c->r[29] + (uint32_t)104), c->r[31]);
  c->mem_w32((c->r[29] + (uint32_t)100), c->r[21]);
  c->mem_w32((c->r[29] + (uint32_t)96), c->r[20]);
  c->mem_w32((c->r[29] + (uint32_t)92), c->r[19]);
  c->mem_w32((c->r[29] + (uint32_t)88), c->r[18]);
  c->r[31] = 0x8009367Cu;
  c->mem_w32((c->r[29] + (uint32_t)80), c->r[16]); func_80099450(c);
  c->r[5] = (uint32_t)32784u << 16;
  c->r[5] = c->r[5] + (uint32_t)24000;
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)23696), (uint16_t)c->r[0]);
  c->r[31] = 0x80093694u;
  c->r[4] = c->r[0] + (uint32_t)32; func_80097760(c);
  c->r[16] = c->r[0] + c->r[0];
  c->r[3] = (uint32_t)32784u << 16;
  c->r[3] = c->r[3] + (uint32_t)23080;
  c->r[2] = c->r[16] & 65535u;
  L_800936A4:;
  c->r[2] = c->r[2] << 1;
  c->r[2] = c->r[2] + c->r[3];
  c->mem_w16((c->r[2] + (uint32_t)0), (uint16_t)c->r[0]);
  c->r[16] = c->r[16] + (uint32_t)1;
  c->r[2] = c->r[16] & 65535u;
  c->r[2] = (uint32_t)(c->r[2] < (uint32_t)192);
  { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[16] & 65535u; if (_t) goto L_800936A4; }
  c->r[16] = c->r[0] + c->r[0];
  c->r[2] = c->r[16] & 65535u;
  L_800936CC:;
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w8((c->r[1] + (uint32_t)23048), (uint8_t)c->r[0]);
  c->r[16] = c->r[16] + (uint32_t)1;
  c->r[2] = c->r[16] & 65535u;
  c->r[2] = (uint32_t)(c->r[2] < (uint32_t)24);
  { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[16] & 65535u; if (_t) goto L_800936CC; }
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)23920), (uint16_t)c->r[0]);
  c->r[16] = c->r[0] + c->r[0];
  c->r[2] = c->r[16] & 65535u;
  L_800936FC:;
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w8((c->r[1] + (uint32_t)23832), (uint8_t)c->r[0]);
  c->r[16] = c->r[16] + (uint32_t)1;
  c->r[2] = c->r[16] & 65535u;
  c->r[2] = (uint32_t)(c->r[2] < (uint32_t)16);
  { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[16] & 65535u; if (_t) goto L_800936FC; }
  c->r[2] = c->r[17] << 24;
  c->r[3] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[2] = (uint32_t)(c->r[3] < (uint32_t)24);
  { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)6u << 16; if (_t) goto L_80093744; }
  c->r[2] = c->r[0] + (uint32_t)24;
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w8((c->r[1] + (uint32_t)23788), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)6u << 16; goto L_8009374C;
  L_80093744:;
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w8((c->r[1] + (uint32_t)23788), (uint8_t)c->r[3]);
  L_8009374C:;
  c->r[2] = c->r[2] | 147u;
  c->r[3] = (uint32_t)32784u << 16;
  c->r[3] = (uint32_t)(int8_t)c->mem_r8((c->r[3] + (uint32_t)23788));
  c->r[16] = c->r[0] + c->r[0];
  c->mem_w32((c->r[29] + (uint32_t)20), c->r[2]);
  c->r[2] = c->r[0] + (uint32_t)4096;
  c->mem_w16((c->r[29] + (uint32_t)36), (uint16_t)c->r[2]);
  c->r[2] = c->r[0] + (uint32_t)4096;
  c->mem_w32((c->r[29] + (uint32_t)44), c->r[2]);
  c->r[2] = c->r[0] | 33023u;
  c->mem_w16((c->r[29] + (uint32_t)74), (uint16_t)c->r[2]);
  c->r[2] = c->r[0] + (uint32_t)16384;
  c->mem_w16((c->r[29] + (uint32_t)24), (uint16_t)c->r[0]);
  c->mem_w16((c->r[29] + (uint32_t)26), (uint16_t)c->r[0]);
  { int _t = ((int32_t)c->r[3] <= 0); c->mem_w16((c->r[29] + (uint32_t)76), (uint16_t)c->r[2]); if (_t) goto L_80093900; }
  c->r[21] = c->r[0] + (uint32_t)24;
  c->r[17] = c->r[0] + (uint32_t)255;
  c->r[20] = c->r[0] + (uint32_t)-1;
  c->r[19] = c->r[0] + (uint32_t)64;
  c->r[18] = c->r[0] + (uint32_t)1;
  c->r[4] = c->r[29] + (uint32_t)16;
  L_800937A4:;
  c->r[3] = c->r[16] & 65535u;
  c->r[2] = c->r[3] << 3;
  c->r[2] = c->r[2] - c->r[3];
  c->r[2] = c->r[2] << 3;
  c->r[3] = c->r[18] << (c->r[3] & 31);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21706), (uint16_t)c->r[21]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21704), (uint16_t)c->r[17]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w8((c->r[1] + (uint32_t)21733), (uint8_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21708), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21710), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21720), (uint16_t)c->r[20]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21722), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21724), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21726), (uint16_t)c->r[17]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21712), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21716), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w8((c->r[1] + (uint32_t)21714), (uint8_t)c->r[19]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21758), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21734), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21736), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21738), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21740), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21746), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21748), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21750), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21752), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21754), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[1] = c->r[1] + c->r[2];
  c->mem_w16((c->r[1] + (uint32_t)21742), (uint16_t)c->r[0]);
  c->r[31] = 0x800938D4u;
  c->mem_w32((c->r[29] + (uint32_t)16), c->r[3]); func_80099970(c);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)23824), (uint16_t)c->r[16]);
  c->r[31] = 0x800938E4u;
  c->r[4] = c->r[0] + (uint32_t)1; func_80094B50(c);
  c->r[16] = c->r[16] + (uint32_t)1;
  c->r[3] = (uint32_t)32784u << 16;
  c->r[3] = (uint32_t)(int8_t)c->mem_r8((c->r[3] + (uint32_t)23788));
  c->r[2] = c->r[16] & 65535u;
  c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
  { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[29] + (uint32_t)16; if (_t) goto L_800937A4; }
  L_80093900:;
  c->r[2] = (uint32_t)32784u << 16;
  c->r[2] = c->r[2] + (uint32_t)23544;
  c->r[3] = c->r[0] + (uint32_t)16383;
  c->mem_w32((c->r[2] + (uint32_t)0), c->r[0]);
  c->mem_w16((c->r[2] + (uint32_t)8), (uint16_t)c->r[3]);
  c->mem_w16((c->r[2] + (uint32_t)10), (uint16_t)c->r[3]);
  c->mem_w32((c->r[2] + (uint32_t)4), c->r[0]);
  c->r[2] = c->r[0] + (uint32_t)128;
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21688), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21690), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)23536), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21692), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21694), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21696), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)21698), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w8((c->r[1] + (uint32_t)23848), (uint8_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w16((c->r[1] + (uint32_t)23768), (uint16_t)c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->mem_w32((c->r[1] + (uint32_t)23792), c->r[0]);
  c->r[1] = (uint32_t)32784u << 16;
  c->r[31] = 0x8009397Cu;
  c->mem_w16((c->r[1] + (uint32_t)23770), (uint16_t)c->r[2]); func_800931C0(c);
  c->r[31] = c->mem_r32((c->r[29] + (uint32_t)104));
  c->r[21] = c->mem_r32((c->r[29] + (uint32_t)100));
  c->r[20] = c->mem_r32((c->r[29] + (uint32_t)96));
  c->r[19] = c->mem_r32((c->r[29] + (uint32_t)92));
  c->r[18] = c->mem_r32((c->r[29] + (uint32_t)88));
  c->r[17] = c->mem_r32((c->r[29] + (uint32_t)84));
  c->r[16] = c->mem_r32((c->r[29] + (uint32_t)80));
  c->r[29] = c->r[29] + (uint32_t)112; return;
  return;
}

// eov_voiceTableInit — guest-ABI thunk (arg0 in r4; the body leaves r2 as the gen epilogue does).
static void eov_voiceTableInit(Core* c) { Input::voiceTableInit(c); }

// FUN_80092E3C — SPU voice-attribute stage: sets the volume fields of one voice slot. 6,377 substrate
// dispatches per 6000 replay frames. Body is port_gen output (verbatim gen body), not a rebuild.
// ORACLE: gen_func_80092E3C
void Input::setVoiceVolume(Core* c) {
    c->r[2] = c->r[4] & 65535u;
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)24);
    { int _t = (c->r[2] != c->r[0]); c->r[7] = c->r[5] + c->r[0]; if (_t) goto L_80092E54; }
    c->r[2] = c->r[0] + (uint32_t)-1; goto L_80092E98;
  L_80092E54:;
    c->r[3] = c->r[4] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[5] = c->r[3] << 4;
    c->r[1] = (uint32_t)32784u << 16;
    c->r[1] = c->r[1] + c->r[5];
    c->mem_w16((c->r[1] + (uint32_t)23082), (uint16_t)c->r[6]);
    c->r[4] = (uint32_t)32784u << 16;
    c->r[4] = c->r[4] + c->r[3];
    c->r[4] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)23048));
    c->r[2] = c->r[0] + c->r[0];
    c->r[1] = (uint32_t)32784u << 16;
    c->r[1] = c->r[1] + c->r[5];
    c->mem_w16((c->r[1] + (uint32_t)23080), (uint16_t)c->r[7]);
    c->r[4] = c->r[4] | 3u;
    c->r[1] = (uint32_t)32784u << 16;
    c->r[1] = c->r[1] + c->r[3];
    c->mem_w8((c->r[1] + (uint32_t)23048), (uint8_t)c->r[4]);
  L_80092E98:;
     return;
}

void Input::registerOverrides(Game* /*game*/) {
  overrides::install(0x80092E3Cu, "Input::setVoiceVolume", &Input::setVoiceVolume,
                     gen_func_80092E3C, shard_set_override);
  overrides::install(0x80093650u, "Input::voiceTableInit", eov_voiceTableInit,
                     gen_func_80093650, shard_set_override);
}
