// UiGroupArgs::read — see ui_group_args.h.
#include "ui_group_args.h"
#include "core.h"

UiGroupArgs UiGroupArgs::read(Core* c, bool sprite) {
  const uint32_t placement = c->r[4], attrs = c->r[7];
  UiGroupArgs a;
  a.x        = c->mem_r16s(placement + 0u);
  a.y        = c->mem_r16s(placement + 2u);
  a.wOv      = c->mem_r16s(placement + 4u);
  a.hOv      = c->mem_r16s(placement + 6u);
  a.templPtr = c->r[5];
  a.dataBase = c->r[6];
  a.attrByte = c->mem_r8(attrs + 0u);
  a.clutSemi = c->mem_r16(attrs + 2u);
  a.otBucket = c->mem_r8(attrs + 1u);
  a.sprite   = sprite;
  return a;
}
