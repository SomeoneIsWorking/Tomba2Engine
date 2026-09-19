// class UiGroupCapture — implementation. See ui_group_capture.h for why one page keeps ONE ordered
// chrome list and where the paint order comes from.
#include "ui_group_capture.h"
#include "card_menu.h"
#include "core.h"
#include "engine.h"
#include "game.h"
#include "game_ctx.h" // eng(c) / rend(c)
#include "guest_call.h"
#include "options_page.h"
#include "panel.h" // Panel::pushFill / pushCorners — the ONE panel geometry
#include "pause_menu.h"
#include "producer_scope.h"
#include "render.h" // Render::emitUiFt4 / emitUiSprites + rsub.mode.psxRender() gate
#include "render_queue.h"
#include "save_prompt.h"
#include "start_page.h"
#include <algorithm>
#include <lucent/log.h>
#include <numeric>

namespace {
// The raised page scope, or nullptr when none is. ONE lookup for every route* below, so a page added
// to the list is added for panels and groups alike and the two can never disagree about which page
// is on the stack.
UiGroupCapture *raisedScope(Core *c) {
  if (c->rsub.mode.psxRender()) {
    return nullptr; // read-only overlay gate
  }
  Engine &e = eng(c);
  if (e.pauseMenu.capture.capturing()) {
    return &e.pauseMenu.capture;
  }
  if (e.startPage.capture.capturing()) {
    return &e.startPage.capture;
  }
  if (e.optionsPage.capture.capturing()) {
    return &e.optionsPage.capture;
  }
  if (e.cardMenu.capture.capturing()) {
    return &e.cardMenu.capture;
  }
  if (e.savePrompt.capture.capturing()) {
    return &e.savePrompt.capture;
  }
  return nullptr;
}
} // namespace

void UiGroupCapture::route(Core *c, const UiGroupArgs &a) {
  if (UiGroupCapture *s = raisedScope(c)) {
    s->fileGroup(a);
    return;
  }
  // GAME.BIN's Save/Continue/Load/Quit machine calls the shared group leaf directly rather than
  // through one of the page controllers above. Its classified scene is itself the scope: emit the
  // exact tapped group now, through the same PageChromeItem implementation every page drains later.
  if (rend(c)->classifyScene() == Render::SceneKind::SaveContinueMenu) {
    PageChromeItem item;
    item.kind = PageChromeItem::Kind::Group;
    item.otBucket = a.otBucket;
    item.group = a;
    ProducerScope scope(&c->rsub.producerScope, 0x8007E6DCu, "saveContinueMenuGroup");
    UiGroupCapture{}.emit(c, item, RQ_OVERLAY);
    return;
  }
  // A group that reaches here is DROPPED — no page scope is raised and the scene is not the one
  // classified scope that emits directly. Say so, with the guest caller and the scene that failed to
  // claim it, because "which page should own this" is the question a drop always raises.
  //
  // A DROP IS NOT AUTOMATICALLY A DEFECT. Some chrome is drawn natively from game state by an
  // independent producer — the field HUD and the dialog box are named above — and for those,
  // ignoring the guest's group emission is correct. So this is a lead, not a verdict: check the
  // picture before calling one of these missing. It is reported because the alternative, which is
  // what this was, is a branch that says nothing at all and hides a producer that never ran
  // (issues 0013, 0014, 0015).
  lucent::debug("uigroup",
                "DROPPED group template {:08X} at ({},{}) bucket {} ra {:08X} — no page scope raised, "
                "scene {} does not claim it",
                a.templPtr,
                a.x,
                a.y,
                a.otBucket,
                c->r[31], // the guest caller: which emitter wanted this, i.e. which page should own it
                (int)rend(c)->classifyScene());
}

bool UiGroupCapture::routePanelFill(Core *c, uint32_t rectPtr, int32_t uvIndex, uint16_t attr, int32_t otBucket) {
  UiGroupCapture *s = raisedScope(c);
  if (!s) {
    return false; // no page owns this panel — the tap draws it inline
  }
  s->filePanelFill(c, rectPtr, uvIndex, attr, otBucket);
  return true;
}

bool UiGroupCapture::routePanelCorners(Core *c, uint32_t rectPtr, uint16_t style, uint32_t shadow, int32_t otBucket) {
  UiGroupCapture *s = raisedScope(c);
  if (!s) {
    return false;
  }
  s->filePanelCorners(c, rectPtr, style, shadow, otBucket);
  return true;
}

void UiGroupCapture::fileGroup(const UiGroupArgs &a) {
  PageChromeItem it;
  it.kind = PageChromeItem::Kind::Group;
  it.otBucket = a.otBucket;
  it.group = a;
  mItems.push_back(it);
}

// Both panel forms resolve the rect NOW — see PageChromeItem's note: rectPtr is the caller's stack.
static void readRect(Core *c, uint32_t rectPtr, PageChromeItem &it) {
  it.rx = c->mem_r16s(rectPtr + 0u);
  it.ry = c->mem_r16s(rectPtr + 2u);
  it.rw = c->mem_r16s(rectPtr + 4u);
  it.rh = c->mem_r16s(rectPtr + 6u);
}

void UiGroupCapture::filePanelFill(Core *c, uint32_t rectPtr, int32_t uvIndex, uint16_t attr, int32_t otBucket) {
  PageChromeItem it;
  it.kind = PageChromeItem::Kind::PanelFill;
  it.otBucket = (uint8_t)otBucket;
  readRect(c, rectPtr, it);
  it.uvIndex = uvIndex;
  it.attr = attr;
  mItems.push_back(it);
}

void UiGroupCapture::filePanelCorners(Core *c, uint32_t rectPtr, uint16_t style, uint32_t shadow, int32_t otBucket) {
  PageChromeItem it;
  it.kind = PageChromeItem::Kind::PanelCorners;
  it.otBucket = (uint8_t)otBucket;
  readRect(c, rectPtr, it);
  it.attr = style;
  it.shadow = shadow;
  mItems.push_back(it);
}

std::vector<int> UiGroupCapture::paintOrder() const {
  std::vector<int> order(mItems.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [this](int i, int j) {
    const uint8_t bi = mItems[i].otBucket, bj = mItems[j].otBucket;
    return bi != bj ? bi > bj : i > j;
  });
  return order;
}

void UiGroupCapture::emit(Core *c, const PageChromeItem &it, int layer) const {
  switch (it.kind) {
  case PageChromeItem::Kind::Group: {
    const UiGroupArgs &a = it.group;
    if (a.sprite) {
      rend(c)->emitUiSprites(a.x, a.y, a.templPtr, a.dataBase, a.attrByte, a.clutSemi, layer);
    } else {
      rend(c)->emitUiFt4(a.x, a.y, a.wOv, a.hOv, a.templPtr, a.dataBase, a.attrByte, a.clutSemi, layer);
    }
    return;
  }
  // The panel geometry has ONE implementation; deferring a panel only moves WHEN it is drawn.
  case PageChromeItem::Kind::PanelFill:
    Panel::pushFillAt(c, it.rx, it.ry, it.rw, it.rh, it.uvIndex, it.attr);
    return;
  case PageChromeItem::Kind::PanelCorners:
    Panel::pushCornersAt(c, it.rx, it.ry, it.rw, it.rh, it.attr, it.shadow);
    return;
  }
}

int UiGroupCapture::drawAll(Core *c, const char *channel, int layer) {
  int drawn = 0, panels = 0;
  for (int i : paintOrder()) {
    const PageChromeItem &it = mItems[i];
    lucent::debug(channel,
                  "{} bucket={:3} templ={:08X} at ({},{}) wh=({},{}) attr={:02X} clutSemi={:04X}",
                  it.kind != PageChromeItem::Kind::Group ? "PANEL"
                  : it.group.sprite                      ? "SPR"
                                                         : "FT4",
                  it.otBucket,
                  it.group.templPtr,
                  it.group.x,
                  it.group.y,
                  it.group.wOv,
                  it.group.hOv,
                  it.group.attrByte,
                  it.group.clutSemi);
    emit(c, it, layer);
    ++drawn;
    panels += (it.kind != PageChromeItem::Kind::Group);
  }
  lucent::debug(channel,
                "frame drew {} item(s) — {} panel(s), {} group(s){}",
                drawn,
                panels,
                drawn - panels,
                drawn ? "" : " — NOTHING was filed under this page's scope this frame");
  clear();
  return drawn;
}

bool UiGroupCapture::beginScope() {
  const bool outer = !capturing();
  if (outer) {
    clear();
  }
  begin();
  return outer;
}

bool UiGroupCapture::endScope(bool outer) {
  if (outer) {
    end();
  }
  return outer;
}

bool UiGroupCapture::runGuestController(Core *c, std::uint32_t address, const char *who) {
  const bool outer = beginScope();
  psx::cpu::callOriginalToReturn(*c, address, psx::cpu::ExecutionBudget::currentTurn(*c), who);
  return endScope(outer);
}
