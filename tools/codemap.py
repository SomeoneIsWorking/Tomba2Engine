#!/usr/bin/env python3
"""codemap.py — index the PC-native engine's reverse-engineering ownership by GUEST ADDRESS.

WHY THIS EXISTS (read this first): the native port has a thousand-odd hand-written reimplementations
of specific Tomba!2 functions (the live count is in the header of the generated docs/code-map.md —
never hard-coded in prose, which is how "~350" survived into three documents long after it was
wrong), scattered across game/**.cpp and the psxport runtime, each tied to a guest MIPS address only
by its symbol name (`ov_800753D4`), a header comment (`// 0x800753D4 — ...`), or — for the whole
anonymous-namespace family — by nothing at all except its `overrides::install` call site. There is
no other index. Worse, the OLD override system that wired address->native was
REMOVED (top-down PC-driven now), so the great majority of these natives are currently ORPHANED:
real, correct code that nothing calls. Without a map you cannot answer the one question that saves
hours — "is FUN_XXXX already owned natively, and where?" — so you re-derive code that already exists
(this tool was written after exactly that happened: native_cb_loadidx duplicated ov_load_texgroup).

WHAT IT DOES: scans the native sources, extracts for each native function:
  - the GUEST ADDRESS(es) it implements (from the `ov_<hex>` name and/or the header comment),
  - its file:line and symbol,
  - the addresses it DEPENDS on (rec_dispatch / call_fn / rc1..rc4 / super_call targets — i.e.
    still-PSX leaves it calls; these are the things that BREAK now overrides are gone),
  - which other native symbols call it directly (C call graph),
  - a reachability verdict: LIVE (transitively C-called from a native_boot dispatch root) vs ORPHAN.

USAGE:
  tools/codemap.py                      # write docs/code-map.md (the committed index)
  tools/codemap.py --addr 800753d4      # look up one address: who implements it + WHERE IT IS INSTALLED
                                        #   + who depends on it (warns ⚠ DUAL-OWNERSHIP if authoritatively
                                        #   owned in >=2 files, ⚠ CLAIM-WITHOUT-INSTALL if two files claim
                                        #   it by name and only one installs, and ⛔ DELIBERATELY ABSENT
                                        #   when docs/port-map.md says the layer was removed on purpose)
  tools/codemap.py --dup-installs       # guest addrs INSTALLED from 2+ files — the source twin of the
                                        #   runtime duplicate-owner abort; expect 0, trust this for #32
  tools/codemap.py --uninstalled-claims # addrs 2+ files claim BY NAME while only one INSTALLS: the other
                                        #   claim is a host-side twin or a stale orphan, never the owner
  tools/codemap.py --selftest           # prove the index can still answer POSITIVELY (see selftest())
  tools/codemap.py --shape-census       # how many LIVE examples of each ownership shape the tree holds,
                                        #   with the denominator. The tree's own answer to "is shape X
                                        #   still exercised here", and where --selftest fixtures come
                                        #   from — a fixture typed by hand is a fixture that can rot.
  tools/codemap.py --selftest-nc        # run --selftest against 4 deliberately-broken trees and require
                                        #   it to FAIL each. Proves the selftest is not a blanket yes.
                                        #   ~53s (it loads the index 4x), so it is NOT in the pre-commit
                                        #   gate; run it whenever you change selftest() or the scanner.
  tools/codemap.py --conflicts          # broad cross-file NAMING smell (over-reports inline helpers /
                                        #   consumers that install nothing; use --dup-installs + --addr)
  tools/codemap.py --substrate-fallthrough  # native-owned addrs that are a DISPATCH TARGET but NOT
                                        #   override-registered — callers silently hit the emulated
                                        #   substrate (register + MIRROR_VERIFY to native-ize; --all
                                        #   includes soft-attributed owners)
  tools/codemap.py --unowned-rank [f]   # THE PORT TARGET QUEUE: still-unowned guest fns ranked by
                                        #   recdep hotness (default f = scratch/logs/recdep_rank.txt),
                                        #   resolved through the same index --addr uses. `--top N`.
                                        #   Hotness is ONE input — cross-check `portmap.py next`.
  tools/codemap.py --orphans            # list owned addresses whose native is currently ORPHANED
  tools/codemap.py --stdout             # print the full markdown to stdout instead of writing the file
"""
import os, re, sys, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_GLOBS = ["engine/**/*.cpp", "engine/**/*.h", "game/**/*.cpp", "game/**/*.h",
             "runtime/recomp/**/*.cpp", "runtime/recomp/**/*.c"]
# The INSTALL-SITE corpus is deliberately WIDER than the native-definition corpus above. The PSX
# platform lives in the SUBMODULE (external/psxport/runtime/recomp/), not in a top-level runtime/ —
# `runtime/recomp/**` in SRC_GLOBS matches nothing in this checkout. That is fine for native DEFS
# (framework code is not this game's port surface) but NOT for override installs: mem.cpp installs
# 0x8009A420 (ov_guestMemset) from the submodule, and before this glob existed `--addr 8009A420`
# answered "NO native owner found" for an address with a live, shipping install. An address with a
# live install has an owner, period, wherever the install call site happens to live.
INSTALL_GLOBS = SRC_GLOBS + ["external/psxport/runtime/**/*.cpp", "external/psxport/runtime/**/*.c"]
# Native-dispatch ROOTS: symbols native_boot.cpp calls directly (top-down) to enter native code.
# Everything reachable from these by direct C call is LIVE; the rest is ORPHANED (was override-only).
ROOTS = {"ov_game_stage_main", "ov_start_bin_stage", "native_task0_bootstrap",
         "ov_game_main", "native_boot_run"}

DEF_RE  = re.compile(r'^\s*(?:static\s+)?(?:inline\s+)?[\w:*&<>]+\s+((?:ov_|native_|eng_|beh_|leaf_)\w+)\s*\(\s*Core\s*\*')
# PC-game-structure natives are C++ CLASS METHODS (e.g. `void Camera::lookAt()`), which take no Core*
# param (they hold it as a member). Index those too; the owned guest FUN_/addr is read from a trailing
# `// FUN_xxxx` on the def line or the comment block above (same association logic as free functions).
METHOD_RE = re.compile(r'^\s*(?:static\s+)?(?:inline\s+)?[\w:*&<>]+\s+(\w+::\w+)\s*\(')
# A SECOND class of native carries NO recognized prefix at all: a free function named
# `<description>_<hexsuffix>` (grid_query_47cbc, child_spawn_40410, hitbox_build_3b220, ...) that
# takes `Core*` exactly like an `ov_`/`native_` native and is tagged the same way (a `// FUN_xxxx —`
# comment immediately above, or — see FILE_HEADER_ADDR_RE below — a file-level header tag). DEF_RE's
# prefix requirement made every one of these invisible to `--addr` ("NO native owner found") despite
# being real, tagged, called-by-direct-C++-call natives. Only reached when DEF_RE/METHOD_RE both miss;
# still requires the `impl` (below) to be non-empty, so an untagged helper leaf (`s16`, `leaf1`, ...)
# that happens to take `Core*` first is NOT mistaken for a native (see the is_freefn skip-if-empty guard).
FREEFN_RE = re.compile(r'^\s*(?:static\s+)?(?:inline\s+)?[\w:*&<>]+\s+(\w+)\s*\(\s*Core\s*\*')
ADDR_RE = re.compile(r'0x(8[0-9A-Fa-f]{7})')
FUN_RE  = re.compile(r'FUN_(8[0-9a-fA-F]{7})')
# `leaf_<hex>` is the naming used by the bulk byte-faithful ports in game/core/field_owned_leaves.cpp
# (190 of them). Without `leaf` here every one answered `--addr` with "NO native owner found", so
# ownership questions about them were silently wrong — the exact failure that sent a 2026-07-21
# investigation down a chain of "all unowned substrate" conclusions that were not true.
NAMEHEX = re.compile(r'^(?:ov|native|eng|leaf)_([0-9A-Fa-f]{6,8})$')
DEP_RE  = re.compile(r'(?:rec_dispatch|rec_super_call|super_call|call_fn|rc[0-4]|rec_coro_redirect)\s*\(\s*c\s*,\s*0x(8[0-9A-Fa-f]{7})')
# A file-level header ("game/player/hitbox.cpp — PC-native ownership of FUN_8003B220.") tags the ONE
# guest address the file exists to own, for files where the tag sits at the top (file/module doc
# comment) rather than immediately above the def (e.g. hitbox.cpp's def is preceded by an unrelated
# one-line comment, with unrelated `#include`s and a tiny helper fn separating it from the real header).
# Deliberately strict — "ownership of FUN_xxxx"/"ownership of 0x..." immediately adjacent, so it does
# NOT fire on multi-native files whose header describes a SUBSYSTEM ("ownership of the engine's
# geometry SUBMIT path") or lists several addresses in prose (release_trigger_motion.h) — those already
# resolve per-method via their own adjacent tags.
FILE_HEADER_ADDR_RE = re.compile(r'ownership of\s+(?:FUN_(8[0-9A-Fa-f]{7})|0x(8[0-9A-Fa-f]{7}))', re.IGNORECASE)


def tag_portion(text):
    """A header line names the owner LEFT of the first separator (—/:/-). Anything to the RIGHT is
    description prose and may reference OTHER addresses (e.g. `// FUN_80107e20 — transition variant …
    1 effect 0x8003e264 …`); those must NOT be counted as owner tags."""
    for sep in ("—", " - ", ":"):
        if sep in text:
            return text.split(sep, 1)[0]
    return text


def comment_above(lines, i):
    """The single comment line immediately above line i (or "" if none) — i.e. the FIRST line of the
    contiguous `//` block right above a def/decl, which is where this tree's tag convention always
    puts the owned address (see scan_decl_tags for why scanning further lines is unsafe)."""
    c = i - 1
    if c >= 0 and lines[c].lstrip().startswith("//"):
        while c - 1 >= 0 and lines[c - 1].lstrip().startswith("//"):
            c -= 1
        return lines[c].strip()
    return ""


def file_header_addr(lines):
    """The leading contiguous comment/blank block at the top of the file (the file's own doc-comment,
    stopping at the first real code line), searched for the single-address 'ownership of FUN_xxxx'
    tag. Returns None for the common case (no such phrase, or a multi-native/subsystem file whose
    header doesn't name one specific address that way)."""
    block = []
    for ln in lines:
        s = ln.strip()
        if s.startswith("//") or s == "":
            block.append(ln)
        else:
            break
    m = FILE_HEADER_ADDR_RE.search("\n".join(block))
    return (m.group(1) or m.group(2)).upper() if m else None


def load_override_table():
    """Authoritative addr->symbol map recovered from the pre-removal override registrations
    (tools/codemap_overrides.tsv, snapshotted from git faeb436^). Ground truth for any native
    that predates the override removal; new top-down natives fall back to the name/comment heuristic."""
    sym2addrs, path = {}, os.path.join(ROOT, "tools/codemap_overrides.tsv")
    if os.path.exists(path):
        for ln in open(path):
            parts = ln.rstrip("\n").split("\t")
            if len(parts) == 2 and parts[0]:
                sym2addrs.setdefault(parts[1], []).append(parts[0].upper())
    return sym2addrs


def load_behavior_table():
    """Authoritative addr->symbol map for per-object behavior handlers registered in
    BehaviorDispatch::kTable (game/object/behavior_dispatch.cpp) — the pc_skip=true-only native
    shortcut table (see CLAUDE.md engine-overrides + game.h pc_skip). These `beh_*` fns take no
    address in their own name (unlike `ov_<hex>`) and their header comment sits at the TOP of their
    file, not adjacent to the def — so the name/comment heuristic below misses them entirely (this
    is what left the whole `beh_*` family — ~50 owned handlers — reporting 'NO native owner found'
    to `--addr`, even though they're live and gated correctly). The table itself is the ground truth."""
    path = os.path.join(ROOT, "game/object/behavior_dispatch.cpp")
    sym2addrs = {}
    if os.path.exists(path):
        for m in re.finditer(r'\{\s*0x([0-9A-Fa-f]{8})u\s*,\s*(beh_\w+)\s*,', open(path, encoding="utf-8").read()):
            sym2addrs.setdefault(m.group(2), []).append(m.group(1).upper())
    return sym2addrs


# --- PlatformHle ownership (the SECOND owning table, invisible to the source scan) -----------------
#
# This scanner only sees natives in game/ + runtime/. The BIOS/hardware-sync primitives are owned by
# a different mechanism entirely: PlatformHle (external/psxport/runtime/recomp/sync_overrides.cpp),
# wired from addresses the GAME states in GameConfig::hle (game/core/game_config.cpp). Nothing about
# that ownership appears as a native def in the scanned corpus, so `--addr` used to answer
# "NO native owner found" for every one of them — a FALSE NEGATIVE that reads as "free to port".
#
# That false negative has real cost: 0x800834A0 (gpuTimeoutArm) sat at the top of the `recdep`
# histogram at 33,152 calls while PlatformHle had owned it all along, and both `--addr` and the
# histogram said "port this". Porting it would have been a double-install of a primitive whose whole
# point is to NOT run the guest body (that body calls libetc VSync, which this port traps+aborts).
# overlay_router.cpp's recdep dump now annotates the same fact; this is its `--addr` counterpart.
#
# Both halves are parsed from source, never hardcoded, so a re-wiring of either file stays honest.
HLE_CFG_SRC = "game/core/game_config.cpp"
HLE_REG_SRC = "external/psxport/runtime/recomp/sync_overrides.cpp"


def load_platform_hle_table():
    """addr -> (config field, handler fn) for every entry PlatformHle::initBuiltins() installs.

    Joins two source files: `reg(h.<field>, <handler>);` in sync_overrides.cpp gives field->handler,
    `.<field> = 0x<hex>u` in game_config.cpp gives field->addr. A field wired but left 0 in the
    config is NOT installed (initBuiltins skips it) and is deliberately absent here.

    Returns (table, note). `note` is non-empty when the join could not be performed — an empty table
    from a moved/renamed source must NOT be reported as "nothing is HLE-owned"."""
    table, missing = {}, []
    reg_path, cfg_path = os.path.join(ROOT, HLE_REG_SRC), os.path.join(ROOT, HLE_CFG_SRC)
    for label, p in (("registrar " + HLE_REG_SRC, reg_path), ("config " + HLE_CFG_SRC, cfg_path)):
        if not os.path.exists(p):
            missing.append(label)
    if missing:
        return {}, "could not read " + " and ".join(missing)
    field2fn = dict(re.findall(r'\breg\(\s*h\.(\w+)\s*,\s*(\w+)\s*\)', open(reg_path, encoding="utf-8").read()))
    cfg_txt = open(cfg_path, encoding="utf-8").read()
    for field, fn in field2fn.items():
        m = re.search(r'\.' + field + r'\s*=\s*0x([0-9A-Fa-f]+)u?\s*,', cfg_txt)
        if m and int(m.group(1), 16):
            table[m.group(1).upper().zfill(8)] = (field, fn)
    if not field2fn:
        return {}, f"no `reg(h.<field>, …)` entries parsed from {HLE_REG_SRC} (registrar shape changed?)"
    if not table:
        return {}, f"{len(field2fn)} registrar entries found, but none resolved to an address in {HLE_CFG_SRC}"
    return table, ""


# --- CROSS-REFERENCE docs/port-map.md (the DELIBERATELY-ABSENT ledger) ----------------------------
#
# This map answers "where does it live". It cannot answer "should it exist at all", and that gap has
# a sharp edge: `docs/port-map.md` records render layers whose producer was DELETED ON PURPOSE
# (PROTOCOL.md's absolute rule — an UNPORTED effect is better than a TAPPED one; a tap that recovers
# a transform from GTE registers or a pre-composed guest matrix is banned, so the layers those taps
# drew are now honestly blank). Without this cross-reference an agent reads "NO native owner found",
# walks into `--unowned-rank` — a list this file itself labels THE PORT TARGET QUEUE — and
# re-implements a layer that was removed by user directive, quite possibly by re-adding the banned
# tap.
#
# Two join keys, both mechanical, no prose sniffing:
#   * ADDRESS  — every guest address named in a step's `scope` field (NOT `notes`: notes are long
#                prose that name data addresses, callees and dependencies, so joining on them
#                produces mostly noise — measured on this tree), plus an optional explicit `addrs`
#                field for steps whose scope names none.
#   * OWNER FILE — the paths in a step's `owner` field. This is the key that actually fires for the
#                deleted-tap steps: their entry addresses (0x8003B320 submitQuad, 0x8013CDD4 the
#                margin-quad emitter) ARE natively owned — the producer still exists, it just no
#                longer draws — so `--addr` returns a LIVE owner and the absence is invisible on the
#                address key alone.
#
# A step is DELIBERATELY ABSENT only when it carries an explicit `absent:` field. Nothing is inferred
# from the wording of `notes`: a checker that guessed from prose would be a new instrument with no
# way to be audited, and it would go silently wrong the day someone rephrased a note.
PORTMAP_DOC = "docs/port-map.md"


def load_portmap():
    """Returns (steps, note). Each step: {title, status, absent, addrs:set, files:set}.

    `note` is non-empty when the document could not be read or parsed — an empty cross-reference from
    a moved/renamed doc must NOT be reported as "nothing is deliberately absent"."""
    path = os.path.join(ROOT, PORTMAP_DOC)
    if not os.path.exists(path):
        return [], f"{PORTMAP_DOC} not found — deliberate-absence cross-reference NOT performed"
    txt = open(path, encoding="utf-8", errors="replace").read()
    steps = []
    for block in re.split(r"(?m)^## +", txt)[1:]:
        title = block.splitlines()[0].strip()
        fields = {}
        for k in ("scope", "status", "owner", "absent", "addrs"):
            m = re.search(r"(?im)^\s*[-*]?\s*\*\*%s:\*\*\s*(.*?)\s*$" % k, block)
            if m and m.group(1).strip():
                fields[k] = m.group(1).strip()
        addrs = set()
        for src in (fields.get("scope", ""), fields.get("addrs", "")):
            for m in re.finditer(r'FUN_(8[0-9A-Fa-f]{7})|0x(8[0-9A-Fa-f]{7})', src):
                addrs.add((m.group(1) or m.group(2)).upper())
        files = set(re.findall(r'\b((?:game|runtime|external|tools|docs)/[\w./-]+\.(?:cpp|c|h))', fields.get("owner", "")))
        steps.append(dict(title=title, status=fields.get("status", "?"),
                          absent=fields.get("absent", ""), addrs=addrs, files=files))
    if not steps:
        return [], f"{PORTMAP_DOC} parsed to ZERO steps (format changed?) — cross-reference is EMPTY BY FAILURE"
    return steps, ""


def portmap_hits(steps, addr, owner_files):
    """Steps matching this address, by address key or by owner-file key."""
    return [s for s in steps if addr.upper() in s["addrs"] or (s["files"] & set(owner_files))]


def print_portmap_xref(steps, note, addr, owner_files, indent="  "):
    """Print the port-map cross-reference for one address. ALWAYS prints something: a hit, an
    explicit no-hit carrying its denominator, or the reason the lookup could not run."""
    if note:
        print(f"{indent}WARNING: port-map cross-reference UNAVAILABLE — {note}.")
        return
    hits = portmap_hits(steps, addr, owner_files)
    absent = [s for s in hits if s["absent"]]
    for s in absent:
        print(f"{indent}⛔ DELIBERATELY ABSENT — {PORTMAP_DOC} step `{s['title']}` (status: {s['status']})")
        print(f"{indent}   {s['absent']}")
        print(f"{indent}   DO NOT re-implement this from the codemap alone. Read the step's `notes` in "
              f"{PORTMAP_DOC} first: it names WHY the layer was removed and what the real fix is.")
    for s in [h for h in hits if not h["absent"]]:
        print(f"{indent}port-map: step `{s['title']}` (status: {s['status']}) — see {PORTMAP_DOC}")
    if not hits:
        print(f"{indent}port-map: no step names this address or its owner file "
              f"(cross-checked {len(steps)} steps, {sum(1 for s in steps if s['absent'])} marked "
              f"deliberately-absent, in {PORTMAP_DOC}).")


def collect_files():
    files = []
    for g in SRC_GLOBS:
        files += glob.glob(os.path.join(ROOT, g), recursive=True)
    return sorted(set(files))


CLASS_OPEN_RE = re.compile(r'^\s*class\s+(\w+)\b[^;{]*\{')
DECL_RE = re.compile(r'^\s*(?:static\s+)?(?:virtual\s+)?[\w:*&<>]+\s+(\w+)\s*\([^;{]*\)\s*(?:const)?\s*(?:override)?\s*;')

# --- IS THAT ADDRESS EVEN A FUNCTION? the recompiled-body oracle -----------------------------------
#
# WHY THIS EXISTS. A header decl tag is prose, and prose mentions DATA addresses. mesh_quads.h:31 says
# "The engine's packed sin/cos LUT at 0x800A6490 ..." above `static void trig(...)`, and
# scan_decl_tags read that as "MeshQuads::trig owns guest function 0x800A6490" — so `--addr 800A6490`
# answered with a confident LIVE owner for an address that is a 4096-entry table, not code. A wrong
# owner does not fail loudly: it mints a plausible row (a graphics-producer key, a port-map hit) that
# nothing downstream can detect. The recompiler is the authority on what is CODE: it emitted a body
# for every reachable guest function, in the main executable (`gen_func_<hex>`) and in each overlay
# (`ov_<ov>_gen_<hex>` / `ov_<ov>_func_<hex>`). An address with no emitted body is not a function.
GEN_GLOBS = ["generated/*.h", "generated/*.c"]
GEN_FUNC_RE = re.compile(r'\b[a-z0-9_]*(?:gen|func)_(8[0-9A-Fa-f]{7})\b')
# Floor under which the oracle is treated as ABSENT rather than as "these are all the functions".
# generated/ is GITIGNORED, so a fresh clone that has not run the recompiler has no oracle at all --
# and a guard that rejects every tag because its corpus is empty is the silent-instrument failure this
# tool exists to prevent. The real tree emits ~8000; 1000 is a floor, not a target.
GEN_MIN = 1000


def load_recomp_funcs():
    """({ADDR}, note) -- every guest address the recompiler emitted a BODY for. A NON-EMPTY note means
    the oracle is UNAVAILABLE: every guard consulting it must then DISABLE itself and say so in the
    output, never reject on an empty corpus."""
    files = sorted({f for g in GEN_GLOBS for f in glob.glob(os.path.join(ROOT, g))})
    addrs = set()
    for path in files:
        addrs |= {m.group(1).upper()
                  for m in GEN_FUNC_RE.finditer(open(path, encoding="utf-8", errors="replace").read())}
    if len(addrs) < GEN_MIN:
        return set(), (f"generated/ yielded only {len(addrs)} recompiled function bodies from "
                       f"{len(files)} file(s), under the {GEN_MIN} floor -- generated/ is gitignored, "
                       f"so a tree that has not run the recompiler has NO oracle. The NOT-A-FUNCTION "
                       f"guard on header declaration tags is DISABLED for this run, and a DATA address "
                       f"mentioned in a decl comment can again be indexed as a function owner.")
    return addrs, ""


GEN_FUNCS, GEN_NOTE = load_recomp_funcs()
# Every candidate decl tag the guards REFUSED, so `--addr` can explain the resulting negative instead
# of turning a fabricated owner into an unexplained "NO native owner found".
DECL_TAG_REJECTS = []
DECL_TAG_SEEN = [0]      # denominator: declarations whose adjacent comment carried ANY guest address
QUALIFIED_RE = re.compile(r'\b[A-Z]\w*::\w+')


def decl_tag_verdict(sym, first_line, gen_funcs):
    """(ADDR, "") if `first_line` is a genuine ownership tag for `sym`; (None, why) if it is refused.

    Pure, so --selftest can drive it with synthetic tags -- both a shape that MUST be refused and a
    control that MUST be accepted -- rather than pinning tree addresses that rot."""
    m3 = re.search(r'FUN_(8[0-9a-fA-F]{7})|0x(8[0-9A-Fa-f]{7})', first_line)
    if not m3:
        return None, ""                      # no address at all: not a rejection, just not a tag
    addr = (m3.group(1) or m3.group(2)).upper()
    # GUARD 1 -- CROSS-REFERENCE, NOT A CLAIM. mesh_quads.h:36 reads
    # `// Math::rotmat (FUN_80085480) element math on three Euler angles` above `static void rotmat`:
    # the address belongs to the OTHER native the line names, and gte_math.cpp both defines and
    # INSTALLS it. Indexing it here produced a bogus DUAL-OWNERSHIP warning on `--conflicts` -- a
    # name collision (both classes have a `rotmat`) reported as two natives claiming one guest fn.
    # A qualified `Foo::bar` other than the declared symbol, LEFT of the address, is that shape.
    # Directional on purpose: refusing fails SAFE (a loud "NO native owner"), accepting fails SILENT.
    others = [q for q in QUALIFIED_RE.findall(first_line[:m3.start()]) if q != sym]
    if others:
        return None, (f"the tag names {others[0]} BEFORE the address, so 0x{addr} is a cross-reference "
                      f"to that native, not an ownership claim by {sym}")
    # GUARD 2 -- NOT A FUNCTION (see load_recomp_funcs). Disabled, loudly, when the oracle is absent.
    if gen_funcs and addr not in gen_funcs:
        return None, (f"0x{addr} has NO recompiled function body in generated/ -- it is a DATA address "
                      f"this comment merely mentions, so it cannot be {sym}'s guest function")
    return addr, ""


def scan_decl_tags(files):
    """A second, ORTHOGONAL source of address ownership beyond load_override_table/
    load_behavior_table: a class's method is declared (not defined) in its header with the guest FUN_
    tag on/above the DECLARATION (e.g. Trig::rsin — game/math/trig.h tags `rsin`/`rcos`/`ratan2`/
    `angleCmp` next to their in-class declarations), while the out-of-line DEFINITION in the .cpp has
    no adjacent tag at all (trig.cpp's method bodies open with zero comment above them — the tag lives
    only in the header, nowhere near the def the main parse_file() scanner inspects). Declarations
    themselves are skipped by parse_file's forward-decl guard (correctly — there's no body to scan for
    callees/deps), so without this pass those methods report 'NO native owner found' despite being
    tagged, real, and reached by ordinary direct C++ calls. Declarations inside a class body have no
    `ClassName::` qualifier (`int32_t rsin(...) const;`), so we track the enclosing `class Foo {` to
    reconstruct the qualified symbol the .cpp definition (found by METHOD_RE) will use."""
    tags = {}
    for path in files:
        if not path.endswith(".h") and not path.endswith(".hpp"):
            continue
        lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
        class_stack, depth = [], 0
        for i, line in enumerate(lines):
            m = CLASS_OPEN_RE.match(line)
            if m:
                class_stack.append((m.group(1), depth))
            depth += line.count("{") - line.count("}")
            while class_stack and depth <= class_stack[-1][1]:
                class_stack.pop()
            if not class_stack:
                continue
            dm = DECL_RE.match(line)
            if not dm:
                continue
            defcomment = line[line.index("//"):].strip() if "//" in line else ""
            # First-line-only, untruncated scan (deliberately NOT tag_portion's "left of separator"
            # rule, and NOT a multi-line header scan): this codebase's declaration-tag convention is
            # `methodName(args): guest FUN_xxxx. <description mentioning OTHER addresses>` — the colon
            # comes BEFORE the real address, so tag_portion's split-at-colon truncates it away, and
            # scanning a 2nd/3rd comment line for a fallback picks up a dependency address from the
            # description instead (this exact shape mis-owned Engine::objMatrixCompose with its own
            # `deps` — 0x80085480/80084110/80084470/80051128 — because line 1 (holding the true
            # 0x800518FC) got truncated at ':' and line 2 (a pure dependency list) was scanned next).
            # Every real declaration tag in this tree names its own address somewhere on the line
            # immediately above the declaration (or trailing on the declaration line itself) — never
            # needs a 2nd line — so take the FIRST match on defcomment/first-comment-line only.
            first_line = defcomment or (comment_above(lines, i))
            if not first_line:
                continue
            sym = f"{class_stack[-1][0]}::{dm.group(1)}"
            addr, why = decl_tag_verdict(sym, first_line, GEN_FUNCS)
            if addr or why:
                DECL_TAG_SEEN[0] += 1
            if why:
                DECL_TAG_REJECTS.append((sym, os.path.relpath(path, ROOT), i + 1, why))
                continue
            if not addr:
                continue
            for a in [addr]:
                tags.setdefault(sym, [])
                if a not in tags[sym]:
                    tags[sym].append(a)
    return tags


def load_installs():
    """Authoritative addr->symbol map recovered from the LIVE override registry call sites:
    `overrides::install(0xADDR, "Class::method", native, gen[, setter])`
    (runtime/recomp/override_registry.h). The quoted second argument IS the owning symbol — the
    same qualified name METHOD_RE captures at the DEFINITION — so a native wired ONLY by an install
    (no `// FUN_xxxx` tag on its def line, no "ownership of FUN_xxxx" file header, and absent from
    the faeb436^ snapshot tsv that load_override_table reads) is still attributed to its guest
    address. Without this pass such a native reports 'NO native owner found' AND, worse, stays
    invisible as a SECOND owner of an address some other file already claims — which is exactly how
    cube_text_ledger.cpp's CubeTextLedger::activateSlot silently duplicated scene_events.cpp's
    SceneEvents::armBody on FUN_80040B48 (the --conflicts dual-ownership detector depends on this).

    This keys on the CURRENT wiring idiom. It replaced the older EngineOverrides
    `ov.register_(0xADDR, "sym", fn)` parser, which by 2026-07-28 matched ZERO call sites in the
    tree while `DialogBoxSm::step` (installed on 0x8007D594, live at 963 hits on the bucket replay)
    read as unowned — the exact re-derivation trap this index exists to prevent."""
    sym2addrs = {}
    reg = re.compile(r'overrides::install\s*\(\s*0x([0-9A-Fa-f]{8})u?\s*,\s*"([^"]+)"')
    for path in collect_files():
        try:
            txt = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for m in reg.finditer(txt):
            sym2addrs.setdefault(m.group(2), []).append(m.group(1).upper())
    return sym2addrs


# --- INDEX BY INSTALL SITE (the fix for the anonymous-namespace blind spot) -----------------------
#
# Everything above indexes by native DEFINITION: a symbol whose name, adjacent comment, header tag or
# quoted install name states a guest address. That misses an entire, very common shape — a handler
# that is a FILE-LOCAL STATIC inside an anonymous namespace, carrying no address in its name, no
# per-def tag, and no quoted name at the install:
#
#     namespace {
#     void armTap_8002BC9C(Core* c) { ... }              // no tag, file-local, invisible above
#     }
#     void FxMesh::install() {
#       engine_set_override_main(0x8002BC9Cu, armTap_8002BC9C, gen_func_8002BC9C);   // <-- THE TRUTH
#     }
#
# 26 guest addresses were in exactly this state (11 in fx_mesh.cpp, 5 in options_page.cpp, 2 each in
# cube_text_ledger.cpp / actor_tomba.cpp / panel.cpp-and-friends, 1 in gte_math.cpp, plus
# 0x8009A420 installed from the submodule) and `--addr` answered "NO native owner found" for every
# one — the worst possible lie from this tool, because CLAUDE.md tells every agent to run `--addr`
# BEFORE reimplementing a FUN_xxxx, so the answer directly causes a duplicated port.
#
# THE RULE: an address with a live install HAS an owner. The file containing the install call site is
# the answer to "where do I debug this from", whether or not the handler it names is greppable by
# address. Handler identity is a bonus, not a requirement.
#
# Also fixes the mirror-image failure: mesh_emit_tap.cpp is the SINGLE installer of 0x80027768 and
# appeared 0 times in the generated map, while `--addr 80027768` named only its two CONSUMERS
# (FxMesh::draw, SwingFx::drawMesh) — sending a debugging session to the wrong two files. Its handler
# `meshEmitTap` is anonymous-namespace and its file header says "the SINGLE owner of guest
# FUN_80027768", which FILE_HEADER_ADDR_RE ("ownership of FUN_xxxx") does not match. Indexing the
# install site fixes it without special-casing the file.
INSTALL_SITE_RE = re.compile(
    r'\b(overrides::install|engine_set_override_\w+|shard_set_override|ov_\w+_set_override'
    r'|rec_set_override|install)\s*\(\s*(?:c\s*,\s*)?0x([0-9A-Fa-f]{8})u?\s*,\s*'
    r'(?:"([^"]*)"\s*,\s*)?&?([A-Za-z_][\w:]*(?:<[^;()<>]*>)?)?')


def collect_install_files():
    files = []
    for g in INSTALL_GLOBS:
        files += glob.glob(os.path.join(ROOT, g), recursive=True)
    return sorted(set(f for f in files if f.endswith((".cpp", ".c"))))


# Blank out comments and string/char literals, PRESERVING byte offsets, so the namespace tracker
# below cannot be thrown by a `{` inside a comment or a string. Offset preservation is the point:
# the caller indexes the result with offsets taken from the original text.
_BLANKABLE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)


def _code_only(txt):
    return _BLANKABLE.sub(lambda m: re.sub(r'[^\n]', ' ', m.group(0)), txt)


_NS_TOKEN = re.compile(r'\bnamespace\s+([A-Za-z_][\w:]*)?\s*\{|(\{)|(\})')


def _namespace_stack(code, upto):
    """Namespace names enclosing byte offset `upto` in comment-stripped `code` ("" = ANONYMOUS).
    Brace-tracked rather than "nearest `namespace` line above", so a namespace that has already been
    CLOSED above the point does not count — the naive form reported every handler in panel.cpp as
    anonymous because an unrelated anonymous block appeared earlier in the file."""
    stack, depth = [], 0
    for m in _NS_TOKEN.finditer(code, 0, upto):
        if m.group(3):                        # }
            if stack and stack[-1][1] == depth:
                stack.pop()
            depth -= 1
        else:                                 # { , with or without a `namespace` head
            depth += 1
            if not m.group(2):
                stack.append((m.group(1) or "", depth))
    return [n for n, _ in stack]


def load_install_sites():
    """addr -> [ {file, line, idiom, name, handler, defline, anon, tokenpaste} ] for every override
    install call site that names a LITERAL guest address, across INSTALL_GLOBS.

    `name` is the quoted symbol name when the idiom carries one (`overrides::install`), else "".
    `defline` is the line where the handler's `<sym>(Core*` definition was found in the SAME file, or
    0 when it has none there — the case for a MACRO-GENERATED handler symbol (a `#define` that
    token-pastes `armTap_##hex`, so no such text exists), for a handler the regex could not name at
    all, and for some template instantiations. A missing defline is NOT a missing owner: the install
    site itself is the address's home.
    `anon` is True when the handler's definition sits in an ANONYMOUS namespace in that file, and
    `tokenpaste` is True when the file defines a token-pasting macro — the two structural facts
    `--shape-census` classifies ownership shapes by, computed here so nothing re-reads the corpus."""
    sites = {}
    for path in collect_install_files():
        try:
            txt = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        rel, code = os.path.relpath(path, ROOT), _code_only(txt)
        paste = bool(re.search(r'(?m)^\s*#\s*define\b[^\n]*##', code))
        for m in INSTALL_SITE_RE.finditer(txt):
            handler = m.group(4) or ""
            bare = handler.split("<")[0].split("::")[-1]
            dm = re.search(r'(?m)^(?!\s*//).*\b' + re.escape(bare) + r'\s*\(\s*Core\s*\*', txt) if bare else None
            sites.setdefault(m.group(2).upper(), []).append(dict(
                file=rel, line=txt.count("\n", 0, m.start()) + 1, idiom=m.group(1),
                name=m.group(3) or "", handler=handler,
                defline=(txt.count("\n", 0, dm.start()) + 1) if dm else 0,
                anon=bool(dm) and "" in _namespace_stack(code, dm.start()),
                tokenpaste=paste,
                defbody=_brace_body(txt, dm.start()) if dm else ""))
    return sites


def _brace_body(txt, start):
    """Text of the function definition starting at `start`, to the brace-balanced close. Used to ask
    a single question about a handler: does it FORWARD to a symbol some other file claims? A
    forwarder means the two files are ONE owner (the class's central register fn installing a shim
    around a body that lives in a sibling file); a handler that does NOT mention the other file's
    symbol means they are two independent implementations of one guest function."""
    depth, seen, i, n = 0, False, start, len(txt)
    while i < n:
        if txt[i] == "{":
            depth += 1; seen = True
        elif txt[i] == "}":
            depth -= 1
            if seen and depth <= 0:
                return txt[start:i + 1]
        elif txt[i] == ";" and not seen:
            return txt[start:i + 1]
        i += 1
    return txt[start:start + 4000]


def _forwards_to(inst, sym):
    """True when some install handler for this address forwards to `sym` (or its bare method name)."""
    bare = sym.split("::")[-1]
    return any(re.search(r'\b' + re.escape(bare) + r'\s*\(', s.get("defbody", "")) for s in inst)


def _competing_claim_files(owners, inst):
    """Files that claim the address BY NAME, do not install it, and are not merely the body a
    forwarding install handler calls."""
    inst_files = {s["file"] for s in inst}
    out = {}
    for n in owners:
        if not n.get("authoritative") or n["file"] in inst_files or _forwards_to(inst, n["sym"]):
            continue
        out.setdefault(n["file"], set()).add(n["sym"])
    return {f: sorted(v) for f, v in out.items()}


def synth_install_owners(natives, sites):
    """Owner records for every (address, install-file) pair the definition scan did not already
    produce. These are FIRST-CLASS entries — they land in `idx`, so `--addr`, `--conflicts`,
    `--unowned-rank` and the generated markdown all answer from the same index and cannot disagree.

    Deliberately keyed on (addr, FILE) rather than (addr): mesh_emit_tap.cpp must appear as an owner
    of 0x80027768 even though fx_mesh.cpp and swing_fx.cpp already claim that address by name — the
    installer and the consumers are different answers to different questions, and hiding the
    installer is what made the map point at the wrong files."""
    have = {(a.upper(), n["file"]) for n in natives for a in n["impl"]}
    have_sym = {(a.upper(), n["sym"]) for n in natives for a in n["impl"]}
    seen, out = set(), []
    for a in sorted(sites):
        for s in sites[a]:
            sym = s["handler"] or s["name"] or f"install@{s['file']}:{s['line']}"
            key = (a, s["file"])
            if key in have or key in seen or (a, sym) in have_sym:
                continue
            seen.add(key)
            where = f"{s['idiom']}() at {s['file']}:{s['line']}"
            named = f' registry name "{s["name"]}";' if s["name"] else ""
            out.append(dict(sym=sym, file=s["file"], line=s["defline"] or s["line"], impl=[a],
                            deps=[], desc=f"installed via {where};{named} handler is file-local "
                                          f"(no address tag on its definition)",
                            body="", bstart=-1, bend=-1, is_freefn=False, authoritative=True,
                            via_install=s, forced_live=True))
    return out


def load_registered_addrs():
    """Set of guest addresses CURRENTLY wired to a native override at runtime: EngineOverrides
    `register_(0xADDR, ...)`, any `*set_override(...)` family (shard_set_override / engine_set_override_* /
    rec_set_override / ov_sop_set_override), and BehaviorDispatch::kTable {0xADDR, beh_*} entries. The
    faeb436^ tsv snapshot is historical (pre-removal) and is NOT counted — it does not reflect current
    runtime wiring. Used by --substrate-fallthrough to separate a dispatched-and-wired address from one
    that still falls through to substrate."""
    addrs = set()
    reg = re.compile(r'(?:\.register_\w*|[A-Za-z_]*set_override)\s*\(\s*(?:c\s*,\s*)?0x([0-9A-Fa-f]{8})')
    for path in collect_files():
        try:
            txt = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for m in reg.finditer(txt):
            addrs.add(m.group(1).upper())
    for a_list in load_behavior_table().values():
        addrs.update(a.upper() for a in a_list)
    # The regex above only sees the `<something>set_override(0xADDR, …)` shape. It does NOT see
    # `overrides::install(0xADDR, "name", native, gen, setter)` / a bare `install(0xADDR, …)` (the
    # current idiom — the setter is an ARGUMENT there, not the callee) nor anything installed from
    # the psxport submodule. Union in the install-site index so --substrate-fallthrough cannot
    # report a live, registered address as falling through to the substrate.
    addrs.update(load_install_sites())
    return addrs


# Dispatch idioms that route through the override table (→ substrate when the target is UNREGISTERED).
# PRECISE capture of the TARGET address only (not any 0x8 on the line): the target is the first address
# argument. Two arg shapes: `idiom(c, 0xADDR...)` (rec_dispatch/guest_leaf/…) and `callObj/call(c, arg,
# …, 0xADDR)` (address is a later arg). guest_fn is DELIBERATELY EXCLUDED — it is an explicit "run the
# substrate leaf" call (a native that intends the emulated body), not a fallthrough.
DISPATCH_TARGET_RES = [
    re.compile(r'\b(?:rec_dispatch|rec_super_call|super_call|guest_leaf|guest_dispatch|call_fn|rc[0-4])\s*\(\s*c\s*,\s*0x([0-9A-Fa-f]{8})'),
    re.compile(r'\b(?:callObj\d|call\d)\s*\(\s*c\s*,[^;{}]*?0x([0-9A-Fa-f]{8})'),
]


def scan_dispatched_addrs():
    """Set of guest addresses reached as the TARGET of a dispatch idiom with a literal address. Dynamic
    dispatches (rec_dispatch(c, handler) with a variable) carry no literal and are correctly ignored."""
    addrs = set()
    for path in collect_files():
        try:
            txt = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for rx in DISPATCH_TARGET_RES:
            for m in rx.finditer(txt):
                addrs.add(m.group(1).upper())
    return addrs


OVR = load_override_table()
OVR.update(load_behavior_table())
OVR.update(scan_decl_tags(collect_files()))
OVR.update(load_installs())


def parse_file(path, natives):
    rel = os.path.relpath(path, ROOT)
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    pending_freefn = []  # candidates with no per-def tag; may be claimed by the file-header fallback
    i = 0
    while i < len(lines):
        m = DEF_RE.match(lines[i])
        is_method = False
        is_freefn = False
        if not m:
            m = METHOD_RE.match(lines[i])
            is_method = True
        if not m:
            m = FREEFN_RE.match(lines[i])
            is_freefn = True
        if m:
            # Reject a forward DECLARATION masquerading as a definition: a prototype ends in `;`
            # before any `{` ever appears on the line (e.g. `uint32_t foo(Core*, uint32_t);  //
            # FUN_xxxx, native (bar.cpp)`). Without this guard the def-line trailing-comment FUN
            # tag makes the brace-balance scanner below treat everything from the prototype to the
            # next unbalanced `}` as the "body" of a phantom native — a real bug this tool hit once
            # `beh_`/`native_`-prefixed forward decls started carrying their own FUN_ tag comments.
            code_part = lines[i].split("//", 1)[0]
            if ";" in code_part and "{" not in code_part:
                m = None
        if not m:
            i += 1
            continue
        sym = m.group(1)
        # gather the contiguous comment block immediately above the def
        c = i - 1
        comment = []
        while c >= 0 and lines[c].lstrip().startswith("//"):
            comment.append(lines[c].strip()); c -= 1
        comment.reverse()
        # a trailing `// ...` on the def line itself: class methods tag their guest FUN address there
        # (e.g. `void Camera::lookAt() {   // FUN_8006D02C`). Scan it first so it wins the association.
        defcomment = lines[i][lines[i].index("//"):].strip() if "//" in lines[i] else ""
        # gather the body until brace balance returns to 0 (handles one-liners and multiline)
        body, depth, started = [], 0, False
        j = i
        while j < len(lines):
            body.append(lines[j])
            depth += lines[j].count("{") - lines[j].count("}")
            if "{" in lines[j]:
                started = True
            if started and depth <= 0:
                break
            j += 1
        bodytext = "\n".join(body)

        # implemented address(es). The override table is AUTHORITATIVE — when a symbol is in it, use
        # ONLY those addresses (its comment also names helper/dependency addresses we must NOT count as
        # "implemented here"). New top-down natives absent from the table fall back to name-hex + the
        # header-comment address(es) before the first em-dash/colon.
        impl = list(OVR.get(sym, []))
        # AUTHORITATIVE attribution = a real ownership source (override tsv / behavior table / decl-tag /
        # live EngineOverrides register_) or a name-for-address (ov_<hex>/gen_func_<hex>). SOFT = the
        # comment/header-prose fallbacks below, which also fire on a fn that merely MENTIONS an address
        # it traces or reads (a diagnostic tracer, a data-list head) — those must NOT count toward the
        # --conflicts dual-ownership signal, or it drowns in false positives.
        impl_auth = bool(impl)
        if not impl:
            nh = NAMEHEX.match(sym)
            if nh:
                impl.append(nh.group(1).upper())
                impl_auth = True
            # def-line trailing comment (class-method FUN tag) takes precedence for the owned address.
            # Only scan the tag portion (left of any separator) — addresses in description prose don't own.
            dtag = tag_portion(defcomment)
            for a in [x.upper() for x in FUN_RE.findall(dtag)] + ADDR_RE.findall(dtag):
                if a.upper() not in impl:
                    impl.append(a.upper())
            header = []
            for cl in comment:
                header.append(cl)
                if "—" in cl or " - " in cl or ":" in cl:
                    break
            htext = " ".join(tag_portion(cl) for cl in header)
            for a in [x.upper() for x in FUN_RE.findall(htext)] + ADDR_RE.findall(htext):
                if a.upper() not in impl:
                    impl.append(a.upper())
            if not impl:  # last resort: first address anywhere in the comment block, in TEXTUAL order.
                # A `Foo::bar — … guest 0x80059D28 …` class-method header names its owner in prose;
                # taking the earliest hit avoids picking up a later callee reference (e.g. FUN_8005950C).
                # For a name-agnostic free fn (is_freefn), scanning the WHOLE block is unsafe: unlike the
                # disciplined "Class::method — native ownership of FUN_xxxx" header every tagged method
                # uses, a free fn's preceding comment is often a multi-paragraph DESIGN NOTE that mentions
                # several unrelated addresses before ever naming its own (e.g. gte_op's comment discusses
                # `gte_math.cpp ov_mat_mul = FUN_80084110` — another function's address — while gte_op
                # itself owns none; scanning the full block wrongly credited gte_op with 0x80084110,
                # already correctly owned by Math::matMul). Every real free-fn tag observed in this tree
                # names its address on the comment block's FIRST line, so restrict to that line only.
                scan_text = comment[0] if (is_freefn and comment) else " ".join(comment)
                m2 = re.search(r'0x(8[0-9A-Fa-f]{7})|FUN_(8[0-9a-fA-F]{7})', scan_text)
                if m2:
                    impl.append((m2.group(1) or m2.group(2)).upper())

        # A class method / name-agnostic free fn is only a NATIVE OWNER if it implements a guest address
        # (FUN tag / comment addr). Un-owned helper methods/leaf fns tree-wide must NOT pollute the index —
        # EXCEPT: a name-agnostic free fn (is_freefn) with no address of its OWN gets one more chance below,
        # via the file-header fallback, before being dropped (see pending_freefn).
        if (is_method or is_freefn) and not impl:
            if is_freefn:
                deps = sorted({d.upper() for d in DEP_RE.findall(bodytext)})
                desc = re.sub(r'^[/\s]*((0x8[0-9A-Fa-f]{7}|FUN_8[0-9a-fA-F]{7}|/)\s*)+[—:-]?\s*', '',
                               comment[0]).strip() if comment else ""
                pending_freefn.append(dict(sym=sym, file=rel, line=i + 1, deps=deps, desc=desc, body=bodytext,
                                            bstart=i, bend=j, is_freefn=True))
            i = j + 1
            continue
        deps = sorted({d.upper() for d in DEP_RE.findall(bodytext)})
        desc = ""
        if comment:
            # first comment line, stripped of the leading address tokens, as a one-line summary
            desc = re.sub(r'^[/\s]*((0x8[0-9A-Fa-f]{7}|FUN_8[0-9a-fA-F]{7}|/)\s*)+[—:-]?\s*', '', comment[0]).strip()
        natives.append(dict(sym=sym, file=rel, line=i + 1, impl=impl, deps=deps, desc=desc, body=bodytext,
                             bstart=i, bend=j, is_freefn=is_freefn, authoritative=impl_auth))
        i = j + 1

    # File-header fallback (Fix for hitbox.cpp-style files): a file's own leading doc-comment names
    # ONE guest address it exists to own ("... ownership of FUN_8003B220."), but the tag sits far from
    # the def (separated by #includes / a tiny unrelated helper fn), so no per-def scan above found it.
    # Only fires when (a) the file actually makes that "ownership of FUN_xxxx" claim, and (b) nothing
    # already indexed in this file claims that address — then attributes it to the LARGEST untagged
    # free-fn candidate in the file (the primary implementation; incidental one-line helpers like `s16`
    # are never the biggest body in a file dedicated to one leaf).
    if pending_freefn:
        addr = file_header_addr(lines)
        if addr and not any(addr in n["impl"] for n in natives if n["file"] == rel):
            best = max(pending_freefn, key=lambda n: n["bend"] - n["bstart"])
            best["impl"] = [addr]
            best["authoritative"] = True   # explicit "ownership of FUN_xxxx" file-header claim
            natives.append(best)


def ordinary_corpus(files, natives):
    """Full source text of the whole corpus with every indexed native's OWN body blanked out.
    This is where the vast majority of real call sites live: ordinary (non-native-tagged) game
    code — `beh_*.cpp`, `demo.cpp`, `sop.cpp`, HLE adapter shims — invoking a native method via
    `c->game->cd.dc40Sync(...)`, `c->engine.asset.loadDescriptorChunk(...)`, `obj.build(...)`, or a
    bare in-class call. Blanking each native's own body prevents its own signature/self-recursion
    from counting as "called" (a def line like `void Asset::loadDescriptorChunk(...) {` contains
    the callee text `loadDescriptorChunk(` too — that must NOT self-seed liveness)."""
    by_file = {}
    for n in natives:
        if n["bstart"] < 0:
            continue   # install-site owner: no body of its own to blank (a negative range would
                       # index from the END of the file and silently delete an unrelated call site)
        by_file.setdefault(n["file"], []).append((n["bstart"], n["bend"]))
    chunks = []
    for f in files:
        rel = os.path.relpath(f, ROOT)
        lines = open(f, encoding="utf-8", errors="replace").read().splitlines()
        for lo, hi in by_file.get(rel, []):
            for k in range(lo, min(hi + 1, len(lines))):
                lines[k] = ""
        chunks.append("\n".join(lines))
    return "\n".join(chunks)


def build(natives, files):
    by_sym = {n["sym"]: n for n in natives}
    # Callee detection runs over natives that have a BODY. An install-site owner is a bare handler
    # name with no body, and folding it in actively broke liveness: the synthetic `gov_turnBiasCompute`
    # collided with the real `ActorTomba::gov_turnBiasCompute` on the same bare name, which pushed
    # that name out of the unique-bare-name table and reported a live method as ORPHAN.
    sym_set = {n["sym"] for n in natives if n["bstart"] >= 0}
    callers = {s: set() for s in sym_set}

    # --- callee detection: two complementary forms -------------------------------------------
    # (1) free-function / qualified-static syntax: ov_foo(...), native_bar(...), Class::method(...)
    #     (this alone is what the tool originally recognized — it MISSES all instance-call syntax:
    #     `obj.method(...)`, `ptr->method(...)`, and bare in-class `method(...)`, which is how nearly
    #     every OOP native is actually invoked — c->game->cd.dc40Sync(...), c->mRender->mNodeXform.
    #     buildWithOffset(...), c->engine.asset.loadDescriptorChunk(...). That gap is WHY the tool was
    #     reporting 231/237 natives ORPHAN — almost all of them are wired, just via `.`/`->`.)
    # NOTE: `beh_\w+` is included here even though DEF_RE recognizes it as a def prefix and the
    # call-detection loop below never required a trailing `(` — this is deliberate: `beh_*` handlers
    # are wired as FUNCTION-POINTER VALUES in a registration table (BehaviorDispatch::kTable in
    # game/object/behavior_dispatch.cpp — `{ 0xADDR, beh_foo, "foo" }`), called later via `b.fn(c)`
    # indirection, never via literal `beh_foo(...)` call syntax anywhere in the tree. Before this was
    # added, EVERY kTable-registered beh_* was invisible to find_callees() (qualified_re didn't match
    # the `beh_` prefix at all) and reported ORPHAN despite being live — 65 of 66 ORPHAN rows in one
    # audit were this exact false positive. The same "bare identifier used as a value, not a call" shape
    # also covers other pointer-table registrations (EngineOverrides::register_, PlatformHle::register_)
    # for symbols that already match one of these prefixes or the Class::method form.
    qualified_re = re.compile(r'\b(ov_\w+|native_\w+|eng_\w+|beh_\w+|[A-Z][A-Za-z0-9_]*::[A-Za-z_]\w*)\b')
    # (2) instance-call syntax for METHOD natives: `.name(`, `->name(`, or bare `name(` all share the
    #     same trailing token — the callee's bare method name immediately before `(`. We can't see the
    #     receiver's static type without a real parser, so to avoid a common method name (e.g. `run`,
    #     `build`) spuriously wiring an unrelated native, only bare names that are UNIQUE across every
    #     indexed method-native are matched this way; ambiguous names fall back to form (1) only
    #     (ClassName::method(...) — still recognized, just not the shorthand instance-call form).
    bare2sym, name_count = {}, {}
    # Name-agnostic free-fn natives (FREEFN_RE — grid_query_47cbc, hitbox_build_3b220, ...) have no
    # `::` and no recognized prefix either, so qualified_re above never sees a call to them (`foo(c)`
    # is just a bare identifier). They ARE always called by that exact bare name (there's no receiver
    # syntax for a free function) — fold them into the same bare-name table as unique method names, one
    # bucket keyed by "the whole symbol is its own bare name" instead of "the part after `::`".
    for s in sym_set:
        if "::" in s:
            bare = s.split("::")[-1]
        else:
            bare = s
        name_count[bare] = name_count.get(bare, 0) + 1
        bare2sym[bare] = s
    unique_bares = [b for b, c in name_count.items() if c == 1]
    bare_re = re.compile(r'\b(' + "|".join(re.escape(b) for b in unique_bares) + r')\s*\(') \
        if unique_bares else None

    # (3) ambiguous method names (e.g. `init` owned by Font, CutsceneCamera, Pool, ...): the bare
    #     name alone is too common to attribute safely, but the RECEIVER right before `.`/`->` almost
    #     always echoes the class name in some cheap lowercase form — `c->engine.font.init()` for
    #     `Font::init`, `cam.init()` for `CutsceneCamera::init`, `c->engine.demo.stageMain()` for
    #     `Demo::stageMain` vs. `c->engine.stageMain()` for `Engine::stageMain`. Build per-symbol
    #     receiver-hint patterns from the class name's camelCase segments (full name, each segment,
    #     and a short abbreviation of the last segment) and require the receiver to match one of them
    #     immediately before the call — this disambiguates without a real type-checker.
    seg_re = re.compile(r'[A-Z][a-z0-9]*')
    ambiguous_re = {}
    for s in sym_set:
        if "::" not in s:
            continue
        cls, bare = s.split("::", 1)
        if name_count.get(bare, 0) <= 1:
            continue  # handled by the unique bare-name path above
        segs = seg_re.findall(cls) or [cls]
        hints = {cls.lower()}
        hints.update(seg.lower() for seg in segs)
        hints.add(segs[-1][:3].lower())
        hints = {h for h in hints if len(h) >= 3}
        if not hints:
            continue
        pat = r'\b(?:' + "|".join(re.escape(h) for h in hints) + r')\w*\s*(?:\.|->)\s*' + re.escape(bare) + r'\s*\('
        ambiguous_re[s] = re.compile(pat)

    def find_callees(text):
        found = set(cal for cal in qualified_re.findall(text) if cal in sym_set)
        if bare_re:
            found.update(bare2sym[b] for b in bare_re.findall(text))
        for s, rx in ambiguous_re.items():
            if rx.search(text):
                found.add(s)
        return found

    # native-to-native call graph (for transitive reachability + the "C callers" report column)
    for n in natives:
        for cal in find_callees(n["body"]):
            if cal != n["sym"]:
                callers[cal].add(n["sym"])

    # REGISTRY-WIRED NATIVES ARE LIVE (added 2026-07-30). A native handed to the override registry —
    # `install(0xADDR, "name", SYMBOL, gen, setter)` or `engine_set_override_<mod>(0xADDR, SYMBOL, gen)`
    # — is invoked by GUEST dispatch through g_<mod>_override[], never by a C++ call anywhere in the
    # tree. find_callees() therefore cannot see it, and the symbol was reported ORPHAN, which this
    # file's own legend defines as "genuinely dead code until something calls it".
    #
    # That was wrong for 170 of the 181 ORPHAN rows — 94% — measured before this fix. The beh_* case
    # was already special-cased above for exactly this reason (a pointer-table registration read as a
    # value, not a call); this generalises it to the registry, which the beh_ prefix rule misses for
    # any symbol not matching a known prefix, e.g. every `leaf_<addr>` in field_owned_leaves.cpp.
    registry_wired = set()
    for f in files:
        try:
            txt = open(f, errors='replace').read()
        except OSError:
            continue
        for m in re.finditer(r'\binstall\s*\(\s*(?:0x[0-9A-Fa-f]+u?|\w+)\s*,\s*"[^"]*"\s*,\s*&?([A-Za-z_][\w:]*)', txt):
            registry_wired.add(m.group(1).split('::')[-1] if '::' not in m.group(1) else m.group(1))
        for m in re.finditer(r'\bengine_set_override_\w+\s*\(\s*(?:0x[0-9A-Fa-f]+u?|\w+)\s*,\s*&?([A-Za-z_][\w:]*)', txt):
            registry_wired.add(m.group(1))

    # reachability from ROOTS via the native-to-native graph
    live = set()
    stack = [r for r in ROOTS if r in sym_set] + [w for w in registry_wired if w in sym_set]
    while stack:
        s = stack.pop()
        if s in live:
            continue
        live.add(s)
        for n in [by_sym[s]] if s in by_sym else []:
            for cal in find_callees(n["body"]):
                if cal not in live:
                    stack.append(cal)

    # additionally: anything invoked from ORDINARY (non-native-tagged) game/engine/runtime code —
    # this is the actual call graph for OOP natives, since most callers are plain game logic
    # (behavior scripts, scene/demo code, HLE adapter shims), not other codemap-indexed natives.
    ordinary = ordinary_corpus(files, natives)
    ordinary_hit = find_callees(ordinary)
    for s in ordinary_hit:
        if s not in live:
            live.add(s)
            stack = [s]
            while stack:
                cur = stack.pop()
                for n in [by_sym[cur]] if cur in by_sym else []:
                    for cal in find_callees(n["body"]):
                        if cal not in live:
                            live.add(cal)
                            stack.append(cal)
    return by_sym, callers, live, ordinary_hit


def addr_index(natives):
    idx = {}
    for n in natives:
        for a in n["impl"]:
            idx.setdefault(a.upper(), []).append(n)
    return idx


def reconcile_freefn_claims(natives):
    """A name-agnostic free fn (FREEFN_RE) is often a THIN DISPATCH WRAPPER whose own doc-comment
    names the address it CALLS, not one it implements (e.g. beh_scene_ui_trigger.cpp's
    `render_and_return` — "dispatch the per-object render-state update FUN_800517F8 (owned)" — is a
    2-line wrapper around the ALREADY-owned GraphicsBind::renderUpdate; several `beh_*` behavior files
    each carry their own such wrapper around the same shared Engine::animTick/walkStart/etc.). Because
    parse_file resolves one file at a time, it can't see whether some OTHER file properly (via
    DEF_RE/METHOD_RE + a real per-def tag) already owns the same address — so this cross-file pass
    runs once, after every file is parsed, and drops any freefn's claim on an address that a non-freefn
    native already owns. Addresses shared among two non-freefn natives are left untouched (that's the
    deliberate pc_skip fork pattern — doSkip()/doFaithful() legitimately both implement one address)."""
    non_freefn_addrs = {a.upper() for n in natives if not n.get("is_freefn") for a in n["impl"]}
    kept = []
    for n in natives:
        if n.get("is_freefn") and any(a.upper() in non_freefn_addrs for a in n["impl"]):
            continue
        kept.append(n)
    return kept


def claim_vs_install_lines(addr, owners, inst):
    """The ⚠ CLAIM-WITHOUT-INSTALL lines for one address (empty list when there is nothing to say)."""
    if not inst:
        return []
    dead = _competing_claim_files(owners, inst)
    if not dead:
        return []
    inst_files = sorted({s["file"] for s in inst})
    out = [f"  ⚠ CLAIM-WITHOUT-INSTALL: 0x{addr} is INSTALLED from {', '.join(inst_files)} — "
           f"that is the owner a guest call reaches. {len(dead)} other file(s) claim the same address "
           f"by name, install nothing, and are not the body a forwarding install handler calls:"]
    for f in sorted(dead):
        out.append(f"      {f}  ({', '.join(dead[f])})")
    out.append("      Each is either a HOST-SIDE TWIN (native-ABI, reached by direct C++ call — fine, "
               "but it is a second implementation of one guest function) or a STALE ORPHAN. Do not "
               "read it as the owner; check its callers with `--addr`.")
    return out


def uninstalled_claims(idx, sites):
    rows = []
    for a, ns in idx.items():
        inst = sites.get(a.zfill(8), [])
        if not inst:
            continue
        dead = _competing_claim_files(ns, inst)
        if not dead:
            continue
        rows.append((a, sorted({s["file"] for s in inst}), sorted(dead.items())))
    return sorted(rows)


# --- SHAPE CENSUS: the tree's own answer to "which ownership shapes still EXIST here?" ------------
#
# WHY THIS EXISTS, and it is the root cause of a gate that was red for a day with a message that was
# FALSE. `SELFTEST_POSITIVE` below pins (address, file) constants. Commit abf3cf9 deleted
# game/render/fx_mesh.cpp and game/render/mesh_emit_tap.cpp — the GTE-register render taps — in the
# same commit that introduced those constants, so three fixtures named files that no longer exist.
# Nothing linked a fixture to the tree, so the selftest could not tell "the SCANNER lost a shape"
# (a real regression) from "the fixture's EXEMPLAR was deleted" (bookkeeping), and it reported the
# former. A permanently-unsatisfiable fixture is worse than no fixture: it buries the real signal.
#
# So a shape is now defined by a PREDICATE over the tree, not by an address someone typed. The census
# answers, mechanically and with a denominator, "how many live examples of this shape does the tree
# hold, and which are they" — which makes a rotted fixture re-pointable by running the tool instead
# of by archaeology, and makes "the scanner still covers shape X" a measurement rather than a claim.
SHAPES = {
    "install-only": (
        "the definition scan produced NO owner in the installing file — the install site IS the owner",
        lambda a, s, claim: s["file"] not in claim),
    "consumers-claim-elsewhere": (
        "sole installer, its file absent from the definition scan, while OTHER files claim the address "
        "by name — `--addr` would otherwise name only the CONSUMERS",
        lambda a, s, claim: s["file"] not in claim and bool(claim)),
    "no-textual-def": (
        "handler symbol has NO textual definition in the installing file (macro-generated, or a "
        "handler the install regex could not name) — the install LINE is the owner",
        lambda a, s, claim: s["defline"] == 0),
    "token-paste-handler": (
        "the no-textual-def sub-shape whose handler is token-pasted by a `#define` in that same file",
        lambda a, s, claim: s["defline"] == 0 and s["tokenpaste"]),
    "template-handler": (
        "a TEMPLATE instantiation as the handler",
        lambda a, s, claim: "<" in (s["handler"] or "")),
    "anon-ns-unnamed": (
        "handler defined in an ANONYMOUS namespace, installed with no quoted registry name and no "
        "address tag on its definition",
        lambda a, s, claim: s["anon"] and not s["name"]),
    "submodule-install": (
        "the install call site lives in the psxport SUBMODULE, outside the native-definition corpus",
        lambda a, s, claim: s["file"].startswith("external/psxport/")),
}


def shape_census(natives, sites):
    """shape id -> sorted [(addr, file, line)] of every LIVE example in this tree.

    `claim` is the DEFINITION-scan attribution only (install-synthesised owners excluded), because
    every shape here is about what the definition scan MISSED; folding the synthesised owners back in
    would make each predicate trivially false and the census would certify coverage it never checked."""
    claim = {}
    for n in natives:
        if n.get("via_install"):
            continue
        for a in n["impl"]:
            claim.setdefault(a.upper(), set()).add(n["file"])
    out = {k: [] for k in SHAPES}
    for a in sorted(sites):
        sole = len(sites[a]) == 1
        for s in sites[a]:
            files = claim.get(a, set())
            for k, (_, pred) in SHAPES.items():
                if k == "consumers-claim-elsewhere" and not sole:
                    continue
                if pred(a, s, files):
                    out[k].append((a, s["file"], s["line"]))
    return out


# --- SELFTEST: prove the index can still answer POSITIVELY ----------------------------------------
#
# The project rule is that a diagnostic which can print nothing is lying, so this tool ships a case
# that MUST produce a positive. It is wired into tools/precommit_gate.sh, because a self-test nobody
# runs is the same bug one level up.
#
# Each case is a REAL address in this tree whose ownership is expressed by a shape that `--addr` was
# blind to before 2026-08-05, one per distinct mechanism. If a future refactor of the scanner
# silently re-loses any of them, this fails loudly instead of quietly reporting "NO native owner".
# The NEGATIVE controls at the end matter just as much: they prove the index still says NO to an
# address nothing owns, i.e. that a green selftest is discrimination and not a blanket yes.
#
# Column 4 names the SHAPES key the case is an exemplar OF (empty for the definition-scan shapes the
# census does not classify — those are asserted by the pinned case alone). Every non-empty one is
# cross-checked against the census, so a fixture cannot drift away from the shape it claims to prove,
# and a fixture whose file has been DELETED is reported as ROTTED with live replacements listed.
SELFTEST_POSITIVE = [
    ("8004FFB4", "game/ui/panel.cpp",                "anonymous-namespace handler, no address tag, no quoted name", "anon-ns-unnamed"),
    ("80003A4C", "external/psxport/runtime/recomp/pad_input.cpp", "handler with NO textual definition — the install LINE is the owner", "no-textual-def"),
    ("8007F104", "game/ui/options_page.cpp",         "TEMPLATE instantiation as the handler", "template-handler"),
    ("80040AA4", "game/object/cube_text_ledger.cpp", "overrides::install with a quoted name, method untagged", ""),
    ("80055C9C", "game/player/actor_tomba.cpp",      "file-local gov_* forwarder, no tag", ""),
    ("80077FB0", "game/math/gte_math.cpp",           "static eov_* guest-ABI shim, no tag", ""),
    ("8005019C", "game/ui/panel.cpp",                "anonymous-namespace tap", "anon-ns-unnamed"),
    ("8007E1B8", "game/render/ui_ft4_tap.cpp",       "single installer that the definition scan missed entirely", "consumers-claim-elsewhere"),
    ("8009A420", "external/psxport/runtime/recomp/mem.cpp", "install from the psxport SUBMODULE", "submodule-install"),
]
# A shape the scanner IMPLEMENTS but that no longer has a live example here, so no fixture can assert
# it. Named rather than dropped: silence would read as "covered". The selftest prints the count with
# its denominator, and FAILS the moment an example reappears un-pinned — the coverage claim in
# `--addr`'s blind-spot list is then a measurement again instead of a memory.
#
# token-paste-handler: the exemplars were fx_mesh.cpp's FX_CONTROLLER_SCOPE / FX_A00_CONTROLLER_SCOPE,
# which token-pasted `armTap_##hex` / `a00Tap_##hex`. Both died with the file in abf3cf9. The
# MECHANISM they exercised (defline == 0 -> the install line is the owner) is still asserted, by
# 0x80003A4C above; only the token-paste sub-form is unexemplified.
SELFTEST_UNEXEMPLIFIED = ["token-paste-handler"]
# Addresses that must resolve to NOTHING. 0x80000000/0x8FFFFFF8 are not code; 0x800834A0 is
# PlatformHle-owned and must be reported as such, never as a scanned native.
SELFTEST_NEGATIVE = ["80000004", "8FFFFFF8"]


def selftest():
    natives, files, sites, by_sym, callers, live, ordinary_hit, idx = load_index()
    pm_steps, pm_note = load_portmap()
    fails = []
    print(f"corpus: {len(natives)} indexed natives, {len(idx)} owned addresses, "
          f"{sum(len(v) for v in sites.values())} install sites in {len(collect_install_files())} files, "
          f"{len(pm_steps)} port-map steps.")
    census = shape_census(natives, sites)
    for a, want_file, why, shape in SELFTEST_POSITIVE:
        # DISCRIMINATE THE TWO FAILURE MODES BEFORE MEASURING. A fixture whose exemplar FILE has been
        # deleted proves nothing about the scanner, and reporting it as "the index has stopped
        # resolving an ownership shape" sends the reader hunting a regression that never happened —
        # which is exactly what this gate did between abf3cf9 and today. Say which it is, and hand
        # over live replacements so re-pointing is a copy rather than an excavation.
        if not os.path.exists(os.path.join(ROOT, want_file)):
            alt = census.get(shape, [])[:3]
            hint = ("; live examples of this shape to re-point at: "
                    + ", ".join(f"0x{x} -> {f}:{ln}" for x, f, ln in alt)) if alt else \
                   f"; NO live example of shape `{shape}` remains — move it to SELFTEST_UNEXEMPLIFIED " \
                   f"with the evidence, do not delete it silently"
            print(f"  [ROT ] 0x{a} -> {want_file:52s} ({why})")
            fails.append(f"ROTTED FIXTURE 0x{a}: its exemplar file {want_file} DOES NOT EXIST. This is "
                         f"bookkeeping, NOT a scanner regression{hint}")
            continue
        got = sorted({n["file"] for n in idx.get(a, [])} | {s["file"] for s in sites.get(a, [])})
        ok = want_file in got
        print(f"  [{'ok ' if ok else 'FAIL'}] 0x{a} -> {want_file:52s} ({why})")
        if not ok:
            fails.append(f"0x{a}: expected owner file {want_file}, index says {got or '(nothing)'} — {why}")
        # A fixture that has drifted off the shape it claims to prove is a fixture proving something
        # else under the old label — the quiet way coverage rots without anything going red.
        if shape and (a, want_file) not in {(x, f) for x, f, _ in census.get(shape, [])}:
            fails.append(f"0x{a} is pinned as an exemplar of shape `{shape}` but the census does not "
                         f"classify it as one ({len(census.get(shape, []))} live example(s) of that shape)")
    # SHAPE COVERAGE, with its denominator. Each count is the tree's own answer, so "this shape is
    # covered" stops being a memory of what someone fixed in August and becomes a measurement.
    # COVERED means "some pinned fixture is STRUCTURALLY a member of this shape", not "some fixture
    # carries this label". A fixture labelled `anon-ns-unnamed` is also an `install-only` case, and
    # label-matching reported the superset shape as uncovered while nothing was appended to `fails` —
    # a marker that printed FAIL next to a verdict of 0 failures. A mark that can disagree with the
    # exit code is the same defect this whole selftest exists to prevent, one level up.
    pins = {(a, f) for a, f, _, _ in SELFTEST_POSITIVE}
    for k in SHAPES:
        rows, unex = census[k], k in SELFTEST_UNEXEMPLIFIED
        covering = sorted(pins & {(a, f) for a, f, _ in rows})
        mark = "ok " if (rows and covering) or (not rows and unex) else "FAIL"
        note = f"covered by {len(covering)} pinned fixture(s): " + \
               ", ".join(f"0x{a}" for a, _ in covering) if covering else \
               ("NO live example — declared unexemplified, NOT asserted" if unex else
                "live examples exist but NO pinned fixture is one of them")
        print(f"  [{mark}] shape {k:26s} {len(rows):4d} live example(s)  ({note})")
        if rows and not covering and not unex:
            fails.append(f"shape `{k}` has {len(rows)} live example(s) and NOT ONE is pinned in "
                         f"SELFTEST_POSITIVE — nothing asserts the scanner still resolves it; pin one "
                         f"(e.g. 0x{rows[0][0]} -> {rows[0][1]}:{rows[0][2]})")
        if unex and rows:
            fails.append(f"shape `{k}` was declared unexemplified but {len(rows)} live example(s) now "
                         f"exist (first: 0x{rows[0][0]} {rows[0][1]}:{rows[0][2]}) — pin one in "
                         f"SELFTEST_POSITIVE and drop it from SELFTEST_UNEXEMPLIFIED; the blind-spot "
                         f"list --addr prints claims this shape is covered")
        if not rows and not unex:
            fails.append(f"shape `{k}` has ZERO live examples in the tree, so nothing asserts the "
                         f"scanner still resolves it — either the shape genuinely left the codebase "
                         f"(declare it in SELFTEST_UNEXEMPLIFIED) or the classifier broke")
    for a in SELFTEST_NEGATIVE:
        got = idx.get(a, []) or sites.get(a, [])
        print(f"  [{'ok ' if not got else 'FAIL'}] 0x{a} -> (nothing), as it must be  [negative control]")
        if got:
            fails.append(f"0x{a}: expected NO owner, index claims one — the index says yes to everything")
    # The blind-spot list printed by the negative branch of --addr claims installs are found across
    # the psxport submodule and that macro/template/anon-namespace handlers are covered. Assert the
    # corpus really reaches there, so the claim cannot rot into a comforting lie.
    if not any(os.path.relpath(f, ROOT).startswith("external/psxport/") for f in collect_install_files()):
        fails.append("INSTALL_GLOBS no longer reaches external/psxport/runtime — the blind-spot list "
                     "printed by --addr claims it does.")
    if pm_note:
        fails.append(f"port-map cross-reference unavailable: {pm_note}")
    # PROVE THE DELIBERATELY-ABSENT FILTER FIRES. On the current tree every `absent:` step's address
    # is natively OWNED, so `--unowned-rank` legitimately excludes ZERO rows — and a filter that has
    # never fired is indistinguishable from a filter that is broken. Feed it a case that MUST be
    # excluded, and a control that MUST NOT be.
    probe = [dict(title="<selftest>", status="todo", absent="selftest fixture",
                  addrs={"8FFFFFF8"}, files={"nowhere/none.cpp"}),
             dict(title="<selftest-control>", status="todo", absent="",
                  addrs={"8FFFFFF0"}, files=set())]
    if not [s for s in portmap_hits(probe, "8FFFFFF8", []) if s["absent"]]:
        fails.append("the deliberately-absent filter does NOT fire on a step that declares the address")
    if [s for s in portmap_hits(probe, "8FFFFFF0", []) if s["absent"]]:
        fails.append("the deliberately-absent filter fires on a step with NO absent: field")
    if not [s for s in portmap_hits(probe, "80000000", ["nowhere/none.cpp"]) if s["absent"]]:
        fails.append("the deliberately-absent filter does NOT fire on the OWNER-FILE key")
    # PROVE THE DECL-TAG GUARDS FIRE, AND THAT THEY ARE NOT A BLANKET NO. Both guards refuse a
    # candidate ownership tag, and a refusal is invisible in the index -- exactly the shape that rots
    # into either a blanket no (every tag refused, --addr goes quiet tree-wide) or a dead guard (the
    # regression that put a DATA address in the index as a LIVE function owner). Synthetic fixtures,
    # so nothing here can rot with a tree address; the ACCEPT controls are the half that matters.
    ORACLE = {"80085480", "800AAAAA"}
    guard_cases = [
        ("MeshQuads::rotmat", "// Math::rotmat (FUN_80085480) element math on three Euler angles.",
         None, "cross-reference: another native named before the address"),
        ("MeshQuads::trig",   "// The engine's packed sin/cos LUT at 0x800A6490 (word = cos<<16|sin)",
         None, "not-a-function: no recompiled body for that address"),
        ("Math::rotmat",      "// Math::rotmat (FUN_80085480) -- libgte RotMatrix.",
         "80085480", "ACCEPT control: the qualified name IS the declared symbol"),
        ("Trig::rsin",        "// rsin(a): guest 0x800AAAAA. Reads the LUT that FUN_80085480 also uses.",
         "800AAAAA", "ACCEPT control: colon-style tag, later address is description prose"),
        ("Foo::bar",          "// plain prose with no address at all",
         None, "no address: not a tag, and not counted as a rejection"),
    ]
    for csym, cline, want, why in guard_cases:
        got, cwhy = decl_tag_verdict(csym, cline, ORACLE)
        ok = got == want
        print(f"  [{'ok ' if ok else 'FAIL'}] decl-tag guard: {why} -> "
              f"{('0x' + got) if got else 'refused/none'}")
        if not ok:
            fails.append(f"decl-tag guard fixture ({why}) returned {got!r}, expected {want!r} "
                         f"[{cwhy or 'no reason given'}]")
    # ...and that the guard is DISABLED, not inverted, when the oracle is missing.
    if decl_tag_verdict("MeshQuads::trig", "// LUT at 0x800A6490", set())[0] != "800A6490":
        fails.append("the not-a-function guard still rejects with an EMPTY oracle -- a missing "
                     "generated/ must disable the guard, never reject every tag")
    # On the REAL tree both reasons must have live examples: a guard with zero hits is unasserted.
    r_xref = [r for r in DECL_TAG_REJECTS if "cross-reference" in r[3]]
    r_data = [r for r in DECL_TAG_REJECTS if "NO recompiled function body" in r[3]]
    print(f"  [{'ok ' if GEN_FUNCS else 'WARN'}] recompiled-function oracle: {len(GEN_FUNCS)} guest "
          f"function bodies in generated/ (floor {GEN_MIN})"
          + (f" -- UNAVAILABLE: {GEN_NOTE}" if GEN_NOTE else ""))
    print(f"  [ -- ] decl-tag guards refused {len(DECL_TAG_REJECTS)} of {DECL_TAG_SEEN[0]} "
          f"address-carrying declaration tag(s): "
          f"{len(r_xref)} cross-reference, {len(r_data)} not-a-function"
          + (f" (first: {r_xref[0][1]}:{r_xref[0][2]} {r_xref[0][0]})" if r_xref else "")
          + (f" (first: {r_data[0][1]}:{r_data[0][2]} {r_data[0][0]})" if r_data else ""))
    if not r_xref:
        fails.append("the decl-tag CROSS-REFERENCE guard refused nothing in this tree -- it is "
                     "asserted only by a synthetic fixture; if the tree genuinely no longer holds "
                     "the shape, say so here rather than leaving a guard nothing measures")
    if GEN_FUNCS and not r_data:
        fails.append("the decl-tag NOT-A-FUNCTION guard refused nothing in this tree -- same problem")
    real_absent = [s for s in pm_steps if s["absent"]]
    print(f"  [{'ok ' if real_absent else 'FAIL'}] {PORTMAP_DOC} carries {len(real_absent)} step(s) "
          f"with an `absent:` field (of {len(pm_steps)}); the filter is exercised on a fixture too")
    if not real_absent:
        fails.append(f"no {PORTMAP_DOC} step carries `absent:` — the cross-reference can only ever "
                     f"print the negative, which is the failure mode this selftest exists to catch")
    unresolved = sorted(a for a in sites if not idx.get(a))
    print(f"  [{'ok ' if not unresolved else 'FAIL'}] every installed address resolves through --addr "
          f"({len(sites)} installed addresses, {len(unresolved)} unresolved)")
    if unresolved:
        fails.append("addresses with a live install that --addr cannot resolve: " + ", ".join("0x"+x for x in unresolved))
    for f in fails:
        print("FAIL: " + f)
    print(f"\n{len(SELFTEST_POSITIVE)} positive case(s), {len(SELFTEST_NEGATIVE)} negative control(s), "
          f"{len(SHAPES)} shape(s) censused ({len(SELFTEST_UNEXEMPLIFIED)} declared unexemplified: "
          f"{', '.join(SELFTEST_UNEXEMPLIFIED) or 'none'}), 6 invariant(s) — {len(fails)} failure(s).")
    return 1 if fails else 0


def selftest_negative_controls():
    """Run --selftest against four DELIBERATELY BROKEN trees and require it to fail each one.

    WHY THIS SHIPS. `--selftest` went green in this commit, and a self-test that has only ever been
    seen green proves nothing about what it covers — the same argument that put `--selftest` in the
    pre-commit hook, one level up. Worse, the failure this repair is ABOUT was invisible for exactly
    that reason: the gate was red, but red for a rotted fixture, and nobody could tell that from red
    for a scanner regression because no control had ever produced either on purpose.

    Each control mutates ONE thing in memory (never the tree) and asserts the selftest fails AND that
    its message names the right cause. A control that merely asserts "fails" would pass for the wrong
    reason — a broken scanner fails everything."""
    import contextlib, io
    global SELFTEST_POSITIVE, SELFTEST_UNEXEMPLIFIED, INSTALL_SITE_RE
    saved = (SELFTEST_POSITIVE, SELFTEST_UNEXEMPLIFIED, INSTALL_SITE_RE)
    dead = "game/render/fx_mesh.cpp"   # deleted in abf3cf9 — the real rot, replayed
    controls = [
        ("ROTTED FIXTURE (exemplar file deleted)",
         lambda: globals().__setitem__("SELFTEST_POSITIVE",
             saved[0] + [("8002BC9C", dead, "the abf3cf9 rot, replayed", "anon-ns-unnamed")]),
         "ROTTED FIXTURE"),
        ("SCANNER REGRESSION (install-site scan resolves nothing)",
         lambda: globals().__setitem__("INSTALL_SITE_RE", re.compile(r'(?!x)x()()()')),
         "has ZERO live examples in the tree"),
        ("FIXTURE DRIFT (pinned as a shape it is not an example of)",
         lambda: globals().__setitem__("SELFTEST_POSITIVE",
             [(a, f, w, "template-handler" if a == "8009A420" else s) for a, f, w, s in saved[0]]),
         "but the census does not classify it as one"),
        ("UNEXEMPLIFIED SHAPE REAPPEARS (declared absent, examples exist)",
         lambda: globals().__setitem__("SELFTEST_UNEXEMPLIFIED", ["template-handler"]),
         "was declared unexemplified but"),
    ]
    bad = 0
    print(f"NEGATIVE CONTROLS for --selftest: {len(controls)} deliberately-broken trees, each of which "
          f"MUST make it exit non-zero for the STATED reason.")
    for label, mutate, want in controls:
        SELFTEST_POSITIVE, SELFTEST_UNEXEMPLIFIED, INSTALL_SITE_RE = saved
        mutate()
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = selftest()
        out = buf.getvalue()
        ok = rc != 0 and want in out
        print(f"  [{'ok ' if ok else 'FAIL'}] {label}\n"
              f"         exit={rc} (want non-zero), message contains {want!r}: {want in out}")
        if not ok:
            bad += 1
            print("         --- its output ---\n" + "".join("         " + l + "\n"
                                                            for l in out.splitlines() if "FAIL" in l))
    SELFTEST_POSITIVE, SELFTEST_UNEXEMPLIFIED, INSTALL_SITE_RE = saved
    print(f"\n{len(controls)} control(s), {bad} that did NOT fire. A control that does not fire means "
          f"--selftest is blind to that failure mode.")
    return 1 if bad else 0


def load_index():
    """The one index every command answers from: definition-scanned natives + install-site owners."""
    natives = []
    files = collect_files()
    for f in files:
        parse_file(f, natives)
    natives = reconcile_freefn_claims(natives)
    sites = load_install_sites()
    natives += synth_install_owners(natives, sites)
    by_sym, callers, live, ordinary_hit = build(natives, files)
    live |= {n["sym"] for n in natives if n.get("forced_live")}
    return natives, files, sites, by_sym, callers, live, ordinary_hit, addr_index(natives)


def main():
    args = sys.argv[1:]
    if "--selftest" in args:
        sys.exit(selftest())
    if "--selftest-nc" in args:
        sys.exit(selftest_negative_controls())
    if "--shape-census" in args:
        natives, files, sites, *_ = load_index()
        census = shape_census(natives, sites)
        n_sites = sum(len(v) for v in sites.values())
        print(f"corpus: {n_sites} install sites over {len(sites)} addresses in "
              f"{len(collect_install_files())} files.")
        for k, (why, _) in SHAPES.items():
            rows = census[k]
            tag = "  [declared UNEXEMPLIFIED]" if k in SELFTEST_UNEXEMPLIFIED else ""
            print(f"\n{k}: {len(rows)} of {n_sites} install site(s){tag}\n  {why}")
            for a, f, ln in rows[:8]:
                print(f"    0x{a}  {f}:{ln}")
            if len(rows) > 8:
                print(f"    ... {len(rows) - 8} more")
        sys.exit(0)
    natives, files, sites, by_sym, callers, live, ordinary_hit, idx = load_index()
    pm_steps, pm_note = load_portmap()
    if "--addr" in args:
        a = args[args.index("--addr") + 1].upper().replace("0X", "")
        owners = idx.get(a, [])
        # PlatformHle owns BIOS/hardware-sync primitives outside this scanner's corpus — report that
        # BEFORE the "no owner" line, which would otherwise read as "free to port" (see the table's
        # comment: 0x800834A0 / gpuTimeoutArm).
        hle_tbl, hle_note = load_platform_hle_table()
        hle = hle_tbl.get(a.zfill(8))
        if hle:
            print(f"0x{a}: OWNED by PlatformHle as `{hle[1]}()`  [{HLE_REG_SRC}]")
            print(f"    wired from GameConfig::hle.{hle[0]} ({HLE_CFG_SRC}) — a BIOS/libgpu/libetc/libcd/"
                  f"libmdec primitive, NOT a porting target.")
            print(f"    The recompiled body never runs; do NOT install a second override on this address.")
        inst = sites.get(a.zfill(8), [])
        if not owners and not hle:
            print(f"0x{a}: NO native owner found.")
            for rsym, rfile, rline, rwhy in DECL_TAG_REJECTS:
                if f"0x{a.upper()}" in rwhy or a.upper() in rwhy:
                    print(f"    NOTE: a header declaration tag for {rsym} ({rfile}:{rline}) mentions "
                          f"this address and was REFUSED -- {rwhy}")
            if GEN_NOTE:
                print(f"    WARNING: not-a-function guard DISABLED -- {GEN_NOTE}")
            # State the reach of that negative: what was actually searched, with its DENOMINATOR, and
            # what this method genuinely cannot see. A bare "NO native owner" from a scan that
            # silently saw nothing is a lie — and so is a blind-spot list that names the wrong blind
            # spots, which is strictly worse because it reads as MORE trustworthy than it is. The
            # list below is kept true BY CONSTRUCTION: `--selftest` asserts each named blind spot
            # still holds and each un-named one is genuinely covered.
            n_def_files = len(files)
            n_inst_files = len(collect_install_files())
            n_sites = sum(len(v) for v in sites.values())
            # Name the roots that ACTUALLY matched, not the globs. `engine/**` and `runtime/recomp/**`
            # are in SRC_GLOBS and match nothing in this checkout (the platform lives in the
            # submodule), so printing the glob list would overstate the search.
            roots = sorted({os.path.relpath(f, ROOT).split("/")[0] for f in files})
            print(f"    SEARCHED (denominator): {len(natives)} indexed natives from {n_def_files} "
                  f"source files under {'/, '.join(roots)}/ (of SRC_GLOBS "
                  f"{' '.join(SRC_GLOBS)} — the rest match no file in this checkout); "
                  f"{n_sites} override install sites (literal-address) across {n_inst_files} files "
                  f"incl. external/psxport/runtime/; {len(hle_tbl)} PlatformHle entries; "
                  f"{len(pm_steps)} {PORTMAP_DOC} steps."
                  + (f"  WARNING: PlatformHle table UNAVAILABLE, {hle_note}" if hle_note else ""))
            print( "    BLIND TO (a hit here would NOT have been found):")
            print( "      1. an install whose address is not a LITERAL 0x8xxxxxxx on the call line "
                   "(computed, table-driven, or token-pasted by a macro into the ADDRESS argument);")
            print(f"      2. a native definition outside {'/, '.join(roots)}/ that neither installs "
                   "nor is named by an install — e.g. a reimplementation living in external/psxport "
                   "that is only ever reached by a direct C++ call;")
            print( "      3. a native that reimplements this guest fn but records the address NOWHERE "
                   "— no name-hex, no adjacent/def-line/file-header tag, no header decl tag, no "
                   "install. Nothing in the tree connects it to this address, so nothing can find it;")
            print( "      4. ownership expressed at RUNTIME only (a handler stored into a dispatch "
                   "table at run time rather than installed from a call site);")
            print( "      5. overlay/interpreted-only guest code with no native counterpart at all.")
            print( "    NOT blind to (each asserted by a LIVE fixture in --selftest, and counted by "
                   "--shape-census): a file-local static in an ANONYMOUS NAMESPACE with no address in "
                   "its name and no quoted install name; a handler with NO textual definition (the "
                   "install line is the owner); a template instantiation; an install from the psxport "
                   "submodule; a sole installer whose file the definition scan never saw.")
            print( "    COVERED BUT UNEXEMPLIFIED — implemented, and NOT asserted, because the tree "
                   "currently holds zero examples to assert it with (run --shape-census for the "
                   f"counts): {', '.join(SELFTEST_UNEXEMPLIFIED) or 'none'}.")
        elif hle_note:
            print(f"    WARNING: PlatformHle ownership NOT checked — {hle_note}")
        if inst:
            print(f"  INSTALLED — {len(inst)} live override install site(s). DEBUG FROM HERE: the file "
                  f"holding the install owns this address at runtime.")
            for s in inst:
                nm = f' as "{s["name"]}"' if s["name"] else ""
                dl = f", handler defined at line {s['defline']}" if s["defline"] else \
                     " (handler has no textual definition in this file — macro-generated or a template)"
                print(f"    {s['file']}:{s['line']}  {s['idiom']}() -> {s['handler'] or '?'}{nm}{dl}")
        for n in owners:
            st = "LIVE" if n["sym"] in live else "ORPHAN"
            print(f"0x{a}: {n['sym']}  [{st}]  {n['file']}:{n['line']}")
            if n["desc"]: print(f"    desc: {n['desc']}")
            if n["deps"]: print(f"    depends on (still-PSX leaves): {', '.join('0x'+d for d in n['deps'])}")
            cs = sorted(callers.get(n["sym"], []))
            extra = " + ordinary game/engine code" if n["sym"] in ordinary_hit else ""
            print(f"    C callers: {(', '.join(cs) if cs else '(none)') + extra}")
        # who depends on this address?
        dep_users = [n for n in natives if a in n["deps"]]
        if dep_users:
            print(f"  depended-on by: {', '.join(sorted(set(n['sym'] for n in dep_users)))}")
        cf = sorted(set(n["file"] for n in owners if n.get("authoritative")))
        if len(cf) >= 2:
            print(f"  ⚠ DUAL-OWNERSHIP: 0x{a} is authoritatively implemented in {len(cf)} DIFFERENT files "
                  f"({', '.join(cf)}). Two natives claiming one guest address is a DUPLICATION bug "
                  f"unless it is a deliberate same-class pc_skip doSkip()/doFaithful() fork — "
                  f"consolidate to one owner (delegate one body to the other).")
        # CLAIM vs INSTALL. Two files can both claim an address BY NAME while only one INSTALLS it —
        # 0x80078240 is claimed as `Trig::vecLen` (trig.cpp) and `Math::approxDist3` (gte_math.cpp),
        # and only the latter is installed. `--dup-installs` is legitimately 0 there (one installer)
        # and `--conflicts` never saw it (the install carries the quoted name, not a def tag), so
        # NOTHING surfaced it. The distinction matters: only the installed one answers a guest call;
        # the other is a host-side twin reached by direct C++ call, or a stale orphan.
        for line in claim_vs_install_lines(a, owners, inst):
            print(line)
        print_portmap_xref(pm_steps, pm_note, a, [n["file"] for n in owners] + [s["file"] for s in inst])
        return

    if "--uninstalled-claims" in args:
        rows = uninstalled_claims(idx, sites)
        for a, installed, claimed in rows:
            print(f"0x{a}: INSTALLED owner -> {', '.join(installed)}")
            for f, syms in claimed:
                print(f"    claims by name but does NOT install: {f}  ({', '.join(syms)})")
        print(f"\n{len(rows)} guest address(es) claimed by name in 2+ files where exactly the "
              f"install-site file is the runtime owner. Scanned {len(idx)} owned addresses against "
              f"{sum(len(v) for v in sites.values())} install sites. Each row is EITHER a legitimate "
              f"host-side twin (a native-ABI reimplementation called directly by C++, e.g. a math "
              f"leaf) OR a stale orphan — check with `--addr <hex>` and delete the duplicate if it "
              f"has no caller. It is NOT a double-install: `--dup-installs` stays the signal for that.")
        return

    if "--unowned-rank" in args:
        # THE PORT TARGET QUEUE: rank the still-unowned guest functions by how hot they are, so a
        # porting batch picks its targets from the tool instead of hand-greping docs/code-map.md.
        # Resolve ownership through `idx` — the SAME index `--addr` answers from — so this command and
        # `--addr` can never disagree. That is the whole reason it lives here rather than in a shell
        # one-liner: a `^| 0xADDR |` grep over docs/code-map.md is a SEPARATE oracle that can drift from
        # the real one, and ownership is not expressed only by that table (registry seeding, leaf tags
        # and method tags all feed `idx` too). The grep happens to agree on the current tree — both see
        # 836 addresses — but agreeing today is not a guarantee, and a target queue that silently lists
        # an already-owned function sends someone to duplicate a port.
        #
        # Input is a rank file of "0xADDR<space>HITS" lines — scratch/logs/recdep_rank.txt by default,
        # produced by the recdep meter. Hits are a HOTNESS signal, not a correctness one: a cold
        # function is not necessarily a bad target (it may be the next step on the RE dependency
        # chain), so treat this as ONE input alongside `portmap.py next`, never as the sole order.
        i = args.index("--unowned-rank")
        rankfile = args[i + 1] if i + 1 < len(args) and not args[i + 1].startswith("-") \
                   else os.path.join(ROOT, "scratch/logs/recdep_rank.txt")
        try:
            lines = open(rankfile).read().splitlines()
        except OSError as e:
            print(f"cannot read rank file {rankfile}: {e}")
            print("expected lines of '0xADDR <hits>' — generate one with the recdep meter.")
            return
        # `idx` covers only natives this scanner can SEE. PlatformHle owns the BIOS/hardware-sync
        # primitives from a different table entirely, so filtering on `idx` alone put 0x800834A0
        # (gpuTimeoutArm) at the TOP of this queue at 33,152 hits — an address PlatformHle had owned
        # all along. A queue whose #1 entry is already owned is worse than no queue: it dispatches a
        # full porting session at a double-install. Exclude them, and COUNT what was excluded so the
        # filter cannot go silently dead.
        hle_tbl, hle_note = load_platform_hle_table()
        ranked, unowned, bad, hle_hits = 0, [], 0, []
        for l in lines:
            p = l.split()
            if len(p) != 2 or not p[0].lower().startswith("0x"):
                bad += 1
                continue
            ranked += 1
            a = p[0][2:].upper()
            try: hits = int(p[1])
            except ValueError: bad += 1; continue
            if a.zfill(8) in hle_tbl:
                hle_hits.append((a, hits, hle_tbl[a.zfill(8)][1]))
            elif not idx.get(a):
                unowned.append((a, hits))
        # DELIBERATELY-ABSENT layers must not sit in a list headed "THE PORT TARGET QUEUE". A render
        # layer whose producer was deleted under PROTOCOL.md's no-tap rule looks exactly like an
        # unported one from here — same "no native owner", same hotness — so the queue was actively
        # inviting someone to rebuild it, quite possibly by re-adding the banned tap. Pull them out,
        # and NAME them with their port-map step rather than dropping them silently.
        absent_rows = []
        if not pm_note:
            keep = []
            for a, hits in unowned:
                hit = [s for s in portmap_hits(pm_steps, a, []) if s["absent"]]
                (absent_rows if hit else keep).append((a, hits, hit[0] if hit else None))
            unowned = [(a, h) for a, h, _ in keep]
            absent_rows = [(a, h, s) for a, h, s in absent_rows]
        limit = 40
        if "--top" in args:
            limit = int(args[args.index("--top") + 1])
        for a, hits in unowned[:limit]:
            dep_users = sorted(set(n["sym"] for n in natives if a in n["deps"]))
            note = f"   depended-on by: {', '.join(dep_users)}" if dep_users else ""
            print(f"  0x{a}  {hits:>8}{note}")
        print(f"\n{len(unowned)} unowned of {ranked} ranked ({len(idx)} addresses owned overall"
              + f", {len(hle_tbl)} more owned by PlatformHle)"
              + (f"; {bad} unparseable line(s) skipped" if bad else "")
              + f". Showing top {min(limit, len(unowned))}.")
        # Name the excluded entries rather than dropping them silently: a hot address vanishing from
        # the queue with no explanation reads as a bug in the meter.
        if hle_hits:
            print(f"Excluded {len(hle_hits)} PlatformHle-owned address(es) that the recdep meter still "
                  f"counts (the counter sits before the g_<mod>_override[] consult — see "
                  f"overlay_router.cpp): "
                  + ", ".join(f"0x{a} ({h} hits, {fn})" for a, h, fn in sorted(hle_hits, key=lambda t: -t[1])))
        if hle_note:
            print(f"WARNING: PlatformHle-owned addresses were NOT excluded — {hle_note}. "
                  f"Treat every entry above as UNVERIFIED ownership until that is fixed.")
        if absent_rows:
            print(f"\n⛔ EXCLUDED — {len(absent_rows)} address(es) are DELIBERATELY ABSENT per "
                  f"{PORTMAP_DOC}, NOT unported. These are NOT port targets; re-implementing one "
                  f"undoes a decision, and may re-add a banned tap:")
            for a, hits, s in sorted(absent_rows, key=lambda t: -t[1]):
                print(f"    0x{a}  {hits:>8}   step `{s['title']}` ({s['status']}) — {s['absent']}")
        if pm_note:
            print(f"WARNING: deliberately-absent layers were NOT excluded — {pm_note}. A layer this "
                  f"project removed ON PURPOSE may be listed above as a port target.")
        else:
            print(f"Cross-checked {len(pm_steps)} {PORTMAP_DOC} steps "
                  f"({sum(1 for s in pm_steps if s['absent'])} marked deliberately-absent); "
                  f"{len(absent_rows)} queue entr(ies) excluded on that basis.")
        print("Hotness is one input, not the order — cross-check `portmap.py next` for the RE chain, "
              "and `--addr <hex>` before writing anything.")
        return

    if "--dup-installs" in args:
        # The SOURCE-LEVEL twin of the runtime duplicate-owner abort (override_registry.cpp): an address
        # is a real double-owner only if TWO files each INSTALL an override for it. This keys on the
        # actual install call site — engine_set_override_<mod>(0xADDR, …) / overrides::install(0xADDR, …)
        # / a bare install(0xADDR, …) in a register_* fn — NOT on a "// FUN_XXXX" banner or an
        # address-named helper, which is why it agrees with the guard while --conflicts (below) does not:
        # a host-twin like Render::guestStrLen reproduces FUN_80079528 and is CALLED inline, it does not
        # install on the address, so it is not an owner of it. This is the check to trust for kanban #32.
        ipat = re.compile(r'(?:engine_set_override_(?:main|a00|game)|overrides::install|shard_set_override'
                          r'|(?<![A-Za-z_])install)\(\s*(0x[0-9A-Fa-f]+)')
        byaddr = {}
        for f in files:
            if not f.endswith((".cpp", ".c")):
                continue
            try: s = open(f).read()
            except OSError: continue
            rel = os.path.relpath(f, ROOT)
            for m in ipat.finditer(s):
                byaddr.setdefault(m.group(1).lower(), set()).add(rel)
        rows = sorted((a, sorted(fs)) for a, fs in byaddr.items() if len(fs) >= 2)
        for a, fs in rows:
            print(f"{a}: installed from {len(fs)} files")
            for f in fs:
                print(f"    {f}")
        print(f"\n{len(rows)} address(es) installed from 2+ files — a REAL double-install (the runtime "
              f"guard aborts on this). Expect 0; anything here is a bug to fix before it ships.")
        return

    if "--conflicts" in args:
        # BROAD reference smell — every file that authoritatively NAMES the address (a registration, an
        # address-named symbol, or an explicit ownership banner). This OVER-reports: an inline host-twin
        # helper (Render::guestStrLen for FUN_80079528) or a producer's consumer (FxMesh::draw reading
        # the mesh writer) carries a banner but installs nothing, so it is not a competing owner. For the
        # signal that matches the runtime duplicate-owner abort, use `--dup-installs`. Kept because a
        # bannered native that NOBODY installs while another file owns the address is a stale-orphan
        # candidate (no-tombstones) — cross-check each row with `--addr` + `--dup-installs`.
        rows = []
        for a, ns in idx.items():
            auth = [n for n in ns if n.get("authoritative")]
            inst = sites.get(a.zfill(8), [])
            if inst:
                # With install sites indexed, the install file appears as an owner of its address.
                # That would list every ordinary "central register fn installs a gov_*/eov_* shim
                # around a body in a sibling file" as a cross-file conflict (15 such rows on this
                # tree — actor_tomba's action handlers, panel's fill tap, engine's mode handlers).
                # A forwarding shim and the body it calls are ONE owner, so count only files that
                # neither install nor are called by an install handler.
                cf = sorted(set(_competing_claim_files(auth, inst)) | {s["file"] for s in inst})
            else:
                cf = sorted(set(n["file"] for n in auth))
            if len(cf) >= 2:
                rows.append((a, cf, sorted(set(n["sym"] for n in auth if n["file"] in cf))))
        for a, cf, syms in sorted(rows):
            print(f"0x{a}: {len(cf)} files — {', '.join(syms)}")
            for f in cf:
                print(f"    {f}")
        print(f"\n{len(rows)} guest address(es) with CROSS-FILE authoritative NAMING (a broad smell that "
              f"OVER-reports helpers/consumers — use `--dup-installs` for the real double-install signal, "
              f"and `--addr <hex>` to classify each as owner / inline-helper / stale-orphan).")
        return

    if "--substrate-fallthrough" in args:
        # An address that HAS a native owner AND is the TARGET of a dispatch idiom (rec_dispatch/
        # guest_leaf/…) but is NOT override-registered → those callers silently run the EMULATED body
        # while any direct-native callers run the port. That split is how FUN_800518FC hid (fixed
        # 2026-07-15). Register + MIRROR_VERIFY to native-ize. Boot/stage handlers reached only by a
        # direct native_boot call (never a dispatch target) do NOT appear — the precise target capture
        # excludes them. Still a candidate list: a few may be intentionally direct-call-only. `authOnly`
        # (default) restricts to authoritative owners; pass `--all` to include soft-attributed owners.
        auth_only = "--all" not in args
        registered = load_registered_addrs()
        dispatched = scan_dispatched_addrs()
        rows = []
        for a in sorted(set(idx) & dispatched - registered):
            ns = [n for n in idx[a] if n.get("authoritative")] if auth_only else idx[a]
            if ns:
                rows.append((a, ns))
        for a, ns in rows:
            syms = ", ".join(sorted(set(n["sym"] for n in ns)))
            print(f"0x{a}: native owner {syms} — dispatch target, NOT override-registered → callers hit SUBSTRATE")
            for n in ns:
                print(f"    {n['file']}:{n['line']}")
        print(f"\n{len(rows)} native-owned address(es) dispatched but not override-registered — dispatch/"
              f"guest_leaf callers fall through to the emulated substrate. Register + MIRROR_VERIFY to "
              f"native-ize (review each; a few may be intentionally direct-call-only).")
        return

    if "--orphans" in args:
        rows = [(a, n) for a, ns in idx.items() for n in ns if n["sym"] not in live]
        for a, n in sorted(rows, key=lambda r: (r[0], r[1]["sym"])):
            print(f"0x{a}  {n['sym']:32s} {n['file']}:{n['line']}")
        print(f"\n{len(rows)} owned addresses currently ORPHANED "
              f"(native exists, nothing calls it — not even via `.`/`->`/bare method syntax).")
        return

    # default: emit the markdown index
    out = []
    out.append("# Code map — guest address → PC-native owner\n")
    out.append("> GENERATED by `tools/codemap.py` — do not edit by hand; rerun the tool.\n")
    out.append("Before reimplementing any `FUN_xxxx`, look it up here (or `tools/codemap.py --addr <hex>`).")
    out.append("A native may exist already. **LIVE** = reachable by a real call from either a native_boot")
    out.append("dispatch root or ordinary (non-native-tagged) game/engine code — free-function syntax")
    out.append("(`ov_foo(...)`), qualified static syntax (`Class::method(...)`), or C++ instance-call")
    out.append("syntax (`obj.method(...)`, `ptr->method(...)`, bare in-class `method(...)`). **ORPHAN** =")
    out.append("native exists but no call site of any of those forms was found anywhere in the tree — it")
    out.append("is genuinely dead code until something calls it.\n")
    n_live = sum(1 for n in natives if n["sym"] in live)
    n_sites = sum(len(v) for v in sites.values())
    out.append(f"Totals: {len(natives)} native fns, {len(idx)} owned addresses, "
               f"{n_live} LIVE / {len(natives)-n_live} ORPHAN. "
               f"{n_sites} override install sites over {len(sites)} addresses.\n")
    out.append("**A row can come from a DEFINITION or from an INSTALL SITE.** An address whose "
               "handler is a file-local static in an anonymous namespace (no address in its name, no "
               "tag, no quoted registry name) has no findable definition — the `overrides::install` "
               "/ `engine_set_override_*` call site is its only ownership record, and the file "
               "holding that call site is where you debug it from. Those rows say so in the summary "
               "column.\n")
    out.append(f"**Cross-check `{PORTMAP_DOC}` before porting anything.** This map answers WHERE "
               "code lives; it cannot answer whether a layer should exist. Layers whose producer was "
               "DELETED ON PURPOSE (the no-tap rule) look identical here to unported ones. "
               "`--addr <hex>` performs the cross-reference; the section at the end of this file "
               "lists every deliberately-absent step.\n")
    out.append("| addr | status | symbol | file:line | depends-on (still-PSX) | summary |")
    out.append("|------|--------|--------|-----------|------------------------|---------|")
    for a in sorted(idx):
        for n in idx[a]:
            st = "LIVE" if n["sym"] in live else "ORPHAN"
            deps = " ".join("0x" + d for d in n["deps"][:6]) + (" …" if len(n["deps"]) > 6 else "")
            desc = (n["desc"][:70] + "…") if len(n["desc"]) > 70 else n["desc"]
            desc = desc.replace("|", "\\|")
            out.append(f"| 0x{a} | {st} | `{n['sym']}` | {n['file']}:{n['line']} | {deps} | {desc} |")

    # SECOND OWNING TABLE. This document is grepped directly ("is 0xADDR in code-map.md?"), so a table
    # listing only the source-scanned natives answers "not owned" for every BIOS/hardware-sync
    # primitive — the false negative that put 0x800834A0 at the top of the port queue at 33,152 hits
    # when PlatformHle had owned it all along. These are NOT porting targets: their recompiled bodies
    # deliberately never run (the arm/check pair would call libetc VSync, which this port traps).
    hle_tbl, hle_note = load_platform_hle_table()
    out.append("\n## PlatformHle-owned (BIOS / hardware-sync primitives — NOT porting targets)\n")
    out.append("Owned by a DIFFERENT mechanism than the table above: `PlatformHle` "
               f"(`{HLE_REG_SRC}`), wired from the addresses this game states in "
               f"`GameConfig::hle` (`{HLE_CFG_SRC}`). No native def exists for these, so the scanner "
               "above cannot see them — grepping only that table reports them as unowned. The "
               "recompiled body NEVER runs; installing an override on one is a double-install.\n")
    if hle_tbl:
        out.append("| addr | handler | GameConfig::hle field |")
        out.append("|------|---------|-----------------------|")
        for a in sorted(hle_tbl):
            field, fn = hle_tbl[a]
            out.append(f"| 0x{a} | `{fn}` | `{field}` |")
        out.append(f"\n{len(hle_tbl)} PlatformHle-owned address(es).")
    else:
        # Never render an empty section as "there are none" — say the join failed and why.
        out.append(f"**TABLE UNAVAILABLE — {hle_note}.** This section is EMPTY BECAUSE THE LOOKUP "
                   "FAILED, not because no address is PlatformHle-owned. Do not read it as "
                   "'nothing here is owned'; fix the paths above and regenerate.")
    # THIRD SECTION: the deliberately-absent ledger, cross-referenced from docs/port-map.md. This
    # document is grepped directly, so an address that is honestly blank BY DECISION must say so
    # here — otherwise the grep answers "not owned", which reads as "free to port".
    out.append(f"\n## Deliberately ABSENT — do NOT port from this map alone (`{PORTMAP_DOC}`)\n")
    if pm_note:
        out.append(f"**SECTION UNAVAILABLE — {pm_note}.** It is empty BECAUSE THE LOOKUP FAILED, not "
                   "because nothing is deliberately absent. Fix the path and regenerate.")
    else:
        absent_steps = [s for s in pm_steps if s["absent"]]
        out.append(f"Cross-referenced against {len(pm_steps)} `{PORTMAP_DOC}` steps; "
                   f"{len(absent_steps)} carry an explicit `absent:` field. A step here means the "
                   "layer's PICTURE was removed by decision (usually PROTOCOL.md's absolute no-tap "
                   "rule). Its entry function may still be natively OWNED above — the producer "
                   "exists, it just no longer draws — so an owner row is NOT evidence the layer is "
                   "present. Read the step's `notes` in the port map before touching any of it.\n")
        if absent_steps:
            out.append("| port-map step | status | why it is absent | guest addrs | owner files |")
            out.append("|---------------|--------|------------------|-------------|-------------|")
            for s in sorted(absent_steps, key=lambda x: x["title"]):
                why = s["absent"].replace("|", "\\|")
                out.append(f"| `{s['title']}` | {s['status']} | {why} | "
                           f"{' '.join('0x'+x for x in sorted(s['addrs'])) or '—'} | "
                           f"{' '.join(sorted(s['files'])) or '—'} |")
        else:
            out.append("**0 steps carry an `absent:` field.** That is a statement about the PORT MAP, "
                       "not about the code: it means no step has been marked, NOT that nothing was "
                       "deliberately deleted. If you know of a layer removed on purpose, add "
                       "`- **absent:** <one line why>` to its step in the port map.")
    text = "\n".join(out) + "\n"
    if "--stdout" in args:
        sys.stdout.write(text)
    else:
        dest = os.path.join(ROOT, "docs/code-map.md")
        open(dest, "w").write(text)
        print(f"wrote {os.path.relpath(dest, ROOT)}: {len(natives)} natives, {len(idx)} addresses, "
              f"{n_live} LIVE / {len(natives)-n_live} ORPHAN")


if __name__ == "__main__":
    main()
