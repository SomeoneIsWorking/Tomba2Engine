#!/usr/bin/env python3
"""gen_annotate.py — make a recompiled `gen_` body readable for RE.

The `generated/` shards are a mechanical transliteration of the PSX MIPS, so they ARE the
instruction stream — but every constant address arrives as the two-instruction lui/addiu pair
the assembler emitted:

    c->r[2] = (uint32_t)32778u << 16;
    c->r[30] = c->r[2] + (uint32_t)-10756;      <-- this is 0x8009D5FC

Resolving those by hand is the single most repeated act in RE'ing one of these functions, and it
is exactly the kind of arithmetic a human gets wrong late at night. This tool does it, then
annotates each resolved address with what is known to live there — the scratchpad cross-function
ABI, the sprite-effect family's shared leaves — and names the native owner of every dispatch
target by asking codemap.

Usage:
    tools/gen_annotate.py 8010C7F4              # find the body by guest address
    tools/gen_annotate.py 8010C7F4 --bare       # no colour (for piping into a file)

It is a READING aid. It never claims to understand the function; it resolves constants and labels
what it recognises, and stays silent about the rest.
"""
import argparse
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(REPO, "generated")

# ---------------------------------------------------------------------------------------------
# What lives at an address. Scratchpad entries are the implicit ABI the sprite-effect family uses
# to pass values between the gate, the scaler and the packet writer — the single biggest source of
# "why does this function read a magic address" confusion. Keep additions sourced, not guessed.
KNOWN_ADDR = {
    0x1F800080: "OT key published by FUN_800317CC",
    0x1F800084: "MAC0 published by FUN_800317CC == sprite scaleX (native: SpriteAnchor::baseScale)",
    0x1F800088: "sprite scaleY slot read by the packet writers",
    0x1F80008C: "SXY2 (projected screen XY) published by FUN_800317CC",
    0x1F800090: "IR0 depth-cue factor: 0 = identity, non-zero fades colours toward GTE far colour",
    0x1F8000F8: "scene camera view matrix (pure camera, published per frame)",
    0x1F80010C: "scene camera translation",
    0x1F800160: "field/effect base point (three s16 VX,VY,VZ)",
}

KNOWN_FN = {
    0x80027A4C: "8-byte record writer        -> Render::spriteRecordsEmit",
    0x8002847C: "36-byte four-corner writer  -> emitAnimQuadRecords",
    0x800317CC: "RTPS + OT-key gate, returns 0 on EMIT / 1 on SKIP; publishes 0x1F800080/84/8C",
    0x800329E0: "load pure scene camera into GTE CR0-7, set DQA=arg, DQB=0",
    0x800328BC: "wrapper: FUN_80027A4C(recList, node+0x44)",
    0x80083E80: "rsin  -> Trig::rsin",
    0x80083F50: "rcos  -> Trig::rcos",
    0x80085690: "ratan2 -> Trig::ratan2",
    0x80078240: "3-D length approximation -> Trig::vecLen",
    0x80084110: "3x3 matrix multiply -> Math::matMul",
    0x80084220: "MVMVA apply         -> Math::applyMatlv",
}

RE_LUI = re.compile(r"c->r\[(\d+)\] = \(uint32_t\)(\d+)u << 16;")
RE_ADD_CONST = re.compile(r"c->r\[(\d+)\] = c->r\[(\d+)\] \+ \(uint32_t\)(-?\d+);")
RE_MOVE = re.compile(r"c->r\[(\d+)\] = c->r\[(\d+)\] \+ c->r\[0\];")
RE_MEM = re.compile(r"mem_(r|w)(8|16|32)\(\(?c->r\[(\d+)\](?: \+ \(uint32_t\)(-?\d+))?\)?")
RE_ASSIGN = re.compile(r"c->r\[(\d+)\] = ")
RE_DISPATCH = re.compile(r"rec_dispatch\(c, 0x([0-9A-Fa-f]+)u?\)")
RE_CALL = re.compile(r"\b((?:ov_[a-z0-9]+_)?func_)([0-9A-Fa-f]{8})\(c\)")


def codemap_owner(addr):
    """Ask the project's own ownership index who owns this guest address."""
    try:
        out = subprocess.run(
            [os.path.join(REPO, "tools", "codemap.py"), "--addr", f"{addr:08X}"],
            capture_output=True, text=True, timeout=30,
        ).stdout
    except Exception:
        return None
    first = out.strip().splitlines()[0] if out.strip() else ""
    if "NO native owner" in first:
        return "NO native owner"
    m = re.match(r"0x[0-9A-Fa-f]+:\s+(\S+)", first)
    return m.group(1) if m else None


def find_body(addr_hex):
    """Locate the gen body for a guest address across the shards."""
    want = addr_hex.upper()
    # MAIN.EXE bodies are `gen_func_XXXX`; overlay bodies are `ov_<tag>_gen_XXXX`. Match both —
    # matching only the overlay form made this tool silently useless on every MAIN.EXE function.
    pat = re.compile(r"^void ((?:ov_[a-z0-9]+_)?gen_(?:func_)?" + want + r")\(Core\* c\) \{")
    for name in sorted(os.listdir(GEN)):
        if not name.endswith(".c"):
            continue
        path = os.path.join(GEN, name)
        with open(path, errors="replace") as fh:
            lines = fh.readlines()
        for i, line in enumerate(lines):
            m = pat.match(line)
            if m:
                for j in range(i, len(lines)):
                    if lines[j].rstrip() == "}":
                        return path, i, j, m.group(1), lines[i:j + 1]
    return None


def annotate(body):
    """Constant-fold the lui/addiu register pairs and label every resolved address."""
    regs = {}            # reg -> known constant value
    out = []
    for raw in body:
        line = raw.rstrip("\n")
        notes = []

        # EVERY register this line assigns. Anything not re-established as a constant below is
        # invalidated at the end of the line — without this the folder happily applies an offset to
        # a register that has since been overwritten by a memory load, and prints a confident,
        # WRONG address (real instance: r2 reloaded by mem_r16 then + -1024 read as 0x800BFC00).
        assigned = {int(g) for g in RE_ASSIGN.findall(line)}
        still_const = set()

        m = RE_LUI.search(line)
        if m:
            reg = int(m.group(1))
            regs[reg] = (int(m.group(2)) << 16) & 0xFFFFFFFF
            still_const.add(reg)

        m = RE_ADD_CONST.search(line)
        if m:
            dst, src, off = int(m.group(1)), int(m.group(2)), int(m.group(3))
            if src in regs and (src not in assigned or src in still_const or src == dst):
                val = (regs[src] + off) & 0xFFFFFFFF
                regs[dst] = val
                still_const.add(dst)
                notes.append(f"= 0x{val:08X}")
                if val in KNOWN_ADDR:
                    notes.append(KNOWN_ADDR[val])

        m = RE_MOVE.search(line)
        if m:
            dst, src = int(m.group(1)), int(m.group(2))
            if src in regs:
                regs[dst] = regs[src]
                still_const.add(dst)

        # memory access through a register we have folded -> name the absolute address
        for mm in RE_MEM.finditer(line):
            reg = int(mm.group(3))
            off = int(mm.group(4)) if mm.group(4) else 0
            if reg in regs:
                val = (regs[reg] + off) & 0xFFFFFFFF
                tag = f"[0x{val:08X}]"
                if val in KNOWN_ADDR:
                    tag += f" {KNOWN_ADDR[val]}"
                if tag not in notes:
                    notes.append(tag)

        for mm in RE_DISPATCH.finditer(line):
            fn = int(mm.group(1), 16)
            desc = KNOWN_FN.get(fn)
            owner = codemap_owner(fn)
            bits = [f"-> FUN_{fn:08X}"]
            if desc:
                bits.append(desc)
            elif owner:
                bits.append(owner)
            notes.append("  ".join(bits))

        for mm in RE_CALL.finditer(line):
            fn = int(mm.group(2), 16)
            if fn in KNOWN_FN:
                notes.append(f"-> FUN_{fn:08X}  {KNOWN_FN[fn]}")

        # a plain lui with no following addiu is still a base worth showing
        if RE_LUI.search(line) and not RE_ADD_CONST.search(line):
            reg = int(RE_LUI.search(line).group(1))
            notes.append(f"base 0x{regs[reg]:08X}")

        # drop every register this line clobbered by means we do not model as constant-producing
        for reg in assigned - still_const:
            regs.pop(reg, None)

        out.append((line, notes))
    return out


def main():
    ap = argparse.ArgumentParser(description="Annotate a recompiled gen_ body for RE.")
    ap.add_argument("addr", help="guest address, e.g. 8010C7F4")
    ap.add_argument("--bare", action="store_true", help="no ANSI colour")
    args = ap.parse_args()

    addr = args.addr.lower().replace("0x", "")
    found = find_body(addr)
    if not found:
        print(f"no gen body found for {addr.upper()} under {GEN}", file=sys.stderr)
        return 2
    path, lo, hi, sym, body = found

    dim, reset = ("", "") if args.bare else ("\033[36m", "\033[0m")
    rel = os.path.relpath(path, REPO)
    print(f"# {sym}   {rel}:{lo + 1}-{hi + 1}   ({hi - lo + 1} lines)")
    print(f"# resolved addresses and known-leaf names are annotations, NOT decompilation")
    for line, notes in annotate(body):
        if notes:
            print(f"{line}{dim}   // {' | '.join(notes)}{reset}")
        else:
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
