#!/usr/bin/env python3
"""render_fns.py — every CUSTOM RENDER FN the game can ever install on a node, found statically.

WHY THIS EXISTS. `pc_render` draws a type-0x20 node by dispatching on the function pointer at
`node+0x18` (Render::fieldObjectsRender's whitelist). A node whose fn is not on that whitelist gets
NO picture from the display pass. `PSXPORT_DEBUG=nofx` names the ones a given run reached — but a run
only visits the areas and triggers the replay happens to cover, so "nofx is quiet" never means "the
whitelist is complete". This is the static counterpart: it finds every `sw <code-address>, 0x18(rN)`
in MAIN.EXE and the overlays, i.e. every fn the game is CAPABLE of installing, whether or not any
recorded replay reaches it.

    tools/render_fns.py <dump.bin> [more dumps...]

Prints each render fn with the writer that installs it, and (when the repo is the cwd) marks the ones
already on the pc_render whitelist by grepping game/render/render_walk.cpp. The gap between the two
columns IS the render frontier's work-list.

HOW. MIPS materialises an absolute address as `lui rX, hi` + `addiu rX, rX, lo`, so a linear scan
tracking only those two forms resolves the register at the store. That is the same shape find_refs.py
tracks and it is sufficient here: a render fn is always a link-time constant, never computed.

WHAT IT IS AND IS NOT — measured, so nobody has to trust it blind (2026-07-28, 8 field dumps):

  * It is a LOWER BOUND, not a roster. 15 fns found; several fns known to be real render fns —
    0x80027CB4, 0x800281EC, 0x80033080, 0x8010BF54, 0x8013E08C, 0x801143C4, 0x8013CDD4 — do NOT
    appear, because they are installed by COPYING a pointer out of the effect-descriptor table at
    0x800A21C0 rather than by materialising a constant. A quiet run of this tool proves nothing.
  * It is a CANDIDATE generator, not a verdict. Offset 0x18 is a generic struct field, so a hit only
    means "a function pointer is stored there". Of the 15: 4 are already whitelisted, 4 more are
    known render fns owned by another route (a controller SCOPE at guest-execution time, which the
    whitelist cannot show), 1 — 0x8013D454, the faucet's water jet — was a REAL uncovered gap and is
    now fixed, and the rest are unconfirmed and need per-fn RE.
  * Every hit IS a real function entry: the raw scan reported 68, of which ~50 were fiction
    (page-aligned lui-only values, unaligned words, unrelated callback slots). The prologue filter
    removes them, and the survivors were cross-checked against the recompiler's own function
    boundaries in generated/ — all 15 are recompiled entries. Do NOT use Ghidra's boundaries for
    this cross-check; instrument I016 records that the bucket_f470 project mis-sizes functions and
    renders several of these as mid-function fragments.

Confirm a candidate with PSXPORT_DEBUG=nofx (what a RUN skipped) plus the fxmesh/fxsprite channels.
"""
import re, struct, subprocess, sys, os

BASE = 0x80000000
NODE_RENDER_FN = 0x18          # the field pc_render dispatches on


def sx(v):
    return v - 0x10000 if v >= 0x8000 else v


def is_code(a):
    return (a & 3) == 0 and ((0x80010000 <= a < 0x800A0000) or (0x80100000 <= a < 0x80160000))


def prologue(d, a):
    """Does `a` begin with `addiu sp, sp, -N`? The engine's every render fn does."""
    o = a - BASE
    if o < 0 or o + 4 > len(d):
        return False
    ins = struct.unpack("<I", d[o:o + 4])[0]
    return (ins >> 16) == 0x27BD and sx(ins & 0xFFFF) < 0


NODE_TYPE = 0x0B               # the byte Render::fieldObjectsRender switches on; 0x20 = custom-fn node
TYPE_20 = 0x20
TYPE_WINDOW = 0x40             # how far back to look for the `sb <type>, 0x0B(rN)` on the same node


def node_type_near(d, off, base_reg):
    """The constant this constructor wrote to node+0x0B shortly before the store — ADVISORY ONLY.

    ⚠ THIS READ IS NOT SOUND, AND IT MUST NEVER SUPPRESS A ROW. It was written to be a filter — only
    a type-0x20 node reaches pc_render's render-fn whitelist at all, so classifying the type would
    turn 11 candidates into 2. It was then FALSIFIED against ground truth on the first case checked:
    it reports 0x8013D454 as type 3, while `PSXPORT_DEBUG=nofx` on a live run prints
    "type-0x20 node 800EE9D8: render fn 0x8013D454" — and that node's effect (the faucet's water jet)
    is real and now drawn. The heuristic picks up whatever `sb <const>, 0x0B(base)` happens to sit in
    the preceding window, which can belong to a different construction phase, a different object
    aliased to the same register, or a branch not taken.

    So the number is printed as a HINT and nothing more: every non-whitelisted fn stays a [GAP].
    A wrong hint costs a reader one check; a wrong filter would have silently hidden the one real
    producer gap this tool has found so far.
    """
    start = max(0, off - TYPE_WINDOW)
    ty = None
    for o in range(start, off, 4):
        ins = struct.unpack("<I", d[o:o + 4])[0]
        op, rs, rt, imm = ins >> 26, (ins >> 21) & 31, (ins >> 16) & 31, ins & 0xFFFF
        if op == 0x28 and sx(imm) == NODE_TYPE and rs == base_reg:      # sb rt, 0x0B(base)
            # Resolve the value register back to its `li`. A type that is COMPUTED (e.g. the guest
            # writing `v1 < 4`) legitimately stays unknown — and unknown is treated as a candidate,
            # never dropped, so a miss here can only cost noise, not coverage.
            ty = 0 if rt == 0 else None
            for o2 in range(max(0, o - TYPE_WINDOW), o, 4):
                i2 = struct.unpack("<I", d[o2:o2 + 4])[0]
                op2, rs2, rt2 = i2 >> 26, (i2 >> 21) & 31, (i2 >> 16) & 31
                if rt2 != rt:
                    continue
                if op2 in (9, 0x0D) and rs2 == 0:        # addiu/ori rt, r0, N  — the two `li` forms
                    ty = i2 & 0xFFFF
                elif op2 in (0x20, 0x21, 0x23, 0x24, 0x25, 0x0F) or op2 == 0:
                    ty = None                            # loaded or computed: not a constant
    return ty


def scan(dump):
    """-> {render_fn: set((writer_addr, node_type))} for one 2 MB KSEG0 image."""
    d = open(dump, "rb").read()
    hi = {}                                     # reg -> value from the last lui/addiu chain
    out = {}
    for off in range(0, min(len(d), 0x200000) - 4, 4):
        ins = struct.unpack("<I", d[off:off + 4])[0]
        op, rs, rt, imm = ins >> 26, (ins >> 21) & 31, (ins >> 16) & 31, ins & 0xFFFF
        if op == 0x0F:                          # lui
            hi[rt] = imm << 16
        elif op == 9:                           # addiu rt, rs, imm
            base = hi.get(rs)
            hi[rt] = None if base is None else (base + sx(imm)) & 0xFFFFFFFF
        elif op == 0x2B and sx(imm) == NODE_RENDER_FN:   # sw rt, 0x18(rs)
            v = hi.get(rt)
            # THE FILTER THAT MAKES THIS TRUSTWORTHY. Offset 0x18 is a generic struct field and the
            # lui/addiu tracker cannot know which lui belongs to which store, so the raw hits are
            # mostly noise: page-aligned lui-only values (0x80100000), unaligned words (0x8009D85F),
            # and libgpu/libcd callback slots that merely happen to live at +0x18. Requiring the
            # target to BEGIN WITH A FUNCTION PROLOGUE (`addiu sp, sp, -N`) in the same image throws
            # all three classes out — a render fn is a real function, and every one in this engine
            # opens a frame. Without this the tool reports 68 fns of which ~50 are fiction.
            if v is not None and is_code(v) and prologue(d, v):
                out.setdefault(v, set()).add((BASE + off, node_type_near(d, off, rs)))
        elif op == 3 or (op == 0 and (ins & 0x3F) in (8, 9)):
            # A CALL clobbers the caller-saved set, but the callee-saved registers survive it — and a
            # render fn is often materialised into an s-register well before the store. Clearing the
            # whole map on every branch (the first version of this) lost most of the known fns; only
            # clearing what the ABI actually clobbers finds them.
            for r in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 31):
                hi.pop(r, None)
    return out


def whitelisted():
    """The addresses render_walk.cpp already dispatches. Empty if it is not readable from here."""
    try:
        src = open("game/render/render_walk.cpp").read()
    except OSError:
        return None
    body = src[src.find("void Render::fieldObjectsRender"):]
    return {int(m, 16) for m in re.findall(r"rfn == 0x([0-9A-Fa-f]{8})u", body)}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    # Attribute every hit to the DUMP it came from. The overlay window holds different code in
    # different dumps, so a bare writer address is ambiguous and sends the next reader at the wrong
    # bytes — which is exactly what happened the first time this list was followed up: four writer
    # addresses decoded as a nop, a `jr ra`, an `addu` and an `lbu` in the dump that happened to be
    # opened, because those hits came from c18_a1 and c18_a5 rather than bucket_f470.
    found = {}
    for dump in sys.argv[1:]:
        tag = os.path.basename(dump)
        for fn, writers in scan(dump).items():
            for wr, ty in writers:
                found.setdefault(fn, set()).add((wr, tag, ty))
    wl = whitelisted()
    print(f"{len(found)} distinct render fns installed at node+0x{NODE_RENDER_FN:02X} "
          f"across {len(sys.argv) - 1} dump(s)"
          + (f"; {len(wl)} on the pc_render whitelist" if wl is not None else ""))
    missing = 0
    for fn in sorted(found):
        rows = sorted(found[fn])
        types = {t for _, _, t in rows}
        if wl is None:
            mark = "     "
        elif fn in wl:
            mark = "[ok] "
        else:
            mark = "[GAP]"                      # see node_type_near: the type is a hint, not a filter
            missing += 1
        tys = ",".join("?" if t is None else f"0x{t:02X}" for t in sorted(types, key=lambda t: (t is None, t)))
        ws = "  ".join(f"{w:08X}@{tag}" for w, tag, _ in rows[:3])
        print(f"  {mark} 0x{fn:08X}  type={tys:9s} installed by {ws}")
    if wl is not None:
        print(f"\n{missing} render fn(s) with no whitelist entry — each is a producer-gap CANDIDATE.")
        print("type= is an ADVISORY HINT, never a filter: only a type-0x20 node reaches the whitelist,")
        print("but the read is unsound and was falsified on 0x8013D454 (it says 3; nofx says 0x20 and")
        print("the effect is real). Treat a non-0x20 hint as 'check this one second', not 'skip it'.")
        print("Confirm with PSXPORT_DEBUG=nofx (it names what a RUN skipped) plus the fxmesh/fxsprite")
        print("channels: several are owned by another route (a controller SCOPE at guest-execution")
        print("time), which the whitelist cannot show.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
