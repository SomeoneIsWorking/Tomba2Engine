// class Render — the PC-native RENDER SUBSYSTEM umbrella owned by Core.
//
// PROPER OOP: one instance per Core, reached as `rend(c)->...`. This class exists to group the
// per-Core render-side subsystems (currently just NodeXform; scene-graph submit / walk / project etc.
// will migrate in here over time, out of the submit.cpp / render_frame.cpp / render_walk.cpp grab-bags).
//
// Owned by Core via a POINTER (`Core::mRender`) — construction/destruction lives in the Core ctor/dtor
// in runtime/recomp/core.cpp; back-pointer `mCore` is wired there, and each embedded sub-subsystem's
// own back-pointer is wired there too. Callers reach members as `rend(c)->mNodeXform.build(node)`.
#pragma once
#include "effect_lerp.h" // class EffectLerp — the effect-node actor-transform interpolation tier           // class Lighting — per-area light registry (sun / lava+torch)
#include "fx_motes.h"    // class MoteStreaks — the area-8 streak shadow
#include "guest_face_gate.h"  // struct GuestFaceGateCensus — the guest's own per-face draw/no-draw test
#include "guest_rng_mirror.h" // class GuestRngMirror — read-only stand-in for the guest PRNG
#include "lighting.h"
#include "margin_render.h" // class MarginRenderer — widescreen margin collect-and-flush
#include "mesh_quads.h"    // struct MeshOtBias — the mesh writer's per-caller ordering-bias argument
#include "node_xform.h"
#include "projection.h"    // EObjXform (per-Core active per-object xform lives on Render below)
#include "render_native.h" // class NativeScenePass — the decoupled native render subsystem
#include <unordered_set>
#include <vector>
class Core;

class Render {
public:
  Core *mCore = nullptr;

  // ---- render-side per-Core subsystems ------------------------------------
  // (The host-only render SUBSTRATE members — mode/diag/otAttr/dualviewSnapshot/stats/projprim/
  // pgxp/projParams — moved to the framework-owned `class RenderSubstrate` (runtime/recomp/
  // render_substrate.h), reached as `mCore->rsub.<member>`, so the framework no longer includes this
  // game umbrella just to reach a projection cache or compare-mode toggle. Byte-neutral host layout.)
  // Active per-object xform for the GT3/GT4 submitters. Set once per render command by the per-object
  // flush (projSetActive), read by the per-vertex projection (projVertexActive), cleared by
  // projClearActive. Was file-scope in projection.cpp; per-Core here so SBS's two cores don't
  // share a transform between their emits (2026-07-03).
  EObjXform mActiveXform{};
  bool mActiveXformSet = false;
  NodeXform mNodeXform; // scene-node WORLD-TRANSFORM builder (guest FUN_80051844)
  // PSXPORT_BDTAG per-node attribution names for the walk dispatch cases (render_walk.cpp).
  // The gp0 classify is deferred one frame, so the names live in a per-Core ring, not walk locals.
  NativeScenePass mNativeScene; // decoupled native render pass (collect + drawObject)
  MarginRenderer margin;        // widescreen margin re-include (collect in cull, flush post-walk)
  // Per-frame tally of faces THIS PORT DRAWS that the real game would have refused to draw, by
  // guest rule. Per-Core so SBS's two cores never share it. Reported by guestGateFlush().
  GuestFaceGateCensus mGuestGate;
  // Report and reset mGuestGate — one line per frame on the `guestgate` channel. A frame in which the
  // guest would drop nothing STILL PRINTS, with its denominator, because "0 drops" and "the census never
  // ran" are the two answers this whole investigation kept confusing for each other.
  void guestGateFlush();
  Lighting lighting; // per-area light registry (selected once per frame by shadeSelect)
  // Light config selected for this frame by shadeSelect(); the hot per-face shading routine reads the
  // cached pointer instead of re-reading guest RAM. Falls back to the SUN default when unset.
  const LightConfig *mShadeCfg = nullptr;

  // AREA-SCOPED CACHE trust latches — read-only, host-only (no guest writes). Two guest structures
  // that a per-area/per-beat SETUP step (re)populates and a per-tick STEP function refreshes while its
  // owner is active — SCENE_ENT_TABLE (0x800F2418: count@+6, grid-ptr@+0xC, refreshed by the SOP
  // cam-frustum prepass) and PARALLAX_BG_SM (0x800ED018: W@+0x10/H@+0x11, tilemap-ptr@+0x14, refreshed
  // by the backdrop tile scroller) — go STALE for a handful of ticks at the exact moment SOP narration
  // hands off to the field-area load: the narration's own prepass/scroller stops ticking them, the
  // field-area machine's equivalent (still substrate) hasn't started yet, and the just-loaded field
  // overlay has REPURPOSED the memory their leftover pointers still reference. Reading through a stale
  // pointer into repurposed bytes is garbage geometry / a garbage tilemap (found via the narration-end
  // -> fisherman-cutscene loading-screen bug: pc_render showed a tiled noise/atlas-grid garbage frame
  // where the oracle — pure PSX render, same recomp gameplay — shows the expected black load-hold,
  // because the PSX render path simply isn't drawing from these structures during the same window).
  //
  // Both structures happen to get explicitly RE-ZEROED (count/W momentarily 0) as part of the SAME
  // "next area setup" step that eventually repopulates them fresh — that zero is the observable proof
  // their owner has taken over again. Track (per structure) whether that zero has been seen since the
  // last narration -> field handoff before trusting a nonzero value; each structure's own EXISTING
  // "count==0 / W==0 -> skip" guard already makes the zero-window itself safe to read. Mirrors the
  // ScreenFade held-fade latch: pure state derived from guest READS, no guest writes, per-Core host
  // memory only.
  bool mSceneTableTrusted = false;     // may fieldEntityRender(SCENE_ENT_TABLE) run this frame?
  bool mBackdropTrusted = false;       // may render_bg_tilemap_native(PARALLAX_BG_SM) run this frame?
  bool mAreaCacheWasNarration = false; // previous frame's sop_narration value (shared edge detector)

  // Area 21's early-phase sky follows the raw signed camera pitch, not PARALLAX_BG_SM's wrapped tile
  // scroll. Capture and rotate that one producer input beside the framework's camera/backdrop captures
  // so the same native producer can rebuild both presentation instants without reading future state.
  int16_t mArea21SkyPitchCur = 0;
  int16_t mArea21SkyPitchPrev = 0;
  bool mArea21SkyPitchValid = false;
  bool mArea21SkyCapturedThisFrame = false;

  // NATIVE-DRAWN-NODE provenance (bug #48, docs/findings/render.md "Z-fight sweep 2026-07-14"):
  // node addresses Render::sceneNative's object loop WILL draw this frame via perObjFlush (the real
  // per-object float path — real identity + real per-vertex depth). cmdListDispatch
  // (perobj_dispatch.cpp) walks the SAME per-node cmd list (node+8 count, node+0xC0 cmd array,
  // geomblk=cmd+0x40) from the substrate walk cluster (perObjRenderDispatch's cases), so any node
  // present here has its substrate guest-OT emission made fully redundant — register it into the
  // native-cover span registry instead of letting it fall through to obj_depth's billboard
  // promotion (which produced the observed exact-duplicate class: the SAME mesh painted twice, once
  // by perObjFlush with real identity/depth, once by the guest-OT copy at a flat billboard depth
  // with dbg_node==0). PROVENANCE-DRIVEN: nativeObjDrawn re-derives this set straight from guest
  // state using perObjFlush's OWN inclusion test (see render_walk.cpp's banner for why it can't just
  // be a registry perObjFlush writes — call ordering), never a heuristic address-range/list-
  // membership guess — objects on OTHER walk lists (e.g. the Bcf4 aux list) that this test never
  // visits are simply absent from the set and stay uncovered, exactly as before. Cached per s_frame.
  // `PSXPORT_DEBUG=nofx` dedupe set — per-Core, NOT a function-local static. Under SBS the two
  // cores would share a static and each would silence the other's first sighting of a fn.
  std::unordered_set<uint32_t> mNofxSeen;

  std::unordered_set<uint32_t> mNativeDrawnNodes;
  int mNativeDrawnFrame = -1;
  bool nativeObjDrawn(Core *c, uint32_t node); // cmdListDispatch: will perObjFlush draw this node this frame?

  // WHICH GUEST EMITTER perObjFlush's prims STAND IN FOR, for the graphics-producer DB's native leg
  // (external/psxport/docs/plans/graphics-producer-db.md). perObjFlush is the picture half of guest
  // FUN_8003CDD8 and draws every per-object geomblk as generic GT3/GT4, but the GUEST routes each cmd
  // through perModeDispatch to ONE OF ELEVEN per-mode emitters — so the row is keyed by the per-mode
  // emitter, not by FUN_8003CDD8. Keying on the dispatcher would be the failure producer_scope.h's own
  // comment warns about: a shared CALLER shadowing every real emitter and collapsing the entire world
  // layer into one meaningless row.
  //
  // THE KEY IS RESOLVED FROM DATA, not written down — measured 2026-08-12, and the measurement was
  // CORRECTED the same day, so read the whole note. The guest dispatch chain DOES execute on the leg
  // where the census measures: `PSXPORT_DEBUG=ovhit` over a 200-frame field replay shows FUN_8003CDD8
  // dispatched 1195 times, FUN_8003F698 9507 times, and the generic-overlay emitter FUN_80146478 6245
  // times. What does NOT run is the NATIVE body of any override: PSXPORT_GATE=1 sets psx_fallback, and
  // overrides::runEntry routes every registered address to its `gen` body on that leg (482 registry
  // entries, ZERO with native hits, 268 with oracle hits). An instrumented native body therefore logs
  // nothing while its guest function is executing thousands of times — which is exactly how the first
  // reading of this ("the chain does not execute") came to be wrong.
  //
  // WHY THE RECORDER DESIGN STILL HAD TO GO: an earlier version had cmdListDispatch record the routing
  // for perObjFlush to read. That record lived in the NATIVE body, which the gate leg bypasses, so it
  // could never fire on the leg the census measures — dead for the routing reason, not because the guest
  // was idle. Resolving from data has no such dependency and is simpler.
  //
  // SO THE KEY IS NOT A COUNTERFACTUAL ON THIS LEG: the resolution below reads the same MODE_FORCE /
  // MODE_BYTE / MODE_TABLE state the executing gen body reads, so it names the emitter the guest
  // ACTUALLY used. `primsGuest` stays 0 for these rows because the guest leg's own attribution is
  // structurally blind for packet-pool stores (docs/findings/render.md), NOT because nothing ran —
  // a guest-side comparison is possible in principle and is blocked by that, not by absence.
  //
  // ONE INPUT IS UNOBSERVABLE HERE: `flag & 1` also forces the generic emitter, and `flag` is a per-call
  // argument of FUN_8003CDD8 — a function that never runs on this leg, so the value cannot be sampled
  // even in principle. The key is resolved with flag = 0 (routing enabled). A cmd the guest would have
  // forced generic is therefore keyed to its per-mode emitter instead. Not measurable from here; it is
  // named at the call site rather than left as a silent inaccuracy.
  //
  // FUN_8003F698's routing rule, in ONE place: which guest emitter a cmd with this `flag` reaches.
  // `*caseLabelOut` = the jump-table label used, or 0 for the generic-emitter path. perModeDispatch
  // dispatches through it; the producer DB keys perObjFlush's rows by it.
  uint32_t resolvePerModeEmitter(Core *c, uint32_t flag, uint32_t *caseLabelOut);

  // ---- object-render projection ops (impl in projection.cpp) ----------
  // Compose an EObjXform from the object's REAL WORLD coordinates: its world rotation matrix (cmd+0x18)
  // and world position (cmd+0x2C), transformed by the live scene camera (scratchpad view matrix
  // 0x1F8000F8 / translation 0x1F80010C). Projection constants are the camera's (CR24-26). No gte_op.
  void projComposeObject(uint32_t cmd, EObjXform *out);
  // Host-supplied-transform variant: composes the same camera-x-object core as projComposeObject, but
  // Robj/Tobj are HOST float values the caller already computed (margin's read-only path — a culled
  // node's cmd+0x18/0x2C are stale, so we can't read them from guest RAM). Camera state is still read
  // from scratchpad (never stale). No guest reads of Robj/Tobj, no guest writes anywhere.
  void projComposeObjectHost(const float Robj[3][3], const float Tobj[3], EObjXform *out);
  // Compose an EObjXform from the scene camera ALONE (no per-object matrix) — for geometry already in
  // WORLD space (field entity render loop), where view = Rcam·world + Tcam directly. No gte_op.
  void projComposeCamera(EObjXform *out);
  // Per-command active xform (owned by the GT3/GT4 submitters).
  void projSetActive(const EObjXform *w);
  void projClearActive();
  bool projActive() const {
    return mActiveXformSet;
  }
  void projVertexActive(int vx, int vy, int vz, ProjVtx *out);
  // Same projection, and additionally report the GTE FLAG (CR31) bits this vertex would have raised.
  // The guest's geometry submitters drop a face whose CR31 error bit is set, so a caller that wants to
  // run the guest's own test needs the report — see game/render/guest_face_gate.h.
  uint32_t projVertexActiveFlags(int vx, int vy, int vz, ProjVtx *out);
  // Pack the ACTIVE float xform into the CR0-7 + CR24/25/26 layout the fps60 midpoint reprojection consumes.
  void projActiveCr(uint32_t cr[11]);
  // THE decode of Tomba!2's scratchpad scene view matrix (0x1F8000F8 = CR0-4 halfword packing, T at
  // +0x14 = CR5-7) into float R/T. Exact inverse of projActiveCr's packing, and the ONE place that
  // knows the layout: the framework's fps60ReadSceneCam seam and the cube-text selftest's
  // camera-composed negative control both call it, so neither can transcribe a halfword wrong on its
  // own. `projection_test.cpp` round-trips pack->unpack to keep the pair honest.
  static void readSceneViewMatrix(Core *c, float R[3][3], float T[3]);

  // ---- per-frame render orchestrators (called by Engine::fieldFrame/X) ----
  // frame  (guest 0x8003F9A8) — the primary per-frame render orchestrator; runs the non-walk PSX
  //   passes (the walk cluster is owned by SceneNative::render in render_walk.cpp).
  //   Was ov_render_frame in render_frame.cpp.
  // frameX (guest 0x8003FA44) — the mid-transition variant (reduced pass set). Was ov_render_frame_x.
  void frame();
  void frameX();

  // abortUnimplemented (USER 2026-07-15): FAIL-FAST for the ONE native renderer. pc_render produces the
  // picture ONLY from native scene producers (sceneNative for the field world, …). A scene — or a scene
  // LAYER (e.g. the field's 2D HUD/dialog/menu overlay) — with no native producer must NOT fall back to
  // transcribing the guest OT/GP0: it aborts here with a precise identity (stage / sm[0x4a] / sm[0x4c] /
  // overlay signature) so the crash sequence IS the native-renderer work queue. There is no fallback and
  // no env escape hatch; the guest-OT walk survives ONLY under psx_render (the reference renderer).
  [[noreturn]] void abortUnimplemented(const char *scene);

  // perObjFlush: per-object native GT3/GT4 flush — composes the float camera×object transform from
  // the object's real world coords and submits every geomblk cmd on node+0xC0 through gt3gt4.
  // Taxi-parameter c->r[4] = node (recomp-shaped body, mirrors the guest ABI).
  void perObjFlush();
  // NO perObjFlushPreComposed. The F174-class node types (renderWalk table types 1/4 — text labels
  // etc.) hold a PRE-COMPOSED MATRIX at cmd+0x18, and the native flush for them worked by factoring
  // the scene camera back out of it. That is the tap the USER banned on 2026-08-04, so the whole
  // method is gone and those nodes get NO generic native mesh flush. The one class with a real
  // producer is the cube-text banner — CubeTextBanner (game/render/cube_text_banner.cpp), which
  // builds its transform from the banner's own state instead.

  // terrain (guest 0x8002AB5C): the field terrain render entry. Picks the area's light config for
  // the frame, then either super-calls the recomp body (dual-core diff neutralize path) or runs the
  // PC-native float terrain render. Taxi-parameter c->r[4] = node. Was ov_terrain.
  void terrain();

  // terrainRenderAll: scan the field's render-list (0x800F2624) for terrain nodes (render fn ==
  // 0x8002AB5C) and call terrain() on each — the ENUMERATION half of sceneNative's terrain block. ONE
  // call sequence used by both: the real per-logic-frame walk (render_walk.cpp sceneNative) and Fps60's
  // Tier-1 present-time camera-lerp re-render (fps60.cpp present_vk) — see docs/fps60-rework.md
  // "Object-tier attempt ... Why Tier 1 isn't built" for why re-invoking the SAME sequence (not a
  // parallel one) is what makes the lerped-camera re-render provably the same render path.
  void terrainRenderAll();

  // Per-frame WORLD-pass gates, shared by the real render (sceneNative) and the fps60 interp re-render
  // (Fps60::tier1Render) so the in-between can never draw a pass the real frame skipped (#67 bug class:
  // the interp re-run bypassing a real-frame gate paints content only every other present). Pure guest
  // READS of state unchanged since the real frame (fps60's present-time invariant).
  bool worldVoidBeat() const; // SOP narration VOID beat (0x800bf9b4==5): no terrain/scene-table/backdrop
  bool fieldAreaInit() const; // GAME field-area object-placement init frame: models unattached, no world
  // attractItemLive: does the DEMO-stage attract item's world EXIST this frame? s48==7 also covers the
  // item's LAUNCH frames (the s7 phase machine's cooperative area load yields), and on those the three
  // field entity heads still point at the PREVIOUS item's torn-down nodes — walking them reads whatever
  // the incoming area stream wrote over the freed memory. Reads the phase machine's own "item built"
  // latch; the full RE + the measurement is on the body (game/render/render_attract.cpp), kanban #86.
  static constexpr uint32_t kAttractItemBuiltLatch = 0x1F80019Au; // written 1 by phase0's tail
  bool attractItemLive() const;

  // shadeSelect: pick this area's light config once per world frame (cheap guest-RAM fingerprint via
  // Lighting::areaKeyFrom); caches the result in mShadeCfg for the per-face shading routine.
  void shadeSelect();

  // gt3gt4 (gen_func_800803DC's first body): the generic GT3/GT4 renderer — split the geomblk's packed
  // prim counts (low16 tri, high16 quad) and run the two native submitters in sequence.
  void gt3gt4(uint32_t geomblk, uint32_t otbase);

  // sceneNative: the master scene-render walker (per-frame terrain + entity/object walk over
  // the 3 doubly-linked object lists, with backdrop + collectable-quad + native BG tilemap).
  // Called by game_tomba2.cpp's ov_draw_otag every field-stage frame (and by margin_render's
  // widescreen re-include pass). Was ov_scene_native.
  void sceneNative();

  // areaCacheTrustTick: the once-per-logic-frame edge detector behind mSceneTableTrusted /
  // mBackdropTrusted. Called from Engine::drawOTag BEFORE the psx_render/pc_render branch, because it
  // tracks GUEST state (has the area cache been repopulated?) and is not part of building the picture.
  // Living inside sceneNative made it pc_render-only, which permanently suppressed the backdrop and the
  // scene table after any mid-scene switch out of psx_render — kanban #41, see the writeup on the body.
  void areaCacheTrustTick();

  // ---- pc_render scene DISPATCH: classify the current scene, then run its native producer -------------
  // The picture for every pc_render frame comes from ONE of these per-scene producers (no guest-OT
  // transcription). drawOTag (game_tomba2.cpp) calls renderScene() after the psx_render reference guard;
  // renderScene classifies by stage/sub-state and dispatches. Each producer owns its DisplayPassGuard +
  // fps60 eligibility. A stage with no producer aborts (abortUnimplemented) — the crash IS the backlog.
  enum class SceneKind { Loading, StartBoot, Title, Field, HutInterior, SaveContinueMenu, SopNarration, Unknown };
  SceneKind classifyScene();     // stage 0x801FE00C + sub-state selectors -> which scene this frame is
  void renderScene();            // classify + dispatch to the producer below
  void renderLoading();          // #0 task-switch handoff — black loader (task not yet initialized)
  void renderStartBoot();        // #1 START.BIN loader — black frame
  void renderTitle();            // #2 DEMO/title front-end (s2 = titleNative; other substates = black)
  void renderField();            // #3 walkable field — native world (sceneNative)
  void renderHutInterior();      // #4 hut/door authored sub-scene — objects-only (fieldObjectsRender)
  void renderSaveContinueMenu(); // #5 black-backed GAME.BIN Save/Continue/Load/Quit dialog
  void renderSopNarration();     // #6 SOP intro narration — native world (sceneNative + void-beat guard)
  void renderAttract();          // #7 DEMO/title ATTRACT (sm[0x48]==7) — live 3D field world (sceneNative)
  // fadeTileRender(node): native producer for the full-screen FADE/FLASH tile (guest FUN_800726D4,
  // render-walk case 0x8003C138). Reads the node's level halfword (node+0x10 -> +0, signed; negative
  // = no tile) and draws one full-screen quad, R=G=B=level, blended unless level==255. Read-only.
  void fadeTileRender(uint32_t node);

  // Dialog/prompt TEXT is produced by the FUN_8007CC00 tap (Panel::pushDialogGlyphs, game/ui/
  // panel.cpp) — fires whenever the guest dialog emitter draws, with the highlight palette the
  // earlier flat-list producer here couldn't see (it lacked the DialogBox pointer).

  // titleNative: the DEMO/title (stage 0x801062E4, substate sm[0x48]==2) read-only native producer.
  // The title = a black backdrop + 2 logo sprites (op 0x65, fixed layout, fn 0x8010696C) + 3 menu FT4
  // quads (fn 0x8007E2F8) + native font text. Its builders write GUEST packets (byte-exact) which
  // pc_render does not walk; titleNative re-emits the same picture to the native queue from source state
  // (constants / menu-cursor state), host-only. See docs/native-render-rebuild.md #2. Font/menu emit
  // lands incrementally (font-glyph dual-emit + owning 0x8007E2F8); logo is exact from decoded packets.
  void titleNative();

  // s3MenuNative: the DEMO/title front-end SECOND menu page (sm[0x48]==3, reached from the title by
  // confirming New Game). RE'd (docs/findings/render.md '#2b'): Demo::s3 unconditionally draws the SAME
  // chrome + a page-1 menu (FUN_80106824(param1=1) -> item templates 0x90/0x91) — NOT the SOP narration.
  // Read-only producer; built from the shared data-driven menu emitter below.
  void s3MenuNative();
  void renderCardBrowser(); // DEMO/title Load-Game memory-card browser (sm[0x48]==4) — 2D producer

  // --- DEMO/title front-end OPTIONS (sm[0x48]==6) — one producer per page of the 5-page sub-machine.
  // The page is selected by task-sm[0x50] (written by the page controller FUN_8007B45C); each page has
  // its own guest builder, so each gets its own producer method. All read-only (guest RAM + the menu
  // template tables); the page TEXT arrives from the global font/icon taps. See render_options.cpp.
  // optionsPageNative: the title chrome Demo::s6 composites UNDER the Screen-adjust page (sm[0x50]==3).
  // Every other element of every page is produced at its own guest emitter under the page scope in
  // game/ui/options_page.cpp, which is what makes ONE producer serve both the title and in-game entry
  // points (#38).
  void optionsPageNative();
  // optionsBackdrop / optionsSolidBox: the DRAW halves the page's ported emitters call —
  // FUN_8007FC24's full-screen dark-blue gradient (+ widescreen pillarbox) and FUN_8007FCC8's flat
  // TILE rectangle (dark blue when the low 7 flag bits are clear, black otherwise). Both are pushed by
  // OptionsPage::drawCollected, in the guest's paint order.
  void optionsBackdrop();
  void optionsSolidBox(int x, int y, int w, int h, uint32_t flags);

  // --- shared DATA-DRIVEN menu emitter (reproduces the guest menu builders, read-only) --------------
  // menuChrome: black backdrop + the 2 logo sprites (FUN_80106690) shared by every front-end menu page.
  void menuChrome();
  // menuItemsAndCursor: reproduces FUN_80106824(param1, param2) — the cursor (template 0x98) + the two
  // item text-images (templates {0x8e,0x8f} for page 0 / {0x90,0x91} for page 1), with the selected item
  // RAW (bright) and the other modulated 0x50 (dim), per the live selection. param2 selects which item is
  // bright AND indexes the cursor-X table @0x80107704.
  void menuItemsAndCursor(int param1, int param2);
  // emitMenuFt4: reproduces FUN_8007e1b8 (POLY_FT4 case) — resolve a menu template (idx -> table
  // @0x80017334 -> header @base 0x80158000 -> `count` 16-byte entries) and emit each as a native FT4 quad,
  // reading the game's OWN geometry (position/uv/clut/tpage) from guest RAM. attr high-nibble 0 -> RAW,
  // else modulated by (attr,attr,attr). Decoder validated field-for-field against the title's known-good
  // quads (templates 0x8e/0x8f/0x98). Retires the hand-decoded per-item constants titleNative used.
  void emitMenuFt4(int anchorX, int anchorY, uint32_t templateIdx, uint32_t attr, int layer);
  // emitMenuSprites: reproduces FUN_8007e6dc — the SPRITE sibling of emitMenuFt4. Same template
  // resolution (idx -> table @0x80017334 -> header -> `count` 16-byte entries), but each entry becomes
  // one SPRT (op 0x64/0x65): u,v = entry[0,1] with the ONE texture corner, size w,h = entry[10,11],
  // position = anchor + entry[14,15], clut = entry[2,3], and ONE tpage for the whole group taken from
  // the FIRST entry ([6,7]). Used for sprite-template groups such as the Controls page's pad diagram
  // (template 225); do NOT route those through emitMenuFt4, whose per-vertex UV bytes ([4,5]/[8,9]/
  // [12,13]) are only meaningful for FT4 templates.
  void emitMenuSprites(int anchorX, int anchorY, uint32_t templateIdx, uint32_t attr, int layer);

  // narrationSwirlRender: native producer for the SOP intro-narration full-screen SWIRL — the type-0x20
  // render node (0x800ED9E8) whose CUSTOM render fn (node+0x18 = 0x8010BF54, SOP overlay) the substrate
  // walk dispatches via its default case. Draws the 36-byte quad-record mesh @0x8010CC08 twice (two
  // blades, +0x800 yaw apart) with the RE'd transform chain in host float. Read-only; real depth.
  // See game/render/narration_swirl.cpp for the full RE. Dispatched from fieldObjectsRender's walk.
  void narrationSwirlRender(uint32_t node);

  // fxSpriteRender: native producer for the FUN_80027A4C world-anchored SCALED-SPRITE family (torch/roof
  // flames — kanban #12/#23). The type-0x20 render node whose custom fn (node+0x18) is one of the three
  // emitters FUN_80027CB4 (uniform scale) / FUN_80027E5C (scale*node[6]>>4) / FUN_800281EC (per-particle
  // loop). Projects the node's WORLD anchor(s) natively through projComposeCamera (the fps60-lerped scene
  // camera) — so the flame LERPS like every other native producer — derives the base pixel scale from the
  // RTPS depth-cue relation the emitters set up (MAC0 = n·DQA, DQA=6/4, n=(H·0x20000/SZ3+1)/2), then walks
  // the 8-byte quad records host-side. Read-only, real depth. See game/render/fx_sprite.cpp for the full
  // RE. Dispatched from fieldObjectsRender's type-0x20 walk.
  void fxSpriteRender(uint32_t node);
  // fxSpriteEmit: the family body, with the EMITTER given explicitly. fxSpriteRender passes the node's
  // own render fn; a composite dispatcher (impactBurstRender) passes the emitter it actually calls.
  void fxSpriteEmit(uint32_t node, uint32_t emitterFn);
  // impactBurstRender (FUN_80033080, kanban #15): the composite weapon-impact node's byte-scaled
  // sprite half. Body in fx_sprite.cpp.
  void impactBurstRender(uint32_t node);
  // impactPlumeRender (FUN_800288AC): the same composite node's packed-mesh half, rebuilt from its
  // persistent animation record, angles and anchor. Body in fx_impact.cpp.
  void impactPlumeRender(uint32_t node);

  // objectHighlightRender (FUN_8002AE0C): queue A's mesh/tether tail, rebuilt from the node's
  // persistent angles, anchor and highlight scale in the read-only display pass. The guest leaf still
  // owns its scratch/GTE/packet writes. Body in game/render/object_highlight.cpp.
  void objectHighlightRender(uint32_t node, int32_t scaleInput);

  // The FUN_800328EC family (game/render/fx_sprite.cpp). FUN_800328EC is a three-instruction wrapper
  // that zeroes the depth cue and tails into FUN_8002847C — the SAME four-corner writer
  // fxAnimSpriteRender reproduces — but its controllers carry a different node layout, and none of
  // them was dispatched, so nothing reached the picture.
  //   altSpriteEmit          — the shared tail: project the anchor, gate, emit the model list.
  //   fxAltAnimSpriteRender  — FUN_8012E868, the animation-script member.
  //   waterJetSpriteRender   — FUN_8013D454's sprite branch (its mesh branch remains unported).
  // One emission of the family, as data. Every field is something a controller genuinely varies —
  // FUN_8012D9E8 alone differs in the anchor offset, the scale shift, and (separately) the gate bias
  // and the depth bias, so a positional argument list stopped being readable at four callers.
  struct AltSprite {
    uint32_t node = 0;
    uint32_t anchorX = 0; // world anchor; Y and Z follow at +4 and +8
    uint32_t rec0 = 0;    // the four-corner record list to emit
    // SIGNED. FUN_8012D9E8 reads its numerator with a sign-extending lh (`(int16_t)mem_r16(node+0x70)`),
    // and a negative numerator is meaningful — it mirrors the model. Holding it unsigned turned a small
    // negative into ~65536x the intended scale; caught 2026-07-28 by the static-RE verify pass.
    int32_t numerX = 0, numerY = 0;
    int dqa = 6;
    int gateBias = 0;  // what FUN_800317CC's OT-key range gate is given
    int depthBias = 0; // what the controller then applies to the key (often the same, not always)
    int shift = 8;     // scale = MAC0 * numer >> shift
  };
  void altSpriteEmit(const AltSprite &a);
  void fxAltAnimSpriteRender(uint32_t node);
  void waterJetSpriteRender(uint32_t node);
  void fxRotSpriteTailRender(uint32_t node);
  // fxCuedSpriteRender (FUN_80113768, A0A/area 10): the family member that DRIVES the writer's depth
  // cue (IR0 = node[7] << 5, far colour black) instead of programming the identity, and shifts MAC0
  // before scaling rather than after. Found by the 22-area nofx sweep. Body in fx_sprite.cpp.
  void fxCuedSpriteRender(uint32_t node);

  // fxRingSpriteRender (FUN_80110C14, A0D/area 13): the family member that draws 21 sprites from ONE
  // node — an arc of items around the node's anchor, each with its own radius, bob phase and size,
  // record list picked per item out of a 16-slot MAIN.EXE bank. Body in fx_sprite.cpp.
  void fxRingSpriteRender(uint32_t node);

  // fxParticleFieldRender (FUN_8010C7F4, A0L/area 21): the 64-particle wind-blown field, and the only
  // four-corner-writer member that DRIVES the depth cue — IR0 is the distance from a reference point,
  // against a black far colour, so the field fades out with range. Body in fx_sprite.cpp.
  void fxParticleFieldRender(uint32_t node);

  // fxMotionTrailRender (FUN_801113B4 -> FUN_80110B00, A03/area 3): the screen-space ADDITIVE motion
  // trail — an 11-slot screen-position history joined into a glowing two-tier ribbon. NOT a
  // sprite-family member: no GTE, no projection, no DQA, so SpriteAnchor does not apply.
  // Body in fx_trail.cpp.
  void fxMotionTrailRender(uint32_t node);

  // fxDotFieldRender (FUN_801110BC, A0B/area 11): the camera-following DOT HAZE — 513 white specks on
  // a wrapping 2048-unit world lattice, sized 2x2 when near. NOT a sprite-family member (no DQA, own
  // gates). Body in fx_dotfield.cpp.
  void fxDotFieldRender(uint32_t node);

  // fxBackdropPlaneRender (FUN_80110CA4, A0E/area 14): the backdrop PLANE + its mirrored reflection
  // and the additive glow band along the seam (kanban #48). Scene geometry, not a sprite-family
  // member. Only HALF the guest render fn — its 0x801104D0 sprite tail is unported.
  // Body in fx_backdrop_plane.cpp.
  void fxBackdropPlaneRender(uint32_t node);

  // fxBackdropSparkRender (FUN_801104D0, A0E/area 14): the backdrop's 200-slot spark pool — the tail
  // half FUN_80110CA4 tail-calls (kanban #67). Draw-only: the guest's own body keeps the pool
  // simulated, so this reads slot state and emits. Body in fx_backdrop_plane.cpp.
  void fxBackdropSparkRender(uint32_t node);

  // fxMoteStreakRender (FUN_80116904, A08/area 8): 32 world motes drawn as doubled-length motion
  // STREAKS from each mote's current screen position past its previous one. One-frame-differential,
  // so it carries its own host-side screen-position shadow (mMoteStreaks) — the guest's array is not
  // safe to read. NOT a sprite-family member. Body in fx_motes.cpp.
  void fxMoteStreakRender(uint32_t node);

  // fxAnimSpriteRender: native producer for the SECOND world-anchored sprite family — emitter
  // FUN_800286CC + packet writer FUN_8002847C (36-byte, four-corner, per-vertex-coloured quad records
  // selected by an ANIMATION SCRIPT byte). This is Tomba's movement DUST PUFF (kanban #39) and the
  // same family draws the impact starburst. Anchored and scaled exactly like fxSpriteRender (world
  // anchor at node+0x2C/0x30, DQA=6 depth-cue scale) but with a per-axis scale pair at node+0x34 and
  // the frame's record list looked up in node+0x50[script byte]. Projects natively through the
  // fps60-lerped scene camera, so the puff interpolates. Read-only, real depth. See
  // game/render/fx_sprite.cpp. Dispatched from fieldObjectsRender's type-0x20 walk.
  void fxAnimSpriteRender(uint32_t node);

  // spriteRecordsEmit: the ONE host-side walk of the FUN_80027A4C 8-byte quad-record format — the
  // record list drawn about a native-projected float anchor at (scaleX,scaleY), with the writer's DPCS
  // depth cue applied to each record colour (ir0 = 0 -> identity, as the flame emitters program it).
  // Shared by fxSpriteRender and a0fVortexRender. Body in fx_sprite.cpp. Read-only.
  void spriteRecordsEmit(uint32_t rec0,
                         uint32_t clutPage,
                         float anchorXf,
                         float anchorYf,
                         float od,
                         int32_t scaleX,
                         int32_t scaleY,
                         int32_t ir0 = 0,
                         const int32_t *farColour = nullptr);

  // WORLD LINES — ropes / chains / tethers (game/render/fx_line.cpp, kanban #56 systemic + #54).
  // pc_render had no line primitive at all, so every GP0 line the guest emits (op 0x40..0x5F) was
  // invisible: the bucket's rope, the fisherman's line, hanging chains. worldLineDraw is the port of
  // the shared leaf FUN_8013DD34 — the stroke between two WORLD points, projected natively (so it
  // lerps at fps60) and expanded HERE into screen-space quads, which keeps the render queue quads-only.
  // The three callers are ported alongside it, each reading its own object's state:
  //   ropeAnchorRender  (FUN_8013E9D8) — a hanging object's rope up to the object at node+0x14
  //   ropeChainRender   (FUN_8013EA64) — the 8-point chain the node carries at node+0x60
  //   tetherLineRender  (FUN_80122974) — the 4-mode tether, mode 3 = the 8-segment fishing line
  // Dispatched from fieldObjectsRender (type-0x20 render fn, and the type-1 object class).
  void worldLineDraw(int ax, int ay, int az, int bx, int by, int bz);
  void ropeAnchorRender(uint32_t node);
  void ropeChainRender(uint32_t node);

  // shockwaveRingRender (FUN_8013E08C): the expanding ring — a 15-point radius-256 circle in the XZ
  // plane, scaled and faded by the single field node+0x50, drawn as 7 overlapping 3-point spans with
  // a doubled (+2,+1) outline stroke. See fx_line.cpp for the full RE.
  void shockwaveRingRender(uint32_t node);
  void tetherLineRender(uint32_t node);

  // BEAM / SEE-SAW ribbon (game/render/fx_beam.cpp) — the native producer for guest FUN_8003B704,
  // one of the three submitQuad caller classes the 2026-08-04 tap retirement left picture-less
  // (docs/unported-render-inventory.md R2). A textured ribbon of half-width 0x14 stretched from the
  // scene's tracked anchor *(0x800E7F5C) to the node's own world position, split at the midpoint
  // when the node's kind field asks for it. The emitter loads the PURE CAMERA into the GTE control
  // registers before projecting, so its corners are world-space and this producer needs nothing but
  // the node's own angles/position and the native camera — no register read-back, no tap.
  // beamNodeReached answers "does FUN_8003EEC0's own jump table route this node to the emitter",
  // read live from guest memory so the dispatch cannot drift from the game's routing.
  bool beamNodeReached(uint32_t node) const;
  void beamQuadRender(uint32_t node);

  // ropeStripRender — FUN_801365C4's picture (game/render/fx_rope_strip.cpp): the tiled vertical
  // quad strip (rope / cable / chain) that reaches the guest picture only through FUN_8003B320, a
  // substrate mirror that emits no native primitive. `length` is the emitter's a1, tiled in 120-unit
  // segments. Its transform contract belongs to its ONE caller FUN_80136748, which composes a
  // camera-only rotation — so the corners are world space and this projects them with the native
  // scene camera. See that file's banner and kanban #103.
  void ropeStripRender(uint32_t node, int32_t length);

  // impactRingRender (game/render/fx_ring.cpp): native producer for the IMPACT ANNULUS — the type-0x20
  // node whose render fn is 0x8002ECD8. It resolves the ring's screen centre + pixel scale (world
  // anchor through the native camera, or the fixed HUD position) and animates the inner/outer radii
  // from the single byte node+5, then draws through impactAnnulusDraw.
  // impactAnnulusDraw is the shared leaf FUN_8002E680: a flat gouraud annulus built from 5 authored
  // wedge angles replicated over their 8 dihedral images, with a radial half->full colour gradient and
  // additive blend. It is kept separate because the guest leaf has ELEVEN call sites — the other ten
  // reach it from the A01/A06/A08/A0J overlays and can be ported onto this same producer. Read-only.
  void impactRingRender(uint32_t node);
  void impactAnnulusDraw(float cx,
                         float cy,
                         float ord,
                         float scale,
                         int innerR,
                         int outerR,
                         uint32_t colInner,
                         uint32_t colOuter,
                         int layer,
                         int orderMode);

  // a0fVortexRender (game/render/fx_vortex.cpp): native producer for area 15's central PORTAL — the
  // type-0x20 node whose custom render fn is the A0F overlay's FUN_801143C4 (kanban #44). Three layers:
  // the node's own animated sprite at the world anchor, a rotating SPHERE of particle sprites built
  // from the node's radius, and (in state 1) a three-glow lens flare along the anchor→screen-centre
  // line. All projected with the native (fps60-lerped) camera. Read-only.
  void a0fVortexRender(uint32_t node);

  // meshQuadRecordsEmit (game/render/mesh_quads.cpp): the ONE host-side walk of the engine's packed-mesh
  // quad-record format (FUN_80027768's 36-byte records). Projects every vertex through the ACTIVE object
  // xform (projSetActive first), applies the caller's material style, and draws each record as a
  // native world quad. Returns the number drawn.
  // `ot` carries the emitting controller's sort-bias argument when the producer has RE'd it; see
  // MeshOtBias in mesh_quads.h for why that is opt-in and what the default preserves. `screenBbox`
  // (x0,y0,x1,y1) is UNIONED with what this call actually emitted, so a producer can report the box
  // it claims to have drawn into and an A/B pixel diff can be checked against it — the caller seeds
  // it inverted and reads it back.
  // MeshQuadStyle also carries the writer's a1 CLUT-row argument and the independently RE'd fog/tpage
  // policy of FUN_8013CDD4. Each defaults to a bit-neutral value; callers opt into only facts proved
  // at their own controller.
  int meshQuadRecordsEmit(uint32_t mesh,
                          const MeshQuadStyle &style,
                          const MeshOtBias &ot = MeshOtBias{},
                          float *screenBbox = nullptr);

  // propQuadRender (game/render/prop_quad.cpp): display-pass picture owner for FUN_8013CDD4's GT4
  // prop quads (drum / windmill caps). Rebuilds the transform and material policy from persistent
  // object/node state and reuses meshQuadRecordsEmit; never reads GTE state, guest packets, or OT.
  void propQuadRender(uint32_t object);

  // radialPlumeRender (game/render/fx_plume.cpp): native producer for the FOUR-COPY RADIAL PLUME —
  // the type-0x20 node whose custom render fn is FUN_8002BC9C, the most resident of the effect-mesh
  // controllers the 2026-08-04 tap retirement left with no producer at all (docs/unported-render-
  // inventory.md row R1). Draws the animation script's current mesh four times, a quarter turn apart
  // about the node's own Y axis, from the node's own angles/position — no GTE register is read.
  void radialPlumeRender(uint32_t node);

  // rigidMeshEffectRender (game/render/fx_rigid_mesh.cpp): native display producer for A00 guest
  // FUN_8013ED08. Rebuilds its packed-mesh transform from node position/angles/scale bytes, applies
  // the controller's explicit identity depth cue, and projects through the interpolated native camera.
  void rigidMeshEffectRender(uint32_t node);

  // swingStarburstRender (game/render/fx_swing.cpp): native producer for the ten-copy lavender
  // weapon-swing starburst emitted by FUN_8002A834 through the shared packed-mesh writer. Rebuilds
  // every transform from the controller's own particle bytes + world anchor; no GTE readback/tap.
  void swingStarburstRender(uint32_t node);

  // dustEffectRender (game/render/fx_dust.cpp): native producer for Tomba's movement DUST PUFF
  // (kanban #39) — the type-0x20 node whose custom render fn is FUN_80029F6C. Draws the additive
  // position-history TRAIL and the four-copy puff MESH from the node's own ring/age state, projected
  // natively so both layers lerp at fps60. Read-only. Dispatched from fieldObjectsRender's walk.
  void dustEffectRender(uint32_t node);
  int dustTrailEmit(const EffectPoints &pts, const EObjXform &cam, int sub);
  void dustPuffEmit(uint32_t node, const EffectPoints &pts, int sub);

  // The effect-node interpolation tier (game/render/effect_lerp.h): world points the effect producers
  // own, captured on the real frame and blended on the fps60 in-between so effects lerp like the rest.
  EffectLerp mEffectLerp;
  GuestRngMirror mRngMirror; // read-only PRNG stand-in (claim C018) — never writes 0x80105EE8
  MoteStreaks mMoteStreaks;  // area-8 streaks: last logic frame's projected mote positions

  // fieldHudRender (game/render/field_hud.cpp): native producer for the field HUD family
  // FUN_80025D98 -> {FUN_80025744 status row, FUN_80025934 item ring, FUN_80025B78 weapon strip}
  // (kanban #13 — the equipped-weapon strip was entirely absent under pc_render). Reads the HUD
  // state struct 0x800ED058 + the same gate globals the guest dispatcher reads; emits RQ_HUD quads
  // via the generalized emitUiFt4/emitUiSprites cores below. Read-only.
  void fieldHudRender();
  void fieldHudStatusRow();                                   // FUN_80025744
  void fieldHudItemRing(int offsetMode, uint32_t bucketAttr); // FUN_80025934
  void fieldHudWeaponStrip();                                 // FUN_80025B78
  // fieldHudMinimap (game/render/minimap.cpp): native producer for the area MINIMAP the HUD dispatcher
  // routes to the overlay-resident drawers 0x80113628 (area mode 2) / 0x801140A0 (mode 7) — kanban #43.
  // Draws the 64x64 map image plus the blinking player dot, the dot placed by the area's own linear
  // world->map transform. Read-only, RQ_OVERLAY.
  void fieldHudMinimap(int areaMode);
  // emitUiFt4/emitUiSprites: the GENERAL forms of FUN_8007e1b8 / FUN_8007e6dc (template ptr + data
  // base + placement {x,y,wOverride,hOverride} + attr {mode/color byte, clut-override|semi-flag}) —
  // the menu wrappers below keep their fixed menu bases. mode-nibble cases other than 0 (flip/rotate
  // variants) are not yet built: those entries draw nothing and warn once (`fieldhud` channel).
  void emitUiFt4(int x,
                 int y,
                 int wOv,
                 int hOv,
                 uint32_t templPtr,
                 uint32_t dataBase,
                 uint8_t attrByte,
                 uint16_t clutSemi,
                 int layer,
                 int daX0 = 0,
                 int daY0 = 0,
                 int daX1 = 1023,
                 int daY1 = 511);
  void emitUiSprites(int x,
                     int y,
                     uint32_t templPtr,
                     uint32_t dataBase,
                     uint8_t attrByte,
                     uint16_t clutSemi,
                     int layer,
                     int daX0 = 0,
                     int daY0 = 0,
                     int daX1 = 1023,
                     int daY1 = 511);
  // cineBarsRender: native producer for the cinematic LETTERBOX bars (UI-effect manager slot type 1,
  // base 0x80100400). Reads the slot table read-only and emits the top/bottom black bars. Emits nothing
  // when disarmed. See game/render/cine_bars.cpp. Call from cutscene-capable scenes.
  void cineBarsRender();

  // fieldObjectsRender: the field's OBJECT pass — walk the 3 entity lists + Tomba's G-block and flush each
  // live object's geomblk (perObjFlush → projComposeObject → gt3gt4). Factored OUT of sceneNative (which
  // mutates per-frame trust latches and can't be re-run) so Fps60's interp present can RE-RUN it under the
  // lerped per-object transforms (mObjOverrideOn) — the SAME object walk the real frame ran
  // (docs/fps60-rework.md step 2b). Pure reads + queue emits, no state mutation.
  void fieldObjectsRender();

  // ---- kanban #72 / #55 instrument: RING-NODE (stunned-enemy stars) CENSUS -----------------------
  // Answers "why are the stars absent" in ANY run, including one where they never appear. It runs
  // BEFORE fieldObjectsRender's `node+1 == 0` visibility skip, because the ring node is INVISIBLE and
  // is therefore structurally unreachable by `nofx` (which sits behind that skip and so can only ever
  // report nodes that are already being drawn — a blind spot, since the whole symptom is invisibility).
  // Emits ONE line per frame carrying its DENOMINATOR (nodes walked across all 3 heads, type-0x20
  // count) so "no ring nodes" is distinguishable from "the walk never ran". For each ring node it
  // prints the owner pointer node+0x14 and the two bytes the behaviour 0x8002B7B0 derives from it —
  // the retire gate *(owner+0x1B)&0x40 and the visibility source *(owner+1) — which is exactly the
  // state that decides whether the stars live or self-retire on their first tick.
  void ringNodeCensus();

  // ---- HEADS[0] flush-all census (kanban #77) --------------------------------------------------
  // The guest has NO render walk over HEADS[0] (0x800FB168) — its nodes reach vanilla's picture only
  // through the cull's class-keyed render queues, consumed by gen_func_8003BB50. This walk flushes the
  // whole list instead, which is how geometry vanilla never shows gets on screen. The census measures
  // the arm; it does NOT gate it. Read the banner in render_walk.cpp before changing that — it records
  // the two gates that were tried and measured wrong.
  bool nodeInCullRenderQueue(Core *c, uint32_t node); // DIAGNOSTIC ONLY (queues are reset by now)
  static constexpr int H0_CAP = 128;                  // recorded nodes per frame; beyond it we count
  struct H0Node {
    uint32_t node;
    uint8_t objClass;
    uint8_t type;
  };
  struct H0Census {
    int frame = -1;
    long live = 0, queuedNow = 0, overflow = 0;
    long byClass[16] = {0};
    int n = 0;
    H0Node node[H0_CAP];
  };
  H0Census mH0;
  void heads0Census(uint32_t node);
  void heads0Flush(int frame);

  // ---- BILLBOARD display-pass producer (#67 RE work — REDIRECT doctrine, USER: both frame kinds
  // derive from game state) ---------------------------------------------------------------------
  // The billboardEmit particle system (perobj_billboard.cpp — AP gems / flames / apples / splash /
  // windmill sprites) runs at GUEST-EXECUTION time inside the substrate render walk; its packets are
  // guest state, but its PICTURE must come from the display pass so the fps60 interp re-run derives
  // it under lerped inputs like every other world prim. Each real frame, billboardEmit RECORDS one
  // BbRec per emitted particle (host memory only): the particle's local quad corners (the ×5 ints
  // func_8003B220 built), the node's composed MAT_OUT rotation + world anchor, and the RESOLVED
  // material words (post node+92 override + node+13 case patches). billboardsRender (called from
  // fieldObjectsRender, so field + hut + tier1Render's interp re-run all reach it) projects each
  // record through the SAME float camera path the world uses (sceneCam choke — fps60-lerped at the
  // interp present) and emits an RQ_WORLD quad with real per-particle identity (dbg_node=node).
  struct BbRec {
    uint32_t node, particle;                 // identity (node = dbg_node; particle addr = stable key)
    int16_t cx[4], cy[4];                    // local corner ints (already ×5; z=0 in local space)
    float rotR[3][3];                        // the NODE's own object rotation, rebuilt in float from the
                                             // node's euler/scale fields (BbObjectRot) — unit scale, no
                                             // camera in it and no GTE register read.
    float wx, wy, wz;                        // world anchor = the node's own (s16 +46, +50, +54)
    uint32_t wColor, wUv0, wUv1, wUv2, wUv3; // resolved record words BUF+4/+12/+20/+28/+36
  };

  // The object rotation the ACTIVE billboard compose variant owns, published by that compose method
  // before it hands off to billboardEmit and consumed by billboardEmit's record capture. It is a
  // recomputation from the node's own fields, never a read-back of what the substrate composed —
  // see the BbObjectRot banner in perobj_billboard.cpp. `mBbRotValid` is false outside a compose,
  // and billboardEmit records nothing then, so a future 5th compose sibling that forgets to publish
  // drops its particles instead of silently inheriting the previous node's rotation.
  float mBbRot[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  bool mBbRotValid = false;
  std::vector<BbRec> mBbRecs; // this logic frame's records, guest-walk emit order
  // (No BbRec prev buffer: effect particles have no stable cross-frame identity — the sub-lists
  // reuse/walk particle addresses — so they never lerp; they draw at their own frame's state under
  // the lerped camera.)
  //

  void bbFrameReset() {
    mBbRecs.clear();
  } // per logic frame, pre-walk

  void billboardsRender(); // display-pass producer: project + emit every BbRec

  // emitRecordQuad — the shared "already-projected quad + PSX material words" emit used by every
  // display-pass producer (billboardsRender's particles, CubeTextBanner's glyphs and planks). Takes
  // screen-space corners + per-vertex depth and the four FT4/GT4 material words, decodes tpage/clut
  // RAW out of them (NOT from GpuState's live s_tp_*/s_da_*, which hold unrelated stale state at
  // display time) and pushes one RQ_WORLD quad tagged with the owning node.
  //   wCol[0]>>24 = the GP0 op byte (bit0 raw, bit1 semi); wUv0>>16 = clut; wUv1>>16 = tpage.
  static void emitRecordQuad(Core *c,
                             uint32_t node,
                             const uint32_t wCol[4],
                             uint32_t wUv0,
                             uint32_t wUv1,
                             uint32_t wUv2,
                             uint32_t wUv3,
                             const float *px,
                             const float *py,
                             const float *dep);

  // textLabelEmit (FUN_80039F4C, text_label.cpp): the per-character 3D text-label renderer —
  // a faithful substrate-mirror orchestrator (mesh pass + per-char glyph packets, all callees still
  // substrate). It produces GUEST PACKETS ONLY: the display-pass capture it used to do was a tap and
  // is deleted, so this node class has no pc_render picture (portmap render-producer-cube-text-banner).
  void textLabelEmit();

  // fieldEntityRender: world-space GT3/GT4 scene-table renderer. Walks the entity-list struct at
  // `es` (per-list ptr headers at es+0x10..; packed geometry base at es+0xC; count at es+6),
  // dispatches each entry through submit_poly_gt3_native / submit_poly_gt4_native under the
  // camera-composed eproj xform. Called for the ground scene table 0x800F2418 (in Render::frame
  // groundnative diag branch and in the field object walk in render_walk). Was
  // ov_field_entity_render (taxi-parameter c->r[4]).
  void fieldEntityRender(uint32_t es);

  // backdropRender (guest overlay FUN_80115598, the seaside field's state-0 background drawer): draws
  // the scrolling sky/parallax tilemap as native RQ_BACKGROUND quads. Reads the PARALLAX_BG_SM struct at
  // `t4`; its only per-frame-varying inputs are the scroll offsets at t4+0x28/+0x2A (game-logic-driven —
  // ParallaxBg::step, not camera projection), read through Fps60::bgScroll so Fps60's Tier-1 present-time
  // re-render (fps60.cpp tier1Render) can feed it a wrap-aware lerped scroll instead of a guest read. Was
  // the file-local `render_bg_tilemap_native` (render_walk.cpp); promoted to a method so tier1Render can
  // call the SAME pass, mirroring terrainRenderAll/fieldEntityRender.
  void backdropRender(uint32_t t4);

  // Area 21's reached early phase selects a four-quad gouraud gradient and returns before the later
  // tilemap branch. These methods own that picture and its one temporal input. The guest helper
  // remains the sole owner of packet-pool/OT writes.
  bool area21SkyGradientActive() const;
  void area21SkyGradientCapture();
  // area21SkyGradientRender: guest picture producer FUN_8010BB64.
  void area21SkyGradientRender(float t);
  void area21SkyGradientSwapPrev();

  // backdropTexpagePublishTick — publish THIS frame's backdrop atlas texpage (the VRAM page the guest's
  // own background drawer samples), the key gpu_native's OT walk uses to band the guest's 16x16 backdrop
  // tiles RQ_BACKGROUND instead of RQ_HUD. Per-LOGIC-FRAME guest-state tracking, so Engine::drawOTag runs
  // it BEFORE the render-mode branch, exactly like areaCacheTrustTick — publishing it from inside a
  // pc_render producer is what made psx_render paint the sea over the whole world. Read-only.
  void backdropTexpagePublishTick();

  // backdropTilemapDrawer — resolve the resident field/narration backdrop drawer the guest would
  // dispatch (gen_func_8003DF04 @0x8003DF04) and report whether it is the SHARED tilemap routine (the one
  // that reads PARALLAX_BG_SM @0x800ED018 and emits 16x16 textured-sprite tiles) plus its baked per-tile
  // V texel bias into `vAdd`. Every area's backdrop drawer is that same routine compiled per overlay; the
  // only thing that varies is the V bias (seaside 0x80115598 samples (tile&0xF0)+8, every other area
  // (tile&0xF0)), which is read straight from the resident drawer's code — ground truth per area, no
  // scene heuristic. Returns false when the resident drawer is NOT the shared tilemap routine: area 14's
  // backdrop is GTE scene geometry (#47), while area 21's reached early branch is the separately owned
  // gouraud gradient and returns before its phase-dependent tilemap loop. Read-only (no guest writes).
  // `drawerVAOut` (optional) receives the RESIDENT drawer's address, and only when the resolution
  // SUCCEEDS — it is the producer DB's key for backdropRender's prims, per area rather than hardcoded.
  bool backdropTilemapDrawer(int &vAdd, uint32_t *drawerVAOut = nullptr);

  // ---- SUBSTRATE MIRROR: per-object cmd-list dispatch (guest FUN_8003CDD8 / FUN_8003F698) --------
  // These run UNDER the render-underneath architecture (issue #32): the substrate walk cluster calls
  // them as PLAIN intra-shard C calls (func_8003CDD8/func_8003F698), never through rec_dispatch, so
  // they are owned via the shard override table (shard_set_override) — same mechanism RenderObserver
  // already uses for the sibling 0x8003xxxx render leaves. Guest-writing here is LEGAL/REQUIRED (the
  // scratchpad GTE compose + the OT/packet-pool writes made by the callee are part of BOTH SBS cores'
  // faithful RAM) — this is NOT the read-only pc_render display pass. See perobj_dispatch.cpp for the
  // full RE (byte-exact substrate mirror via the shared gte_op/gte_write_ctrl primitives, not a
  // pc_render rebuild).
  // cmdListDispatch (FUN_8003CDD8): a0=node (r4), a1=flag (r5). For each active render command on the
  // node's persistent list, composes the WORLD object transform (camera-rot x object-local, via the
  // same MVMVA ops submit.cpp's later-133 comment already RE'd) into CR0-7, then dispatches through
  // perModeDispatch.
  void cmdListDispatch();
  // perModeDispatch (FUN_8003F698): a0=geomblk (r4), a1=otbase (r5), a2=flag (r6). Selects the area's
  // per-mode renderer via the mode-select byte + jump table, or falls back to the generic GT3/GT4
  // packet emitter (func_800803DC) when the generic-force flag or a2&1 is set.
  void perModeDispatch();

  // ---- SUBSTRATE MIRROR: per-object render-type dispatch + billboard/special-effect leaves --------
  // Guest FUN_8003CCA4 / FUN_8003C2D4 / FUN_8003C464 / FUN_8003C8F4 — the walk cluster's per-object
  // TYPE dispatch (CCA4) and 3 of its "special effect" leaf renderers (C2D4/C464/C8F4 build a
  // camera-composed billboard quad per particle and emit it into the OT/packet pool — see
  // perobj_billboard.cpp for the full RE). Same ownership mechanism as cmdListDispatch/perModeDispatch
  // above: reached as plain intra-shard C calls, owned via shard_set_override. These were previously
  // wrapped by the transparent RenderObserver (depth-tag) wrapper — CCA4/C2D4/C464/C8F4 fold that
  // wrapping in directly now that they're native (see perobj_billboard.cpp), so RenderObserver no
  // longer wraps these 4 addresses.
  // perObjRenderDispatch (FUN_8003CCA4): a0=node (r4). Case 0/4 funnels here call cmdListDispatch();
  // the other cases wrap it with one of 5 still-substrate special-effect leaves.
  void perObjRenderDispatch();
  // billboardCompose1/2 (FUN_8003C2D4 / FUN_8003C464): a0=node (r4). Compose a pure Z-rotation "local"
  // matrix (C464 additionally seeds its base operand via the still-substrate FUN_800517BC) through the
  // persistent camera MATRIX, transform the node's world position, then hand off to billboardEmit.
  void billboardCompose1();
  void billboardCompose2();
  // billboardCompose3 (FUN_8003C788): a0=node. Seeds MAT_A=identity, folds the node's stored matrix
  // (node+152) via matMul into MAT_ROTZ, runs the shared CAM2 world-translate tail on MAT_ROTZ → billboardEmit.
  void billboardCompose3();
  // billboardComposeC5F8 (FUN_8003C5F8): a0=node. Fourth sibling — like C2D4 but builds a FULL
  // 3-Euler-angle local rotation (Math::rotMatSoft on the SVECTOR @ node+84) into MAT_ROTZ, then the
  // same matMul + CAM2 world-translate tail → billboardEmit.
  void billboardComposeC5F8();

  // ── Secondary EFFECT modifiers (game/render/effect_mod.cpp) ───────────────────────────────────
  // Five sibling post-passes over the render packet pool. perObjRenderDispatch records the pool
  // write pointer before and after building an object's packets, then hands that [lo,hi) span to
  // one of these to re-colour / re-blend what was just emitted. They MUTATE GUEST MEMORY (the pool
  // itself), so each is byte-faithful to its guest body — see docs/findings/render.md.
  //   node = the object node carrying the effect parameters; [lo,hi) = the packet span to rewrite.
  void effectSemiOn(uint32_t node, uint32_t lo, uint32_t hi);   // FUN_8003F3F4 — set semi bit
  void effectSemiOff(uint32_t node, uint32_t lo, uint32_t hi);  // FUN_8003F4C4 — clear semi bit
  void effectClutSwap(uint32_t node, uint32_t lo, uint32_t hi); // FUN_8003F344 — node CLUT -> packets
  void effectFlatTint(uint32_t node, uint32_t lo, uint32_t hi); // FUN_8003F594 — flat colour + semi
  void effectColorAdd(uint32_t node, uint32_t lo, uint32_t hi); // FUN_8003D584 — per-channel modulate

  // composeTintGate (FUN_8003EF9C): per-type render gate — snapshots the packet pool, emits the
  // object's geometry, and in mode 2 colour-adds over exactly the primitives just emitted. Modes
  // other than 0/2 draw nothing. See render/compose_tint_gate.cpp.
  static void composeTintGate(Core *c);

  // subPartWalk (FUN_8003F174): draws a node built from SUB-PARTS — for each entry of the node's
  // +0xC0 pointer array it loads that sub-part's own GTE transform (8 control words at sub+0x18)
  // and submits its geomblk (sub+0x40). Two distinct counts: node[+8] caps the walk from the top,
  // node[+9] is the do/while bound. See render/subpart_walk.cpp.
  static void subPartWalk(Core *c);

  // Set (as a counter) around a guest-time submit whose PICTURE a native display-pass producer has
  // taken over — currently only the cube-text banner's sub-parts (CubeTextBanner). The native GT3/GT4
  // submitters check it and skip ONLY their drawWorldQuad; every guest-visible effect still happens,
  // so this is a host-side handover, not a behaviour change. Without it the object is drawn twice,
  // and the guest-time copy has no valid eproj transform (the substrate feeds those submitters
  // through the GTE), so it lands hundreds of pixels off screen.
  int mNativeDrawSuppress = 0;

  // sharedTransformWalk (FUN_8003F07C): the sibling of subPartWalk — loads ONE transform (the frame's
  // view matrix from scratchpad 0x1F8000F8) and submits every sub-part under it. F174 is the
  // articulated case, this is the rigid one. See render/subpart_walk_shared.cpp.
  static void sharedTransformWalk(Core *c);
  // billboardEmit (FUN_8003C8F4): a0=node (r4), a1=flag (r5). Walks the node's active particle
  // sub-list, RTPT/RTPS-projects each particle's quad corners, culls off-screen, buckets into the OT
  // by averaged depth, and emits a 10-word (tag+9) GT4-style packet into the packet pool.
  void billboardEmit();

  // ---- SUBSTRATE MIRROR: the render-WALK loop (guest FUN_8003C048) --------------------------------
  // renderWalk (FUN_8003C048, no args): iterates the global render-node list (head @0x800F2624, next
  // ptr @node+36), skipping dead nodes (mem8(node+1)==0) and out-of-range case indices
  // (mem8(node+11)>=33), and dispatches each live node through a 33-entry jump table (@0x800104B8) to
  // one of: perObjRenderDispatch/billboardCompose1/billboardCompose2/billboardCompose3/billboardComposeC5F8
  // (owned siblings, reached natively via the override registry), several still-substrate leaves
  // (func_8003F174/EF9C/80039F4C/800726D4 +
  // rec_dispatch to 0x8012A43C/801295B4/80129114/8013DD58), a "generic particle" case (0x8003C188,
  // mode-4 direct dispatch or a func_8003B054+80084660/90+8003B320 packet-emit sequence), a fully
  // dynamic per-node dispatch through node+24 (case 0x8003C29C), and a no-op skip entry. THE point of
  // owning this loop: it is the caller CCA4Frame/CmdListFrame's register-faithfulness fixes assumed —
  // gen keeps the loop's node pointer and next pointer LIVE in the real r16/r17 registers (and two
  // constants in r18/r19) across every nested call, which the still-substrate callees' own prologues
  // spill as "caller state" (see perobj_billboard.cpp's CCA4Frame / perobj_dispatch.cpp's
  // CmdListFrame). See render_walk_dispatch.cpp for the full RE + the f118 residual this closes.
  void renderWalk();

  // ---- SUBSTRATE MIRROR: per-AREA-TYPE overlay dispatch (guest FUN_8003D0BC) -----------------------
  // overlayTypeDispatch (FUN_8003D0BC, a0=list — never touched by this fn's own body, plain
  // pass-through from the caller, gen_func_8003F9A8 passes SCENE_ENT_TABLE @0x800F2418): reads the
  // AREA_TYPE byte @0x800BF870 (the same render-mode-select byte perModeDispatch/renderWalk's case
  // 0x8003C188 both read); if >=22, no-op. Else dispatches through a 22-entry jump table
  // (@0x80014EF0) to one of 20 per-area-type overlay leaves — area type 0 reaches the ALREADY-OWNED
  // OverlayGroundGt3Gt4::entityLoop (FUN_801401B8) -> gt3/gt4; the other 19 are still-substrate. See
  // overlay_type_dispatch.cpp for the full RE.
  void overlayTypeDispatch();

  // ---- SUBSTRATE MIRROR: libgpu GPU-DMA completion-callback QUEUE (wide-RE, wired 2026-07-10) -----
  // 0x80082D04/0x80082FB4/0x80083364/0x80082424 — see game/render/wide_re_gpu_dma_queue.cpp for the
  // full RE (struct map, call graph, ring-entry layout). Reached as plain intra-shard C calls from
  // the substrate (NOT via rec_dispatch), owned via engine_set_override_main so the SBS oracle keeps
  // running the pure gen_func_* body.
  // Guest ABI args are read from c->r[4..7] inside each method (same convention as
  // perObjRenderDispatch/billboardCompose1 above) so the override thunk's `native(Core*)` shape
  // needs no per-address adapter.
  void gpuDmaQueueEnqueue(); // FUN_80082D04(fn,argValOrPtr,sizeBytes,arg3) -> queue depth / -1 timeout
  void gpuDmaQueueDrain();   // FUN_80082FB4() — also the GPU-DMA-completion ISR body
  void gpuDmaQueueSync();    // FUN_80083364(mode) — internal DrawSync-shaped blocking/poll
  void gpuDmaSend();         // FUN_80082424(arrayPtr,count) — the OT-linked-list DMA kick

  // ---- SUBSTRATE MIRROR: libgpu GPU-sys jump-table leaves DrawSync/ClearOTagR (wired 2026-07-10) ---
  // 0x80080F6C / 0x80081458 — see game/render/wide_re_libgpu_leaves.cpp for the full RE.
  void drawSync();   // FUN_80080F6C(mode)
  void clearOTagR(); // FUN_80081458(ot,entries)

  // ---- SUBSTRATE MIRROR: libgpu LoadImage()-internal chunked GP0-FIFO pixel streamer (wired 2026-07-10)
  // 0x80082734 — see game/render/wide_re_gpu_loadimage_streamer.cpp for the full RE (struct map,
  // control flow, dead-code note). Guest ABI: a0(r4)=rectPtr, a1(r5)=srcPtr; ret v0.
  void gpuLoadImageStream(); // FUN_80082734(rectPtr,srcPtr) -> -1 timeout/empty, else 0

  // ---- SUBSTRATE MIRROR: the 4 still-substrate OBJECT-LIST WALKERS reached from FUN_8003F9A8's
  // field-frame draw dispatcher (docs/findings/render.md "0x8003F9A8 474-prim attribution resolved") --
  // no args (guest ABI: none read), each walks a fixed object-pointer list/array, dispatching each live
  // entry's TYPE byte through a table to one of: the already-owned perObjRenderDispatch/
  // billboardCompose1/billboardCompose2 (this file's siblings), a handful of still-substrate leaves
  // (called as plain func_XXXX(c), matching gen), or a per-object vtable slot (rec_dispatch, per
  // CLAUDE.md — never dropped). See game/render/objlist_walk.cpp for the full RE (per-function
  // scratchpad cursor layout, jump-table addresses, case-label maps).
  void objListWalk1();         // FUN_8003BB50 — list @0x800F2410, cursor 0x1F80013C/146
  void objListWalk2();         // FUN_8003BCF4 — list @0x800F26C8, cursor 0x1F800148/152 (1st entry only)
  void objListWalk2Continue(); // FUN_8003BED8 — shared tail: continues objListWalk2's SAME guest frame
  void objListWalk3();         // FUN_8003BF00 — list @0x800F2738 (positional array), cursor 0x1F800154/15E
  void objListWalk4();         // FUN_8003EEC0 — list head *0x800F2738 (linked via node+0x24 "next")

private:
  // Native POLY_GT3/GT4 submitters (guest-ABI bodies: rec/otbase/count in r4/r5/r6).
  static void submitPolyGt3Native(Core *c); // gen_func_8007FDB0
  static void submitPolyGt4Native(Core *c); // gen_func_80080114
};
