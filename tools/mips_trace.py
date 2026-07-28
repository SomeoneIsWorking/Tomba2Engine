#!/usr/bin/env python3
"""mips_trace.py — symbolically execute a straight-line MIPS range and print what each STORE writes.

WHY THIS EXISTS. Porting a byte-faithful PSX emitter means answering one question over and over:
"this `sh r12, 44(r16)` — what value is r12, in terms of the inputs?" Ghidra's decompiler answers it
with aliased temporaries (iVar10/iVar17 rotating between iterations), and hand-reading a 3-instruction
window around each store is how this project produced two WRONG generative-rule claims in one session
(see instruments I014/I015 and the E680 angle saga). A register the compiler reuses cannot be read
locally — you need its whole def chain.

So: give every register a SYMBOLIC value (a string expression seeded from its name at the range
start), interpret the range instruction by instruction, and print each store as
`offset(base) <- <expression>`. No guessing, no dataflow done in your head.

    tools/mips_trace.py <dump.bin> <start_hex> <end_hex> [--stores-only] [--base rN]

The dump is a 2 MB PSX RAM image based at 0x80000000 (the same input tools/find_refs.py takes).
Ranges are inclusive of start, exclusive of end. Branches are NOT followed — this is for straight-line
blocks (a loop body between its backward branch targets), which is exactly the shape emitter inner
loops have. Delay slots ARE modelled: a jal/branch delay-slot instruction executes BEFORE the
transfer, which is itself a subtlety that produced one of this session's wrong readings.

Expressions are simplified only where it is safe (constant folding on immediates), so what you get is
the real chain — e.g. `sh (cx - ((rx*cos)>>12)) -> 44(r16)` rather than `sh r12 -> 44(r16)`.
"""
import argparse, struct, sys

REG = ["zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7",
       "s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","fp","ra"]


def sx(v):
    return v - 0x10000 if v >= 0x8000 else v


class Sym:
    """A register's value as a printable expression, with constant folding when it is a literal."""
    __slots__ = ("e", "c")

    def __init__(self, e, c=None):
        self.e, self.c = e, c          # c = integer value when statically known, else None

    def __str__(self):
        if self.c is None:
            return self.e
        # Guest addresses and masks read as hex; small counts read as decimal.
        return f"0x{self.c:08X}" if abs(self.c) >= 0x10000 else str(self.c)


def binop(a, b, op, fn):
    if a.c is not None and b.c is not None:
        return Sym("", fn(a.c, b.c))
    return Sym(f"({a} {op} {b})")


def run(mem, start, end, stores_only, base_filter):
    r = {i: Sym(REG[i]) for i in range(32)}
    r[0] = Sym("", 0)
    lo = hi = Sym("?")
    out = []
    pc = start
    pending = None                      # (kind, text) queued by a branch/jal, fired after its delay slot

    def word(a):
        o = a - 0x80000000
        return struct.unpack("<I", mem[o:o + 4])[0]

    while pc < end:
        w = word(pc)
        op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        sa, fn, imm = (w >> 6) & 31, w & 0x3F, w & 0xFFFF
        note = None

        if op == 0:
            if fn == 0x20 or fn == 0x21:   r[rd] = binop(r[rs], r[rt], "+", lambda a, b: a + b)
            elif fn == 0x22 or fn == 0x23: r[rd] = binop(r[rs], r[rt], "-", lambda a, b: a - b)
            elif fn == 0x24:               r[rd] = binop(r[rs], r[rt], "&", lambda a, b: a & b)
            elif fn == 0x25:               r[rd] = binop(r[rs], r[rt], "|", lambda a, b: a | b)
            elif fn == 0x00:               r[rd] = binop(r[rt], Sym("", sa), "<<", lambda a, b: a << b)
            elif fn == 0x02:               r[rd] = binop(r[rt], Sym("", sa), ">>", lambda a, b: a >> b)
            elif fn == 0x03:               r[rd] = binop(r[rt], Sym("", sa), ">>", lambda a, b: a >> b)
            elif fn == 0x18 or fn == 0x19:
                lo = Sym(f"({r[rs]} * {r[rt]})")
                hi = Sym(f"hi({r[rs]} * {r[rt]})")
            elif fn == 0x12:               r[rd] = lo
            elif fn == 0x10:               r[rd] = hi
            elif fn == 0x08:               note = f"JR {r[rs]}"
            elif fn == 0x2A:               r[rd] = Sym(f"({r[rs]} < {r[rt]})")
        elif op == 9 or op == 8:           r[rt] = binop(r[rs], Sym("", sx(imm)), "+", lambda a, b: a + b)
        elif op == 0x0C:                   r[rt] = binop(r[rs], Sym("", imm), "&", lambda a, b: a & b)
        elif op == 0x0D:                   r[rt] = binop(r[rs], Sym("", imm), "|", lambda a, b: a | b)
        elif op == 0x0F:                   r[rt] = Sym("", imm << 16)
        elif op == 0x0A:                   r[rt] = Sym(f"({r[rs]} < {sx(imm)})")
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25):
            kind = {0x20: "lb", 0x21: "lh", 0x23: "lw", 0x24: "lbu", 0x25: "lhu"}[op]
            r[rt] = Sym(f"{kind}[{r[rs]}{sx(imm):+d}]")
        elif op in (0x28, 0x29, 0x2B):
            kind = {0x28: "sb", 0x29: "sh", 0x2B: "sw"}[op]
            if base_filter is None or rs == base_filter:
                out.append((pc, f"{kind} {sx(imm):+5d}(r{rs}) <- {r[rt]}"))
            note = f"{kind} -> {sx(imm)}(r{rs})"
        elif op == 3:                       note = f"JAL 0x{((w & 0x03FFFFFF) << 2) | 0x80000000:08X}"
        elif op in (4, 5, 6, 7):            note = "branch"

        if not stores_only and note:
            out.append((pc, note))
        pc += 4
    return out, r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump"); ap.add_argument("start"); ap.add_argument("end")
    ap.add_argument("--stores-only", action="store_true")
    ap.add_argument("--base", help="only show stores through this base register, e.g. r16")
    ap.add_argument("--regs", help="comma-separated registers to print at the END of the range, "
                                   "e.g. r4,r17 — this is how you answer 'what feeds this call'")
    a = ap.parse_args()
    mem = open(a.dump, "rb").read()
    bf = int(a.base[1:]) if a.base else None
    rows, regs = run(mem, int(a.start, 16), int(a.end, 16), a.stores_only, bf)
    for pc, text in rows:
        print(f"  {pc:08X}: {text}")
    if a.regs:
        print("  -- register values at end of range --")
        for name in a.regs.split(","):
            n = int(name.strip()[1:])
            print(f"    r{n} = {regs[n]}")


if __name__ == "__main__":
    main()
