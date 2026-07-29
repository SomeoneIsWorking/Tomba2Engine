// game/ai/node_lifecycle_sm.cpp — see node_lifecycle_sm.h. Body is port_gen output, verbatim.
#include "core.h"
#include "game.h"
#include "node_lifecycle_sm.h"
#include "override_registry.h"
#include "rec_decls.h"

extern void func_80040410(Core*);
extern void func_8003FBC4(Core*);
extern void func_8003FC00(Core*);
extern void func_8003FC78(Core*);
extern void func_8003FC8C(Core*);
extern void func_8003FD10(Core*);
extern void func_8003FED8(Core*);
extern void func_8003FFCC(Core*);
extern void func_8004022C(Core*);
extern void func_80040390(Core*);
extern void func_80077E7C(Core*);
extern void func_8007778C(Core*);
extern void func_800517F8(Core*);
extern void func_80040B48(Core*);
extern void func_8003FE00(Core*);
extern void func_8007A624(Core*);

// ORACLE: gen_func_80040558
void NodeLifecycleSm::step(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-24;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)4));
    c->r[4] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[4]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_800406D8; }
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80040590; }
    { int _t = (c->r[3] == c->r[0]);  if (_t) goto L_800405AC; }
     goto L_80040A48;
  L_80040590:;
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_800408D4; }
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80040A40; }
     goto L_80040A48;
  L_800405AC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_800405CC; }
    { int _t = (c->r[2] == c->r[4]);  if (_t) goto L_80040620; }
     goto L_80040A48;
  L_800405CC:;
    c->r[5] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    c->r[31] = 0x800405D8u;
    c->r[4] = c->r[16] + c->r[0]; func_80040410(c);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)64; if (_t) goto L_800405F4; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)64;
  L_800405F4:;
    c->mem_w16((c->r[16] + (uint32_t)128), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)128;
    c->mem_w16((c->r[16] + (uint32_t)130), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)150;
    c->mem_w8((c->r[16] + (uint32_t)41), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + (uint32_t)43), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + (uint32_t)95), (uint8_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)132), (uint16_t)c->r[2]);
    c->mem_w16((c->r[16] + (uint32_t)134), (uint16_t)c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)70), (uint8_t)c->r[0]); goto L_80040A48;
  L_80040620:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)94));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)8);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_800406C0; }
    c->r[2] = c->r[2] + (uint32_t)21216;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x80040650u: goto L_80040650; case 0x80040660u: goto L_80040660; case 0x80040670u: goto L_80040670; case 0x800406C0u: goto L_800406C0; case 0x80040680u: goto L_80040680; case 0x80040690u: goto L_80040690; case 0x800406A0u: goto L_800406A0; case 0x800406B0u: goto L_800406B0; default: rec_dispatch(c, c->r[2]); return; } }
  L_80040650:;
    c->r[31] = 0x80040658u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FBC4(c);
    c->r[2] = c->r[0] + (uint32_t)1; goto L_800406C4;
  L_80040660:;
    c->r[31] = 0x80040668u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FC00(c);
    c->r[2] = c->r[0] + (uint32_t)1; goto L_800406C4;
  L_80040670:;
    c->r[31] = 0x80040678u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x801286F4u);
    c->r[2] = c->r[0] + (uint32_t)1; goto L_800406C4;
  L_80040680:;
    c->r[31] = 0x80040688u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FC78(c);
    c->r[2] = c->r[0] + (uint32_t)1; goto L_800406C4;
  L_80040690:;
    c->r[31] = 0x80040698u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80120188u);
    c->r[2] = c->r[0] + (uint32_t)1; goto L_800406C4;
  L_800406A0:;
    c->r[31] = 0x800406A8u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FC8C(c);
     goto L_800406B8;
  L_800406B0:;
    c->r[31] = 0x800406B8u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x801146E8u);
  L_800406B8:;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80040A48; }
  L_800406C0:;
    c->r[2] = c->r[0] + (uint32_t)1;
  L_800406C4:;
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + (uint32_t)0), (uint8_t)c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)41), (uint8_t)c->r[0]); goto L_80040A48;
  L_800406D8:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[4] = c->r[2] + (uint32_t)-1936;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1936));
    c->r[2] = c->r[0] + (uint32_t)18;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = c->r[0] + (uint32_t)6; if (_t) goto L_80040708; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)489));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80040A48; }
     goto L_80040720;
  L_80040708:;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = c->r[0] + (uint32_t)19; if (_t) goto L_80040720; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)1));
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80040A48; }
  L_80040720:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)6);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_800407A8; }
    c->r[2] = c->r[2] + (uint32_t)21248;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x80040750u: goto L_80040750; case 0x80040760u: goto L_80040760; case 0x80040770u: goto L_80040770; case 0x80040780u: goto L_80040780; case 0x80040790u: goto L_80040790; case 0x800407A0u: goto L_800407A0; default: rec_dispatch(c, c->r[2]); return; } }
  L_80040750:;
    c->r[31] = 0x80040758u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FD10(c);
     goto L_800407A8;
  L_80040760:;
    c->r[31] = 0x80040768u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FED8(c);
     goto L_800407A8;
  L_80040770:;
    c->r[31] = 0x80040778u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FFCC(c);
     goto L_800407A8;
  L_80040780:;
    c->r[31] = 0x80040788u;
    c->r[4] = c->r[16] + c->r[0]; func_8004022C(c);
     goto L_800407A8;
  L_80040790:;
    c->r[31] = 0x80040798u;
    c->r[4] = c->r[16] + c->r[0]; func_80040390(c);
     goto L_800407A8;
  L_800407A0:;
    c->r[31] = 0x800407A8u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80114934u);
  L_800407A8:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)94));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)8);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_800408C8; }
    c->r[2] = c->r[2] + (uint32_t)21272;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x800407E0u: goto L_800407E0; case 0x80040888u: goto L_80040888; case 0x800408C0u: goto L_800408C0; case 0x800407D8u: goto L_800407D8; default: rec_dispatch(c, c->r[2]); return; } }
  L_800407D8:;
    c->r[31] = 0x800407E0u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x8012B118u);
  L_800407E0:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[3] = c->r[2] + (uint32_t)-2040;
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)14));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80040834; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)15));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)106));
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_80040834; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)40));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_800408C8; }
    c->mem_w8((c->r[16] + (uint32_t)1), (uint8_t)c->r[2]);
    c->r[31] = 0x8004082Cu;
    c->r[4] = c->r[16] + c->r[0]; func_80077E7C(c);
     goto L_80040878;
  L_80040834:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)40));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_800408C8; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1936));
    c->r[2] = c->r[0] + (uint32_t)8;
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_80040868; }
    c->r[31] = 0x80040860u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x8012E168u);
     goto L_80040870;
  L_80040868:;
    c->r[31] = 0x80040870u;
    c->r[4] = c->r[16] + c->r[0]; func_8007778C(c);
  L_80040870:;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_800408C8; }
  L_80040878:;
    c->r[31] = 0x80040880u;
    c->r[4] = c->r[16] + c->r[0]; func_800517F8(c);
    c->mem_w8((c->r[16] + (uint32_t)41), (uint8_t)c->r[0]); goto L_800408CC;
  L_80040888:;
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)16));
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)1));
    c->mem_w8((c->r[16] + (uint32_t)1), (uint8_t)c->r[2]);
    c->r[2] = c->r[2] & 255u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_800408C8; }
    c->r[31] = 0x800408B0u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x8012866Cu);
    c->r[31] = 0x800408B8u;
    c->r[4] = c->r[16] + c->r[0]; func_80077E7C(c);
    c->mem_w8((c->r[16] + (uint32_t)41), (uint8_t)c->r[0]); goto L_800408CC;
  L_800408C0:;
    c->r[31] = 0x800408C8u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x801201E0u);
  L_800408C8:;
    c->mem_w8((c->r[16] + (uint32_t)41), (uint8_t)c->r[0]);
  L_800408CC:;
    c->mem_w8((c->r[16] + (uint32_t)95), (uint8_t)c->r[0]); goto L_80040A48;
  L_800408D4:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)5);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_80040964; }
    c->r[2] = c->r[2] + (uint32_t)21304;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x80040964u: goto L_80040964; case 0x80040904u: goto L_80040904; case 0x8004094Cu: goto L_8004094C; case 0x8004095Cu: goto L_8004095C; default: rec_dispatch(c, c->r[2]); return; } }
  L_80040904:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8004092C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1327));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8004092C; }
    c->r[31] = 0x8004092Cu;
    c->r[4] = c->r[0] + (uint32_t)56; func_80040B48(c);
  L_8004092C:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)94));
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8004099C; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)16));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[3] + (uint32_t)94), (uint8_t)c->r[2]); goto L_80040964;
  L_8004094C:;
    c->r[31] = 0x80040954u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FE00(c);
     goto L_80040964;
  L_8004095C:;
    c->r[31] = 0x80040964u;
    c->r[4] = c->r[16] + c->r[0]; func_8003FED8(c);
  L_80040964:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)94));
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8004099C; }
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)16));
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)1));
    c->r[4] = c->r[16] + c->r[0];
    c->r[31] = 0x8004098Cu;
    c->mem_w8((c->r[16] + (uint32_t)1), (uint8_t)c->r[2]); rec_dispatch(c, 0x8012866Cu);
    c->r[31] = 0x80040994u;
    c->r[4] = c->r[16] + c->r[0]; func_80077E7C(c);
     goto L_80040A48;
  L_8004099C:;
    c->r[3] = c->r[2] + (uint32_t)-2040;
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)14));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_800409EC; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)15));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)106));
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_800409EC; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)40));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80040A48; }
    c->mem_w8((c->r[16] + (uint32_t)1), (uint8_t)c->r[2]);
    c->r[31] = 0x800409E4u;
    c->r[4] = c->r[16] + c->r[0]; func_80077E7C(c);
     goto L_80040A30;
  L_800409EC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)40));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_80040A48; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1936));
    c->r[2] = c->r[0] + (uint32_t)8;
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_80040A20; }
    c->r[31] = 0x80040A18u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x8012E168u);
     goto L_80040A28;
  L_80040A20:;
    c->r[31] = 0x80040A28u;
    c->r[4] = c->r[16] + c->r[0]; func_8007778C(c);
  L_80040A28:;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80040A48; }
  L_80040A30:;
    c->r[31] = 0x80040A38u;
    c->r[4] = c->r[16] + c->r[0]; func_800517F8(c);
     goto L_80040A48;
  L_80040A40:;
    c->r[31] = 0x80040A48u;
    c->r[4] = c->r[16] + c->r[0]; func_8007A624(c);
  L_80040A48:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)24; return;
    return;
}

void NodeLifecycleSm::registerOverrides(Game*) {
  engine_set_override_main(0x80040558u, &NodeLifecycleSm::step, gen_func_80040558);
}
